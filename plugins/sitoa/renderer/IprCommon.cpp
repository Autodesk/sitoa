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
#include "common/Tools.h"
#include "loader/Properties.h"
#include "renderer/IprCommon.h"
#include "renderer/Renderer.h"

#include <xsi_group.h>

// Find all the associated models (so its instances) under a model.
//
// @param in_mdl                 The input model
// @param out_associatedModels   Returned array of associated models
//
// @return false if the model is not a model, or if it has no associated models
//
bool FindAssociatedModels(const Model &in_mdl, CRefArray &out_associatedModels)
{
   if (in_mdl.GetType() != L"#model")
      return false;
   CRefArray groups = in_mdl.GetGroups();

   for (LONG gIndex=0; gIndex<groups.GetCount(); gIndex++)
   {
      Group group = groups.GetItem(gIndex);
      CRefArray members = group.GetMembers();
      for (LONG mIndex=0; mIndex<members.GetCount(); mIndex++)
      {
         if (members.GetItem(mIndex).IsA(siModelID))
         {
            Model mdl = members.GetItem(mIndex);
            if (mdl.GetModelKind() == siModelKind_Instance)
               out_associatedModels.Add(members.GetItem(mIndex));
         }
      }
   }

   Model parentModel = in_mdl.GetModel();
   if ((parentModel != in_mdl) && (parentModel != Application().GetActiveSceneRoot()))
      FindAssociatedModels(parentModel, out_associatedModels);

   return true;
}


// Update the matrix of the nodes originated by a Softimage object or model
//
// @param in_xsiObj   the Softimage object or model
// @param in_frame    the frame time
//
void UpdateShapeMatrix(X3DObject &in_xsiObj, double in_frame)
{  
   CRefArray associatedModels;
   Model ownerModel = in_xsiObj.GetType() == L"#model" ? (Model)in_xsiObj : in_xsiObj.GetModel();

   // GetMessageQueue()->LogMsg(L"** UpdateShapeMatrix of " + in_xsiObj.GetName() + " , owner = " + ownerModel.GetName());

   // If a model is updating, we must also update the associated models (so the instances of in_xsiObj
   // Also, if an object is updating, we must also update the associated models of the model the object belong to
   // So, in both case, let's collect the associated models into associatedModels
   if (ownerModel != Application().GetActiveSceneRoot()) 
      FindAssociatedModels(ownerModel, associatedModels);

   CStringArray families;
   families.Add(siMeshFamily);
   families.Add(siGeometryFamily);
   
   CRefArray shapesArray = in_xsiObj.FindChildren(L"", L"", families);
   
   // Adding the object itself to list (for cases of polymeshes with some other polymeshes below it)
   shapesArray.Add(in_xsiObj);

   double frame(in_frame);
   // "frame" is used to look up the existing shape node (if any). If we are in flythrough mode,
   // the node was created at time flythrough_frame, and never destroyed since then.
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      frame = GetRenderInstance()->GetFlythroughFrame();

   LONG nbShapes = shapesArray.GetCount();
   for (LONG i=0; i<nbShapes; i++)
   {
      X3DObject xsiObj(shapesArray[i]);

      vector <AtNode*> nodes;
      vector <AtNode*> ::iterator nodeIter;

      AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, frame);
      if (node)
         nodes.push_back(node);
      else // if the node is a node group we will loop over all its members
      {
         vector <AtNode*> *tempV;
         if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, in_xsiObj, frame))
            nodes = *tempV;
      }

      for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
      {
         node = *nodeIter;
         UpdateNodeMatrix(node, xsiObj, in_frame);
      }

      nodes.clear();
   }

   // so now let's update the associated models also.
   for (LONG i=0; i<associatedModels.GetCount(); i++)
   {
      X3DObject assMdl = associatedModels.GetItem(i);
      // GetMessageQueue()->LogMessage(L"  --> recurse UpdateShapeMatrix for " + assMdl.GetName());
      UpdateShapeMatrix(assMdl, in_frame);
   }
}


// Computes the matrix of an instanced object. See Instance.cpp comments when setting ginstanceName
// 
// @param in_nodeParentList   the array of strings associated with the ginstance name
// @param in_xsiObj           the instanced object
// @param in_frame            the frame time
//
// @return    the matrix to set
//
CMatrix4 GetTheNodeMatrix(CStringArray &in_nodeParentList, const X3DObject &in_xsiObj, double in_frame)
{
   CMatrix4 resultMatrix, modelMasterMatrix, instancedModelMatrix;
   CMatrix4 objMatrix = in_xsiObj.GetKinematics().GetGlobal().GetTransform(in_frame).GetMatrix4();
   CRef ref;
   Model instanceModel, modelMaster;
   CString modelInstanceName;

   // let's walk backward the list of instanced models
   // in_nodeParentList.GetCount()-1 is the object node name, so corresponding to nodeObj
   // We multiply the transf matrix (with respect to each master model), so reproducing
   // the way we compute the matrix of a power instance in Instances.cpp.
   for (LONG i=in_nodeParentList.GetCount()-2; i>=0; i--)
   {
      modelInstanceName = in_nodeParentList[i];
      ref.Set(modelInstanceName);
      if (!ref.IsA(siModelID))
         continue; // not really an option, this should not happen

      Model instanceModel(ref);
      modelMaster = instanceModel.GetInstanceMaster();
      // get the model master matrix and invert it
      modelMasterMatrix = modelMaster.GetKinematics().GetGlobal().GetTransform(in_frame).GetMatrix4();
      modelMasterMatrix.InvertInPlace();
      // by multiplying, we basically have the matrix of the object with respect to the model master
      objMatrix.MulInPlace(modelMasterMatrix);
      // get the matrix of the instanced model
      instancedModelMatrix = instanceModel.GetKinematics().GetGlobal().GetTransform(in_frame).GetMatrix4();
      // multiply to get the absolute matrix of the instanced object
      resultMatrix.Mul(objMatrix, instancedModelMatrix);
      objMatrix = resultMatrix;
   }

   return resultMatrix;
}


// Update the matrix of a node originated by a Softimage object or model
//
// @param in_node     the Arnold node to be updates
// @param in_xsiObj   the Softimage object or model
// @param in_frame    the frame time
//
void UpdateNodeMatrix(AtNode* in_node, const X3DObject &in_xsiObj, double in_frame)
{
   if (!in_node)
      return;

   // GetMessageQueue()->LogMessage(L">>>> Enter UpdateNodeMatrix, xsiObj " + xsiObj.GetName() + L", node = " + (CString)AiNodeEntryGetName(AiNodeGetNodeEntry(node)));
   // GetMessageQueue()->LogMessage(L">>>>                          nodeId = " + (CString)AiNodeGetInt(node, "id"));

   // First, check if this is an instanced node.
   // If so, in fact, we'll need to set the instanced node matrix depending on the srt of the master objects 
   // with respect to the master model coordinate system 
   bool isInstance(false);
   Model modelMaster;
   X3DObject baseObj;
   CStringArray nodeParentList;
   if (in_xsiObj.GetType() == L"#model")
   {
      Model model(in_xsiObj.GetRef());
      if (model.GetModelKind() == siModelKind_Instance)
      {
         isInstance = true;
         // the moved object is an instance, and the node points to the instanced object
         CString nodeName = CNodeUtilities().GetName(in_node);
         nodeParentList = nodeName.Split(L" ");

         CString baseNodeName = CStringUtilities().GetMasterBaseNodeName(nodeName);
         CString baseSoftObjectName = CStringUtilities().GetSoftimageNameFromSItoAName(baseNodeName);
         CRef baseRef;
         baseRef.Set(baseSoftObjectName);
         baseObj = baseRef;
         if (!baseObj.IsValid())
            return;
      }
   }

   CRefArray objProperties(in_xsiObj.GetProperties());

   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(in_xsiObj.GetRef(), transfKeys, defKeys, in_frame);
   LONG nbTransfKeys = transfKeys.GetCount();

   AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);
   CMatrix4 nodeMatrix, theMatrix;

   for (LONG iKey=0; iKey<nbTransfKeys; iKey++)
   {
      if (isInstance)
      {
         // GetMessageQueue()->LogMessage(L"UpdateNodeMatrix, xsiObj = " + in_xsiObj.GetName() + L", baseSoftObjectName = " + baseObj.GetFullName());
         theMatrix = GetTheNodeMatrix(nodeParentList, baseObj, transfKeys[iKey]);
      }
      else //plain matrix
      {
         theMatrix = in_xsiObj.GetKinematics().GetGlobal().GetTransform(transfKeys[iKey]).GetMatrix4();
         // if this is a photometric_light, we must conform the spot axes to the lights' one
         if (AiNodeIs(in_node, ATSTRING::photometric_light))
            theMatrix = TransformToPhotometricLight(theMatrix);
      }

      // Get the transform matrix
      AtMatrix nodeMatrix;
      CUtilities().S2A(theMatrix, nodeMatrix);
      AiArraySetMtx(matrices, iKey, nodeMatrix);
   }

   AiNodeSetArray(in_node, "matrix", matrices);
}


// Update the objects that depend on an Arnold Parameters property
//
// @param in_cp           the Arnold Parameters property
// @param in_frame        the frame time
//
void UpdateParameters(const CustomProperty &in_cp, double in_frame)
{   
   // Getting all members of group to apply the data
   int classID = in_cp.GetParent().GetClassID();
   if (classID==siGroupID || classID==siPartitionID)
   {
      // Getting group and members
      Group xsiGroup(in_cp.GetParent());
      CRefArray members(xsiGroup.GetMembers());
      LONG nmembers = members.GetCount();

      for (LONG i=0; i<nmembers; i++)
         UpdateObjectParameters(X3DObject(members[i]), in_cp, in_frame);
   }
   else
   {
      // get the owner of the custom property
      X3DObject xsiObj(in_cp.GetParent());
      UpdateObjectParameters(xsiObj, in_cp, in_frame);
   }
}


// Update an object that depends on an Arnold Parameters property
//
// @param in_xsiObj             the Softimage object
// @param in_cp                 the Arnold Parameters property
// @param in_frame              the frame time
//
void UpdateObjectParameters(const X3DObject &in_xsiObj, const CustomProperty &in_cp, double in_frame)
{   
   // Gettting Parameters
   CParameterRefArray paramsArray = in_cp.GetParameters();

   // in case of a pointcloud, we should reload the arnold parameters with some care, to avoid
   // annoying warnings. Since many types of node can originate from a pointcloud (see #1184 and #1038)
   // we'll have to filter by node type in LoadCustomParameters
   bool filterParameters = in_xsiObj.GetType() == L"pointcloud";
   
   vector <AtNode*> nodes;
   vector <AtNode*> ::iterator nodeIter;

   AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(in_xsiObj, in_frame);
   if (node)
      nodes.push_back(node);
   else // if the node is a node group we will loop over all its members
   {
      vector <AtNode*> *tempV;
      if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, in_xsiObj, in_frame))
         nodes = *tempV;
   }

   for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
   {
      node = *nodeIter;
      // Updating parameters
      LoadArnoldParameters(node, paramsArray, in_frame, filterParameters);
   }

   nodes.clear();
}


// Update the visibility of an object, group or partition
//
// @param in_ref          the owner of the Arnold Visibility property
// @param in_frame        the frame time
//
void UpdateVisibility(const CRef &in_ref, double in_frame)
{
   // Getting all members of group to apply the data
   int classID = in_ref.GetClassID();
   if (classID==siGroupID || classID==siPartitionID)
   {
      // Getting group and members
      Group xsiGroup(in_ref);

      // Check if this group has an Arnold Viz prop, that will win on everything else
      CRefArray groupProperties = xsiGroup.GetProperties();
      Property prop;
      uint8_t arnoldGroupVisibility = AI_RAY_ALL;
      groupProperties.Find(L"arnold_visibility", prop);
      bool arnoldVisibilityOnGroup = prop.IsValid();
      if (arnoldVisibilityOnGroup)
         arnoldGroupVisibility = GetVisibility(groupProperties, in_frame);

      CRefArray members(xsiGroup.GetMembers());

      for (LONG i=0; i<members.GetCount(); i++)
      {
         X3DObject xsiObj(members[i]);
         UpdateObjectVisibility(xsiObj, CObjectUtilities().GetId(xsiObj), true, in_frame, arnoldVisibilityOnGroup, arnoldGroupVisibility);
      }
   }
   else
   {
      // update the xsi object
      X3DObject xsiObj(in_ref);
      UpdateObjectVisibility(xsiObj, CObjectUtilities().GetId(xsiObj), true, in_frame, false, 0);
   }
}


// Update an object or model visibility
//
// @param in_xsiObj                       the Softimage object or model
// @param in_objId                        the id of the object, or the master object if we're updating the models associated with in_xsiObj
// @param in_checkHideMasterFlag          true if the hide master flag of model is to be honored
// @param in_frame                        the frame time
// @param in_visibilityOnGroup            true if the group visibility must be used
// @param in_groupVisibility              the group visibility (if in_visibilityOnGroup is true)
//
void UpdateObjectVisibility(const X3DObject &in_xsiObj, ULONG in_objId, bool in_checkHideMasterFlag, double in_frame,
                            bool in_visibilityOnGroup, uint8_t in_groupVisibility)
{   
   uint8_t visibility = 0;
   bool isInstanceModel(false);
   uint8_t instanceModelVisibility(AI_RAY_ALL);

   AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(in_xsiObj, in_frame);
   if (node && !in_visibilityOnGroup) // a polymesh, get visibility also considering the Instance Master Hidden flag
      visibility = GetVisibilityFromObject(in_xsiObj, in_frame);
   // new for #1374
   else if (in_xsiObj.IsA(siModelID))
   {
      Model model(in_xsiObj.GetRef());
      if (model.GetModelKind() == siModelKind_Instance) // it's an instance model triggering the viz
      {
         CRefArray modelProperties = model.GetProperties();
         instanceModelVisibility = GetVisibility(modelProperties, in_frame);
         isInstanceModel = true;
      }
   }

   vector <AtNode*> nodes;
   vector <AtNode*> ::iterator nodeIter;
   bool isGroup(false);

   if (node)
      nodes.push_back(node);
   else // if the node is a node group we will loop over all its members
   {
      vector <AtNode*> *tempV;
      if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, in_xsiObj, in_frame))
      {
         nodes = *tempV;
         isGroup = true;
      }
   }

   for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
   {
      node = *nodeIter;
      // Updating Visibility
      if (in_visibilityOnGroup)
         CNodeSetter::SetByte(node, "visibility", in_groupVisibility, true);
      else if (isInstanceModel)
         CNodeSetter::SetByte(node, "visibility", instanceModelVisibility, true);
      else
      {
         if (isGroup)
         {
            // It's a group. Get the visibility from the cloned shape
            // Note that for instances this works, since all the ginstances share the same id of the master shape
            int id = AiNodeGetInt(node, "id");
            // Update only if the group member is "the same" that triggered the ipr update.
            // The other members of the group should stay untouched
            if ((ULONG)id == in_objId)
            {
               // Get the visibility of the master shape. In case of a hair object, in_checkHideMasterFlag
               // is true, since UpdateVisibility was called by the ipr change.
               // In case of an instance object, we are recursing UpdateVisibility (see the bottom section)
               // and so in_checkHideMasterFlag is false.
               // In case of an instance of a hair, same thing, in_checkHideMasterFlag is false.
               visibility = GetVisibilityFromObjectId(id, in_frame, in_checkHideMasterFlag);
               CNodeSetter::SetByte(node, "visibility", visibility, true);
            }
         }
         else // simple shape
            CNodeSetter::SetByte(node, "visibility", visibility, true);
      }
   }
   nodes.clear();

   // We must also update the associated models (so the instances of xsiObj)
   // So, let's collect the associated models into associatedModels
   if (!in_xsiObj.IsA(siModelID))
   {
      CRefArray associatedModels;
      Model ownerModel = in_xsiObj.GetModel();
      if (ownerModel != Application().GetActiveSceneRoot()) 
         FindAssociatedModels(ownerModel, associatedModels);

      // let's update the associated models also (if any).
      for (LONG i=0; i<associatedModels.GetCount(); i++)
      {
         X3DObject assMdl = associatedModels.GetItem(i);
         // Recursing with in_checkHideMasterFlag=false, so all the instances
         // will ignore the Instance Master Hidden flag of the master
         UpdateObjectVisibility(assMdl, in_objId, false, in_frame, in_visibilityOnGroup, in_groupVisibility);
      }
   }
}


// Update the sidedness of an object, group or partition
//
// @param in_ref          the owner of the Arnold Sidedness property
// @param in_frame        the frame time
//
void UpdateSidedness(const CRef &in_ref, double in_frame)
{
   // Getting all members of group to apply the data
   int classID = in_ref.GetClassID();
   if (classID==siGroupID || classID==siPartitionID)
   {
      // Getting group and members
      Group xsiGroup(in_ref);
      // Check if this group has an Arnold Viz prop, that will win on everything else
      CRefArray groupProperties = xsiGroup.GetProperties();

      uint8_t arnoldGroupSidedness;
      bool arnoldSidednessOnGroup = GetSidedness(groupProperties, in_frame, arnoldGroupSidedness);

      CRefArray members(xsiGroup.GetMembers());

      for (LONG i=0; i<members.GetCount(); i++)
         UpdateObjectSidedness(X3DObject(members[i]), 0, 0, in_frame, arnoldSidednessOnGroup, arnoldGroupSidedness);
   }
   else
   {
      // Getting owner of the customProperty
      X3DObject xsiObj(in_ref);
      // Updating parameters
      UpdateObjectSidedness(xsiObj, 0, 0, in_frame, false, 0);
   }
}


// Update an object or model sidedness
//
// @param in_xsiObj                       the Softimage object or model
// @param in_objId                        the id of the object, or the master object if we're updating the models associated with in_xsiObj
// @param in_sidedness                    the sidedness value to set
// @param in_frame                        the frame time
// @param in_sidednessOnGroup             true if the group sidedness must be used
// @param in_groupSidedness               the group sidedness (if in_sidednessOnGroup is true)
//
void UpdateObjectSidedness(const X3DObject &in_xsiObj, ULONG in_objId, uint8_t in_sidedness, double in_frame,
                           bool in_sidednessOnGroup, uint8_t in_groupSidedness)
{   
   uint8_t sidedness;

   if (in_sidednessOnGroup)
      sidedness = in_groupSidedness;
   else
   {
      // Gettting object sidedness in case we are not recursing. Else, in_sidedness is already the
      // sidedness of the object belonging to a model being instanced
      if (in_objId == 0)
      {
         // UpdateSidedness is called on a change of the sidedness property. So, we can avoid
         // checking the GetSidedness result.
         CRefArray polyProperties = in_xsiObj.GetProperties();
         GetSidedness(polyProperties, in_frame, sidedness);
      }
      else
         sidedness = in_sidedness;
   }

   vector <AtNode*> nodes;
   vector <AtNode*> ::iterator nodeIter;
   bool isGroup(false);

   AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(in_xsiObj, in_frame);
   if (node)
      nodes.push_back(node);
   else // if the node is a node group we will loop over all its members
   {
      vector <AtNode*> *tempV;
      if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, in_xsiObj, in_frame))
      {
         nodes = *tempV;
         isGroup = true;
      }
   }

   for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
   {
      node = *nodeIter;

      if (in_sidednessOnGroup)
         CNodeSetter::SetByte(node, "sidedness", in_groupSidedness, true);
      else
      {
         if (isGroup)
         {
            // Note that for instances this works, since all the ginstances share the same id of the master shape
            int id = AiNodeGetInt(node, "id");
            // Update only if the group member is "the same" that triggered the ipr update.
            // The other members of the group should stay untouched
            if ((ULONG)id == in_objId)
               CNodeSetter::SetByte(node, "sidedness", sidedness, true);
         }
         else // simple shape
            CNodeSetter::SetByte(node, "sidedness", sidedness, true);
      }
   }
   nodes.clear();

   // We must also update the associated models (so the instances of xsiObj)
   // So, let's collect the associated models into associatedModels
   if (!in_xsiObj.IsA(siModelID))
   {
      CRefArray associatedModels;
      Model ownerModel = in_xsiObj.GetModel();
      if (ownerModel != Application().GetActiveSceneRoot()) 
         FindAssociatedModels(ownerModel, associatedModels);

      // let's update the associated models also (if any).
      for (LONG i=0; i<associatedModels.GetCount(); i++)
      {
         X3DObject assMdl = associatedModels.GetItem(i);
         // Recursing with the current object id and its sideness
         UpdateObjectSidedness(assMdl, CObjectUtilities().GetId(in_xsiObj), sidedness, in_frame, in_sidednessOnGroup, in_groupSidedness);
      }
   }
}


// Update the matte of an object, group or partition
//
// @param in_cp        the matte property
// @param in_frame     the frame time
//
void UpdateMatte(const CustomProperty &in_cp, double in_frame)
{
   if (GetRenderOptions()->m_ignore_matte)
      return;

   // Getting all members of group to apply the data on
   int classID = in_cp.GetParent().GetClassID();
   if (classID==siGroupID || classID==siPartitionID)
   {
      // Getting group and members
      Group xsiGroup(in_cp.GetParent());
      CRefArray members(xsiGroup.GetMembers());
      LONG nbMembers = members.GetCount();

      for (LONG i=0; i<nbMembers; i++)
         UpdateObjectMatte(X3DObject(members[i]), in_cp, in_frame);
   }
   else
   {
      // get the property owner
      X3DObject xsiObj(in_cp.GetParent());
      UpdateObjectMatte(xsiObj, in_cp, in_frame);
   }
}


// Update the matte of an object
//
// @param in_xsiObj            the Softimage object
// @param in_cp                the matte property
// @param in_frame             the frame time
//
void UpdateObjectMatte(const X3DObject &in_xsiObj, const CustomProperty &in_cp, double in_frame)
{
   vector <AtNode*> nodes;
   vector <AtNode*> ::iterator nodeIter;

   AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(in_xsiObj, in_frame);
   if (node)
      nodes.push_back(node);
   else // if the node is a node group we will loop over all its members
   {
      vector <AtNode*> *tempV;
      if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, in_xsiObj, in_frame))
         nodes = *tempV;
   }

   for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
   {
      node = *nodeIter;
      LoadMatte(node, in_cp, in_frame);
   }

   nodes.clear();
}


// Update the visible objects when in an isolate selection region session
//
// @param in_visibleObjects  The render context visible objects 
// @param in_frame           The frame time
//
void UpdateIsolateSelection(const CRefArray &in_visibleObjects, double in_frame)
{
   // if no objects are passed, then we're not in isolate selection mode
   LONG nbVisibleObjects = in_visibleObjects.GetCount();
   if (nbVisibleObjects == 0)
      return;

   CRefArray visibleObjects;
   // get the branch selection, as we do when rendering the selection only
   for (LONG i=0; i<in_visibleObjects.GetCount(); i++)
   {
      CRef ref(in_visibleObjects.GetItem(i));
      bool branchSel = ProjectItem(ref).GetSelected(siBranch);
      AddCRefToArray(visibleObjects, ref, branchSel);
   }

   // visibleObjects is the array to use from now on
   nbVisibleObjects = visibleObjects.GetCount();

   uint8_t visibility;

   // since one Softimage object could be referred to by many Arnold nodes, 
   // for instance with ICE-instanced shapes, let's get the visibility of each object
   // only once, and store it into a map of <CString (the softimage object name), int (its visibility)>
   map <CString, uint8_t> visibleObjectsNames;
   for (LONG i=0; i<nbVisibleObjects; i++)
   {
      CRef ref = visibleObjects.GetItem(i);
      X3DObject xsiObj(ref);

      CString objType = xsiObj.GetType();
      if (objType != L"light")
      {
         visibility = GetVisibility(xsiObj.GetProperties(), in_frame);
         // insert the object name together with the object visibility
         visibleObjectsNames.insert(pair < CString, uint8_t > (visibleObjects.GetItem(i).GetAsText(), visibility));
      }
   }

   CString softName;

   // Let's iterate over the Arnold nodes, and retrieve the Softimage object that originated it.
   // This is possible due to the good naming convention we established a few versions ago
   AtNodeIterator *iter = AiUniverseGetNodeIterator(NULL, AI_NODE_SHAPE);
   while (!AiNodeIteratorFinished(iter))
   {
      AtNode *node = AiNodeIteratorGetNext(iter);
      if (!node)
         break;
      CString nodeName = CNodeUtilities().GetName(node);
      // get the name of the Softimage object that originated node 
      softName = CStringUtilities().GetSoftimageNameFromSItoAName(nodeName);
      if (softName == L"")
         continue;

      visibility = 0;

      // is this object name part of the map we built ?
      map <CString, uint8_t>::iterator vizIter = visibleObjectsNames.find(softName);
      if (vizIter != visibleObjectsNames.end()) // visible
      {
         if (AiNodeGetByte(node, "visibility") != 0) // already visible, skip
            continue;
         else
            visibility = vizIter->second;
      }
      // else hide it, visibility stays initialized to 0

      CNodeSetter::SetByte(node, "visibility", visibility, true);
   }

   AiNodeIteratorDestroy(iter);
   visibleObjectsNames.clear();
   visibleObjects.Clear();
}



