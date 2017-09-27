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
#include <vector>
#include <map>

#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(txt2d_scalarimage_explicitMethods);

enum txt2d_scalarimage_explicitParams 
{
   p_tex,
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
   p_alpha,
   p_bump_inuse,
   p_alpha_output,
   p_alpha_factor,
   p_eccmax,
   p_maxminor,
   p_disc_r,
   p_bilinear,
   p_filtered,
   p_bump_filtered,
};

node_parameters
{
   AiParameterRGBA( "tex"          , 0, 0, 0, 0);
   AiParameterStr ( "tspace_id"    , "" );
   AiParameterVec ( "repeats"      , 1.0f ,1.0f ,1.0f );
   AiParameterBool( "alt_x"        , false );
   AiParameterBool( "alt_y"        , false );
   AiParameterBool( "alt_z"        , false );
   AiParameterVec ( "min"          , 0.0f, 0.0f, 0.0f );
   AiParameterVec ( "max"          , 1.0f, 1.0f, 1.0f );
   AiParameterVec ( "step"         , 0.001f, 0.001f, 0.001f );    // Not Implemented 
   AiParameterFlt ( "factor"       , 5.0f );                      // Not Implemented 
   AiParameterBool( "torus_u"      , false );                     // Not Implemented 
   AiParameterBool( "torus_v"      , false );                     // Not Implemented 
   AiParameterBool( "alpha"        , false );                     // Not Implemented 
   AiParameterBool( "bump_inuse"   , false );                     // Not Implemented 
   AiParameterBool( "alpha_output" , false ); 
   AiParameterFlt ( "alpha_factor" , 1.0f ); 
   AiParameterFlt ( "eccmax"       , 20.0f );                     // Not Implemented 
   AiParameterFlt ( "maxminor"     , 16.0f );                     // Not Implemented 
   AiParameterFlt ( "disc_r"       , 0.3f );                      // Not Implemented 
   AiParameterBool( "bilinear"     , true );                      // Not Implemented 
   AiParameterBool( "filtered"     , false );                     // Not Implemented 
   AiParameterBool( "bump_filtered", false );                     // Not Implemented 
}


namespace{

// parameter(s) with instance value
typedef struct
{
   AtString tspace_id;
} UserData;

typedef pair<const string, UserData> ObjectName_UserData_Pair;
typedef map <const string, UserData> ObjectName_UserData_Map;

typedef struct 
{
   AtString tspace_id; // the shader's string attribute
   bool     alt_x, alt_y, torus_u, torus_v;
   bool     alpha_output;

   bool hasUserData;
   ObjectName_UserData_Map userData;
} ShaderData;

}

node_initialize
{
   ShaderData *data = new ShaderData;
   AiNodeSetLocalData(node, (void*)data);
}

node_update
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

   data->tspace_id    = AiNodeGetStr(node, "tspace_id");
   data->alt_x        = AiNodeGetBool(node, "alt_x");
   data->alt_y        = AiNodeGetBool(node, "alt_y");
   data->torus_u      = AiNodeGetBool(node, "torus_u");
   data->torus_v      = AiNodeGetBool(node, "torus_v");
   data->alpha_output = AiNodeGetBool(node, "alpha_output");

   data->userData.clear();

   // collect the list of the names of the objects that have instance parameter
   // values for this shader. We search for user data ending with "_tspace_id".
   // If such a user data exist, its name is made of a object name + "_tspace_id"
   const char* attrName = "_tspace_id";
   vector <string> objNames;
   AtUserParamIterator *iter = AiNodeGetUserParamIterator(node);
   // iterate the shader user data 
   while (!AiUserParamIteratorFinished(iter))
   {
      const AtUserParamEntry *upEntry = AiUserParamIteratorGetNext(iter);
      const char* upName = AiUserParamGetName(upEntry);
      const char* attrStart = strstr(upName, attrName);
      if (attrStart)
      {
         if (strlen(attrStart) == strlen(attrName))
         {
            // found "_tspace_id" as the suffix of the user data name
            size_t objNameLenght = strlen(upName) - strlen(attrStart);
            char* objName = (char*)alloca(objNameLenght + 1);
            memcpy(objName, upName, objNameLenght);
            objName[objNameLenght] = '\0';
            objNames.push_back(string(objName));
         }
      }
   }
   AiUserParamIteratorDestroy(iter);

   data->hasUserData = objNames.size() > 0;
   if (!data->hasUserData)
      return;

   string attr, suffix;
   for (vector <string>::iterator it = objNames.begin(); it != objNames.end(); it++)
   {
      UserData ud;
      // *it is the object for which the shader has parameters with instance values
      AtNode *object = AiNodeLookUpByName(it->c_str());
      // #1388: use the old-fashion tspace_id shader parameter for curves.
      if (object && AiNodeIs(object, AtString("curves")))
         attr = "tspace_id";
      else
      {
         // get the value of the instance parameter
         suffix = "_tspace_id";
         attr = *it + suffix;
      }

      ud.tspace_id = AiNodeGetStr(node, attr.c_str());
      // insert the user data in the map, using the object name as key
      data->userData.insert(ObjectName_UserData_Pair(*it, ud));
   }
}

node_finish
{
   delete (ShaderData*)AiNodeGetLocalData(node);
}

static const char* gWrapSuffix = "_wrap";

shader_evaluate
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
   UserData *ud = NULL;
   if (data->hasUserData)
   {
      // get the object name and find its user data store in the shader data map
      string nodeName(GetShaderOwnerName(sg));
      ObjectName_UserData_Map::iterator it = data->userData.find(nodeName);
      if (it != data->userData.end())
         ud = &it->second;
   }

   AtString tspace_id = ud && !ud->tspace_id.empty() ? ud->tspace_id : data->tspace_id;

   bool wrap_u(false), wrap_v(false);
   AtArray* wrap_settings;
   // #1324. Get the name of the wrapping attribute by the tspace_id exported by the
   // object, and not by the shader's tspace_id parameter
   size_t tspace_id_length = strlen(tspace_id);
   char* projection_wrap = (char*)alloca(tspace_id_length + 6);
   memcpy(projection_wrap, tspace_id, tspace_id_length);
   memcpy(projection_wrap + tspace_id_length, gWrapSuffix, 6);
   if (AiUDataGetArray(AtString(projection_wrap), wrap_settings))
   {
      wrap_u = AiArrayGetBool(wrap_settings, 0);
      wrap_v = AiArrayGetBool(wrap_settings, 1);
   }

   // Grab the original state of the uv and derivatives
   float u(sg->u), v(sg->v);
   float dudx(sg->dudx), dudy(sg->dudy), dvdx(sg->dvdx), dvdy(sg->dvdy);

   AtVector2 uvPoint;
   AtVector  uvwPoint;

   bool getDerivatives(false);
   bool is_homogenous = AiUserParamGetType(AiUDataGetParameter(tspace_id)) == AI_TYPE_VECTOR;
   if (is_homogenous)
   {
      if (AiUDataGetVec(tspace_id, uvwPoint))
      {
         // homogenous coordinates from camera projection, divide u and v by w
         u = uvwPoint.x / uvwPoint.z;
         v = uvwPoint.y / uvwPoint.z;

         AtVector altuv_dx, altuv_dy;
         if (AiUDataGetDxyDerivativesVec(tspace_id, altuv_dx, altuv_dy))
         {
            AtVector dx = uvwPoint + altuv_dx;
            AtVector dy = uvwPoint + altuv_dy;
            dudx = dx.x / dx.z - u;
            dudy = dy.x / dy.z - u;
            dvdx = dx.y / dx.z - v;
            dvdy = dy.y / dy.z - v;
         }
         else
            dudx = dudy = dvdx = dvdy = 0.0f;
      }
   }
   else if (AiUDataGetVec2(tspace_id, uvPoint))
   {
      u = uvPoint.x;
      v = uvPoint.y;
      getDerivatives = true;
   }
   
   if (getDerivatives)
   {
      AtVector2 altuv_dx, altuv_dy;
      if (AiUDataGetDxyDerivativesVec2(tspace_id, altuv_dx, altuv_dy))
      {
         dudx = altuv_dx.x;
         dudy = altuv_dy.x;
         dvdx = altuv_dx.y;
         dvdy = altuv_dy.y;
      }
      else
         dudx = dudy = dvdx = dvdy = 0.0f;
   }

   // Repeats, Alternates & UVRemap
   AtVector repeats = AiShaderEvalParamVec(p_repeats);
   AtVector min     = AiShaderEvalParamVec(p_min);
   AtVector max     = AiShaderEvalParamVec(p_max);

   compute_uv(u, v, repeats, min, max, wrap_u || data->torus_u, wrap_v || data->torus_v, data->alt_x, data->alt_y, dudx, dudy, dvdx, dvdy);
   // For #1578, we must let uv > 1 go, since they could be needed in case of <udim>

   // Backup original UVs and derivatives
   BACKUP_SG_UVS;

   sg->u = u;
   sg->v = v;
   sg->dudx = dudx;
   sg->dudy = dudy;
   sg->dvdx = dvdx;
   sg->dvdy = dvdy;

   // Evaluate texture
   AtRGBA tex = AiShaderEvalParamRGBA(p_tex);
   if (data->alpha_output)
   {
      float alphaFactor = AiShaderEvalParamFlt(p_alpha_factor);
      sg->out.FLT() = tex.a * alphaFactor;
   }
   else
      sg->out.FLT() = (0.299f*tex.r + 0.587f*tex.g + 0.114f*tex.b);

   RESTORE_SG_UVS;
}
