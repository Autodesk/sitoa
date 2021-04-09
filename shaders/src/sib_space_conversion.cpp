/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBSpaceConversionMethods);

enum SIBSpaceConversionParams
{
   p_type,
   p_transform,
   p_vector_input,
   p_transform_input
};

enum eInputType
{
   ePOINT,
   eVECTOR,
   eNORMAL
};

enum TransformType
{
   TO_WORLD,
   TO_CAMERA,
   TO_OBJECT,
   FROM_WORLD,
   FROM_CAMERA,
   FROM_OBJECT,
   _INPUT_TRANSFORM
};

node_parameters
{
   AtMatrix m = AiM4Identity();
   AiParameterInt( "type"           , 1 ); //"Point" = 0, "Vector" = 1, "Normal" = 2
   AiParameterInt( "transform"      , 2 ); //"to World" = 0, "to Camera" = 1, "to Object" = 2, "from World" = 3, "from Camera" = 4, "from Object" = 5, "Input Transform" = 6
   AiParameterVec( "vector_input"   , 0.0f , 0.0f , 0.0f );
   AiParameterMtx( "transform_input", m );
}

namespace {

typedef struct 
{
   int type, transform;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->type      = AiNodeGetInt(node, "type");
   data->transform = AiNodeGetInt(node, "transform");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtVector vector_input = AiShaderEvalParamVec(p_vector_input);
   AtMatrix matrix = AiM4Identity();

   switch (data->transform)
   {
      case TO_WORLD:
         break;
      case TO_CAMERA:
         AiWorldToCameraMatrix(AiUniverseGetCamera(), sg->time, matrix);  
         break;
      case TO_OBJECT:
         matrix = sg->Minv;
         break;
      case FROM_WORLD:
         break;
      case FROM_CAMERA:
         AiCameraToWorldMatrix(AiUniverseGetCamera(), sg->time, matrix);
         break;
      case FROM_OBJECT:
         matrix = sg->M;
         break;
      case _INPUT_TRANSFORM:
         matrix = *AiShaderEvalParamMtx(p_transform_input);
         break;
   }

   switch (data->type)
   {
      case ePOINT:
         vector_input = AiM4PointByMatrixMult(matrix, vector_input);
         break;
      case eVECTOR:
         vector_input = AiM4VectorByMatrixMult(matrix, vector_input);
         break;
      case eNORMAL:
         if (data->transform != _INPUT_TRANSFORM)
         {
            AtMatrix inverse_transpose = AiM4Invert(matrix);
            vector_input = AiM4VectorByMatrixTMult(inverse_transpose, vector_input);            
         }
         else
            vector_input = AiM4VectorByMatrixTMult(matrix, vector_input);

         vector_input = AiV3Normalize(vector_input);
         break;
   }
  
   sg->out.VEC() = vector_input;
}
