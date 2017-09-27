/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/Tools.h"
#include "loader/Instances.h"
#include "loader/Lights.h"
#include "loader/Loader.h"
#include "loader/Properties.h"
#include "renderer/Renderer.h"


// Initialize by the Softimage object name
//
// @param in_name    the Softimage object fullname
// @param in_frame   the frame time
//
CMasterData::CMasterData(CString &in_name, double in_frame) 
{
   m_ref.Set(in_name);
   m_isValid = m_ref.IsValid();
   if (!m_isValid)
      return;
   m_object = m_ref;
   m_isValid = m_object.IsValid();
   m_isLight = GetRenderInstance()->LightMap().Find(in_name, in_frame) != NULL;

   CRefArray properties = m_object.GetProperties();
   Property vizProperty = properties.GetItem(L"Visibility");
   m_hideMaster = ParAcc_GetValue(vizProperty, L"hidemaster", in_frame);
   if (m_hideMaster)
      m_visibility = GetVisibility(properties, in_frame, false);

   m_id = CObjectUtilities().GetId(m_object);
}


CStatus LoadInstances(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{
   CStatus status;

   CRefArray sceneModels = Application().GetActiveSceneRoot().FindChildren(L"", siModelNullPrimType, CStringArray(), true);
   CRefArray instancedModels, sortedInstancedModels;

   for (LONG i=0; i<sceneModels.GetCount(); ++i)
   {
      // check if the instance is selected
      if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, sceneModels[i]))
         continue;

      // getting the object
      Model model(sceneModels[i]);
      if (model.GetModelKind() == siModelKind_Instance)
         instancedModels.Add(sceneModels[i]);
   }

   // sort the instances based on usage order!
   // if an instance A is nested under another instance B, A will be inserted
   // into the sorted array BEFORE B
   SortInstances(instancedModels, sortedInstancedModels, 0);

   for (LONG i=0; i<sortedInstancedModels.GetCount(); ++i)
   {
      Model model(sortedInstancedModels[i]);
      // GetMessageQueue()->LogMessage(L"  LoadInstances: model = " + model.GetName()); 
      status = LoadSingleInstance(model, in_frame);
      if (status != CStatus::OK) // something went wrong
         break;
   }

   return status;
}


CStatus LoadSingleInstance(Model &in_instanceModel, double in_frame)
{
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   CString instanceName = in_instanceModel.GetFullName();
   // GetMessageQueue()->LogMessage(L"LoadSingleInstance: instanceName = " + instanceName); 
   Model modelMaster(in_instanceModel.GetInstanceMaster());
   //GetMessageQueue()->LogMessage(L"LoadSingleInstance: modelMaster = " + modelMaster.GetName());

   Property prop, visProperty;
   CRefArray modelProperties = in_instanceModel.GetProperties();

   in_instanceModel.GetPropertyFromName(L"Visibility", visProperty);

   // If the instance model is invisible we quit (#1374)
   if (!ParAcc_GetValue(visProperty, L"rendvis", in_frame))
      return CStatus::OK;

   // Instanced models can have arnold properties, not directly, but by the groups or partitions owning them.
   // So, it makes sense to ask for them, that override the shapes' visibility etc. (#1116)
   uint8_t arnoldModelVisibility = AI_RAY_ALL;
   modelProperties.Find(L"arnold_visibility", prop);
   bool arnoldVisibilityOnModel = prop.IsValid();
   if (arnoldVisibilityOnModel)
      arnoldModelVisibility = GetVisibility(modelProperties, in_frame);
   // same thing for sidedness
   uint8_t arnoldModelSidedness;
   bool arnoldSidednessOnModel = GetSidedness(modelProperties, in_frame, arnoldModelSidedness);

   // Get motion blur key times
   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(modelMaster.GetRef(), transfKeys, defKeys, in_frame, true);

   // Getting the instanced objects and lights
   CRefArray objectsArray = GetObjectsAndLightsUnderMaster(modelMaster);
   // Getting the instance models under this instance models (nested instances, see trac#437)
   CRefArray instancesArray = GetInstancedModelsUnderMaster(modelMaster);
   // merge under objectsArray
   objectsArray+= instancesArray;

   // trac#437:
   // Note that this work only if we correctly exported the instances depending
   // on the nesting level (deeper nested first).
   // This way, we have the group of the nested instances exported BEFORE 
   // they are instanced, and there is no need to (horribly) PostLoad as in Ice.cpp

   vector<AtNode*> memberVector; // array to store the members
   vector <AtNode*> nodes;
   vector <AtNode*>::iterator nodeIter;

   bool atLeastOneInstancedLight(false);

   // This map to store only once the informations of a give Softimage master object
   CMasterDataMap masterDataMap;
   // loop for the objects under the model
   LONG nbObjects = objectsArray.GetCount();
   for (LONG iObject=0; iObject<nbObjects; iObject++)
   {
      X3DObject object(objectsArray[iObject]);  

      AtNode* masterNode = GetRenderInstance()->NodeMap().GetExportedNode(object, in_frame);
      if (masterNode)
         nodes.push_back(masterNode);
      else
      {
         vector <AtNode*> *tempV;
         if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, object, in_frame))
            nodes = *tempV;
      }

      // loop over all the master nodes
      for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
      {
         masterNode = *nodeIter;

         CString masterNodeName = CNodeUtilities().GetName(masterNode);
         // the name of the base node, for example if masterNode is already an instance, so it has " " in its name 
         CString baseNodeName = CStringUtilities().GetMasterBaseNodeName(masterNodeName);
         // the name of the base Softimage object
         CString baseSoftObjectName = CStringUtilities().GetSoftimageNameFromSItoAName(baseNodeName);
         // check if this master object is in the masterData map already, else add it
         CMasterDataMap::iterator masterDataMapIt = masterDataMap.find(baseSoftObjectName);
         if (masterDataMapIt == masterDataMap.end())
         {
            CMasterData masteData(baseSoftObjectName, in_frame);
            masterDataMap.insert(CMasterDataPair(baseSoftObjectName, masteData));
            masterDataMapIt = masterDataMap.find(baseSoftObjectName);
         }
         CMasterData *masterData = &masterDataMapIt->second;

         if (!masterData->m_isValid)
            continue;
         // get the matrices of the masterNode.
         AtArray *masterMatrices = AiNodeGetArray(masterNode, "matrix");

         int nbTransfKeys = AiArrayGetNumKeys(masterMatrices) < transfKeys.GetCount() ? 
                            AiArrayGetNumKeys(masterMatrices) : transfKeys.GetCount();
         AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);

         for (int ikey=0; ikey<nbTransfKeys; ikey++)
         {
            double keyFrame = transfKeys[ikey];
            // Model transform
            AtMatrix matrixModel, matrixModelInv;
            CUtilities().S2A(modelMaster.GetKinematics().GetGlobal().GetTransform(keyFrame), matrixModel);            
            matrixModelInv = AiM4Invert(matrixModel);
            // Child transform
            AtMatrix matrixChild, matrixChildInv;
            matrixChild = AiArrayGetMtx(masterMatrices, ikey);
            matrixChildInv = AiM4Mult(matrixChild, matrixModelInv);
            // output transform
            AtMatrix matrixOutput, matrixOutputInv;
            CUtilities().S2A(in_instanceModel.GetKinematics().GetGlobal().GetTransform(keyFrame), matrixOutput);
            matrixOutputInv = AiM4Mult(matrixChildInv, matrixOutput);
            // save to array
            AiArraySetMtx(matrices, ikey, matrixOutputInv);
         } 
         
         if (masterData->m_isLight)
         {
            Light xsiLight = masterData->m_ref;
            AtNode* masterLightNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiLight, in_frame);
            if (masterLightNode)
            {
               CString masterLightName = CNodeUtilities().GetName(masterLightNode);
               // Use the " " to separate the model name and the "master" node name, for #1604,
               // even though this is not a ginstance
               CString lightName(in_instanceModel.GetFullName() + L" " + masterNodeName);
               AtNode* lightNode = DuplicateLightNode(xsiLight, lightName, in_frame);
               if (lightNode)
               {
                  atLeastOneInstancedLight = true;
                  AiNodeSetArray(lightNode, "matrix", matrices);
                  // a light instance has been created, and pushed into memberVector
                  memberVector.push_back(lightNode);
               }
            }
         }
         else
         {
            AtNode* ginstanceNode = AiNode("ginstance"); 
            if (ginstanceNode)
            {
               // An object instance is going to be created, and pushed into memberVector
               memberVector.push_back(ginstanceNode);
               // Getting Master Node Name
               CString masterNodeName = CNodeUtilities().GetName(masterNode);
               // Old way of naming:
               // CString ginstanceName(instanceModel.GetFullName() + L"." + modelMaster.GetFullName() + L"." + masterNodeName);
               // This is the new (trac#437) way of naming ginstances: instance model name plus the master node:
               // master node is either a regular object for first order instances, or the inherited node for power instances.
               // It is very important not to change this naming, since it's what IprCommon.cpp relies on to 
               // be able to reconstruct the matrix for the power instances.
               // The space is used to separate the strings, as it is not a valid char in soft, 
               // but it is in Arnold, so we can use it as separator.
               // Example: sphere_1000 belongs to the model TheSphere, instanced by TheSphere_Instance
               // TheSphere_Instance is then under the model Model that is instanced by Model_Instance.
               // The first ginstance (TheSphere_Instance) will have name = "TheSphere_Instance TheSphere.sphere_1000"
               // The second (power) instance will have name = "Model_Instance TheSphere_Instance TheSphere.sphere_1000"
               // and so on for further power instances.
               // This way, when IprCommon will need to get the matrix of the power instance, it will know
               // that it will need to compose the matrices of TheSphere_Instance and Model_Instance
               CString ginstanceName(in_instanceModel.GetFullName() + L" " + masterNodeName);
               CNodeUtilities().SetName(ginstanceNode, ginstanceName);

               // Same ID as its master (like Softimage/mray does). Arguable decision
               CNodeSetter::SetInt(ginstanceNode, "id", masterData->m_id); 

               // either copy the master node over or create a new instance
               if (AiNodeIs(masterNode, ATSTRING::ginstance))
               {
                  CNodeSetter::SetPointer(ginstanceNode, "node", (AtNode*)AiNodeGetPtr(masterNode, "node"));
                  // clone the user attributes (if any)
                  CloneNodeUserData(ginstanceNode, masterNode);
                  // Override the id (trac#437). For coherence, power instances inherit the id of the base object.
                  // If we comment this line, the ginstances that inherited the members from other
                  // ginstances get the instanced model id, instead of the instanced polymesh id
                  CNodeSetter::SetInt(ginstanceNode, "id", AiNodeGetInt(masterNode, "id"));
                  // copy the visibility
                  if (arnoldVisibilityOnModel) // viz property on model ?
                     CNodeSetter::SetByte(ginstanceNode, "visibility", arnoldModelVisibility, true);
                  else // else, copy it from the master shape
                     CNodeSetter::SetByte(ginstanceNode, "visibility", AiNodeGetByte(masterNode, "visibility"), true);
               }
               else 
               {
                  CNodeSetter::SetPointer(ginstanceNode, "node", masterNode);

                  if (arnoldVisibilityOnModel) // viz property on model ?
                     CNodeSetter::SetByte(ginstanceNode, "visibility", arnoldModelVisibility, true);
                  else // get the visibility from the masterNode
                  {
                     if (masterData->m_hideMaster) // the master was hidden, but we are not. So we need to retrieve the object visibility
                        CNodeSetter::SetByte(ginstanceNode, "visibility", masterData->m_visibility, true);
                     else
                        CNodeSetter::SetByte(ginstanceNode, "visibility", AiNodeGetByte(masterNode, "visibility"), true);
                  }
               }

               CNodeSetter::SetBoolean(ginstanceNode, "inherit_xform", false);
               AiNodeSetArray(ginstanceNode, "matrix", matrices);

               if (arnoldSidednessOnModel) // sidedness property on model ?
                  CNodeSetter::SetByte(ginstanceNode, "sidedness", arnoldModelSidedness, true);
               else
                  CNodeSetter::SetByte(ginstanceNode, "sidedness", AiNodeGetByte(masterNode, "sidedness"), true);
            }
         } // not a light
      } 

      nodes.clear();
   } // for iObjects

   // now let's create the main instance group
   if (memberVector.size() > 0)
      GetRenderInstance()->GroupMap().PushGroup(&memberVector, in_instanceModel, in_frame);

   memberVector.clear();
   masterDataMap.clear();

   // if at least one light was instanced, re-do the light association (#1721)
   if (atLeastOneInstancedLight)
      DoFullLightAssociation(in_frame);

   return CStatus::OK;
}


// Return a list of the objects and lights under a model or hierarchy. 
// If the model is an instance, return what under its master
//
// @param in_xsiObj        the input object or model
//
// @return the array of objects
//
CRefArray GetObjectsAndLightsUnderMaster(const X3DObject &in_xsiObj)
{
   CRefArray result;
   CStringArray families;
   families.Add(siMeshFamily);
   families.Add(siGeometryFamily);
   families.Add(siLightPrimitiveFamily);

   Model model(in_xsiObj.GetRef());
  
   if (model.IsValid())
   {
      if (model.GetModelKind() == siModelKind_Instance)
         model = model.GetInstanceMaster();
      result = model.FindChildren(L"", L"", families, true);
   }
   else
      result = in_xsiObj.FindChildren(L"", L"", families, true);

   return result;
}


// Returns all the model instances under a model
//
// @param in_model        the input model
//
// @return the array of models
//
CRefArray GetInstancedModelsUnderMaster(const Model &in_model)
{
   CRefArray modelsArray = in_model.FindChildren(L"", siModelType, CStringArray(), true);
   CRefArray instanceModelsArray;
   for (LONG i=0; i<modelsArray.GetCount(); i++)
   {
      Model model = (Model)modelsArray.GetItem(i);
      if (model.GetModelKind() == siModelKind_Instance)
         instanceModelsArray.Add(modelsArray.GetItem(i));
   }

   return instanceModelsArray;
}


// Return the lights below a model. 
// If the model is an instance, return the lights below its master
//
// @param in_model        the input model
//
// @return the array of lights
//
CRefArray GetLightsUnderMaster(Model &in_model)
{
   if (in_model.GetModelKind() == siModelKind_Instance)
      in_model = in_model.GetInstanceMaster();
      
   CStringArray families;
   families.Add(siLightPrimitiveFamily);
   
   CRefArray lightsArray = in_model.FindChildren(L"", L"", families, true);
   return lightsArray;
}


eInstanceType GetInstanceType(const X3DObject &in_xsiObj)
{
   eInstanceType instanceType = eInstanceType_Mesh; // Default

   if (in_xsiObj.IsA(siLightID))
      instanceType = eInstanceType_Light;
   else if (in_xsiObj.GetType() == L"pointcloud")
      instanceType = eInstanceType_Ice;
   else if (in_xsiObj.IsA(siModelID))
   {
      Model model = Model(in_xsiObj.GetRef());
      if (model.GetModelKind() == siModelKind_Instance)
         instanceType = eInstanceType_Instance;
   }
   else
   {
      Primitive primitive = CObjectUtilities().GetPrimitiveAtCurrentFrame(in_xsiObj);
      if (primitive.IsA(siHairPrimitiveID))
         instanceType = eInstanceType_Hair;
   }

   return instanceType;
}


/////////////////////////////////
// new functions for supporting instances of instances
/////////////////////////////////

// Returns in out_modelInstancesArray the instanced models under the input model
//
// @param in_model                  The input model (either a master or instanced)
// @param recurse                   True to return all the models, false for only the direct children
// @param out_modelInstancesArray   The returned array
// @return true if at least one instance model was found, else false
//
bool GetInstancedModels(const Model &in_model, bool recurse, CRefArray &out_modelInstancesArray)
{
   //CStringArray temp;
	CRefArray children = in_model.FindChildren(L"", siModelType, CStringArray(), recurse);
	for (LONG i=0; i<children.GetCount(); i++)
	{
		Model model = (Model)children.GetItem(i);
      if (model.GetModelKind() == siModelKind_Instance)
			out_modelInstancesArray.Add(children.GetItem(i));
	}
	return out_modelInstancesArray.GetCount() > 0;
}

// Adds a CRef element into a CRefArray array, only if the element is not an entry already.
// We want to preferve the insertion order, so no stl map/set can be used, which are always
// resorted according to the key/element. Basically, we want the js 
// XsiCollection coll; coll.unique=true; behavior
//
// @param ref_array     The array where to inser in_ref
// @param in_ref        The element to insert
// @return true if the element was inserted, false if the element was already there
//
bool AddRefInUniqueRefArray(CRefArray &io_refArray, const CRef &in_ref)
{
   for (LONG i=0; i<io_refArray.GetCount(); i++)
      if (io_refArray.GetItem(i) == in_ref)
         return false;
   
   io_refArray.Add(in_ref);
   return true;
}


// Sorts the input array of model instances into a new array (of the same size) where
// the deeper nested models are inserted first.
// So, for instance in Model_Instance is an instance of Model, and Model has a Model2_Instance
// as a child, the sorted order will be: Model2_Instance, Model_Instance
// So, after the resorting, we should be safe against any case of power instance 
// (instances of instances of instances of zzzzzzzz).
// In fact, the sorted array will be used to push the groups one after the other.
// When an instance of power 2 will be pushed, it will create a group, and find the 
// power 1 instance already pushed by its groupnode.
//
// @param in_modelsArray    The input model instances array
// @param out_modelsArray   The output model instances array
// @param securityExit      As I may be completely wrong, a security test against infinite recursion (max 10 allowed)
// @return void
//
void SortInstances(CRefArray &in_modelsArray, CRefArray &out_modelsArray, int in_securityExit)
{
   if (in_securityExit > 10)
      return;
   in_securityExit++;

	Model model, master;
	for (LONG i=0; i<in_modelsArray.GetCount(); i++)
	{
		model = (Model)in_modelsArray.GetItem(i); //the instanced model
		// get the master, and go for the instances nested under it
		master = model.GetInstanceMaster();
		
		CRefArray masterInstancesArray;
      // the order matters!!
      // FIRST recurse, to find deeper nested instances
		if (GetInstancedModels(master, false, masterInstancesArray))
			SortInstances(masterInstancesArray, out_modelsArray, in_securityExit);
		// THEN store tha current instance	
		AddRefInUniqueRefArray(out_modelsArray, in_modelsArray.GetItem(i));
	}
}


// these are the categories:
// #define AI_USERDEF_UNDEFINED  0  // Undefined, you should never encounter a parameter of this category
// #define AI_USERDEF_CONSTANT   1  // User-defined: per-object parameter
// #define AI_USERDEF_UNIFORM    2  // User-defined: per-face parameter
// #define AI_USERDEF_VARYING    3  // User-defined: per-vertex parameter

// Copy the attributes from in_masterNode to in_node
//
// @param in_node        The target node to which the attributes must be copied
// @param in_masterNode  The input node where to copy the attributes from
// @return void 
//
void CloneNodeUserData(AtNode* in_node, AtNode* in_masterNode)
{
   AtUserParamIterator *iter = AiNodeGetUserParamIterator(in_masterNode);
   // iterate all the user attributes
   while (!AiUserParamIteratorFinished(iter))
   {
      const AtUserParamEntry *upentry = AiUserParamIteratorGetNext(iter);
      // attribute name
      string attrName(AiUserParamGetName(upentry));
      // get the declaration string of the attribute, for instance "uniform FLOAT"
      string declarationString = GetUserParamaterDeclarationString(upentry);
      if (declarationString == "") // something went wrong
         continue;
      // declare the attribute on in_node
      if (AiNodeDeclare(in_node, attrName.c_str(), declarationString.c_str()))
      {
         // constant attributes (just one value). Copy it
         if (declarationString == "constant BOOL")
            CNodeSetter::SetBoolean(in_node, attrName.c_str(), AiNodeGetBool(in_masterNode, attrName.c_str()));
         else if (declarationString == "constant INT")
            CNodeSetter::SetInt(in_node, attrName.c_str(), AiNodeGetInt(in_masterNode, attrName.c_str()));
         else if (declarationString == "constant FLOAT")
            CNodeSetter::SetFloat(in_node, attrName.c_str(), AiNodeGetFlt(in_masterNode, attrName.c_str()));
         else if (declarationString == "constant VECTOR")
         {
            AtVector v = AiNodeGetVec(in_masterNode, attrName.c_str());
            CNodeSetter::SetVector(in_node, attrName.c_str(), v.x, v.y, v.z);
         }
         else if (declarationString == "constant RGB")
         {
            AtRGB c = AiNodeGetRGB(in_masterNode, attrName.c_str());
            CNodeSetter::SetRGB(in_node, attrName.c_str(), c.r, c.g, c.b);
         }
         else if (declarationString == "constant RGBA")
         {
            AtRGBA c = AiNodeGetRGBA(in_masterNode, attrName.c_str());
            CNodeSetter::SetRGBA(in_node, attrName.c_str(), c.r, c.g, c.b, c.a);
         }
         else // constant array, or uniform or varying or indexed data: clone the array
         {
            AiNodeSetArray(in_node, attrName.c_str(), AiArrayCopy(AiNodeGetArray(in_masterNode, attrName.c_str())));
            if (AiUserParamGetCategory(upentry) == 4) // indexed? Also copy the idxs array
            {
               string indexesName = attrName + "idxs";
               AiNodeSetArray(in_node, indexesName.c_str(), AiArrayCopy(AiNodeGetArray(in_masterNode, indexesName.c_str())));
            }
         }
      }
   }
   AiUserParamIteratorDestroy(iter);
}


// Return the declaration string for an attribute, pointed by in_upentry
//
// @param in_upentry  The user attribute
// @return the declaration string (for instance "varying INT"), or "" in case of error 
//
string GetUserParamaterDeclarationString(const AtUserParamEntry *in_upentry)
{
   string result;
   string categories[4] = {"constant", "uniform", "varying", "indexed"};

   int attrCat = AiUserParamGetCategory(in_upentry);

   if (attrCat < 1 || attrCat > 4)
      return ""; // error
   else
      result = categories[attrCat-1];

   int attrType = AiUserParamGetType(in_upentry);

   switch (attrType)
   {
      case AI_TYPE_BOOLEAN:
         result+= " BOOL";
         break;
      case AI_TYPE_INT:
         result+= " INT";
         break;
      case AI_TYPE_FLOAT:
         result+= " FLOAT";
         break;
      case AI_TYPE_VECTOR:
         result+= " VECTOR";
         break;
      case AI_TYPE_RGB:
         result+= " RGB";
         break;
      case AI_TYPE_RGBA:
         result+= " RGBA";
         break;
      case AI_TYPE_VECTOR2:
         result+= " VECTOR2";
         break;
      case AI_TYPE_ARRAY:
         result+= " ARRAY";
         break;
      default: // unsupported attribute type
         return "";
   }

   if (attrType == AI_TYPE_ARRAY)
   {
      attrType = AiUserParamGetArrayType(in_upentry);

      switch (attrType)
      {
         case AI_TYPE_BOOLEAN:
            result+= " BOOL";
            break;
         case AI_TYPE_INT:
            result+= " INT";
            break;
         case AI_TYPE_FLOAT:
            result+= " FLOAT";
            break;
         case AI_TYPE_VECTOR:
            result+= " VECTOR";
            break;
         case AI_TYPE_RGB:
            result+= " RGB";
            break;
         case AI_TYPE_RGBA:
            result+= " RGBA";
            break;
         case AI_TYPE_VECTOR2:
            result+= " VECTOR2";
            break;
         default: // unsupported attribute type
            return "";
      }
   }

   return result;
}

