/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarMathExponentMethods);

#define OPERATOR_EXPONENT   0
#define OPERATOR_LOGARITHM  1
#define OPERATOR_BIAS       2
#define OPERATOR_GAIN       3

enum SIBScalarMathExponentParams
{
   p_input,
   p_factor,
   p_op
};

node_parameters
{
   AiParameterFlt ( "input" , 0.0f );
   AiParameterFlt ( "factor", 1.0f );
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

   float input = AiShaderEvalParamFlt(p_input);
   float factor = AiShaderEvalParamFlt(p_factor);

   sg->out.FLT() = 0.0f;

   if (data->op == OPERATOR_EXPONENT)
      sg->out.FLT() = powf(input, factor);
   else if (data->op == OPERATOR_LOGARITHM)
   {
      if (factor <= 0.0f || input <= 0.0f || factor == 1.0f)
         sg->out.FLT() = input;
      else
      {
         AtVector2 numdenom;
         numdenom.x = factor < AI_EPSILON ? AI_BIG : logf(factor);
         numdenom.y = input  < AI_EPSILON ? AI_BIG : logf(input);         
         sg->out.FLT() = numdenom.y / numdenom.x;
      }
   }
   else if (data->op == OPERATOR_BIAS)
      sg->out.FLT() = AiBias(input, factor);
   else if (data->op == OPERATOR_GAIN)
      sg->out.FLT() = AiBias(input, factor);
}
