/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "common/Tools.h"

#include <ai_nodes.h>


struct AtNodeLookupKey
{
   CString m_objectName;
   LONG    m_frame;

   AtNodeLookupKey(CString &in_objectName, double in_frame)
   {
      m_objectName = in_objectName;
      m_frame = CTimeUtilities().FrameTimes1000(in_frame);
   }

   bool operator < (const AtNodeLookupKey & other) const
   {
      if (m_objectName < other.m_objectName)
         return true;
      if (m_objectName > other.m_objectName)
         return false;
      return m_frame < other.m_frame;
   }
};


typedef map<AtNodeLookupKey,  AtNode*> AtNodeLookupMap;
typedef pair<AtNodeLookupKey, AtNode*> AtNodeLookupPair;
typedef AtNodeLookupMap::iterator AtNodeLookupIt;


// This is the key for the shader map, which needs the shader's id as key.
// Else, shared shaders are entered many times, if using the shader name as we do.
// An alternative would be to use the name and, for a given shader, the name provided by 
// Application().GetObjectFromID(CObjectUtilities().GetId(shader))
// But it's easier to go by the id, which is the same for all the shared shaders
//
struct AtShaderLookupKey
{
   ULONG   m_id;
   LONG    m_frame;

   AtShaderLookupKey(ULONG in_id, double in_frame)
   {
      m_id = in_id;
      m_frame = CTimeUtilities().FrameTimes1000(in_frame);
   }

   bool operator < (const AtShaderLookupKey & other) const
   {
      if (m_id < other.m_id)
         return true;
      if (m_id > other.m_id)
         return false;
      return m_frame < other.m_frame;
   }
};


