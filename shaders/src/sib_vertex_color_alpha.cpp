/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "map_lookup.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBVertexColorAlphaMethods);

enum SIBVertexColorAlphaParams 
{
   p_vprop,
   p_alpha_only,
};

node_parameters
{
   AiParameterStr ("vprop"     , ""   );
   AiParameterBool("alpha_only", false);
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
   SetUserData(node, data, "_vprop");
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

   sg->out.RGBA() = 0.0f;
   MapLookupUserData* ud = GetLookupUserData(sg, data);

   AtString map = ud && !ud->map.empty() ? ud->map : AiShaderEvalParamStr(p_vprop);
   if (map.empty())
      return;

   const AtUserParamEntry * paramEntry = AiNodeLookUpUserParameter(sg->Op, map);
   
   if (!paramEntry)
      return;

   // in Soft, you can't select a weight map or a texture map, as for the map_lookup shaders.
   // So, we can safely just check for the expected rgba data type
   if (AiUserParamGetType(paramEntry) == AI_TYPE_RGBA) 
   {
      AtRGBA color;
      AiUDataGetRGBA(map, color);

      if (AiShaderEvalParamBool(p_alpha_only))
         sg->out.RGBA() = color.a;
      else
         sg->out.RGBA() = color;
  
      sg->out.RGBA().a = AiClamp(sg->out.RGBA().a, 0.0f, 1.0f); 
   }
}
