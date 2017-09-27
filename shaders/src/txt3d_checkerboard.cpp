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

AI_SHADER_NODE_EXPORT_METHODS(txt3d_checkerboardMethods);

enum CheckerboardParams
{
   p_tex,
   p_tspace_id,
   p_color1,
   p_color2,
   p_repeats,
   p_min,
   p_max,
   p_torus_u,
   p_torus_v,
   p_alt_u,
   p_alt_v,
   p_xsize,
   p_ysize,
   p_alpha_output,
   p_alpha_factor
};

node_parameters
{
   AiParameterRGBA("tex", 0, 0, 0, 0);
   AiParameterStr("tspace_id", "");
   AiParameterRGBA("color1", 0.0f, 0.0f, 0.0f, 1.0f);
   AiParameterRGBA("color2", 1.0f, 1.0f, 1.0f, 1.0f);
   AiParameterVec("repeats", 4.0f, 4.0f, 4.0f);
   AiParameterVec("min", 0.0f, 0.0f, 0.0f);
   AiParameterVec("max", 0.0f, 0.0f, 0.0f);
   AiParameterBool("torus_u", false);
   AiParameterBool("torus_v", false);
   AiParameterBool("alt_x", false);
   AiParameterBool("alt_y", false);
   AiParameterFlt("xsize", 0.5f);
   AiParameterFlt("ysize", 0.5f);
   AiParameterBool("alpha_output", false);
   AiParameterFlt("alpha_factor", false);
}

namespace {

typedef struct 
{
   AtString    tspace_id;
   AtString    projection_wrap;
   bool        alt_x, alt_y;
   bool        torus_u, torus_v;
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
   data->torus_u      = AiNodeGetBool(node, "torus_u");
   data->torus_v      = AiNodeGetBool(node, "torus_v");
   data->alpha_output = AiNodeGetBool(node, "alpha_output");
}

node_finish
{
   AiFree(AiNodeGetLocalData(node));
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

   u-= AiShaderEvalParamFlt(p_xsize) - 0.5f;
   v-= AiShaderEvalParamFlt(p_ysize) - 0.5f;

   // Checkerboard
   AtRGBA result;
   if (u > 0.5f)
      result = AiShaderEvalParamRGBA(v > 0.5f ? p_color1 : p_color2);
   else
      result = AiShaderEvalParamRGBA(v > 0.5f ? p_color2 : p_color1);

   if (data->alpha_output)
   {
      float alpha_factor = AiShaderEvalParamFlt(p_alpha_factor);
      sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = sg->out.RGBA().a = result.a * alpha_factor;
   }
   else
      sg->out.RGBA() = result;
}
