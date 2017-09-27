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

AI_SHADER_NODE_EXPORT_METHODS(TXT2DTextureGridMethods);

enum TXT3DTextureGridParams
{
   p_line_color,
   p_fill_color,
   p_u_width,
   p_v_width,
   p_contrast,
   p_diffusion,
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
   AiParameterRGBA("line_color",   1.0f, 0.8f, 0.6f, 1.0f);
   AiParameterRGBA("fill_color",   0.1f, 0.3f, 0.6f, 1.0f);
   AiParameterFlt ("u_width",      0.2f);
   AiParameterFlt ("v_width",      0.2f);
   AiParameterFlt ("contrast",     1.0f);
   AiParameterFlt ("diffusion",    0.1f);
   AiParameterStr ("tspace_id",    "" );
   AiParameterVec ("repeats",      1.0f , 1.0f , 1.0f);  
   AiParameterBool("alt_x",        false);
   AiParameterBool("alt_y",        false);
   AiParameterBool("alt_z",        false);
   AiParameterVec ("min",          0.0f, 0.0f, 0.0f);
   AiParameterVec ("max",          10.0f, 10.0f, 10.0f);
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
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
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

inline AtRGBA RgbaContrast(const AtRGBA& in_color_1, const AtRGBA& in_color_2, const float in_t)
{
   AtRGBA out;
   float dot_five_plus_half_t  = 0.5f + 0.5f * in_t;
   float dot_five_minus_half_t = 0.5f - 0.5f * in_t;
	out.r = in_color_1.r * dot_five_plus_half_t + in_color_2.r * dot_five_minus_half_t;
	out.g = in_color_1.g * dot_five_plus_half_t + in_color_2.g * dot_five_minus_half_t;
	out.b = in_color_1.b * dot_five_plus_half_t + in_color_2.b * dot_five_minus_half_t;
	out.a = in_color_1.a * dot_five_plus_half_t + in_color_2.a * dot_five_minus_half_t;
   return out;
}

inline AtRGBA RgbaMix(const AtRGBA& in_color_1, const AtRGBA& in_color_2, const float in_t)
{
   AtRGBA out;
   float one_minus_t = 1.0f - in_t;
   out.r =  in_color_1.r * one_minus_t + in_color_2.r * in_t;
   out.g =  in_color_1.g * one_minus_t + in_color_2.g * in_t;
   out.b =  in_color_1.b * one_minus_t + in_color_2.b * in_t;
   out.a =  in_color_1.a * one_minus_t + in_color_2.a * in_t;
   return out;
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

   AtVector coord = AtVector(sg->u, sg->v, 0.0f);

   if (!data->tspace_id.empty())
   {
      AtVector2 uvPoint;
      if (AiUDataGetVec2(data->tspace_id, uvPoint))
      {
         coord.x = uvPoint.x;
         coord.y = uvPoint.y;
      }
   }

   // Repeats, Alternates & UVRemap
   AtVector repeats = AiShaderEvalParamVec(p_repeats);
   AtVector min     = AiShaderEvalParamVec(p_min);
   AtVector max     = AiShaderEvalParamVec(p_max);

   compute_uv(coord.x, coord.y, repeats, min, max, wrap_u, wrap_v, data->alt_x, data->alt_y);

   AtRGBA line_color = AiShaderEvalParamRGBA(p_line_color);
   AtRGBA fill_color = AiShaderEvalParamRGBA(p_fill_color);
   float uw = AiShaderEvalParamFlt(p_u_width) * 0.5f;
   float vw = AiShaderEvalParamFlt(p_v_width) * 0.5f;
   float contrast  = AiShaderEvalParamFlt(p_contrast);
   float diffusion = AiShaderEvalParamFlt(p_diffusion);

   // the following, adapted from sib_grid.cpp
	float u = coord.x - floorf(coord.x);
	float v = coord.y - floorf(coord.y);

	/* mirror about 0.5 */
	u = u > 0.5f ? 1.0f - u : u;
	v = v > 0.5f ? 1.0f - v : v;

   AtRGBA result;
	if ((u > uw) && (v > vw))
   {
      result = RgbaContrast(fill_color, line_color, contrast);
		if (diffusion > 0.0f)
      {
         AtRGBA tmp = RgbaContrast(line_color, fill_color, contrast);
         result = RgbaMix(result, tmp, expf(-((u - uw) * (v - vw) * 4.0f) / diffusion));
		}
	}
   else
      result = RgbaContrast(line_color, fill_color, contrast);

   if (data->alpha_output)
   {
      float alpha_factor = AiShaderEvalParamFlt(p_alpha_factor);
      sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = sg->out.RGBA().a = result.a * alpha_factor;
   }
   else
      sg->out.RGBA() = result;
}

