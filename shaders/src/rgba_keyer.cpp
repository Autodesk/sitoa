/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(rgba_keyerMethods);

enum rgba_keyerParams
{
   p_input,
   p_min_thresh,
   p_max_thresh,
   p_alpha,
   p_inrange,
   p_outrange
};

node_parameters
{
   AiParameterRGB ( "input"     , 0.0f, 1.0f, 0.0f );
   AiParameterRGB ( "min_thresh", -0.05f, 0.95f, -0.05f );
   AiParameterRGB ( "max_thresh", 0.05f, 1.05f, 0.05f );
   AiParameterBool( "alpha"     , false );                 // Not implemented
   AiParameterRGB ( "inrange"   , 1.0f, 0.0f, 0.0f );
   AiParameterRGB ( "outrange"  , 0.0f, 0.0f, 1.0f );
}

node_initialize {}
node_update {}
node_finish {}

shader_evaluate
{
   AtRGB input = AiShaderEvalParamRGB(p_input);
   AtRGB min_thresh = AiShaderEvalParamRGB(p_min_thresh);
   AtRGB max_thresh = AiShaderEvalParamRGB(p_max_thresh);

   if ( input.r >= min_thresh.r && input.r <= max_thresh.r && 
        input.g >= min_thresh.g && input.g <= max_thresh.g &&
        input.b >= min_thresh.b && input.b <= max_thresh.b )
   {
      sg->out.RGB() = AiShaderEvalParamRGB(p_inrange);
   }
   else 
      sg->out.RGB() = AiShaderEvalParamRGB(p_outrange);
}
