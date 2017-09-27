/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

#define OPERATOR_ADD        0
#define OPERATOR_SUBSTRACT  1
#define OPERATOR_MULTIPLY   2
#define OPERATOR_DIVIDE     3
#define OPERATOR_MINIMUM    4
#define OPERATOR_MAXIMUM    5
#define OPERATOR_REMAINDER  6
#define OPERATOR_ARCTAN     7

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarMathBasicMethods);

enum SIBScalarMathBasicParams
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
   int op;
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

   float input1 = AiShaderEvalParamFlt(p_input1);
   float input2 = AiShaderEvalParamFlt(p_input2);

   sg->out.FLT() = 0.0f;

   if (data->op == OPERATOR_ADD)
      sg->out.FLT() = input1 + input2;
   else if (data->op == OPERATOR_SUBSTRACT)
      sg->out.FLT() = input1 - input2;
   else if (data->op == OPERATOR_MULTIPLY)
      sg->out.FLT() = input1 * input2;
   else if (data->op == OPERATOR_DIVIDE)
      sg->out.FLT() = input2 != 0.0f ? input1 / input2 : 0.0f;
   else if (data->op == OPERATOR_MINIMUM)
      sg->out.FLT() = AiMin(input1, input2);
   else if (data->op == OPERATOR_MAXIMUM)
      sg->out.FLT() = AiMax(input1, input2);
   else if (data->op == OPERATOR_REMAINDER)
      sg->out.FLT() = fmodf(input1, input2);
   else if (data->op == OPERATOR_ARCTAN)
      sg->out.FLT() = atan2f(input1, input2);
}
