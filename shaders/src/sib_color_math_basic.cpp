/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorMathBasicMethods);

#define OPERATOR_ADD        0
#define OPERATOR_SUBSTRACT  1
#define OPERATOR_MULTIPLY   2
#define OPERATOR_DIVIDE     3
#define OPERATOR_MINIMUM    4
#define OPERATOR_MAXIMUM    5

enum SIBColorMathBasicParams
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

   AtRGBA result = AI_RGBA_ZERO;
   result.a = input1.a;

   if (data->op == OPERATOR_ADD)
   {
      result.r = input1.r + input2.r;
      result.g = input1.g + input2.g;
      result.b = input1.b + input2.b;

      if (data->alpha)
         result.a += input2.a;
   }
   else if (data->op == OPERATOR_SUBSTRACT)
   {
      result.r = input1.r - input2.r;
      result.g = input1.g - input2.g;
      result.b = input1.b - input2.b;
      
      if (data->alpha)
         result.a -= input2.a;
   }
   else if (data->op == OPERATOR_MULTIPLY)
   {
      result.r = input1.r * input2.r;
      result.g = input1.g * input2.g;
      result.b = input1.b * input2.b;
      
      if (data->alpha)
         result.a *= input2.a;
   }
   else if (data->op == OPERATOR_DIVIDE)
   {
      result.r = input2.r <= AI_EPSILON ? 1.0f : input1.r / input2.r;
      result.g = input2.g <= AI_EPSILON ? 1.0f : input1.g / input2.g;
      result.b = input2.b <= AI_EPSILON ? 1.0f : input1.b / input2.b;
      
      if (data->alpha && input2.a > AI_EPSILON)
         result.a /= input2.a;
   }
   else if (data->op == OPERATOR_MINIMUM)
   {
      result.r = AiMin(input1.r, input2.r);
      result.g = AiMin(input1.g, input2.g);
      result.b = AiMin(input1.b, input2.b);
      
      if (data->alpha && input2.a < result.a)
         result.a = input2.a;
   }
   else if (data->op == OPERATOR_MAXIMUM)
   {
      result.r = AiMax(input1.r, input2.r);
      result.g = AiMax(input1.g, input2.g);
      result.b = AiMax(input1.b, input2.b);

      if (data->alpha && input2.a > result.a)
         result.a = input2.a;
   }

   // Clamping alpha result (0,1) to avoid the following arnold message
   // [arnold] out-of-range sample alpha in pixel (304,112), alpha=1.115000
   result.a = AiClamp(result.a, 0.0f, 1.0f);

   sg->out.RGBA() = result;
}
