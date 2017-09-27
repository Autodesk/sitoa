/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorMathUnaryMethods);

#define OPERATOR_ABSOLUTE   0
#define OPERATOR_NEGATE     1
#define OPERATOR_INVERT     2

enum SIBColorMathUnaryParams
{
   p_input,
   p_op,
   p_alpha
};

node_parameters
{
   AiParameterRGBA( "input", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterInt ( "op"   , 0 );
   AiParameterBool( "alpha", false );
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

   AtRGBA input = AiShaderEvalParamRGBA(p_input);

   AtRGBA result = AI_RGBA_ZERO;
   result.a = input.a;

   if (data->op == OPERATOR_ABSOLUTE)
   {
      result = AiColorABS(input);
      if (data->alpha)
         result.a = std::abs(result.a);
   }
   else if (data->op == OPERATOR_NEGATE)
   {
      result.r = -input.r;
      result.g = -input.g;
      result.b = -input.b;
      if (data->alpha)
        result.a = -result.a;
   }
   else if (data->op == OPERATOR_INVERT)
   {
      result.r = 1.0f - input.r;
      result.g = 1.0f - input.g;
      result.b = 1.0f - input.b;
      if (data->alpha)
         result.a = 1.0f - result.a;
   }

   // Clamping alpha result (0,1) to avoid the following arnold message
   // [arnold] out-of-range sample alpha in pixel (304,112), alpha=1.115000
   result.a = AiClamp(result.a, 0.0f, 1.0f);

   sg->out.RGBA() = result;
}
