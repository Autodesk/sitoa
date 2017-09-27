/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarToColorMethods);

enum SIBScalarToColorParams
{
   p_input,
   p_alpha
};

node_parameters
{
   AiParameterFlt ( "input" , 0.5f );
   AiParameterFlt ( "alpha" , 1.0f );
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = AiShaderEvalParamFlt(p_input);
   sg->out.RGBA().a = AiShaderEvalParamFlt(p_alpha);
}
