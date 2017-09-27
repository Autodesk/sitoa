/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBVectorMathVectorMethods);

#define OPERATOR_NEGATE_V1          0
#define OPERATOR_V1_PLUS_V2         1
#define OPERATOR_V1_MINUS_V2        8
#define OPERATOR_V1_CROSS_V2        2
#define OPERATOR_NORMALIZE_V1       3
#define OPERATOR_MINIMUM_V1_V2      4
#define OPERATOR_MAXIMUM_V1_V2      5
#define OPERATOR_V1_MULT_SCALAR     6
#define OPERATOR_V1_DIV_SCALAR      7

enum SIBColorMathVectorParams
{
   p_vector_input1,
   p_vector_input2,
   p_mode,
   p_scalar_input1
};

node_parameters
{
   AiParameterVec ( "vector_input1", 0.0f, 0.0f, 0.0f );
   AiParameterVec ( "vector_input2", 0.0f, 0.0f, 0.0f );
   AiParameterInt ( "mode"         , 0                );
   AiParameterFlt ( "scalar_input1", 0.0f             );
}

namespace {

typedef struct 
{
   int mode;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->mode = AiNodeGetInt(node, "mode");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   sg->out.VEC() = AI_V3_ZERO;

   AtVector vector_input1, vector_input2;
   vector_input1 = AiShaderEvalParamVec(p_vector_input1);

    if (data->mode == OPERATOR_V1_PLUS_V2 || data->mode == OPERATOR_V1_MINUS_V2 || 
        data->mode == OPERATOR_V1_CROSS_V2 || data->mode == OPERATOR_MINIMUM_V1_V2 ||
        data->mode == OPERATOR_MAXIMUM_V1_V2)
      vector_input2 = AiShaderEvalParamVec(p_vector_input2);

    if (data->mode == OPERATOR_NEGATE_V1)
        sg->out.VEC() = -vector_input1;
    else if (data->mode == OPERATOR_V1_PLUS_V2)
        sg->out.VEC() = vector_input1 + vector_input2;
    else if (data->mode == OPERATOR_V1_MINUS_V2)
        sg->out.VEC() = vector_input1 - vector_input2;
    else if (data->mode == OPERATOR_V1_CROSS_V2)
        sg->out.VEC() = AiV3Cross(vector_input1, vector_input2);
    else if (data->mode == OPERATOR_NORMALIZE_V1)
        sg->out.VEC() = AiV3Normalize(vector_input1);
    else if (data->mode == OPERATOR_MINIMUM_V1_V2)
        sg->out.VEC() = AiV3Min(vector_input1, vector_input2);
    else if (data->mode == OPERATOR_MAXIMUM_V1_V2)
        sg->out.VEC() = AiV3Max(vector_input1, vector_input2);
    else if (data->mode == OPERATOR_V1_MULT_SCALAR)
        sg->out.VEC() = vector_input1 * AiShaderEvalParamFlt(p_scalar_input1);
    else if (data->mode == OPERATOR_V1_DIV_SCALAR)
    {
        float scalar_input = AiShaderEvalParamFlt(p_scalar_input1);
        scalar_input = scalar_input <= AI_EPSILON ? 1.0f : 1.0f / scalar_input; 
        sg->out.VEC() = vector_input1 * scalar_input;
    }
}
