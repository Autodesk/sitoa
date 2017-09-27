/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorToVectorMethods);

enum SIBColorToVectorParams
{
   p_input,
   p_method,
   p_scale
};

node_parameters
{
   AiParameterRGB ( "input" , 1.0f , 1.0f , 1.0f );
   AiParameterInt ( "method", 0 );  // Not implemented
   AiParameterFlt ( "scale" , 1.0f );
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   AtRGB input = AiShaderEvalParamRGB(p_input);
   float scale = AiShaderEvalParamFlt(p_scale);
    
   sg->out.VEC().x = input.r * scale;
   sg->out.VEC().y = input.g * scale;
   sg->out.VEC().z = input.b * scale;
}
