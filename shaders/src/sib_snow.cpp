/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBTextureSnowMethods);

enum SIBTextureSnowParams
{
   p_coord,
   p_snow_col,
   p_surface_col,
   p_threshold,
   p_depth_decay,
   p_thickness,
   p_randomness,
   p_rand_freq
};

node_parameters
{
   AiParameterVec ( "coord",       0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "snow_col",    1.0f, 1.0f, 1.0f, 1.0f );
   AiParameterRGBA( "surface_col", 0.0f, 0.0f, 1.0f, 1.0f );
   AiParameterFlt ( "threshold",   0.8f );
   AiParameterFlt ( "depth_decay", 3.0f );
   AiParameterFlt ( "thickness",   1.0f );
   AiParameterFlt ( "randomness",  0.2f );
   AiParameterFlt ( "rand_freq",   2.0f )
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   float dot = AiV3Dot(sg->N, AI_V3_Y);
   float randomness = AiShaderEvalParamFlt(p_randomness);

   if (randomness > 0.0f) 
   {
      float rand_freq  = AiShaderEvalParamFlt(p_rand_freq) * 0.5f;
      AtVector coord = AiShaderEvalParamVec(p_coord);
      // we make the randomness depend on position rather than direction.
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
   AtRGBA snow_col = AiShaderEvalParamRGBA(p_snow_col);
   sg->out.RGBA() = AiLerp(dot, surface_col, snow_col);
   //RGBA_MIX(col, mi_eval_color(&paras->surface_col) , mi_eval_color(&paras->snow_col), dot);
}
