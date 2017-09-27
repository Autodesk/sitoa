/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

#define OPERATOR_EXPONENT   0
#define OPERATOR_LOGARITHM  1
#define OPERATOR_BIAS       2
#define OPERATOR_GAIN       3

AI_SHADER_NODE_EXPORT_METHODS(SIBColorMathExponentMethods);

enum SIBColorMathExponentParams
{
   p_input,
   p_factor,
   p_op,
   p_alpha
};

node_parameters
{
   AiParameterRGBA( "input"     , 1.0f, 1.0f, 1.0f, 0.0f );
   AiParameterRGBA( "factor"    , 1.0f, 1.0f, 1.0f, 0.0f );
   AiParameterInt ( "op"        , 0 );
   AiParameterBool( "alpha"     , false );
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

   AtRGBA input  = AiShaderEvalParamRGBA(p_input);
   AtRGBA factor = AiShaderEvalParamRGBA(p_factor);

   AtRGBA result = AI_RGBA_ZERO;
   result.a = input.a;

   if (data->op == OPERATOR_EXPONENT)
   {
      result.r = factor.r > 0.0f ? powf(input.r, factor.r) : input.r;
      result.g = factor.g > 0.0f ? powf(input.g, factor.g) : input.g;
      result.b = factor.b > 0.0f ? powf(input.b, factor.b) : input.b;
      
      if (data->alpha)
         result.a = factor.a > 0.0f ? powf(input.a, factor.a) : input.a;
   }
   else if (data->op == OPERATOR_LOGARITHM)
   {
      AtVector2 numdenom;

      if (factor.r < 0.0f || input.r < 0.0f || factor.r == 1.0f)
         result.r = input.r;
      else
      {
         numdenom.x = factor.r < AI_EPSILON ? AI_BIG : logf(factor.r);
         numdenom.y = input.r < AI_EPSILON ? AI_BIG : logf(input.r);
         result.r = numdenom.y / numdenom.x;
      }
       
      if (factor.g < 0.0f || input.g < 0.0f || factor.g == 1.0f)
         result.g = input.g;
      else
      {
         numdenom.x = factor.g < AI_EPSILON ? AI_BIG : logf(factor.g);
         numdenom.y = input.g < AI_EPSILON ? AI_BIG : logf(input.g);
         result.g = numdenom.y / numdenom.x;
      }
      
      if (factor.b < 0.0f || input.b < 0.0f || factor.b == 1.0f)
          result.b = input.b;
      else
      {
         numdenom.x = factor.b < AI_EPSILON ? AI_BIG : logf(factor.b);
         numdenom.y = input.b < AI_EPSILON ? AI_BIG : logf(input.b);
         result.b = numdenom.y / numdenom.x;
      }

      if (data->alpha)
      {
         if (factor.a < 0.0f || input.a < 0.0f || factor.a == 1.0f)
            result.a = input.a;
         else
         {
            numdenom.x = factor.a < AI_EPSILON ? AI_BIG : logf(factor.a);
            numdenom.y = input.a < AI_EPSILON ? AI_BIG : logf(input.a);
            result.a = numdenom.y / numdenom.x;
         }
      }
   }
   else if (data->op == OPERATOR_BIAS)
   {
      result.r = AiBias(input.r, factor.r);
      result.g = AiBias(input.g, factor.g);
      result.b = AiBias(input.b, factor.b);

      if (data->alpha)
         result.a = AiBias(input.a, factor.a);
    }
    else if (data->op == OPERATOR_GAIN)
    {
      result.r = AiGain(input.r, factor.r);
      result.g = AiGain(input.g, factor.g);
      result.b = AiGain(input.b, factor.b);

      if (data->alpha)
         result.a = AiGain(input.a, factor.a);
    }

   // Clamping alpha result (0,1) to avoid the following arnold message
   // [arnold] out-of-range sample alpha in pixel (304,112), alpha=1.115000
   result.a = AiClamp(result.a, 0.0f, 1.0f);

   sg->out.RGBA() = result;
}
