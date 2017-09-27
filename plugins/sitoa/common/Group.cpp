/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/Group.h"
#include "renderer/Renderer.h"

#include <xsi_shader.h>

#include <string>

using namespace XSI;


// Construct by nodes
//
// @param in_nodes    the input nodes array
//
CGroup::CGroup(vector <AtNode*> *in_nodes)
{
   m_nodes = *in_nodes;
}


// = overload
const CGroup& CGroup::operator =(const CGroup &in_arg)
{
   if (this == &in_arg)
      return *this;
   m_nodes = in_arg.m_nodes;
   return *this;
}


// Get a pointer to the nodes
//
// @return  pointer to the nodes array
//
vector <AtNode*> *CGroup::GetNodes()
{
   return &m_nodes;
}


// Erase a node from all the nodes of all the group
//
// @param in_node     the node to be erased
//
void CGroup::EraseNodeFromGroup(AtNode *in_node, bool in_verbose)
{
   for (vector <AtNode*>::iterator iter = m_nodes.begin(); iter != m_nodes.end(); iter++)
   {
      if (*iter == in_node)
      {
         if (in_verbose)
         {
            CString nodeName = CNodeUtilities().GetName(*iter);
            GetMessageQueue()->LogMsg(L"CGroup::EraseNodeFromGroup " + nodeName);
         }
         m_nodes.erase(iter);
         return;
      }
   }
}


// log the names of the nodes
void CGroup::Log(CString in_spaces)
{
   for (vector <AtNode*>::iterator iter = m_nodes.begin(); iter != m_nodes.end(); iter++)
   {
      CString name = CNodeUtilities().GetName(*iter);
      name = in_spaces + L"node = " + name;
      GetMessageQueue()->LogMsg(name);
   }
}


/////////////////////////////////////
/////////////////////////////////////
// CGroupMap
/////////////////////////////////////
/////////////////////////////////////

// Push into map by nodes array, item and frame time
//
// @param in_nodes         the Arnold nodes to be stored into the new group
// @param in_item          Softimage item whose name is used as key
// @param in_frame         the frame time
//
//
void CGroupMap::PushGroup(vector <AtNode*> *in_nodes, ProjectItem in_item, double in_frame)
{
   CGroup group(in_nodes);
   CString item = in_item.GetFullName();
   m_map.insert(pair<AtNodeLookupKey, CGroup>(AtNodeLookupKey(item, in_frame), group));
}



// Get the group by key. If not found, return false
//
// @param out_group    pointer to the CGroup to return
// @param in_key       the key
//
// @return             true if found, else false
//
bool CGroupMap::GetGroup(CGroup **out_group, AtNodeLookupKey in_key)
{
   map<AtNodeLookupKey, CGroup>::iterator iter = m_map.begin();
   iter = m_map.find(in_key);
   if (iter == m_map.end())
      return false;

   *out_group = &iter->second;
   return true;
}


// Get the nodes associated to a Softimage name in the group map
//
// @param out_nodes        the returned pointer to the nodes
// @param in_objectName    the Softimage item whose name is to be found in the map
// @param in_frame         the frame time
//
// @return true if found, else false
//
bool CGroupMap::GetGroupNodes(vector <AtNode*> **out_nodes, CString &in_objectName, double in_frame)
{
   CGroup *group=NULL;
   if (!GetGroup(&group, AtNodeLookupKey(in_objectName, in_frame)))
      return false;
   *out_nodes = group->GetNodes();
   return true;
}


// Get the nodes associated to a Softimage item in the group map
//
// @param out_nodes        the returned pointer to the nodes
// @param in_item          Softimage item whose name is to be found in the map
// @param in_frame         the frame time
//
// @return true if found, else false
//
bool CGroupMap::GetGroupNodes(vector <AtNode*> **out_nodes, ProjectItem in_item, double in_frame)
{
   CString item = in_item.GetFullName();
   return GetGroupNodes(out_nodes, item, in_frame);
}


// Erase a node from all the nodes of all the group
//
// @param in_node     the node to be deleted
// @param in_verbose  if on, log some info about the erasing process
//
void CGroupMap::EraseNodeFromAllGroups(AtNode *in_node, bool in_verbose)
{
   map <AtNodeLookupKey, CGroup>::iterator iter;
   for (iter = m_map.begin(); iter != m_map.end(); iter++)
      iter->second.EraseNodeFromGroup(in_node, in_verbose);
}


// Erase a group from the group map
//
// @param in_objectName    the Softimage object name owner of the group to be erased
// @param in_frame         the frame time
// @param in_verbose       if on, log some info about the erasing process
//
void CGroupMap::EraseGroup(CString &in_objectName, double in_frame, bool in_verbose)
{
   map <AtNodeLookupKey, CGroup>::iterator iter = m_map.find(AtNodeLookupKey(in_objectName, in_frame));
   if (iter != m_map.end())
   {
      if (in_verbose)
         GetMessageQueue()->LogMsg(L"CGroupMap::EraseGroup " + iter->first.m_objectName);
      m_map.erase(iter);
   }
}


// Clear the map
void CGroupMap::Clear()
{
   m_map.clear();
}


// log the names of the nodes of all the groups
void CGroupMap::Log()
{
   GetMessageQueue()->LogMsg(L"------ CGroupMap::Log ------");
   map <AtNodeLookupKey, CGroup>::iterator iter;
   for (iter = m_map.begin(); iter != m_map.end(); iter++)
   {
      GetMessageQueue()->LogMsg(L"Group " + iter->first.m_objectName);
      iter->second.Log(L" ");
   }
   GetMessageQueue()->LogMsg(L"-------------------------");
}

