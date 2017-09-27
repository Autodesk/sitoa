/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBInterpLinearMethods);

enum SIBInterpLinearMethods
{
   p_input,
   p_oldrange_min,
   p_oldrange_max,
   p_newrange_min,
   p_newrange_max
};

node_parameters
{
   AiParameterFlt ( "input"        , 0.5f );
   AiParameterFlt ( "oldrange_min" , 0.0f );
   AiParameterFlt ( "oldrange_max" , 1.0f );
   AiParameterFlt ( "newrange_min" , 0.2f );
   AiParameterFlt ( "newrange_max" , 0.8f );
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   float input = AiShaderEvalParamFlt(p_input);
   float oldrange_min = AiShaderEvalParamFlt(p_oldrange_min);
   float oldrange_max = AiShaderEvalParamFlt(p_oldrange_max);
   float newrange_min = AiShaderEvalParamFlt(p_newrange_min);
   float newrange_max = AiShaderEvalParamFlt(p_newrange_max);

   float old_range = oldrange_max - oldrange_min;

   if (std::abs(old_range) <= AI_EPSILON) 
      old_range = AI_EPSILON;

   sg->out.FLT() = newrange_min + ((newrange_max - newrange_min) * ((input - oldrange_min) / old_range));
}
