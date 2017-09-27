/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include <cstring>
#include <cstdio>

#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(TXT3DTextureSnowMethods);

enum TXT3DTextureSnowParams
{
   p_snow_col,
   p_surface_col,
   p_threshold,
   p_depth_decay,
   p_thickness,
   p_randomness,
   p_rand_freq,
   p_tspace_id,
   p_repeats,
   p_alt_x,
   p_alt_y,
   p_alt_z,
   p_min,
   p_max,
   p_alpha_output,
   p_alpha_factor
};


node_parameters
{
   AiParameterRGBA("snow_col",     1.0f, 1.0f, 1.0f, 1.0f);
   AiParameterRGBA("surface_col",  0.0f, 0.0f, 1.0f, 1.0f);
   AiParameterFlt ("threshold",    0.9f);
   AiParameterFlt ("depth_decay",  5.0f);
   AiParameterFlt ("thickness",    0.7f);
   AiParameterFlt ("randomness",   0.5f);
   AiParameterFlt ("rand_freq",    15.0f)
   AiParameterStr ("tspace_id",    "" );
   AiParameterVec ("repeats",      1.0f , 1.0f , 1.0f);  
   AiParameterBool("alt_x",        false);
   AiParameterBool("alt_y",        false);
   AiParameterBool("alt_z",        false);
   AiParameterVec ("min",          0.0f, 0.0f, 0.0f);
   AiParameterVec ("max",          5.0f, 5.0f, 5.0f);
   AiParameterBool("alpha_output", false);
   AiParameterFlt ("alpha_factor", 1.0f);
}

namespace {

typedef struct 
{
   AtString    tspace_id;
   AtString    projection_wrap;
   bool        alt_x, alt_y;
   bool        alpha_output;
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
   data->alpha_output = AiNodeGetBool(node, "alpha_output");
}

node_finish
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   // copy-pasted from sib_snow.cpp
   float dot = AiV3Dot(sg->N, AI_V3_Y);
   float randomness = AiShaderEvalParamFlt(p_randomness);

   if (randomness > 0.0f) 
   {
      bool wrap_u = false, wrap_v = false;
      AtArray* wrap_settings;
      if (AiUDataGetArray(data->projection_wrap, wrap_settings))
      {
         wrap_u = AiArrayGetBool(wrap_settings, 0);
         wrap_v = AiArrayGetBool(wrap_settings, 1);
      }

      AtVector coord = AtVector(sg->u, sg->v, 0.0f);

      if (data->tspace_id && data->tspace_id[0] != '\0')
      {
         AtVector2 uvPoint;
         if (AiUDataGetVec2(data->tspace_id, uvPoint))
         {
            coord.x = uvPoint.x;
            coord.y = uvPoint.y;
         }

         AtVector repeats = AiShaderEvalParamVec(p_repeats);
         AtVector min     = AiShaderEvalParamVec(p_min);
         AtVector max     = AiShaderEvalParamVec(p_max);

         compute_uv(coord.x, coord.y, repeats, min, max, wrap_u, wrap_v, data->alt_x, data->alt_y);
      }
      else
         coord = sg->P;

      float rand_freq  = AiShaderEvalParamFlt(p_rand_freq) * 0.5f;
      // we make the randomness depend on position rather than direction
      coord *= rand_freq;
      float noise = AiPerlin3(coord); // -1..1
      // conform to mental noise, ranging 0..1 
      noise = (noise + 1.0f) * 0.5f;
      dot -= noise * randomness;
   }

   dot = dot - 1.0f + AiShaderEvalParamFlt(p_threshold);
   if (dot <= 0.0f)
      dot = 0.0f;
   else 
   {
      dot *= AiShaderEvalParamFlt(p_depth_decay);
      if (dot > 1.0f)
         dot = 1.0f;
      dot *= AiShaderEvalParamFlt(p_thickness);
   }

   AtRGBA surface_col = AiShaderEvalParamRGBA(p_surface_col);
   AtRGBA snow_col    = AiShaderEvalParamRGBA(p_snow_col);

   if (AiShaderEvalParamBool(p_alpha_output))
   {
      float alpha_factor = AiShaderEvalParamFlt(p_alpha_factor);
      sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = sg->out.RGBA().a = AiLerp(dot, surface_col.a, snow_col.a) * alpha_factor;
   }
   else
      sg->out.RGBA() = AiLerp(dot, surface_col, snow_col);
}
