/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsLight.h"
#include "common/ParamsShader.h"
#include "loader/Lights.h"
#include "renderer/IprCreateDestroy.h"
#include "renderer/IprLight.h"
#include "renderer/Renderer.h"


void UpdateLight(const Light &in_xsiLight, double in_frame)
{
   double frame(in_frame);
   // "frame" is use to look up the existing light node (if any). If we are in flythrough mode,
   // the node was created at time flythrough_frame, and never destroyed since then.
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      frame = GetRenderInstance()->GetFlythroughFrame();

   AtNode* lightNode = GetRenderInstance()->NodeMap().GetExportedNode(in_xsiLight, frame);
   // Github #86 - Dynamically create light if it's missing (probably because it was hidden when IPR rendering started)
   if (!lightNode)
   {
      CRefArray lights(1);
      lights[0] = in_xsiLight.GetRef();
      CIprCreateDestroy().CreateLights(lights, in_frame);
   }

   Shader xsiShader = GetConnectedShader(ParAcc_GetParameter(in_xsiLight, L"LightShader"));
   
   CLight* p_light = GetRenderInstance()->LightMap().Find(in_xsiLight, frame);
   if (p_light)
   {
      vector <AtNode*> *p_nodes = p_light->GetAllNodes();
      // GetMessageQueue()->LogMessage(L"--- UpdateLight updating " + (CString)p_nodes->size() + L" light nodes");
      for (vector <AtNode*>::iterator iter = p_nodes->begin(); iter!= p_nodes->end(); iter++)
      {
         LoadLightParameters(*iter, in_xsiLight, xsiShader, iter == p_nodes->begin(), in_frame, true);
         UpdateLightFilters(in_xsiLight, xsiShader, *iter, in_frame);
      }
   }
}


void UpdateLightFilters(const Light &in_xsiLight, const Shader &in_lightShader, AtNode *in_lightNode, double in_frame)
{
   AtNode* filterNode;

   CRefArray filterShaders = CollectFilterShaders(in_lightNode, in_xsiLight, in_lightShader);
   AtArray* filterNodes = AiNodeGetArray(in_lightNode, "filters");
   bool     destroyOk;

   if (filterShaders.GetCount() == 0)
   {
      // no filters in the Softimage light
      if (filterNodes && AiArrayGetNumElements(filterNodes) != 0)
      {
         destroyOk = CUtilities().DestroyNodesArray(filterNodes);
         AiNodeSetArray(in_lightNode, "filters", NULL);    
      }
      return;
   }

   // there were no filters, and the user just connected one
   if (!filterNodes)
   {
      LoadLightFilters(in_lightNode, in_xsiLight, in_lightShader, in_frame);
      filterShaders.Clear();
      return;
   }

   // there were filters, the user just connected or disconnected one
   if ((int)AiArrayGetNumElements(filterNodes) != (int)filterShaders.GetCount())
   {
      destroyOk = CUtilities().DestroyNodesArray(filterNodes);
      AiNodeSetArray(in_lightNode, "filters", NULL);    
      LoadLightFilters(in_lightNode, in_xsiLight, in_lightShader, in_frame);
      filterShaders.Clear();
      return;
   }

   // same number of filters. Check if the order is the same
   for (LONG i=0; i < filterShaders.GetCount(); i++)
   {
      Shader filterShader(filterShaders.GetItem(i));
      filterNode = (AtNode*)AiArrayGetPtr(filterNodes, i);
      CString filterName = CStringUtilities().MakeSItoAName(filterShader, in_frame, L"", false);
      CString thisFilterName = CNodeUtilities().GetName(filterNode);
      if (filterName != thisFilterName)
      {
         destroyOk = CUtilities().DestroyNodesArray(filterNodes);
         AiNodeSetArray(in_lightNode, "filters", NULL);    
         LoadLightFilters(in_lightNode, in_xsiLight, in_lightShader, in_frame);
         filterShaders.Clear();
         return;
      }
   }

   // same filters in both Soft and Arnold, we just have to update the parameters
   for (LONG i=0; i < filterShaders.GetCount(); i++)
   {
      Shader filterShader(filterShaders.GetItem(i));
      filterNode = (AtNode*)AiArrayGetPtr(filterNodes, i);
      CString filterName = CStringUtilities().MakeSItoAName(filterShader, in_frame, L"", false);
      CString thisFilterName = CNodeUtilities().GetName(filterNode);

      LoadShaderParameters(filterNode, filterShader.GetParameters(), in_frame, CRef(), RECURSE_TRUE);

      CString filterType = GetShaderNameFromProgId(filterShader.GetProgID());
      if (filterType == "light_blocker") // special case for the light blockers: load the matrix
         LoadBlockerFilterMatrix(filterNode, filterShader, in_frame);
      else if (filterType == "gobo") // and for the gobo's offset, which in Arnold is a point2 and in Soft 2 floats
         LoadGoboFilterOffsetAndRotate(filterNode, filterShader, in_xsiLight, in_frame);
   }

   filterShaders.Clear();
}


void UpdateLightGroup(const Light &in_xsiLight, double in_frame)
{
   // We've receive an event about a change in the light inclusive/exclusive-ness 
   // If light is not valid, it means the user dragged an object in or out of the 
   // associated models of a light
   if (!in_xsiLight.IsValid())
   {
      // Get lights from scene
      CRefArray lightsArray = Application().GetActiveSceneRoot().FindChildren(L"", siLightPrimType, CStringArray());

      LONG nbLights = lightsArray.GetCount();
      for (LONG i=0; i<nbLights; i++)
      {
         Light xsiLight(lightsArray[i]);

         AtNode* lightNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiLight, in_frame);
         if (lightNode) // pushes the light and builds its objects association set
            GetRenderInstance()->LightMap().Push(lightNode, xsiLight, in_frame);
      }
   }
   else
   {
      // the user changed the inclusive/exclusive-ness of a light. and the owner of the
      // parameter (so the light) is passed as in_xsiLight
      UpdateLight(in_xsiLight, in_frame);
      // Updating Members
      AtNode* lightNode = GetRenderInstance()->NodeMap().GetExportedNode(in_xsiLight, in_frame);
      if (lightNode)
         GetRenderInstance()->LightMap().Push(lightNode, in_xsiLight, in_frame);
   }

   DoFullLightAssociation(in_frame);
}

