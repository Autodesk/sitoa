/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "DataSwitch.h"

AI_SHADER_NODE_EXPORT_METHODS(BooleanSwitchMethods);

node_parameters
{
   AiParameterInt  ("input"  , 0);
   AiParameterBool ("default", false);
   AiParameterArray("values",  AiArrayAllocate(0, 0, AI_TYPE_BOOLEAN));
   AiParameterArray("index",   AiArrayAllocate(0, 0, AI_TYPE_INT));
}

node_initialize
{
   AiNodeSetLocalData(node, new CSwitchData);
}

node_update
{
   CSwitchData* data = (CSwitchData*)AiNodeGetLocalData(node);
   data->Init(node);
}

node_finish
{
   delete (CSwitchData*)AiNodeGetLocalData(node);
}

shader_evaluate
{
   CSwitchData* data = (CSwitchData*)AiNodeGetLocalData(node);

   int index = data->HasIndex((short int)AiShaderEvalParamInt(p_input));
   if (index > -1)
   {
      AtArray *values = AiShaderEvalParamArray(p_values);
      sg->out.BOOL() = AiArrayGetBool(values, index);
      return;
   }
   // no index matching input, returning the default value
   sg->out.BOOL() = AiShaderEvalParamBool(p_default);
}

