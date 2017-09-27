/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "map_lookup.h"

AI_SHADER_NODE_EXPORT_METHODS(MapLookupVectorMethods);

enum MapLookupVectorParams
{
   p_map,
   p_factor,
};

node_parameters
{
   AiParameterStr( "map"   , ""   );
   AiParameterFlt( "factor", 1.0f );
}

node_initialize
{
   MapLookupShaderData *data = new MapLookupShaderData;
   AiNodeSetLocalData(node, (void*)data);
}

node_update
{
   MapLookupShaderData *data = (MapLookupShaderData*)AiNodeGetLocalData(node);
   DestroyTextureHandles(data);
   data->userData.clear();
   SetUserData(node, data, "_map");
}

node_finish
{
   MapLookupShaderData *data = (MapLookupShaderData*)AiNodeGetLocalData(node);
   DestroyTextureHandles(data);
   data->userData.clear();
   delete data;
}

shader_evaluate
{
   MapLookupShaderData *data = (MapLookupShaderData*)AiNodeGetLocalData(node);

   sg->out.VEC() = 0.0;
   MapLookupUserData* ud = GetLookupUserData(sg, data);

   CClipData *clipData = ud ? &ud->clip_data : NULL;

   AtString map = ud && !ud->map.empty() ? ud->map : AiShaderEvalParamStr(p_map);
   if (map.empty())
      return;

   float factor = AiShaderEvalParamFlt(p_factor);
   if (factor == 0.0f)
      return;

   AtRGBA color;
   const AtUserParamEntry * paramEntry = AiNodeLookUpUserParameter(sg->Op, map);
   if (!paramEntry)
   {
      // No user data named after map, so it may be a texture map
      // If so, we should find the texture projection exported as user data, or, if not,
      // use the main uv set
      if (clipData && clipData->m_isValid)
      {
         color = clipData->LookupTextureMap(sg);
         sg->out.VEC().x = color.r * factor;
         sg->out.VEC().y = color.g * factor;
         sg->out.VEC().z = color.b * factor;
      }
   }
   else
   {
      int paramType = AiUserParamGetType(paramEntry);
      if (paramType == AI_TYPE_FLOAT)
      {
         // weight map
         float val;
         AiUDataGetFlt(map, val);
         sg->out.VEC() = val * factor;
      }
      else if (paramType == AI_TYPE_RGBA)
      {
         // color map 
         AiUDataGetRGBA(map, color);
         sg->out.VEC().x = color.r * factor;
         sg->out.VEC().y = color.g * factor;
         sg->out.VEC().z = color.b * factor;
      }
   }
}
