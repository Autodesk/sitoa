/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

#define OPERATOR_IS_EQUAL_TO                0
#define OPERATOR_IS_LESS_THAN               1
#define OPERATOR_IS_GREATER_THAN            2
#define OPERATOR_IS_LESS_OR_EQUAL_TO        3
#define OPERATOR_IS_GREATER_OR_EQUAL_TO     4

AI_SHADER_NODE_EXPORT_METHODS(SIBColorMathLogicMethods);

enum SIBColorMathLogicParams
{
   p_input1,
   p_input2,
   p_op,
   p_alpha
};

node_parameters
{
   AiParameterRGBA( "input1", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "input2", 1.0f, 1.0f, 1.0f, 0.0f );
   AiParameterInt ( "op"    , 0 );
   AiParameterBool( "alpha" , false );
}

namespace {

typedef struct 
{
   int  op;
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
   data->op    = AiNodeGetInt(node, "op");
   data->alpha = AiNodeGetBool(node, "alpha");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGBA input1 = AiShaderEvalParamRGBA(p_input1);
   AtRGBA input2 = AiShaderEvalParamRGBA(p_input2);

   bool result = false;

   if (data->op == OPERATOR_IS_EQUAL_TO)
   {
      result = input1 == input2;

      if(data->alpha && result)
         result = input1.a == input2.a;
   }
   else if (data->op == OPERATOR_IS_LESS_THAN)
   {
      result = input1.r < input2.r && input1.g < input2.g && input1.b < input2.b;

      if (data->alpha && result)
         result = (input1.a<input2.a);
   }
   else if (data->op == OPERATOR_IS_GREATER_THAN)
   {
      result = input1.r > input2.r && input1.g > input2.g && input1.b > input2.b;

      if (data->alpha && result)
         result = input1.a > input2.a;
   }
   else if (data->op == OPERATOR_IS_LESS_OR_EQUAL_TO)
   {
      result = input1.r <= input2.r && input1.g <= input2.g && input1.b <= input2.b;

      if (data->alpha && result)
         result = input1.a <= input2.a;
   }
   else if (data->op == OPERATOR_IS_GREATER_OR_EQUAL_TO)
   {
      result = input1.r >= input2.r && input1.g >= input2.g && input1.b >= input2.b;

      if (data->alpha && result)
         result = input1.a >= input2.a;
   }

   sg->out.BOOL() = result;
}
