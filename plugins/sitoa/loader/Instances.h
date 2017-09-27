/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_x3dobject.h>

enum eInstanceType
{
   eInstanceType_Mesh = 0,
   eInstanceType_Light,
   eInstanceType_Hair,
   eInstanceType_Ice,
   eInstanceType_Instance
};


// This class to store the data of a master object to be ginstanced
//
class CMasterData
{
public:
   CMasterData()  : m_isValid(false), m_isLight(false)
   {}
   CMasterData(const CMasterData &in_arg) : 
      m_ref(in_arg.m_ref), m_isValid(in_arg.m_isValid), m_isLight(in_arg.m_isLight), m_hideMaster(in_arg.m_hideMaster),
      m_visibility(in_arg.m_visibility), m_object(in_arg.m_object), m_id(in_arg.m_id)
   {}

   ~CMasterData() 
   {}

   // Initialize by the Softimage object fullname
   CMasterData(CString &in_name, double in_frame);

   CRef      m_ref;
   bool      m_isValid, m_isLight;
   bool      m_hideMaster;
   uint8_t   m_visibility;
   X3DObject m_object;
   int       m_id;
};

// map of the above
typedef map<CString, CMasterData> CMasterDataMap;
typedef pair<CString, CMasterData> CMasterDataPair;


// Load all the instances
CStatus LoadInstances(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly = false);
// Load one single instance into Arnold
CStatus LoadSingleInstance(Model &in_instanceModel, double in_frame);
// Return a list of the objects and lights under a model or hierarchy. If the model is an instance, return what under its master
CRefArray GetObjectsAndLightsUnderMaster(const X3DObject &in_xsiObj);
// Returns all the model instances under this model
CRefArray GetInstancedModelsUnderMaster(const Model &in_model);
// Return the lights below a model. If the model is an instance, return the lights below its master
CRefArray GetLightsUnderMaster(Model &in_model);

// CRefArray GetSubModelsList(const Model &xsiModel, bool recursive);

// Returns the Instance type of the given object.
eInstanceType GetInstanceType(const X3DObject &in_xsiObj);

// new functions for supporting instances of instances

// Returns in out_modelInstancesArray the instanced models under the input model
bool GetInstancedModels(const Model &in_model, bool recurse, CRefArray &modelInstancesArray);
// Adds a CRef element into a CRefArray array, only if the element is not an entry already.
bool AddRefInUniqueRefArray(CRefArray &io_refArray, const CRef &in_ref);
// Sorts the input array of model instances into a new array (of the same size) 
// where the deeper nested models are inserted first.
void SortInstances(CRefArray &in_modelsArray, CRefArray &out_modelsArray, int in_securityExit);
// Copy the attributes from in_masterNode to in_node
void CloneNodeUserData(AtNode* in_node, AtNode* in_masterNode);
// Return the declaration string for an attribute, pointed by in_upentry
string GetUserParamaterDeclarationString(const AtUserParamEntry *in_upentry);

