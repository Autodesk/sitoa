/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBVectorMathScalarMethods);

#define OPERATOR_LENGTH_V1          0
#define OPERATOR_DOT_V1_V2          1
#define OPERATOR_DISTANCE_V1_V2     2

enum SIBColorMathBasicParams
{
   p_vector_input1,
   p_vector_input2,
   p_mode
};

node_parameters
{
   AiParameterVec ( "vector_input1", 0.0f, 0.0f, 0.0f );
   AiParameterVec ( "vector_input2", 0.0f, 0.0f, 0.0f );
   AiParameterInt ( "mode"         , 0                );
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

   sg->out.FLT() = 0.0f;

   AtVector vector_input1, vector_input2;

   vector_input1 = AiShaderEvalParamVec(p_vector_input1);
   
   if (data->mode != OPERATOR_LENGTH_V1)
      vector_input2 = AiShaderEvalParamVec(p_vector_input2);

   if (data->mode == OPERATOR_LENGTH_V1)
      sg->out.FLT() = AiV3Length(vector_input1);
   else if (data->mode == OPERATOR_DOT_V1_V2)
      sg->out.FLT() = AiV3Dot(vector_input1, vector_input2);
   else if (data->mode == OPERATOR_DISTANCE_V1_V2)
      sg->out.FLT() = AiV3Dist(vector_input1, vector_input2);
}
