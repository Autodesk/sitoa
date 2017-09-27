/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(sib_texture_marbleMethods);

enum MarbleParams
{
   p_filler_col,
   p_vein_col1,
   p_vein_col2,
   p_vein_width,
   p_diffusion,
   p_spot_color,
   p_spot_density,
   p_spot_bias,
   p_spot_scale,
   p_amplitude,
   p_ratio,
   p_complexity,
   p_absolute,
   p_frequencies,
   p_coord
};

node_parameters
{
   AiParameterRGB("filler_col",   1.0f, 1.0f, 1.0f);
   AiParameterRGB("vein_col1",    0.8f, 0.8f, 0.8f);
   AiParameterRGB("vein_col2",    0.708f, 0.250f, 0.250f);
   AiParameterFlt("vein_width",   0.2f);
   AiParameterFlt("diffusion",    0.2f);
   AiParameterRGB("spot_color",   0.062f, 0.062f, 0.041f);
   AiParameterFlt("spot_density", 1.0f);
   AiParameterFlt("spot_bias",    0.2f);
   AiParameterFlt("spot_scale",   0.150f);
   AiParameterFlt("amplitude",    1.5f);
   AiParameterFlt("ratio",        0.707f);
   AiParameterFlt("complexity",   5.0f);
   AiParameterBool("absolute",    true);
   AiParameterVec("frequencies",  1.0f, 1.0f, 1.0f);
   AiParameterVec("coord",        0.0f, 0.0f, 0.0f);
};

namespace {

typedef struct 
{
   bool absolute;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->absolute = AiNodeGetBool(node, "absolute");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

static float fractal3(AtVector &pos, float amplitude, float ratio, float complexity, AtVector &frequencies, bool absolute)
{
   float a, fr;
   int wh = (int)complexity;
   AtVector	vec;
   float result = 0.0f;
   
   if (amplitude > 0) 
   {
      a = 2.0f * amplitude;
      vec = pos * frequencies;
      
      if (absolute) 
      {
         float offset = 0.0f;
         if (wh)	
         {
            // repeat
            for(int i=0; i < wh; ++i) 
            {
               result += a * fabs((AiPerlin3(vec) / 2 + 0.5f) - 0.5f);
               vec *= 2;
               offset += a;
               a *= ratio;
            }
         }
         // delta
         fr = complexity - wh;
         if (fr != 0.0f)
         {
            result += fr * a * fabs((AiPerlin3(vec) / 2 + 0.5f) - 0.5f);
            offset += fr*a;
         }
         result -= offset*0.25f;	
      } 
      else 
      {
         if (wh > 0)	
         {
            // repeat
            for (int i=0; i < wh; ++i) 
            {
               result += a * ((AiPerlin3(vec) / 2 + 0.5f) - 0.5f);
               vec *= 2;
               a *= ratio;
            }
         }
         // delta
         fr = complexity - wh;
         if (fr != 0.0f)
            result += fr * a * ((AiPerlin3(vec) / 2 + 0.5f) - 0.5f);
      }
   }
   
   return result;
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGB result;

   AtVector vec = AiShaderEvalParamVec(p_coord);
   // veins
   float vein_width = AiShaderEvalParamFlt(p_vein_width);
   float diffusion = AiShaderEvalParamFlt(p_diffusion);
   AtRGB vein_col1 = AiShaderEvalParamRGB(p_vein_col1);
   AtRGB vein_col2 = AiShaderEvalParamRGB(p_vein_col2);
   AtRGB filler_col = AiShaderEvalParamRGB(p_filler_col);
   // spots
   float spot_bias = AiShaderEvalParamFlt(p_spot_bias) * 4.0f;
   float spot_density = AiShaderEvalParamFlt(p_spot_density);
   float spot_scale = AiShaderEvalParamFlt(p_spot_scale) * 1.5f;
   // fractal
   float amplitude = AiShaderEvalParamFlt(p_amplitude);
   float ratio = AiShaderEvalParamFlt(p_ratio);
   float complexity = AiShaderEvalParamFlt(p_complexity);
   AtVector frequencies = AiShaderEvalParamVec(p_frequencies) * 0.5f;
   float height = fractal3(vec, amplitude, ratio, complexity, frequencies, data->absolute) + vec.y;
   
   int layer = (int)floor(height);
   height = height - (float)(int)floor(height) - vein_width;
   
   // veins
   AtRGB vcol1, vcol2;
   if (layer & 1)
   {
      vcol1 = vein_col1;
      vcol2 = vein_col2;
   }
   else
   {
      vcol1 = vein_col2;
      vcol2 = vein_col1;
   }
   if (height < 0.0f)
      result = vcol1;
   else if (height > 1.0f - vein_width)
      result = vcol2;
   else if (diffusion == 0.0f)
      result = filler_col;
   else
   {
      float intensa = exp(-height / diffusion);
      float intensb = exp(-(1.0f - vein_width - height) / diffusion);
      float intensf = 1.0f - intensa - intensb;
      
      result = filler_col * intensf;
      
      AtRGB tempcol = vcol1 * intensa;
      result += tempcol;
      
      tempcol = vcol2 * intensb;
      result += tempcol;
   }
   
   // spots
   if (spot_bias && spot_density && spot_scale)
   {
      AtVector ns;
      float scale = 1.0f / spot_scale;
      scale = pow(scale, 3.0f);
      
      AtVector2 tmp;
      tmp.x = vec.z;
      tmp.y = vec.y;
      ns.x = vec.x + AiPerlin2(tmp) * scale;
      tmp.x = vec.x;
      tmp.y = vec.z;
      ns.y = vec.y + AiPerlin2(tmp) * scale;
      tmp.x = vec.y;
      tmp.y = vec.x;
      ns.z = vec.z + AiPerlin2(tmp) * scale;
      
      float bright = AiNoise3(ns, 1, 0.0f, 0.0f);
      
      float level = 1.0f - spot_density;
      if (bright > level)
      {
         bright = (bright - level) / (1.0f - level);
         if (spot_bias != 0.5f)
            bright = powf(bright, logf(spot_bias) / logf(0.5f));

         AtRGB spot_color = AiShaderEvalParamRGB(p_spot_color);
         result = (1.0f - bright) * result + bright * spot_color;
      }
   }
   
   sg->out.RGB() = result;
   sg->out.RGBA().a = 1.0f;
}
