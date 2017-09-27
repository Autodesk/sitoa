/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "common/ParamsLight.h"
#include "common/ParamsShader.h"
#include "loader/Lights.h"
#include "loader/Properties.h"
#include "renderer/IprLight.h"
#include "renderer/Renderer.h"
#include "renderer/RenderTree.h"

#include <xsi_group.h>
#include <xsi_model.h>
#include <xsi_primitive.h>
#include <xsi_version.h>

#include <cstring> 
#include <vector> 

/////////////////////////////////////
/////////////////////////////////////
// CLight
/////////////////////////////////////
/////////////////////////////////////

// = overload
const CLight& CLight::operator=(const CLight& in_arg)
{
   if (this == &in_arg)
      return *this;
   m_hasMembers        = in_arg.m_hasMembers;
   m_isInclusive       = in_arg.m_isInclusive;
   m_nodes             = in_arg.m_nodes;
   m_xsiLight          = in_arg.m_xsiLight;
   m_associatedObjects = in_arg.m_associatedObjects;
   return *this;
}


// Get the associated models group, and set m_hasMembers and m_isInclusive accordingly
//
// @return  true if the groups was found (should always be), else false
//
bool CLight::GetAssociatedModelsGroupSettings()
{
   Primitive lightPrimitive = CObjectUtilities().GetPrimitiveAtCurrentFrame(m_xsiLight);
   CRefArray nestedObjects = lightPrimitive.GetNestedObjects();
   for (LONG i = 0; i< nestedObjects.GetCount(); i++)
   {
      if (nestedObjects[i].GetClassID() == siGroupID)
      {
         m_hasMembers = (Group(nestedObjects[i])).GetMembers().GetCount() > 0;
         m_isInclusive = ParAcc_GetValue(lightPrimitive, L"SelectiveInclusive", DBL_MAX);
         return true;
      }            
   }
 
   return false;
}


// Erase a light node from the nodes belonging to this light
//
// @param in_node    the light node to be erased
//
void CLight::EraseNode(AtNode *in_node)
{
   vector <AtNode*>::iterator iter;
   for (iter= m_nodes.begin(); iter!= m_nodes.end(); iter++)
   {
      if (*iter == in_node)
      {
         m_nodes.erase(iter);
         return;
      }
   }
}


// Add an object's id to the association set
//
// @param in_xsiObject    the object to add
//
void CLight::AddAssociatedObject(X3DObject in_xsiObject)
{
   m_associatedObjects.insert(in_xsiObject.GetFullName());
}


// Do the full objects association for this light
void CLight::DoAssociation()
{
   Primitive lightPrimitive = CObjectUtilities().GetPrimitiveAtCurrentFrame(m_xsiLight);
   CRefArray nestedObjects = lightPrimitive.GetNestedObjects();
   for (LONG i = 0; i< nestedObjects.GetCount(); i++)
   {
      if (nestedObjects[i].GetClassID() == siGroupID)
      {
         Group associatedModels = Group(nestedObjects[i]);
         CRefArray members = associatedModels.GetExpandedMembers();
         for (LONG j=0; j<members.GetCount(); j++)
         {
            CRefArray objects;

            X3DObject obj(members[j]);
            Model model(obj);
            if (model.IsValid())
               objects = GetAllShapesBelowModel(model);
            else
               objects.Add(obj);

            for (LONG k=0; k<objects.GetCount(); k++)
            {
               obj = objects[k];
               if (obj.IsValid())
               {
                  // GetMessageQueue()->LogMsg(L"Pushing " + obj.GetFullName() + L", " + in_xsiLight.GetFullName());
                  AddAssociatedObject(obj);
               }
            }

         }
         break;
      }            
   }
}


// Find an object in the association set
//
// @param in_xsiObject    the object to find
//
// @return                true if the object was found, else false
//
bool CLight::FindAssociatedObject(X3DObject in_xsiObject)
{
   return m_associatedObjects.find(in_xsiObject.GetFullName()) != m_associatedObjects.end();
}


// Erase an object from the association set
//
// @param in_objectName    the object name to be erased
//
void CLight::EraseAssociatedObject(CString &in_objectName)
{
   set <CString>::iterator iter = m_associatedObjects.find(in_objectName);
   if (iter != m_associatedObjects.end())
      m_associatedObjects.erase(iter);
}


// Get the associate objects set pointer
set <CString> *CLight::GetAssociatedObjects()
{
   return &m_associatedObjects;
}


// Clear the association set
void CLight::ClearAssociatedObjects()
{
   m_associatedObjects.clear();
}


// Update the light, when in flythrough mode
//
void CLight::FlythroughUpdate()
{
   UpdateLight(m_xsiLight, GetRenderInstance()->GetFrame());
}


// debug
void CLight::Log()
{
   GetMessageQueue()->LogMsg(L"CLight Log for " + m_xsiLight.GetFullName()); 
   GetMessageQueue()->LogMsg(L" has " + (CString)(int)m_nodes.size() + L" nodes:");
   vector <AtNode*>::iterator vIter;
   for (vIter= m_nodes.begin(); vIter!= m_nodes.end(); vIter++)
   {
      CString name = CNodeUtilities().GetName(*vIter);
      GetMessageQueue()->LogMsg(L"  " + name);
   }
   GetMessageQueue()->LogMsg(L" has " + (CString)(int)m_associatedObjects.size() + L" associated object:");
   set <CString>::iterator setIter;
   for (setIter= m_associatedObjects.begin(); setIter!= m_associatedObjects.end(); setIter++)
      GetMessageQueue()->LogMsg(L"  " + *setIter);
}


/////////////////////////////////////
/////////////////////////////////////
// CLightMap
/////////////////////////////////////
/////////////////////////////////////

// Push into map by CLight and key
//
// @param in_light    light to push
// @param in_key      key
//
void CLightMap::Push(CLight in_light, AtNodeLookupKey in_key)
{
   m_map.insert(pair<AtNodeLookupKey, CLight> (in_key, in_light));
}


// Push into map by arnold node, xsi light and key
//
// @param in_light       the Arnold light node
// @param in_xsiLight    the xsi light
// @param in_key         the key
//
void CLightMap::Push(AtNode *in_light, Light in_xsiLight, AtNodeLookupKey in_key)
{
   CLight light(in_light, in_xsiLight);
   light.GetAssociatedModelsGroupSettings();
   // This gets called on both a scene destroy and on an ipr refresh for the light.
   // In the second case, the light is already in the map, so we just have to update it (a further
   // insert would be ignored by the map) because the ipr could have been called because
   // of a change of the inclusive/exclusive dropdown
   map <AtNodeLookupKey, CLight>::iterator iter = m_map.find(in_key);
   if (iter == m_map.end()) // actually pushing the light
   {
      light.DoAssociation();
      Push(light, in_key);
   }
   else // updating the inc/exc flag, and the associated objects
   {
      iter->second.m_hasMembers  = light.m_hasMembers;
      iter->second.m_isInclusive = light.m_isInclusive;
      iter->second.ClearAssociatedObjects();
      iter->second.DoAssociation();
   }
}


// Push into map by arnold node, xsi light and frame time
//
// @param in_light       the Arnold light node
// @param in_xsiLight    the xsi light
// @param in_frame       the frame time
//
void CLightMap::Push(AtNode *in_light, Light in_xsiLight, double in_frame)
{
   CString name = in_xsiLight.GetFullName();
   Push(in_light, in_xsiLight, AtNodeLookupKey(name, in_frame));
}


// Find the light in the map
//
// @param in_name        the xsi light name
// @param in_frame       the frame time
//
// @return  pointer to the CLight, or NULL if not found
//
CLight* CLightMap::Find(CString &in_name, double in_frame)
{
   map <AtNodeLookupKey, CLight>::iterator iter = m_map.find(AtNodeLookupKey(in_name, in_frame));
   if (iter != m_map.end())
      return &iter->second;

   return NULL;
}


// Find the light in the map
//
// @param in_name        the xsi light
// @param in_frame       the frame time
//
// @return  pointer to the CLight, or NULL if not found
//
CLight* CLightMap::Find(const Light &in_xsiLight, double in_frame)
{
	CString name = in_xsiLight.GetFullName();
   return Find(name, in_frame);
}


// Erase a light in the map
//
// @param in_name        the xsi light name
// @param in_frame       the frame time
//
void CLightMap::Erase(CString &in_name, double in_frame)
{
   map <AtNodeLookupKey, CLight>::iterator iter = m_map.find(AtNodeLookupKey(in_name, in_frame));
   if (iter != m_map.end())
   {
      m_map.erase(iter);
      return;
   }
}


// Loop all the lights and return true if at least one exploits association
//
// @return  the loop result
//
bool CLightMap::AtLeastOneLightHasMembers()
{
   map <AtNodeLookupKey, CLight>::iterator iter;
   for (iter= m_map.begin(); iter!= m_map.end(); iter++)
      if (iter->second.m_hasMembers)
         return true;

   return false;
}


// Return an array of lights node pointers that affect the object
//
// @param in_xsiObj       the xsi object to look for
//
// @return the array of the lights (AtNode*)s influencing the object's lighting
//
AtArray* CLightMap::GetLightGroup(const X3DObject &in_xsiObj)
{
   vector <AtNode*> lightsVector;
   AtArray* out_lightGroup = NULL;

   // check if we have to assign light groups to the object
   if (!AtLeastOneLightHasMembers())
      return NULL;

   map <AtNodeLookupKey, CLight>::iterator it;
   // loop all the lights
   for (it = m_map.begin(); it != m_map.end(); it++)
   {
      CLight *p_light = &it->second;
      // get the Arnold light node and all its duplications for this light
      vector <AtNode*> *p_nodes = p_light->GetAllNodes();
      // look for in_xsiObj in the objects association set
      bool isMember = p_light->FindAssociatedObject(in_xsiObj);
      
      // loop all the Arnold light nodes and push them into the result vector if they are inc/exc for in_xsiObj
      for (vector <AtNode*>::iterator iter = p_nodes->begin(); iter!= p_nodes->end(); iter++)
      {
         AtNode *lightNode = *iter;
         if (p_light->m_hasMembers)
         {
            // is xsiObj part of the light group?
            if (p_light->m_isInclusive) // object found in members, let's add the light
            {
               if (isMember)
                  lightsVector.push_back(lightNode);
            }
            else // object not found in members, let's add the light as well
            {
               if (!isMember)
                  lightsVector.push_back(lightNode);
            }
         }
         else
            lightsVector.push_back(lightNode);
      }
   }

   // Convert Vector to AtArray
   if (lightsVector.size() > 0)
      out_lightGroup = AiArrayConvert((int)lightsVector.size(), 1, AI_TYPE_NODE, (AtNode*) &lightsVector[0]);
   else
      out_lightGroup = AiArrayAllocate(0, 1, AI_TYPE_NODE); // to avoid a NULL pointer

   return out_lightGroup;
}


// Cycle all the lights and erase a light node from the nodes belonging to the lights
//
// @param in_node    the light node to be erased
//
void CLightMap::EraseNode(AtNode *in_node)
{
   map <AtNodeLookupKey, CLight>::iterator iter;
   for (iter = m_map.begin(); iter != m_map.end(); iter++)
      iter->second.EraseNode(in_node);
}


// Erase an object from the association set of all the lights
//
// @param in_objectName    the object name to be erased
//
void CLightMap::EraseAssociatedObject(CString &in_objectName)
{
   if (!AtLeastOneLightHasMembers())
      return;

   map <AtNodeLookupKey, CLight>::iterator iter;
   for (iter = m_map.begin(); iter != m_map.end(); iter++)
      iter->second.EraseAssociatedObject(in_objectName);
}


// Update all the lights, when in flythrough mode
//
void CLightMap::FlythroughUpdate()
{
   // iterate and update each entry
   map <AtNodeLookupKey, CLight>::iterator iter;
   for (iter = m_map.begin(); iter != m_map.end(); iter++)
      iter->second.FlythroughUpdate();
}


// Clear the map
void CLightMap::Clear()
{
   m_map.clear();
}


// Debug
void CLightMap::Log()
{
   map <AtNodeLookupKey, CLight>::iterator iter;
   // loop and log all the lights
   for (iter = m_map.begin(); iter != m_map.end(); iter++)
      iter->second.Log();
}



///////////////////////////////
///////////////////////////////
// Done with classes
///////////////////////////////
///////////////////////////////

CStatus LoadLights(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{
   CStatus status;
   // Get lights from scene
   CRefArray lightsArray = Application().GetActiveSceneRoot().FindChildren(L"", siLightPrimType, CStringArray());

   for (LONG i=0; i<lightsArray.GetCount(); i++)
   {
      // check if this light is selected
      if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, lightsArray[i]))
         continue;
      
      Light xsiLight(lightsArray[i]);
      status = LoadSingleLight(xsiLight, in_frame);
      if (status == CStatus::Abort)
         return status;
      // #1307 : do not break in case of CStatus::Fail, just go to the next light
   }

   // GetRenderInstance()->LightMap().Log();

   return CStatus::OK;
}


CStatus LoadSingleLight(const Light &in_xsiLight, double in_frame, bool in_postLoad)
{
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   CString arnoldNodeName;
 
   // Get Light Shader
   Shader xsiShader = GetConnectedShader(ParAcc_GetParameter(in_xsiLight, L"LightShader"));
   if (!xsiShader.IsValid())
      return CStatus::Fail; // The shader is not valid

   if (!GetArnoldLightNodeName(GetShaderNameFromProgId(xsiShader.GetProgID()), arnoldNodeName))
      return CStatus::Fail; // The light is not supported by arnold

   AtNode* lightNode = AiNode(arnoldNodeName.GetAsciiString());
   GetRenderInstance()->NodeMap().PushExportedNode(in_xsiLight, in_frame, lightNode);
   // set the name only if this is not a postloaded scene, meaning it's created because
   // a time shifted ice instance of this light
   if (!in_postLoad)
   {
      CString name = CStringUtilities().MakeSItoAName((SIObject)in_xsiLight, in_frame, L"", false);
      CNodeUtilities().SetName(lightNode, name);
   }

   LoadLightParameters(lightNode, in_xsiLight, xsiShader, true, in_frame, false);
   
   LoadLightFilters(lightNode, in_xsiLight, xsiShader, in_frame);

   CNodeUtilities::SetMotionStartEnd(lightNode);
   // user options
   Property userOptionsProperty;
   in_xsiLight.GetProperties().Find(L"arnold_user_options", userOptionsProperty);
   LoadUserOptions(lightNode, userOptionsProperty, in_frame);
   // user data blobs
   LoadUserDataBlobs(lightNode, in_xsiLight, in_frame); // #728

   // push the light in the global light map, and build the objects association set
   GetRenderInstance()->LightMap().Push(lightNode, in_xsiLight, in_frame);
      
   return CStatus::OK;
}


// Check if a light filter is compatible with a light type
//
// @param in_lightType       the light type
// @param in_filterType      the filter type
//
// @return true if the filter is compatible or if it's a custom filter. Else, false
//
//
bool IsFilterCompatibleWithLight(CString in_lightType, CString in_filterType)
{
   if (in_filterType == L"light_blocker") // blockers ok for all types of light
      return true;
   else if (in_filterType == L"light_decay")
      return !(in_lightType == L"distant_light" || in_lightType == L"skydome_light");
   else if (in_filterType == L"gobo")
      return in_lightType == L"spot_light";
   else if (in_filterType == L"barndoor")
      return in_lightType == L"spot_light";

   return true; // custom filter, green light
}


// Collect all the valid shaders connected to a Softimage light shader
//
// @param in_lightNode       the Arnold light node
// @param in_xsiLight        the Softimage light
// @param in_lightShader     the Softimage light shader
//
// @return the vector of valid filter shaders
//
CRefArray CollectFilterShaders(AtNode *in_lightNode, const Light &in_xsiLight, const Shader &in_lightShader)
{
   CRefArray       result;
   Shader          filterShader;
   int             nbGobos(0), nbBarndoors(0);
   CString         exportedGobo, exportedBarndoor;

   CString lightType = CNodeUtilities().GetEntryName(in_lightNode);

   for (int i=1; i<=MAX_FILTERS; i++)
   {
      Parameter p = in_lightShader.GetParameter(L"filter" + (CString)i);

      if (!CRenderTree().GetParameterShaderSource(p, filterShader))
         continue;

      CString filterType = GetShaderNameFromProgId(filterShader.GetProgID());

      if (!IsFilterCompatibleWithLight(lightType, filterType))
      {
         CString eString = L"[sitoa] Skipping incompatible filter type (" + filterType;
         eString+= ") for ";
         eString+= in_xsiLight.GetFullName();
         GetMessageQueue()->LogMsg(eString, siWarningMsg);
         continue;
      }

      if (filterType == L"barndoor")
      {
         if (nbBarndoors > 0)
         {
            GetMessageQueue()->LogMsg(L"[sitoa] Warning: multiple barndoor filters for " + in_xsiLight.GetName() + ". Only " + exportedBarndoor + " will be used", siWarningMsg);
            continue;
         }
         nbBarndoors++;
         exportedBarndoor = filterShader.GetName();
      }
      else if (filterType == L"gobo")
      {
         if (nbGobos > 0)
         {
            GetMessageQueue()->LogMsg(L"[sitoa] Warning: multiple gobo filters for " + in_xsiLight.GetName() + ". Only " + exportedGobo + " will be used", siWarningMsg);
            continue;
         }
         nbGobos++;
         exportedGobo = filterShader.GetName();
      }

      result.Add(filterShader);
   }

   return result;
}


// Load all the light filters connected to the light shader
//
// @param in_lightNode       the Arnold light node
// @param in_xsiLight        the Softimage light
// @param in_lightShader     the Softimage light shader
// @param in_frame           the frame time
//
// @return CStatus::OK
//
CStatus LoadLightFilters(AtNode *in_lightNode, const Light &in_xsiLight, const Shader &in_lightShader, double in_frame)
{
   // load all the light filters
   vector <AtNode*> lightFilters; // vector for the filter nodes
   // Collect all the valid filters connected to the filter* ports of the light
   CRefArray filterShaders = CollectFilterShaders(in_lightNode, in_xsiLight, in_lightShader);

   for (LONG i=0; i<filterShaders.GetCount(); i++)
   {
      Shader filterShader(filterShaders.GetItem(i));
      CString filterType = GetShaderNameFromProgId(filterShader.GetProgID());
      CString filterName = CStringUtilities().MakeSItoAName(filterShader, in_frame, L"", false);

      AtNode* filterNode = AiNodeLookUpByName(filterName.GetAsciiString());
      if (!filterNode)
      {
         filterNode = LoadShader(filterShader, in_frame, CRef(), RECURSE_FALSE);
         if (!filterNode) // something went wrong
            continue; 
         CNodeUtilities().SetName(filterNode, filterName);

         if (filterType == "light_blocker") // special case for the light blockers: load the matrix
            LoadBlockerFilterMatrix(filterNode, filterShader, in_frame);
         else if (filterType == "gobo") // and for the gobo's offset, which in Arnold is a point2 and in Soft 2 floats (#1516)
            LoadGoboFilterOffsetAndRotate(filterNode, filterShader, in_xsiLight, in_frame);

         lightFilters.push_back(filterNode); // push the filter into the vector
      }
   }

   // Convert vector to AtArray
   if (lightFilters.size() > 0)
   {
      AtArray* filters = AiArrayConvert((int)lightFilters.size(), 1, AI_TYPE_NODE, (AtNode*)&lightFilters[0]);
      AiNodeSetArray(in_lightNode, "filters", filters);
   }

   filterShaders.Clear();
   return CStatus::OK;
}


// Duplicate a light node
// This happens after the polymesh/hair is done and
// 1. Because of a light instance in ice
// 2. Because of an instanced model with lights
//
// @param in_xsiLight      the Softimage light
// @param in_nodeName      the name to be given to the new light node
// @param in_frame         the frame time
//
// @return the created node, or NULL in case of error
//
AtNode* DuplicateLightNode(const Light& in_xsiLight, CString in_nodeName, double in_frame)
{
   // find the light master node and clone it
   AtNode* lightNode = AiNodeClone(GetRenderInstance()->NodeMap().GetExportedNode(in_xsiLight, in_frame));

   if (!lightNode)
      return NULL;

   CNodeUtilities().SetName(lightNode, in_nodeName);
   // add the arnold node to the vector of lights associated with in_xsiLight
   CLight* p_light = GetRenderInstance()->LightMap().Find(in_xsiLight, in_frame);
   if (p_light)
   {
      p_light->PushNode(lightNode);
      // The global light association is called only once after all the light instances are created, for #1721
      // DoFullLightAssociation(in_frame);
   }

   return lightNode;
}


// Do the full shape/lights association
//
// @param in_frame       the frame time
//
void DoFullLightAssociation(double in_frame)
{
   CStringArray families;
   families.Add(siMeshFamily);
   families.Add(siGeometryFamily);

   CRefArray shapesArray = Application().GetActiveSceneRoot().FindChildren(L"", L"", families, true);
    
   LONG nbShapes = shapesArray.GetCount();
   for (LONG ishape=0; ishape<nbShapes; ishape++)
   {
      X3DObject xsiObj(shapesArray[ishape]);

      vector <AtNode*> nodes;
      vector <AtNode*> ::iterator nodeIter;

      AtNode* shapeNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);
      if (shapeNode)
         nodes.push_back(shapeNode);
      else
      {
         vector <AtNode*> *tempV;
         if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, xsiObj, in_frame))
            nodes = *tempV;
      }

      for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
      {
         shapeNode = *nodeIter;
         // skip the light nodes that by mistake (power instances of lights) could be part of the group.
         // Found a case while fixing #1793
         if (CNodeUtilities().GetEntryType(shapeNode) == L"light")
            continue;

         // Resetting 
         CNodeSetter::SetBoolean(shapeNode, "use_light_group", false);
         AiNodeSetArray(shapeNode, "light_group", AiArrayAllocate(0, 0, AI_TYPE_NODE));
         // loops all the lights and return those for which the shape is inclusive/exclusive
         AtArray* light_group = GetRenderInstance()->LightMap().GetLightGroup(xsiObj);
         if (light_group)
         {
            CNodeSetter::SetBoolean(shapeNode, "use_light_group", true);
            if (AiArrayGetNumElements(light_group) > 0)
               AiNodeSetArray(shapeNode, "light_group", light_group);
         }
      }

      nodes.clear();
   }
}


bool GetArnoldLightNodeName(const CString &in_lightShaderName, CString &out_arnoldNodeName)
{
   map<CString, CString> lightType;   
   map<CString, CString>::iterator iter;

   lightType.insert(make_pair(L"arnold_point_light",       L"point_light"));
   lightType.insert(make_pair(L"arnold_distant_light",     L"distant_light"));
   lightType.insert(make_pair(L"arnold_spot_light",        L"spot_light"));
   lightType.insert(make_pair(L"arnold_quad_light",        L"quad_light"));
   lightType.insert(make_pair(L"arnold_cylinder_light",    L"cylinder_light"));
   lightType.insert(make_pair(L"arnold_disk_light",        L"disk_light"));
   lightType.insert(make_pair(L"arnold_skydome_light",     L"skydome_light"));
   lightType.insert(make_pair(L"arnold_mesh_light",        L"mesh_light"));
   lightType.insert(make_pair(L"arnold_photometric_light", L"photometric_light"));
 
   iter = lightType.find(in_lightShaderName);
   if (iter != lightType.end())   
   {
      out_arnoldNodeName = iter->second;
      return true;
   }
   return false; // The light is not supported by arnold   
}

