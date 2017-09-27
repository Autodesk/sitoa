/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"
#include <string>

AI_SHADER_NODE_EXPORT_METHODS(PassthroughFilterMethods);

enum passthrough_filterParams 
{
	p_mode,
	p_color
};

enum eMode
{
   modeMute = 0,
   modeSet,
   modeMultiply
};

node_parameters
{
   AiParameterInt ("mode",  1);
   AiParameterRGBA("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

namespace {

typedef struct 
{
   int mode;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->mode = AiNodeGetInt(node, "mode");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   if (!sg->light_filter) 
      return; // invalid context

   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   if (data->mode == modeMute)
      return;

   AtRGBA color = AiShaderEvalParamRGBA(p_color);

   if (data->mode == modeSet)
   {
      sg->light_filter->Liu.r = color.r;
      sg->light_filter->Liu.g = color.g;
      sg->light_filter->Liu.b = color.b;
   }
   else if (data->mode == modeMultiply)
   {
      sg->light_filter->Liu.r*= color.r;
      sg->light_filter->Liu.g*= color.g;
      sg->light_filter->Liu.b*= color.b;
   }
}


