/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <renderer/AtNodeLookup.h> // to steal AtNodeLookupKey

#include <string>
#include <string.h>
#include <vector>
#include <map>

using namespace std;

////////////////////////////////
// A CGroup is what used to be a sta_groupnode in SItoA < 1.15 code.
// A CGroup is a group of objects (for instance all the nodes exported by an icetree)
// that can be further referenced and cloned (for instance, because of an instance of a pointcloud).
// Example: a point cloud pc0 exports nodes n0 and n1 (say a points node and a curve node).
// After exporting pc0, a group is created, with m_nodes= n0,n1
// Then pc0 is instanced (a Softimage instance) by inst0. To export properly inst0 as a ginstance,
// we'll look in CGgroupMap for the CGroup with key = master-model-name, and ginstance its m_nodes
////////////////////////////////

class CGroup
{
private:
   vector <AtNode*> m_nodes;

public:
   CGroup()
   {};

   ~CGroup()
   {
      m_nodes.clear();
   }

   // Copy constructor
	CGroup(const CGroup& in_arg)
   {
      m_nodes = in_arg.m_nodes;
   }

   // Construct by nodes
   CGroup(vector <AtNode*> *in_nodes);
   // = overload
   const CGroup& operator=(const CGroup& in_arg);
   // Get a pointer to the nodes
   vector <AtNode*> *GetNodes();
   // Erase a node from the nodes
   void EraseNodeFromGroup(AtNode *in_node, bool in_verbose=false);
   // log the names of the nodes
   void Log(CString in_spaces);
};


////////////////////////////////
// CGroupMap is a map of CGroup. Each CGroup is what used to be a sta_groupnode in SItoA < 1.15 code
// CGroupMap has a unique instance, in the RenderInstance class, so the global container of CGroups
////////////////////////////////
class CGroupMap
{
private:
   map<AtNodeLookupKey, CGroup> m_map;
public:
   CGroupMap()
   {}

   ~CGroupMap()
   {
      m_map.clear();
   }

   // Push into map by nodes array, item and frame time
   void PushGroup(vector <AtNode*> *in_nodes, ProjectItem in_item, double in_frame);
   // Get the group by key. If not found, return false
   bool GetGroup(CGroup **out_group, AtNodeLookupKey key);
   // Get the nodes associated to a Softimage name in the group map
   bool GetGroupNodes(vector <AtNode*> **out_nodes, CString &in_objectName, double in_frame);
// Get the nodes associated to a Softimage item in the group map
   bool GetGroupNodes(vector <AtNode*> **out_nodes, ProjectItem in_item, double in_frame);
   // Erase a node from all the nodes of all the groups
   void EraseNodeFromAllGroups(AtNode *in_node, bool in_verbose=false);
   // Erase a group from the group map
   void EraseGroup(CString &in_objectName, double in_frame, bool in_verbose=false);
   // Destroy the map
   void Clear();
   // log the names of the nodes of all the groups
   void Log();
};

