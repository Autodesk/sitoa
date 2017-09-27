/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include <string>
#include <cstdio>

#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBTexprojLookupMethods);

enum sib_texproj_lookupParams 
{
   p_tspace_id,
   p_repeats,
   p_alt_x,
   p_alt_y,
   p_alt_z,
   p_min,
   p_max,
   p_step,
   p_factor,
   p_torus_u,
   p_torus_v,
};

node_parameters
{
   AiParameterStr ( "tspace_id", "" );
   AiParameterVec ( "repeats"  , 1.0f ,1.0f ,1.0f );
   AiParameterBool( "alt_x"    , false);
   AiParameterBool( "alt_y"    , false);
   AiParameterBool( "alt_z"    , false);
   AiParameterVec ( "min"      , 0.0f, 0.0f, 0.0f );
   AiParameterVec ( "max"      , 1.0f, 1.0f, 1.0f );
   AiParameterVec ( "step"     , 0.001f, 0.001f, 0.001f );   // Not Implemented 
   AiParameterFlt ( "factor"   , 5.0f );                     // Not Implemented 
   AiParameterBool( "torus_u"  , false );                 
   AiParameterBool( "torus_v"  , false );                 
}

namespace {

typedef struct 
{
   AtString    tspace_id;
   AtString    projection_wrap;
   bool        alt_x, alt_y;
   bool        torus_u, torus_v;
} ShaderData;

}

node_initialize
{
   ShaderData* data = (ShaderData*)AiMalloc(sizeof(ShaderData));
   AiNodeSetLocalData(node, data);
}

node_update
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
   data->tspace_id = AiNodeGetStr(node, "tspace_id");
   char *s = (char*)alloca(data->tspace_id.length() + 6);
   sprintf(s, "%s_wrap", data->tspace_id.c_str());
   data->projection_wrap = AtString(s);

   data->alt_x   = AiNodeGetBool(node, "alt_x");
   data->alt_y   = AiNodeGetBool(node, "alt_y");
   data->torus_u = AiNodeGetBool(node, "torus_u");
   data->torus_v = AiNodeGetBool(node, "torus_v");
}

node_finish
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
   AiFree(data);
}

shader_evaluate
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

   bool wrap_u(false), wrap_v(false);

   AtArray* wrap_settings;
   if (AiUDataGetArray(data->projection_wrap, wrap_settings))
   {
      wrap_u = AiArrayGetBool(wrap_settings, 0);
      wrap_v = AiArrayGetBool(wrap_settings, 1);
   }

   float u(sg->u), v(sg->v);

   AtVector2 uvPoint;
   AtVector  uvwPoint;
   if (AiUDataGetVec2(data->tspace_id, uvPoint))
   {
      u = uvPoint.x;
      v = uvPoint.y;
      // we don't care about UV derivatives here, this shader just returns UVs
   }
   // note that for this shader we don't maintain a per-shader/per-object map, as we do for
   // other frequently-used shaders such as image_explicit. So, it's ok wasting a little time
   // here in the evaluate to check for the user data type, instead of caching the check.
   // In any case, we check for point3 (camera projection) only if the previous (much more likely,
   // because positive for any 2d data) check failed 
   else if (AiUDataGetVec(data->tspace_id, uvwPoint))
   {
      // homogenous coordinates from camera projection, divide u and v by w
      u = uvwPoint.x / uvwPoint.z;
      v = uvwPoint.y / uvwPoint.z;
   }

   AtVector repeats = AiShaderEvalParamVec(p_repeats);
   AtVector min     = AiShaderEvalParamVec(p_min);
   AtVector max     = AiShaderEvalParamVec(p_max);

   compute_uv(u, v, repeats, min, max, wrap_u || data->torus_u, wrap_v || data->torus_v, data->alt_x, data->alt_y);

   // Return the computed coordinates
   sg->out.VEC().x = u;
   sg->out.VEC().y = v;
   sg->out.VEC().z = 0.0f;
}
