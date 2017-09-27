/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

#include <map>
using namespace std;

enum DataSwitchParams
{
   p_input,
   p_default,
   p_values,
   p_index
};

class CSwitchData
{
private:

   map <short int, short int> m_indices;

public:

   CSwitchData()
   {}

   ~CSwitchData()
   {
      m_indices.clear();
   }

   void Init(AtNode *in_node)
   {
      m_indices.clear();

      AtArray *index = AiNodeGetArray(in_node, "index");
      if (!index)
         return;
      uint32_t nelements = AiArrayGetNumElements(index);
      for (uint32_t i = 0; i < nelements; i++)
         m_indices.insert(pair <short int, short int> (AiArrayGetInt(index, i), i));
   }

   int HasIndex(short int in_index)
   {
      map <short int, short int>::iterator it;
      it = m_indices.find(in_index);
      if (it == m_indices.end())
         return -1;

      return it->second;
   }
};

