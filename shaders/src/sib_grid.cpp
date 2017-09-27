/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBTextureGridMethods);

enum SIBTextureGridParams
{
   p_coord,
   p_line_color,
   p_fill_color,
   p_u_width,
   p_v_width,
   p_contrast,
   p_diffusion
};

node_parameters
{
   AiParameterVec ("coord",      1.0f , 1.0f , 1.0f);  
   AiParameterRGBA("line_color", 1.0f, 0.8f, 0.6f, 1.0f);
   AiParameterRGBA("fill_color", 0.1f, 0.3f, 0.6f, 1.0f);
   AiParameterFlt ("u_width",    0.2f);
   AiParameterFlt ("v_width",    0.2f);
   AiParameterFlt ("contrast",   1.0f);
   AiParameterFlt ("diffusion",  0.1f);
}

node_initialize {}
node_update {}
node_finish {}

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
   AtVector coord = AiShaderEvalParamVec(p_coord);

   AtRGBA line_color = AiShaderEvalParamRGBA(p_line_color);
   AtRGBA fill_color = AiShaderEvalParamRGBA(p_fill_color);
   float contrast    = AiShaderEvalParamFlt(p_contrast);
   float uw = AiShaderEvalParamFlt(p_u_width) * 0.5f;
   float vw = AiShaderEvalParamFlt(p_v_width) * 0.5f;

   // the following, adapted from sib_grid.cpp
	float u = coord.x - floorf(coord.x);
	float v = coord.y - floorf(coord.y);

	/* mirror about 0.5 */
	u = u > 0.5f ? 1.0f - u : u;
	v = v > 0.5f ? 1.0f - v : v;

   AtRGBA result;
	if ((u > uw) && (v > vw))
   {
      float diffusion = AiShaderEvalParamFlt(p_diffusion);
      result = RgbaContrast(fill_color, line_color, contrast);
		if (diffusion > 0.0f)
      {
         AtRGBA tmp = RgbaContrast(line_color, fill_color, contrast);
         result = RgbaMix(result, tmp, expf(-((u - uw) * (v - vw) * 4.0f) / diffusion));
		}
	}
   else
      result = RgbaContrast(line_color, fill_color, contrast);

   sg->out.RGBA() = result;
}

