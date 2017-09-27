/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarInRangeMethods);

enum SIBScalarInRangeParams 
{
   p_input,
   p_min_thresh,
   p_max_thresh,
   p_negate
};

node_parameters
{
   AiParameterFlt ( "input"     , 0.5f  );
   AiParameterFlt ( "min_thresh", 0.45f );
   AiParameterFlt ( "max_thresh", 0.55f );
   AiParameterBool( "negate"    , false );
}

namespace {

typedef struct 
{
   bool negate;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->negate = AiNodeGetBool(node, "negate");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   float input = AiShaderEvalParamFlt(p_input);

   bool result = true;
   if (input < AiShaderEvalParamFlt(p_min_thresh))
      result = false;
   else if (input > AiShaderEvalParamFlt(p_max_thresh))
      result = false;

   sg->out.BOOL() = data->negate ? !result : result;
}
