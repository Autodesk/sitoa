/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <cstdio>
#include <ai.h>
#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(txt3d_marbleMethods);

enum MarbleParams
{
   p_tspace_id,
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
   p_repeats,
   p_min,
   p_max,
   p_torus_u,
   p_torus_v,
   p_alt_u,
   p_alt_v,
   p_alpha_output,
   p_alpha_factor
};

node_parameters
{
   AiParameterStr("tspace_id",     "");
   AiParameterRGBA("filler_col",   1.0f, 1.0f, 1.0f, 1.0f);
   AiParameterRGBA("vein_col1",    0.8f, 0.8f, 0.8f, 1.0f);
   AiParameterRGBA("vein_col2",    0.708f, 0.250f, 0.250f, 1.0f);
   AiParameterFlt("vein_width",    0.2f);
   AiParameterFlt("diffusion",     0.2f);
   AiParameterRGBA("spot_color",   0.062f, 0.062f, 0.041f, 1.0f);
   AiParameterFlt("spot_density",  1.0f);
   AiParameterFlt("spot_bias",     0.2f);
   AiParameterFlt("spot_scale",    0.150f);
   AiParameterFlt("amplitude",     1.5f);
   AiParameterFlt("ratio",         0.707f);
   AiParameterFlt("complexity",    5.0f);
   AiParameterBool("absolute",     true);
   AiParameterVec("frequencies",   1.0f, 1.0f, 1.0f);
   AiParameterVec("repeats",       4.0f, 4.0f, 4.0f);
   AiParameterVec("min",           0.0f, 0.0f, 0.0f);
   AiParameterVec("max",           1.0f, 1.0f, 1.0f);
   AiParameterBool("torus_u",      false);
   AiParameterBool("torus_v",      false);
   AiParameterBool("alt_x",        false);
   AiParameterBool("alt_y",        false);
   AiParameterBool("alpha_output", false);
   AiParameterFlt("alpha_factor",  false);
};

namespace {

typedef struct 
{
   AtString    tspace_id;
   AtString    projection_wrap;
   bool        alt_x, alt_y;
   bool        torus_u, torus_v;
   bool        alpha_output;
   bool        absolute;
} ShaderData;

}

node_initialize
{
   ShaderData* data = (ShaderData*)AiMalloc(sizeof(ShaderData));
   AiNodeSetLocalData(node, data);
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->tspace_id = AiNodeGetStr(node, "tspace_id");
   char* s = (char*)alloca(data->tspace_id.length() + 6);
   sprintf(s, "%s_wrap", data->tspace_id.c_str());
   data->projection_wrap = AtString(s);

   data->alt_x        = AiNodeGetBool(node, "alt_x");
   data->alt_y        = AiNodeGetBool(node, "alt_y");
   data->torus_u      = AiNodeGetBool(node, "torus_u");
   data->torus_v      = AiNodeGetBool(node, "torus_v");
   data->alpha_output = AiNodeGetBool(node, "alpha_output");
   data->absolute     = AiNodeGetBool(node, "absolute");
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

   bool wrap_u = false, wrap_v = false;
   AtArray* wrap_settings;
   if (AiUDataGetArray(data->projection_wrap, wrap_settings))
   {
      wrap_u = AiArrayGetBool(wrap_settings, 0);
      wrap_v = AiArrayGetBool(wrap_settings, 1);
   }

   float u = sg->u, v = sg->v;

   AtVector2 uvPoint;
   if (AiUDataGetVec2(data->tspace_id, uvPoint))
   {
      u = uvPoint.x;
      v = uvPoint.y;
   }

   AtVector repeats = AiShaderEvalParamVec(p_repeats);
   AtVector min     = AiShaderEvalParamVec(p_min);
   AtVector max     = AiShaderEvalParamVec(p_max);

   compute_uv(u, v, repeats, min, max, wrap_u || data->torus_u, wrap_v || data->torus_v, data->alt_x, data->alt_y);

   AtVector vec = AtVector(u, v, 0.0f);

   AtRGBA result;
   
   // veins
   float vein_width = AiShaderEvalParamFlt(p_vein_width);
   float diffusion = AiShaderEvalParamFlt(p_diffusion);
   AtRGBA vein_col1  = AiShaderEvalParamRGBA(p_vein_col1);
   AtRGBA vein_col2  = AiShaderEvalParamRGBA(p_vein_col2);
   AtRGBA filler_col = AiShaderEvalParamRGBA(p_filler_col);
   
   // spots
   AtRGBA spot_color = AiShaderEvalParamRGBA(p_spot_color);
   float spot_bias = AiShaderEvalParamFlt(p_spot_bias) * 4;
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
   AtRGBA vcol1, vcol2;
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

      AtRGBA tempcol = vcol1 * intensa;
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
      
      float bright = AiNoise3(ns, 1, 0, 0);
      
      float level = 1.0f - spot_density;
      if (bright > level)
      {
         bright = (bright - level) / (1.0f-level);
         if (spot_bias != 0.5f)
            bright = powf(bright, logf(spot_bias) / logf(0.5f));

         result = (1.0f - bright) * result + bright * spot_color;
      }
   }

   if (data->alpha_output)
   {
      float alpha_factor = AiShaderEvalParamFlt(p_alpha_factor);
      sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = sg->out.RGBA().a = result.a * alpha_factor;
   }
   else
      sg->out.RGBA() = result;
}
