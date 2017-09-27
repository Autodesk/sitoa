/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorCombineMethods);

enum SIBColorCombineParams
{
   p_red,
   p_green,
   p_blue,
   p_alpha
};

node_parameters
{
   AiParameterFlt( "red"   , 1.0f );
   AiParameterFlt( "green" , 1.0f );
   AiParameterFlt( "blue"  , 1.0f );
   AiParameterFlt( "alpha" , 1.0f );
}

node_initialize {}
node_update{}
node_finish{}

shader_evaluate
{
    sg->out.RGBA().r = AiShaderEvalParamFlt(p_red);      
    sg->out.RGBA().g = AiShaderEvalParamFlt(p_green);      
    sg->out.RGBA().b = AiShaderEvalParamFlt(p_blue);      
    sg->out.RGBA().a = AiShaderEvalParamFlt(p_alpha);      
}
