/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/IprCreateDestroy.h"

// Destroy (during ipr) all the filter nodes of a light
//
// @param in_node     the light node
//
void CIprCreateDestroy::DestroyLightFilters(AtNode *in_node)
{
   AtArray* filterNodes = AiNodeGetArray(in_node, "filters");
   if (!filterNodes)
      return;

   for (uint32_t i=0; i<AiArrayGetNumElements(filterNodes); i++)
      AiNodeDestroy((AtNode*)AiArrayGetPtr(filterNodes, i));
}


// Destroy (during ipr) the light nodes that were created for a Softimage light
//
// @param in_value    the Softimage light who originated the Arnold light nodes
// @param in_frame    the frame time
//
void CIprCreateDestroy::DestroyLight(CValue in_value, double in_frame)
{
   CString nodeName = CStringUtilities().MakeSItoAName(in_value, in_frame, L"", false);

   AtNode* node = AiNodeLookUpByName(nodeName.GetAsciiString());
   if (!node)
      return;

   int nodeType = AiNodeEntryGetType(AiNodeGetNodeEntry(node));
   if (nodeType != AI_NODE_LIGHT)
      return;

   // Destroy the light and all its instances
   CString xsiName = in_value.GetAsText();
   CLight* p_light = GetRenderInstance()->LightMap().Find(xsiName, in_frame);
   if (p_light)
   {
      vector <AtNode*> *p_nodes = p_light->GetAllNodes();

      for (vector <AtNode*>::iterator iter = p_nodes->begin(); iter!= p_nodes->end(); iter++)
      {
         if (*iter)
         {
            DestroyLightFilters(*iter);
            GetRenderInstance()->GroupMap().EraseNodeFromAllGroups(*iter);
            AiNodeDestroy(*iter);
         }
      }
      // erase this light from the lights map 
      GetRenderInstance()->LightMap().Erase(xsiName, in_frame);
   }

   // erase the light from the exported nodes map
   GetRenderInstance()->NodeMap().EraseExportedNode(xsiName, in_frame);
}



// Destroy (during ipr) the light nodes that were created for an array of Softimage lights
// We call this function from the OnObjectRemoved, so to re-do the (expensive) light association only once, 
// and not one time for each light
//
// @param in_value    the Softimage lights who originated the Arnold light nodes
// @param in_frame    the frame time
//
void CIprCreateDestroy::DestroyLights(CValueArray in_values, double in_frame)
{
   if (in_values.GetCount() == 0)
      return;

   // before deleting, check if there was some light association. This must be done here,
   // and NOT after the following block, that modifies the light map member. For example,
   // in case we have just one inclusive light and we delete it, the block will erase the
   // light node from the light map. As a consequence, no inclusive/exclusive member will
   // be found, if AtLeastOneLightHasMembers is run after the block.
   // This would result in the light association not being rebuild for the shapes, that 
   // would then still point to a light_group with light memeber that were destroy,
   // causing Arnold to crash during the next ipr render.
   bool doLightAssociation = GetRenderInstance()->LightMap().AtLeastOneLightHasMembers();

   for (LONG i=0; i<in_values.GetCount(); i++)
   {
      CValue value = in_values[i];
      DestroyLight(value, in_frame);
   }

   // and reconstruct the light association in the scene, is some existed before deleting the light
   if (doLightAssociation)
      DoFullLightAssociation(in_frame);
}


// Destroy (during ipr) a light node that was generated because of a Softimage light instance
//
// @param in_node     the node to be deleted
//
void CIprCreateDestroy::DestroyInstancedLight(AtNode *in_node)
{
   GetRenderInstance()->LightMap().EraseNode(in_node);
   GetRenderInstance()->GroupMap().EraseNodeFromAllGroups(in_node);
   AiNodeDestroy(in_node);
}


// Destroy (during ipr) a vector of light nodes, each element generated because of a Softimage light instance
// We call this function from the OnObjectRemoved, so to re-do the (expensive) light association only once, 
// and not one time for each light
//
// @param in_node     the vector of nodes to be deleted
// @param in_frame    the frame time
//
void CIprCreateDestroy::DestroyInstancedLights(vector <AtNode*> &in_nodes, double in_frame)
{
   if (in_nodes.empty())
      return;

   bool doLightAssociation = GetRenderInstance()->LightMap().AtLeastOneLightHasMembers();

   for (vector <AtNode*>::iterator iter = in_nodes.begin(); iter!= in_nodes.end(); iter++)
      DestroyInstancedLight(*iter);

   // and reconstruct the light association in the scene
   if (doLightAssociation)
      DoFullLightAssociation(in_frame);

   // GetRenderInstance()->LightMap().Log();
}


///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

// Set to NULL the mesh pointer of all the mesh lights pointing to the input mesh node
//
// @param in_meshNode     the mesh node
//
void CIprCreateDestroy::ResetMeshLightsObject(AtNode *in_meshNode)
{
   AtNodeIterator *iter = AiUniverseGetNodeIterator(AI_NODE_LIGHT);
   while (!AiNodeIteratorFinished(iter))
   {
      AtNode *lightNode = AiNodeIteratorGetNext(iter);
      if (!lightNode)
         break;

      if (!AiNodeIs(lightNode, ATSTRING::mesh_light))
         continue;

      AtNode* mesh = (AtNode*)AiNodeGetPtr(lightNode, "mesh");
      if (mesh == in_meshNode)
         CNodeSetter::SetPointer(lightNode, "mesh", NULL);
   }

   AiNodeIteratorDestroy(iter);
}


// Destroy (during ipr) an object
//
// @param in_value    the Softimage object name value
// @param in_frame    the frame time
//
void CIprCreateDestroy::DestroyObject(CValue in_value, double in_frame)
{
   CString xsiName = in_value.GetAsText();

   AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(xsiName, in_frame);
   if (!node)
      return;

   int nodeType = AiNodeEntryGetType(AiNodeGetNodeEntry(node));
   if (nodeType != AI_NODE_SHAPE)
      return;

   // get the name BEFORE we destroy it
   CString nodeName = CNodeUtilities().GetName(node);

   // also, before continuing, we must erase the mesh_lights that point to this node, else Arnold crashes
   ResetMeshLightsObject(node);

   // erase this object from the map of the exported objects
   GetRenderInstance()->NodeMap().EraseExportedNode(xsiName, in_frame);
   // erase this object from the map of the exported groups
   GetRenderInstance()->GroupMap().EraseNodeFromAllGroups(node);
   // erase this object from all the associated objects of all the lights
   GetRenderInstance()->LightMap().EraseAssociatedObject(xsiName);
   // now we can safely destroy
   // GetMessageQueue()->LogMessage(L"DestroyObject " + nodeName);
   AiNodeDestroy(node);

   // erase all the ginstances and clones pointing to this node
   ULONG dotPos = nodeName.FindString(L".SItoA."); // should always be the case   
   // cut out whatever follows .SItoA., so that we also find all the time instances,
   // that will have a time in the name other than in_frame
   if (dotPos != UINT_MAX)
      nodeName = nodeName.GetSubString(0, dotPos + 7);

   vector <AtNode*> ginstances = CNodeUtilities().GetInstancesOf(nodeName);
   vector <AtNode*>::iterator iter;

   for (iter=ginstances.begin(); iter!=ginstances.end(); iter++)
   {
      GetRenderInstance()->GroupMap().EraseNodeFromAllGroups(*iter);

      CString nodeName = CNodeUtilities().GetName(*iter);
      // GetMessageQueue()->LogMessage(L"DestroyObject " + nodeName);
      GetRenderInstance()->NodeMap().EraseExportedNode(*iter);
      AiNodeDestroy(*iter);
   }
}


// Destroy (during ipr) an array of object
//
// @param in_value    the array of the Softimage objects names values
// @param in_frame    the frame time
//
void CIprCreateDestroy::DestroyObjects(CValueArray in_value, double in_frame)
{
   if (in_value.GetCount() == 0)
      return;

   for (LONG i=0; i<in_value.GetCount(); i++)
   {
      CValue value = in_value[i];
      DestroyObject(value, in_frame);
   }
}

// This one destroys the nodes associated to a CGroup that survived the other erasing calls.
// For example, when a hair object is deleted, the object is not found as exported, nor as an instance.
// Instead, since each chunk is exported as a curves, and the curves nodes belong to a CGroup keyed
// by the hair object name, then it must be deleted by looking up the CGroup map
//
// @param in_names    the names of the Softimage objects that were deleted 
// @param in_frame    the frame time
//
void CIprCreateDestroy::DestroyGroupObjects(CStringArray &in_names, double in_frame)
{
   AtNode *node;
   vector <AtNode*> nodes;
   vector <AtNode*> *tempV;
   vector <AtNode*> ::iterator nodeIter;

   for (LONG i=0; i<in_names.GetCount(); i++)
   {
      // GetMessageQueue()->LogMessage("Unresolved: " + in_names[i]);
      if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, in_names[i], in_frame))
      {
         // GetMessageQueue()->LogMessage("Destroying group " + in_names[i]);
         nodes = *tempV; // always work with a copy of the nodes array
         for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
         {
            node = *nodeIter;
            // get the name BEFORE we destroy it
            CString nodeName = CNodeUtilities().GetName(node);
            GetRenderInstance()->GroupMap().EraseNodeFromAllGroups(node);
            GetRenderInstance()->NodeMap().EraseExportedNode(node);
            AiNodeDestroy(node);

            // erase all the ginstances and clones pointing to this node
            vector <AtNode*> ginstances = CNodeUtilities().GetInstancesOf(nodeName);
            vector <AtNode*>::iterator iter;

            for (iter=ginstances.begin(); iter!=ginstances.end(); iter++)
            {
               // CString nodeName = CNodeUtilities().GetName(*iter);
               GetRenderInstance()->GroupMap().EraseNodeFromAllGroups(*iter);
               GetRenderInstance()->NodeMap().EraseExportedNode(*iter);
               AiNodeDestroy(*iter);
            }
         }
         GetRenderInstance()->GroupMap().EraseGroup(in_names[i], in_frame);
      }
   }
}



// Create (during ipr) a set of light nodes for a newly created Softimage light set (for example because the user loaded a model made of lights)
// We call this function from the OnObjectRemoved, so to re-do the (expensive) light association only once, 
// and not one time for each light
//
// @param in_lights           the new Softimage lights
// @param in_frame            the frame time
//
void CIprCreateDestroy::CreateLights(const CRefArray &in_lights, double in_frame)
{
   LONG nbLights = in_lights.GetCount();
   if (nbLights == 0)
      return;

   bool doLightAssociation = GetRenderInstance()->LightMap().AtLeastOneLightHasMembers();

   bool creationOk(false);
   for (LONG i=0; i<nbLights; i++)
   {
      Light light(in_lights[i]);
      if (!light.IsValid())
         continue;

      // Be sure that the light does not exist before creating it.
      // For example, when a light exist and we add a user_option property
      // to it, the dirty list returns the light as well as one of the item, so it is
      // interpreted by ProcessRegion as a new light
      if (!GetRenderInstance()->NodeMap().GetExportedNode(light, in_frame))
      {
         LoadSingleLight(light, in_frame);
         creationOk = true;
      }
   }

   // and reconstruct the light association in the scene
   if (doLightAssociation && creationOk)
   {
      // GetRenderInstance()->LightMap().Log();
      DoFullLightAssociation(in_frame);
   }
}


// Create (during ipr) a set of mesh objects
//
// @param in_objects          the new Softimage objects
// @param in_frame            the frame time
//
void CIprCreateDestroy::CreateObjects(const CRefArray &in_objects, double in_frame)
{
   LONG nbObjects = in_objects.GetCount();
   if (nbObjects == 0)
      return;

   for (LONG i=0; i<nbObjects; i++)
   {
      X3DObject object(in_objects[i]);
      CRefArray dummyArray;
      LoadSinglePolymesh(object, in_frame, dummyArray, false);
   }
}


// Create (during ipr) a set of hair objects
//
// @param in_objects          the new Softimage objects
// @param in_frame            the frame time
//
void CIprCreateDestroy::CreateHairs(const CRefArray &in_objects, double in_frame)
{
   LONG nbObjects = in_objects.GetCount();
   if (nbObjects == 0)
      return;

   for (LONG i=0; i<nbObjects; i++)
   {
      X3DObject object(in_objects[i]);
      LoadSingleHair(object, in_frame);
   }
}

