/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarMathLogicMethods);

#define OPERATOR_EQUAL              0
#define OPERATOR_LESS               1   
#define OPERATOR_GREATER            2   
#define OPERATOR_LESS_OR_EQUAL      3
#define OPERATOR_GREATER_OR_EQUAL   4   

enum SIBScalarMathLogicParams 
{
   p_input1,
   p_input2,
   p_op
};

node_parameters
{
   AiParameterFlt ( "input1", 0.0f );
   AiParameterFlt ( "input2", 1.0f );
   AiParameterInt ( "op"    , 0    );
}

namespace {

typedef struct 
{
   int  op;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->op = AiNodeGetInt(node, "op");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   
   if (data->op == OPERATOR_EQUAL)
      sg->out.BOOL() = (AiShaderEvalParamFlt(p_input1) == AiShaderEvalParamFlt(p_input2));
   else if (data->op == OPERATOR_LESS)
      sg->out.BOOL() = (AiShaderEvalParamFlt(p_input1) < AiShaderEvalParamFlt(p_input2));
   else if (data->op == OPERATOR_GREATER)
      sg->out.BOOL() = (AiShaderEvalParamFlt(p_input1) > AiShaderEvalParamFlt(p_input2));
   else if (data->op == OPERATOR_LESS_OR_EQUAL)
      sg->out.BOOL() = (AiShaderEvalParamFlt(p_input1) <= AiShaderEvalParamFlt(p_input2));
   else if (data->op == OPERATOR_GREATER_OR_EQUAL)
      sg->out.BOOL() = (AiShaderEvalParamFlt(p_input1) >= AiShaderEvalParamFlt(p_input2));
}
