/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(MIBColorIntensityMethods);

enum MIBColorIntensityParams
{
   p_input,
   p_factor
};

node_parameters
{
   AiParameterRGBA( "input"  , 1.0f, 0.7f, 0.5f, 0.0f );
   AiParameterFlt ( "factor" , 1.0f );
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   AtRGBA input = AiShaderEvalParamRGBA(p_input);
   float factor = AiShaderEvalParamFlt(p_factor);
   sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = sg->out.RGBA().a = factor * (0.299f * input.r + 0.587f * input.g + 0.114f * input.b);
}
