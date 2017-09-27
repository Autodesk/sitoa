/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarMathUnaryMethods);

#define OPERATOR_ABSOLUTE   0
#define OPERATOR_NEGATE     1
#define OPERATOR_INVERT     2
#define OPERATOR_COS        3
#define OPERATOR_SIN        4
#define OPERATOR_TAN        5
#define OPERATOR_ARCCOS     6
#define OPERATOR_ARCSIN     7
#define OPERATOR_ARCTAN     8
#define OPERATOR_LOG        9
#define OPERATOR_EXP        10
#define OPERATOR_SQRT       11
#define OPERATOR_FLOOR      12
#define OPERATOR_CEIL       13

enum SIBColorMathUnaryParams
{
   p_input,
   p_op
};

node_parameters
{
   AiParameterFlt ( "input", 0.0f );
   AiParameterInt ( "op"   , 0    );
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

   float input = AiShaderEvalParamFlt(p_input);

   sg->out.FLT() = 0.0f;

   if (data->op == OPERATOR_ABSOLUTE)
      sg->out.FLT() = std::abs(input);
   else if (data->op == OPERATOR_NEGATE)
      sg->out.FLT() = -input;
   else if (data->op == OPERATOR_INVERT)
      sg->out.FLT() = 1.0f - input;
   else if (data->op == OPERATOR_COS)
      sg->out.FLT() = cosf(input);
   else if (data->op == OPERATOR_SIN)
      sg->out.FLT() = sinf(input);
   else if (data->op == OPERATOR_TAN)
      sg->out.FLT() = tanf(input);
   else if (data->op == OPERATOR_ARCCOS)
      sg->out.FLT() = acosf(input);
   else if (data->op == OPERATOR_ARCSIN)
      sg->out.FLT() = asinf(input);
   else if (data->op == OPERATOR_ARCTAN)
      sg->out.FLT() = atanf(input);
   else if (data->op == OPERATOR_LOG)
      sg->out.FLT() = logf(input);
   else if (data->op == OPERATOR_EXP)
      sg->out.FLT() = expf(input);
   else if (data->op == OPERATOR_SQRT)
      sg->out.FLT() = sqrtf(input);
   else if (data->op == OPERATOR_FLOOR)
      sg->out.FLT() = floorf(input);
   else if (data->op == OPERATOR_CEIL)
      sg->out.FLT() = ceilf(input);
}
