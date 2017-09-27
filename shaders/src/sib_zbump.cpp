/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBZBumpMethods);

enum SIBZBumpParams 
{
   p_bump,
   p_inuse,
   p_input,
   p_scale,
   p_spacing,
};

node_parameters
{
   AiParameterVec ( "bump"   , 0.0f, 0.0f, 0.0f );
   AiParameterBool( "inuse"  , true );
   AiParameterRGB ( "input"  , 0.0f, 0.0f, 0.0f );
   AiParameterFlt ( "scale"  , 0.2f );
   AiParameterFlt ( "spacing", 0.01f );              // Not Implemented
}

namespace {

typedef struct
{
   AtNode* input_connector;
   AtNode* bump_connector;
   bool    connected_before;
   bool    inuse;
} ShaderData;

}

typedef struct 
{
   AtNode *node;
   float scale;
} BumpData;

float BumpFunctionZBump(AtShaderGlobals *sg, void *t)
{
   BumpData* user = (BumpData*)t;
   AtNode*   node = user->node;
   float     scale = user->scale;

   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

   // Compute bump height at P
   AtNode *hfshader = data->input_connector;
   AiShaderEvaluate(hfshader, sg);
   return (sg->out.FLT() - 0.5f) * scale;
}

node_initialize
{
   ShaderData* data = (ShaderData*)AiMalloc(sizeof(ShaderData));
   AiNodeSetLocalData(node, data);
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   data->input_connector = AiNodeGetLink(node, "input");
   data->bump_connector = AiNodeGetLink(node, "bump");
   data->connected_before = AiNodeLookUpUserParameter(node, "connected_before") ? true : false;
   data->inuse = AiNodeGetBool(node, "inuse");
   // We will declare a parameter indicating this shaders is doing bump map calculations.
   // Other shaders that are doing the bump calculations can ask for this parameter if
   // this shader is attached to avoid 2 bump calculations
   if (!AiNodeLookUpUserParameter(node, "bump_shader"))
      AiNodeDeclare(node, "bump_shader", "constant BOOL");
}

node_finish
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   if (sg->Rt & AI_RAY_SHADOW)
      return; 

   ShaderData   *data = (ShaderData*)AiNodeGetLocalData(node);

   // If disabled, the bump is controlled by whatever is connected to this node's bump input. If nothing is connected, no bump is computed. 
   if (data->inuse)
   {
      if (!data->input_connector) //no bump connected ?
         return;
   
      AtVector original_N = sg->N;
      AtVector original_P = sg->P;

      BumpData bumpData;
      bumpData.scale = AiShaderEvalParamFlt(p_scale);
      bumpData.node = node;
      AtVector bump_N = AiShaderGlobalsEvaluateBump(sg, BumpFunctionZBump, (void*)&bumpData);

      // Restore P
      sg->P = original_P; 
      // Point the new normal in the same direction as the old one
      AiFaceForward(bump_N, original_N);
      // Previous line actually points the normal in the opposite of where it
      // should go because it is written assuming the second input is Rd
      bump_N = -bump_N;
      // Shader is not connected as @before so we will return the calculated normal
      if (!data->connected_before)
         sg->out.VEC() = bump_N;

      else // Shader is connected as @before, modify shaderglobals variables
      {
         // Setting normal
         sg->N = bump_N;
         // Setup Nf to match new N
         sg->Nf = AiFaceViewer(sg);
      }
   }
   else if (data->bump_connector)
      AiShaderEvaluate(data->bump_connector, sg);
}
