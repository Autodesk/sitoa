/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include <cstring>
#include <cstdio>

#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(txt3d_cellular_v3Methods);

enum txt3d_cellular_v3_params
{
   p_color1,
   p_color2,
   p_alt_x,
   p_alt_y,
   p_alt_z,
   p_repeats,
   p_min,
   p_max,
   p_tspace_id,
   p_step,
   p_factor,
   p_torus_u,
   p_torus_v,
   p_alpha,
   p_bump_inuse,
   p_alpha_output,
   p_alpha_factor
};

node_parameters
{
   AiParameterRGBA( "color1"      , 1.0f, 1.0f, 1.0f , 1.0f );
   AiParameterRGBA( "color2"      , 0.0f, 0.0f, 0.0f , 1.0f);   
   AiParameterBool( "alt_x"       , false );
   AiParameterBool( "alt_y"       , false );
   AiParameterBool( "alt_z"       , false );
   AiParameterVec ( "repeats"     , 1.0f ,1.0f ,1.0f );  
   AiParameterVec ( "min"         , 0.0f, 0.0f, 0.0f );
   AiParameterVec ( "max"         , 20.0f, 20.0f, 20.0f );
   AiParameterStr ( "tspace_id"   , "" );
   AiParameterVec ( "step"        , 0.001f, 0.001f, 0.001f );   // Not Implemented 
   AiParameterFlt ( "factor"      , 5.0f );                     // Not Implemented 
   AiParameterBool( "torus_u"     , false );                    
   AiParameterBool( "torus_v"     , false );                    
   AiParameterBool( "alpha"       , false );                    // Not Implemented 
   AiParameterBool( "bump_inuse"  , false );                    // Not Implemented 
   AiParameterBool( "alpha_output", false );
   AiParameterFlt ( "alpha_factor", 1.0f );
}

namespace {

typedef struct 
{
   AtString    tspace_id;
   AtString    projection_wrap;
   bool        alt_x, alt_y;
   bool        torus_u, torus_v;
   bool        alpha_output;
} ShaderData;

}

node_initialize
{
   ShaderData* data = (ShaderData*)AiMalloc(sizeof(ShaderData));
   AiNodeSetLocalData(node, data);
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->tspace_id = AiNodeGetStr(node, "tspace_id");
   char* s = (char*)alloca(data->tspace_id.length() + 6);
   sprintf(s, "%s_wrap", data->tspace_id.c_str());
   data->projection_wrap = AtString(s);

   data->alt_x        = AiNodeGetBool(node, "alt_x");
   data->alt_y        = AiNodeGetBool(node, "alt_y");
   data->torus_u      = AiNodeGetBool(node, "torus_u");
   data->torus_v      = AiNodeGetBool(node, "torus_v");
   data->alpha_output = AiNodeGetBool(node, "alpha_output");
}

node_finish
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*) AiNodeGetLocalData(node);

   bool wrap_u = false, wrap_v = false;
   AtArray* wrap_settings;
   if (AiUDataGetArray(data->projection_wrap, wrap_settings))
   {
      wrap_u = AiArrayGetBool(wrap_settings, 0);
      wrap_v = AiArrayGetBool(wrap_settings, 1);
   }

   AtVector coord = AtVector(sg->u, sg->v, 0.0f);

   if (data->tspace_id && data->tspace_id[0] != '\0')
   {
      AtVector2 uvPoint;
      if (AiUDataGetVec2(data->tspace_id, uvPoint))
      {
         coord.x = uvPoint.x;
         coord.y = uvPoint.y;
      }

      AtVector repeats = AiShaderEvalParamVec(p_repeats);
      AtVector min     = AiShaderEvalParamVec(p_min);
      AtVector max     = AiShaderEvalParamVec(p_max);

      compute_uv(coord.x, coord.y, repeats, min, max, wrap_u || data->torus_u, wrap_v || data->torus_v, data->alt_x, data->alt_y);
   }
   else
      coord = sg->P;

   AtRGBA color1 = AiShaderEvalParamRGBA(p_color1);
   AtRGBA color2 = AiShaderEvalParamRGBA(p_color2);
   // Values to approximate the original basis function of mental ray
   float magic_scale = 3.0f;
   float magic_remap = 0.35f;

   coord *= magic_scale;

   float cellular_f1[5];
   AiCellular(coord, 1, 1, 1.96f, 1.0f, cellular_f1);
   
   AtRGBA result = AiLerp(AiClamp(cellular_f1[0] * magic_remap, 0.0f, 1.0f), color1, color2);

   if (data->alpha_output)
   {
      float alpha_factor = AiShaderEvalParamFlt(p_alpha_factor);
      sg->out.RGBA().r = sg->out.RGBA().g = sg->out.RGBA().b = sg->out.RGBA().a = result.a * alpha_factor;
   }
   else
      sg->out.RGBA() = result;   
}

