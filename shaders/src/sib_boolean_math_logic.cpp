/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBBooleanMathLogicMethods);

#define OPERATOR_AND       0
#define OPERATOR_OR        1
#define OPERATOR_EQUALS    2

enum SIBBooleanMathLogicParams
{
   p_input1,
   p_input2,
   p_op,
   p_negate
};

node_parameters
{
   AiParameterBool( "input1", true  );
   AiParameterBool( "input2", true  );
   AiParameterInt ( "op"    , 0     );
   AiParameterBool( "negate", false );
}

namespace {

typedef struct 
{
   int  op;
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
   data->op     = AiNodeGetInt(node, "op");
   data->negate = AiNodeGetBool(node, "negate");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   bool result = false;

   if (data->op == OPERATOR_AND)
      result = AiShaderEvalParamBool(p_input1) && AiShaderEvalParamBool(p_input2);
   else if (data->op == OPERATOR_OR)
      result = AiShaderEvalParamBool(p_input1) || AiShaderEvalParamBool(p_input2);
   else if (data->op == OPERATOR_EQUALS)
      result = AiShaderEvalParamBool(p_input1) == AiShaderEvalParamBool(p_input2);

   sg->out.BOOL() = data->negate ? !result : result;
}
