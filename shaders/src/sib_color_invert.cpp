/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorInvertMethods);

enum SIBColorInvertParams
{
   p_input,
   p_alpha
};

node_parameters
{
   AiParameterRGBA( "input" , 0.0f, 0.0f, 0.0f, 1.0f );
   AiParameterBool( "alpha" , false );              
}

namespace {

typedef struct 
{
   bool alpha;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->alpha = AiNodeGetBool(node, "alpha");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGBA input = AiShaderEvalParamRGBA(p_input);

   sg->out.RGBA().r = 1.0f - input.r;
   sg->out.RGBA().g = 1.0f - input.g;
   sg->out.RGBA().b = 1.0f - input.b;
   sg->out.RGBA().a = data->alpha ? 1.0f - input.a : input.a;
}
