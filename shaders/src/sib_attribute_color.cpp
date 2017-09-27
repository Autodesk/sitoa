/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBAttributeColorMethods);

enum SIBAttributeColorParams 
{
   p_attribute,
   p_index,
   p_default
};

node_parameters
{
   AiParameterStr ("attribute"  , "Color");
   AiParameterInt ("index"      , 0); // not supported
   AiParameterRGBA("default"    , 1.0f, 1.0f, 1.0f, 0.0f);
}

namespace {

typedef struct 
{
   AtString attribute;
} ShaderData;

}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->attribute = AiNodeGetStr(node, "attribute");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   if (data->attribute)
   {
      // Try to get the user data fisrt as RGBA, then as RGB
      if (AiUDataGetRGBA(data->attribute, sg->out.RGBA()))
         return;

      AtRGB c;
      if (AiUDataGetRGB(data->attribute, c))
      {
         sg->out.RGBA().r = c.r;
         sg->out.RGBA().g = c.g;
         sg->out.RGBA().b = c.b;
         sg->out.RGBA().a = 1.0f;
         return;
      }
   }

   sg->out.RGBA() = AiShaderEvalParamRGBA(p_default);
}
