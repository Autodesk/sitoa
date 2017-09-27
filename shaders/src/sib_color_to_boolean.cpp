/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorToBooleanMethods);

enum SIBColorToBooleanParams
{
   p_input,
   p_threshold,
   p_alpha
};

node_parameters
{
   AiParameterRGBA("input"     , 1.0f, 1.0f, 1.0f, 1.0f);
   AiParameterRGBA("threshold" , 0.5f, 0.5f, 0.5f, 0.5f);
   AiParameterBool("alpha"     , true);              
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

   AtRGBA input     = AiShaderEvalParamRGBA(p_input);
   AtRGBA threshold = AiShaderEvalParamRGBA(p_threshold);

   if (data->alpha)
      sg->out.BOOL() = ! (input.r <= threshold.r && input.g <= threshold.g && input.b <= threshold.b && input.a <= threshold.a);
   else
      sg->out.BOOL() = ! (input.r <= threshold.r && input.g <= threshold.g && input.b <= threshold.b);
}
