/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(mib_texture_remapMethods);

enum mib_texture_remapParams 
{
   p_input,
   p_transform,
   p_repeat,
   p_alt_x,
   p_alt_y,
   p_alt_z,
   p_torus_x,
   p_torus_y,
   p_torus_z,
   p_min,
   p_max,
   p_offset,
};

node_parameters
{
   AtMatrix m = AiM4Identity();
   AiParameterVec ("input"     , 0.0f, 0.0f, 0.0f);
   AiParameterMtx ("transform" , m); 
   AiParameterVec ("repeat"    , 1.0f, 1.0f, 1.0f);
   AiParameterBool("alt_x"     , false);
   AiParameterBool("alt_y"     , false);
   AiParameterBool("alt_z"     , false);
   AiParameterBool("torus_x"   , false);                    
   AiParameterBool("torus_y"   , false);                    
   AiParameterBool("torus_z"   , false);
   AiParameterVec ("min"       , 0.0f, 0.0f, 0.0f);
   AiParameterVec ("max"       , 1.0f, 1.0f, 1.0f);
   AiParameterVec ("offset"    , 0.0f, 0.0f, 0.0f);  
}

namespace {

typedef struct 
{
   bool        alt_x, alt_y, alt_z;
   bool        torus_x, torus_y, torus_z;
} ShaderData;

}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->alt_x = AiNodeGetBool(node, "alt_x");
   data->alt_y = AiNodeGetBool(node, "alt_y");
   data->alt_z = AiNodeGetBool(node, "alt_z");
   data->torus_x = AiNodeGetBool(node, "torus_x");
   data->torus_y = AiNodeGetBool(node, "torus_y");
   data->torus_z = AiNodeGetBool(node, "torus_z");
}

node_finish
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   AtVector input = AiShaderEvalParamVec(p_input);
   AtMatrix *matrix = AiShaderEvalParamMtx(p_transform);

   input = AiM4PointByMatrixMult(*matrix, input);

   float u = input.x;
   float v = input.y;
   float w = input.z;

   // Repeats, Alternates & UVRemap
   AtVector repeats = AiShaderEvalParamVec(p_repeat);
   AtVector min     = AiShaderEvalParamVec(p_min);
   AtVector max     = AiShaderEvalParamVec(p_max);
   AtVector offset  = AiShaderEvalParamVec(p_offset);

   compute_uvw(u, v, w, repeats, min, max, data->torus_x, data->torus_y, data->torus_z, data->alt_x, data->alt_y, data->alt_z, offset);

   sg->out.VEC().x = u;
   sg->out.VEC().y = v;
   sg->out.VEC().z = w;
}

