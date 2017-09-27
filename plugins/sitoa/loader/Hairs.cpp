/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "loader/Hairs.h"
#include "loader/Instances.h"
#include "loader/Properties.h"
#include "loader/Procedurals.h"
#include "renderer/Renderer.h"

#include <xsi_group.h>
#include <xsi_model.h>
#include <xsi_polygonmesh.h>
#include <xsi_operator.h>
#include <xsi_port.h>

#include <cstring>
#include <vector>


// Collect the hair objects in the scene, sorted by the order they have to be exported
// The hair that instance other objects are pushed at the end. The reason is that if a hair A
// instances another hair B (#1493), B has to be loaded before A
//
// @param in_frame            The frame time
//
// @return the sorted hair objects array
//
CRefArray CollectSortedHairObjects(double in_frame)
{
   CRefArray sortedHair, instancingHair;

   CStringArray families;
   families.Add(siGeometryFamily);   
   CRefArray unsortedHair = Application().GetActiveSceneRoot().FindChildren(L"" ,L"", families, true);

   for (LONG i=0; i<unsortedHair.GetCount(); i++)
   {
      X3DObject hairObj(unsortedHair[i]);
      if (hairObj.GetType() != L"hair")
         continue;

      HairPrimitive hairPrimitive = static_cast<HairPrimitive>(CObjectUtilities().GetPrimitiveAtFrame(hairObj, in_frame));
      // is this an instancer hair object?
      if ((bool)ParAcc_GetValue(hairPrimitive, L"InstanceEnabled", in_frame))
      {
         SIObject group;
         if (GetInstanceGroupName(hairPrimitive, group))
         {
            // if so, add it to a different array, that will be copied later at the bottom of the result array
            instancingHair.Add(unsortedHair[i]);
            continue;
         }
      }

      sortedHair.Add(unsortedHair[i]);
   }
   // add the instancers at the end
   for (LONG i=0; i<instancingHair.GetCount(); i++)
      sortedHair.Add(instancingHair[i]);

   return sortedHair;
}


// Load all hair primitives into Arnold
//
// @param in_frame            The frame time
// @param in_selectedObjs     The selected objects in case in_selectionOnly is true
// @param in_selectionOnly    If true, render only the in_selectedObjs array
//
// @return CStatus::OK if all went well, else the error code
//
CStatus LoadHairs(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{
   CStatus status(CStatus::OK);

   if (GetRenderOptions()->m_ignore_hair)
      return CStatus::OK;

   CRefArray hairArray = CollectSortedHairObjects(in_frame);
 
   for (LONG i = 0; i < hairArray.GetCount(); ++i)
   {
      // check if this hair is selected
      if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, hairArray[i]))
         continue;

      X3DObject hairObj(hairArray[i]);
      status = LoadSingleHair(hairObj, in_frame);
      if (status != CStatus::OK)
         break;
   }

   return status;
}


// Get the instance group of a hair primitive.
//
// @param in_primitive    The input hair primitive
// @param out_group       The returned group
//
// @return true if a group is found, else false
// 
bool GetInstanceGroupName(const HairPrimitive& in_primitive, SIObject &out_group) 
{
   CRefArray nestedObjects = in_primitive.GetNestedObjects();
   for (int i=0; i < nestedObjects.GetCount(); i++)
   {
      SIObject nested = nestedObjects[i];
      if (nested.IsValid() && nested.GetName() == L"Hair" &&
          nested.GetType() == L"hair" && nested.IsA(siParameterID))
      {
         Parameter hairParameter(nested);
         CRefArray nestedObjects2 = hairParameter.GetNestedObjects();
         for (int j=0; j < nestedObjects2.GetCount(); j++)
         {
            SIObject nested2 = nestedObjects2[j];
            if (nested2.IsA(siGroupID))
            {
               // GetMessageQueue()->LogMsg(L"GetInstanceGroupName returning " + nested2.GetFullName());
               out_group = nested2;
               return true;
            }
         }
      }
   }

   return false;
}


// Return a random float between -1.0f and 1.0f
inline float sfrand(unsigned int *seed)
{
   union
   {
       float fres;
       unsigned int ires;
   };

   seed[0] *= 16807;
   *((unsigned int *) &ires) = ( ((unsigned int)seed[0])>>9 ) | 0x40000000;
   return fres-3.0f;
}


// Load a hair primitives into Arnold
//
// @param in_xsiObj           The hair object
// @param in_frame            The frame time
//
// @return CStatus::OK if all went well, else the error code
//
CStatus LoadSingleHair(const X3DObject &in_xsiObj, double in_frame)
{
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   if (!in_xsiObj.GetType().IsEqualNoCase(L"hair"))
         return CStatus::OK;

   HairPrimitive hairPrimitive = static_cast<HairPrimitive>(CObjectUtilities().GetPrimitiveAtFrame(in_xsiObj, in_frame));

   CRefArray hairProperties(in_xsiObj.GetProperties());
   
   // If invisible, return
   if (!ParAcc_GetValue(Property(hairProperties.GetItem(L"Visibility")), L"rendvis", in_frame))
      return CStatus::OK;

   CRefArray ra;
   // is this a procedural ?
   if (hairProperties.GetItem(L"arnold_procedural").IsValid())
      return LoadSingleProcedural(in_xsiObj, in_frame, ra, false);
   // is this a volume ?
   if (hairProperties.GetItem(L"arnold_volume").IsValid())
      return LoadSingleProcedural(in_xsiObj, in_frame, ra, false);
   
   if ((bool)ParAcc_GetValue(hairPrimitive, L"InstanceEnabled", in_frame))
   {
      SIObject group;
      if (GetInstanceGroupName(hairPrimitive, group))
         return LoadSingleHairInstance(in_xsiObj, group, in_frame);
   }

   bool enableMatte = !GetRenderOptions()->m_ignore_matte;

   // Getting Motion Blur Data
   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(in_xsiObj.GetRef(), transfKeys, defKeys, in_frame);

   // Parsing Material
   Material material(in_xsiObj.GetMaterial());

   AtNode* shaderNode = LoadMaterial(material, LOAD_MATERIAL_SURFACE, in_frame, in_xsiObj.GetRef());

   // Visibility & Sidedness
   uint8_t visibility = GetVisibility(hairProperties, in_frame);
   uint8_t sidedness;
   // we don't check for the returned value of GetSidedness, because the bin writer needs a sidedness to write anyway
   GetSidedness(hairProperties, in_frame, sidedness);

   // Getting light group
   AtArray* light_group = GetRenderInstance()->LightMap().GetLightGroup(in_xsiObj);

   CustomProperty paramsProperty, userOptionsProperty;
   hairProperties.Find(L"arnold_parameters", paramsProperty);
   hairProperties.Find(L"arnold_user_options", userOptionsProperty);

   // Calculating hairs
   float renderPercent = ((float)ParAcc_GetValue(hairPrimitive,L"RenderPercentage", in_frame)/100);
   LONG strandMult = (LONG) ParAcc_GetValue(hairPrimitive,L"StrandMult", in_frame);
   if (strandMult == 0)
      strandMult = 1;
   LONG totalHairs = (LONG) ((LONG)ParAcc_GetValue(hairPrimitive,L"TotalHairs", in_frame) * renderPercent * strandMult);
   
   // Hair ID to save as UserData 
   vector <uint32_t> hairsIDVector;
   uint32_t hairID = 0;
   
   // array to store the group members
   vector <AtNode*> memberVector;

   // Motion Blur Loop
   LONG nbDefKeys = defKeys.GetCount();
   
   for (LONG ikey=0; ikey<nbDefKeys; ikey++)
   {
      CRenderHairAccessor hairAccessor = hairPrimitive.GetRenderHairAccessor(totalHairs, CHUNK_SIZE, defKeys[ikey]);

      unsigned int nChunk = 0;
      // This method call calculates and prepares all the data we need
      // It takes a lot of time....
      while (hairAccessor.Next())
      {
         // Getting returned chunk size (it could be less that CHUNK_SIZE)
         LONG chunkSize = hairAccessor.GetChunkHairCount(); 
         
         // chunkSize can be 0 even if hairAccessor.Next() is true (ticket #1004)
         if (chunkSize == 0)
            break;
        
         // Get the number of vertices for each render hair
         // note: this array is used for iterating over the render hair position and radius values
         CLongArray verticesCountArray;
         hairAccessor.GetVerticesCount(verticesCountArray);
         // #1555. Check if there are hairs with 0 points. If so, abort. 
         // Do this only for the first key and chunk, it's a rare case happening only with the Melena plugin, so
         // this one-time check could trap it. And, we don't want to waste too much time doing the check for all the
         // keys and chunks.
         if (ikey == 0 && nChunk == 0)
         {
            for (LONG i=0; i<verticesCountArray.GetCount(); i++)
            {
               if (verticesCountArray[i] < 1)
               {
                  GetMessageQueue()->LogMsg(L"[sitoa] Found hair with 0 vertices for " + in_xsiObj.GetFullName() + L", aborting", siErrorMsg);
                  return CStatus::Fail;
               }
            }
         }
         
         // All hairs will have the same vertex number
         LONG nbVertices = verticesCountArray[0];

         // Get the render hair positions
         CFloatArray vertexPositions;
         hairAccessor.GetVertexPositions(vertexPositions);
         LONG nbPositions = vertexPositions.GetCount();

         // Get the render hair radius
         CFloatArray vertexRadius;
         hairAccessor.GetVertexRadiusValues(vertexRadius);
         LONG nbRadii = vertexRadius.GetCount();

         // Calculating data for catmull-rom
         // (2 vertex more = 6 floats more) * Number of hairs
         unsigned int nbPoints = nbVertices + 2;
         uint32_t nbFloat = vertexPositions.GetCount() + 6 * chunkSize;

         CString chunkNodeName = CStringUtilities().MakeSItoAName((SIObject)in_xsiObj, in_frame, L"", false) + 
                                 L"." + CValue((LONG)nChunk).GetAsText();

         AtNode* curvesNode = AiNodeLookUpByName(chunkNodeName.GetAsciiString());

         if (!curvesNode)
         {
            curvesNode = AiNode("curves");
            memberVector.push_back(curvesNode);
         }
       
         if (curvesNode)
         {
            if (ikey==0) 
            {
               CNodeUtilities().SetName(curvesNode, chunkNodeName);
               CNodeSetter::SetInt(curvesNode, "id", CObjectUtilities().GetId(in_xsiObj));
               CNodeSetter::SetString(curvesNode, "basis", "catmull-rom");
               
               // Setting default min pixel width to 0.25 (hardcoded ticket:351)
               CNodeSetter::SetFloat(curvesNode, "min_pixel_width", 0.25f);
               
               if (shaderNode)
                  AiNodeSetArray(curvesNode, "shader", AiArray(1, 1, AI_TYPE_NODE, shaderNode));

               CNodeSetter::SetByte(curvesNode, "visibility", visibility, true);
               CNodeSetter::SetByte(curvesNode, "sidedness", sidedness, true);

               if (paramsProperty.IsValid())
                  LoadArnoldParameters(curvesNode, paramsProperty.GetParameters(), in_frame);

               CNodeUtilities::SetMotionStartEnd(curvesNode);
               LoadUserOptions(curvesNode, userOptionsProperty, in_frame); // #680
               LoadUserDataBlobs(curvesNode, in_xsiObj, in_frame); // #728

               if (enableMatte)
               {
                  Property matteProperty;
                  hairProperties.Find(L"arnold_matte", matteProperty);
                  LoadMatte(curvesNode, matteProperty, in_frame);      
               }

               // Light Group
               if (light_group)
               {
                  // We have to duplicate the master AtArray*, we cant share the same array between objects
                  CNodeSetter::SetBoolean(curvesNode, "use_light_group", true);
                  if (light_group && AiArrayGetNumElements(light_group) > 0)
                     AiNodeSetArray(curvesNode, "light_group", AiArrayCopy(light_group));
               }
               
               // UVs
               bool uvs_done = false;
               LONG nbUvProperties = hairAccessor.GetUVCount();
               for (LONG uvPropertyIndex=0; uvPropertyIndex<nbUvProperties; uvPropertyIndex++)
               {
                  CString projectionName = hairAccessor.GetUVName(uvPropertyIndex);
               
                  bool skipProjection(false);
                  // loop the previous projections
                  for (LONG previousUvPropertyIndex=0; previousUvPropertyIndex<uvPropertyIndex; previousUvPropertyIndex++)
                  {
                     if (projectionName == hairAccessor.GetUVName(previousUvPropertyIndex))
                     {
                       // the projection name was already found (several texture map properties can share the same projection, #1593)
                        skipProjection = true;
                        break;
                     }
                  }
                  if (skipProjection)
                     continue;

                  // Get the UVs
                  CFloatArray uvValues;
                  hairAccessor.GetUVValues(uvPropertyIndex, uvValues);
                  LONG nbValues = uvValues.GetCount();

                  AtArray* uvs = AiArrayAllocate(nbValues/3, 1, AI_TYPE_VECTOR2);
                  
                  for (LONG i=0; i<nbValues; i+=3)
                  {
                     AtVector2 uv = AtVector2(uvValues[i], uvValues[i+1]);
                     AiArraySetVec2(uvs, i/3, uv);
                  }

                  if (!uvs_done)
                     AiNodeSetArray(curvesNode, "uvs", uvs);
                  // else, declaring the uv set as user data
                  else if (AiNodeDeclare(curvesNode, projectionName.GetAsciiString(), "uniform VECTOR2"))
                     AiNodeSetArray(curvesNode, projectionName.GetAsciiString(), uvs);

                  uvs_done = true;
               }

               // CAV, #1475
               LONG nbCavProperties = hairAccessor.GetVertexColorCount();
               for (LONG cavIndex=0; cavIndex<nbCavProperties; cavIndex++)
               {
                  CFloatArray cavValues;
                  CString cavName = hairAccessor.GetVertexColorName(cavIndex);
                  hairAccessor.GetVertexColorValues(cavIndex, cavValues);
                  LONG nbValues = cavValues.GetCount();

                  if (nbValues < 1)
                     continue;

                  if (AiNodeDeclare(curvesNode, cavName.GetAsciiString(), "uniform RGBA"))
                  {
                     AtArray* rgba = AiArrayAllocate(nbValues/4, 1, AI_TYPE_RGBA);
                     for (LONG i=0; i<nbValues; i+=4)
                     {
                        AtRGBA color = AtRGBA(cavValues[i], cavValues[i+1], cavValues[i+2], cavValues[i+3]);
                        AiArraySetRGBA(rgba, i/4, color);
                     }
                     AiNodeSetArray(curvesNode, cavName.GetAsciiString(), rgba);
                  }
               }

               // Get matrix transform for this chunk (cant share matrix array between chunks)
               unsigned int nbTransfKeys = transfKeys.GetCount();
               AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);

               for (unsigned int ikey=0; ikey<nbTransfKeys; ikey++)
               {
                  AtMatrix matrix;
                  CUtilities().S2A(in_xsiObj.GetKinematics().GetGlobal().GetTransform(transfKeys[ikey]).GetMatrix4(), matrix);
                  AiArraySetMtx(matrices, ikey, matrix);
               }

               // Setting matrix
               AiNodeSetArray(curvesNode, "matrix", matrices);
     
               // + 2 for harcoded catmull-rom basis
               AiNodeSetArray(curvesNode, "num_points", AiArray(1, 1, AI_TYPE_UINT, nbPoints));

               // Allocating radius Array
               AtArray* radiusArray = AiArrayAllocate(nbRadii, 1, AI_TYPE_FLOAT);
               
               for (LONG i=0; i<nbRadii; i++)
                  AiArraySetFlt(radiusArray, i, vertexRadius[i]);

               AiNodeSetArray(curvesNode, "radius", radiusArray);

               // Allocating and assigning the points array once. Later we will get that array and set the keys
               AtArray* totalPoints = AiArrayAllocate(nbFloat, (uint8_t)nbDefKeys, AI_TYPE_FLOAT);
               AiNodeSetArray(curvesNode, "points", totalPoints);
            }

            // points array for this key
            AtArray* pointsArray = AiArrayAllocate(nbFloat, 1, AI_TYPE_FLOAT);

            // For catmull-rom we need to repeat the first and last points for each curve
            LONG iposition = 0;
            LONG posPerHair = (LONG)(nbPositions/chunkSize);

            for (LONG ihair=0; ihair<chunkSize; ihair++)
            {
               unsigned int idx = ihair*posPerHair;
               
               //Adding first coordinates for catmull-rom       
               AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx]);
               AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx+1]);
               AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx+2]);
             
               // Filling the points
               for (LONG i=0; i<posPerHair; i++)
                  AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx+i]);
               
               // Adding last coordinates again for catmull-rom (need 1 vertex more)
               AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx+posPerHair-3]);
               AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx+posPerHair-2]);
               AiArraySetFlt(pointsArray, iposition++, vertexPositions[idx+posPerHair-1]);

               // Adding Hair ID to list
               if (ikey==0) 
                  hairsIDVector.push_back(hairID++);
            }

            // Setting Array to Node Points Array to its key
            AtArray* totalPoints = AiNodeGetArray(curvesNode, "points");
            AiArraySetKey(totalPoints, (uint8_t)ikey, AiArrayMap(pointsArray)); // direct access, deprecated
            // destroy pointsArray once its data has been copied
            AiArrayDestroy(pointsArray);
         }

         // Adding Hair IDs to Chunk
         if (ikey==0 && hairsIDVector.size()>0)
            if (AiNodeDeclare(curvesNode, "curve_id", "uniform UINT"))
               AiNodeSetArray(curvesNode, "curve_id", AiArrayConvert((int)hairsIDVector.size(), 1, AI_TYPE_UINT, (uint32_t*)&hairsIDVector[0]));
         
         hairsIDVector.clear();

         nChunk++;
      } // while (hairAccessor.Next())
   } // mb loop

   // export the members as a group
   if (memberVector.size() > 0)
   {
      // Checking that the node doesnt exists
      if (!GetRenderInstance()->NodeMap().GetExportedNode(in_xsiObj, in_frame))
         GetRenderInstance()->GroupMap().PushGroup(&memberVector, in_xsiObj, in_frame);
   }

   // Releasing lights AtArray* Master
   // It has been duplicated and assigned to each object
   AiArrayDestroy(light_group);
   return CStatus::OK;
}


// Get the map connected to a hair object, driving some instancing distribution, like fuzziness
// 
// @param in_xsiObj           The hair object
// @param in_connectionName   The map port to be evaluated
//
// @return the map (usually a weight map) name
// 
CString GetConnectedMapName(const X3DObject &in_xsiObj, CString in_connectionName)
{
   CRef ref;
   CString refS = in_xsiObj.GetFullName() + L"." + in_connectionName + L".MapCompOp";
   ref.Set(refS);
   if (ref.IsValid())
   {
      Operator op(ref);
      if (op.IsValid())
      {
         Port port = op.GetPortAt(1,2,0);
         if (port.IsValid())
         {
            CRef tRef = port.GetTarget();
            ClusterProperty clusterProp(tRef);
            if (clusterProp.IsValid())
               return clusterProp.GetName();
         }
      }
   }

   return L"";
}


// Get the mesh and hair objects under a given object hierarchy or model, used for hair instancing
// Skip the objects with the procedural property applied
// 
// @param in_xsiObj    The input model or object
// @param in_frame     The frame time
//
// @return the CRef array of meshes
// 
CRefArray GetMeshesAndHairBelowMaster(const X3DObject &in_xsiObj, double in_frame)
{
   CRefArray result, children;
   CStringArray families;
   families.Add(siGeometryFamily);

   if (in_xsiObj.IsA(siModelID))
   {
      Model model(in_xsiObj.GetRef());
      if (model.GetModelKind() == siModelKind_Instance)
         model = model.GetInstanceMaster();
         
      children = model.FindChildren(L"" ,L"", families, true);
   }
   else
      children = in_xsiObj.FindChildren(L"" ,L"", families, true);

   for (LONG i=0; i<children.GetCount(); i++)
   {
      X3DObject obj(children[i]);
      CString objType = obj.GetType();
      if (objType == siPolyMeshType || objType == "hair")
      {
         // #1449: skip objects with procedural property applied. We need actual geo to bend it on hair
         CRefArray polyProperties = in_xsiObj.GetProperties();
         if (polyProperties.GetItem(L"arnold_procedural").IsValid())
            GetMessageQueue()->LogMsg(L"[sitoa] Can't instantiate procedural object " + obj.GetFullName() + L" on hair." , siWarningMsg);
         else
            result.Add(children[i]);
      }
   }

   return result;
}


// Create objects bended around the strands a hair object
//
// @param in_xsiObj          The hair object
// @param in_group           The group of objects to instanciate
// @param in_frame           The frame time
//
// @return CStatus::OK
// 
CStatus LoadSingleHairInstance(const X3DObject &in_xsiObj, SIObject in_group, double in_frame)
{
   HairPrimitive hairPrimitive = static_cast<HairPrimitive>(CObjectUtilities().GetPrimitiveAtFrame(in_xsiObj, in_frame));

   CRefArray hairProperties(in_xsiObj.GetProperties());
   Group group(in_group);

   // Getting Motion Blur Data from the hair
   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(in_xsiObj.GetRef(), transfKeys, defKeys, in_frame);

   // Calculating hairs
   float renderPercent = ((float)ParAcc_GetValue(hairPrimitive, L"RenderPercentage", in_frame)/100);
   LONG strandMult = (LONG) ParAcc_GetValue(hairPrimitive, L"StrandMult", in_frame);
   if (strandMult == 0)
      strandMult = 1;
   LONG totalHairs = (LONG)((LONG)ParAcc_GetValue(hairPrimitive, L"TotalHairs", in_frame) * renderPercent * strandMult);

   int instanceGroupAssignmentType = (LONG)ParAcc_GetValue(hairPrimitive, L"InstanceGroupAssignmentType", in_frame);
   //0 == random, 1 == weight map
   int instanceOrientationType = (LONG)ParAcc_GetValue(hairPrimitive, L"InstanceOrientationType", in_frame);
   //0 == none, 2 == tangent map, 3 == follow object (WHAT ?!?)

   // if assignment is by weighmap, get the map
   CString assignmentWeightMapName(L"");
   if (instanceGroupAssignmentType == 1)
   {
      assignmentWeightMapName = GetConnectedMapName(in_xsiObj, L"InstanceGroupAssignmentMap");
      if (assignmentWeightMapName == L"") // failed wm lookup, so default back to random assignment
         instanceGroupAssignmentType = 0;
   }
   float instanceGroupAssignmentMapFuzziness = 0.0f;
   if (instanceGroupAssignmentType == 1)
   {
      instanceGroupAssignmentMapFuzziness = (float)ParAcc_GetValue(hairPrimitive, L"InstanceGroupAssignmentMapFuzziness", in_frame);
      instanceGroupAssignmentMapFuzziness/= 100.0f; // go to 0..1
   }

   // if rotation is by tangentmap, get the map
   CString tangentMapName(L"");
   if (instanceOrientationType == 3) // follow objects
      instanceOrientationType = 0;
   if (instanceOrientationType == 2)
   {
      tangentMapName = GetConnectedMapName(in_xsiObj, L"InstanceOrientationTangentMap");
      if (tangentMapName == L"") // failed wm lookup, so default back to random assignment
         instanceOrientationType = 0;
   }

   float instanceOrientationSpread=0;
   if (instanceOrientationType == 2)
   {
      instanceOrientationSpread = (float)ParAcc_GetValue(hairPrimitive, L"InstanceOrientationSpread", in_frame);
      instanceOrientationSpread*= (float)AI_DTOR; // go to rad
   }

   // Visibility & Sidedness. Get them only if found under the hair, else they the clones' flags
   // are left untouched (copied together with the rest of the clone)
   uint8_t hairVisibility = AI_RAY_ALL;
   uint8_t hairSidedness;
   Property prop, arnoldParameters, userOptionsProperty;

   hairProperties.Find(L"arnold_visibility", prop);
   bool arnoldVisibilityOnHair = prop.IsValid();
   if (arnoldVisibilityOnHair)
      hairVisibility = GetVisibility(hairProperties, in_frame);

   bool arnoldSidednessOnHair = GetSidedness(hairProperties, in_frame, hairSidedness);

   hairProperties.Find(L"arnold_parameters", arnoldParameters);
   hairProperties.Find(L"arnold_user_options", userOptionsProperty);

   // Motion Blur Loop
   unsigned int nbTransfKeys = transfKeys.GetCount();
   unsigned int nbDefKeys = defKeys.GetCount();

   // If deform mb is on, we need to decide which deform key we have to use to evaluate the master objects.
   // In fact, both the master objects and the hair could have (different) settings.
   // However, only the hair deformation will affect the instances' mb, so we will pick just ONE footprint
   // of the master objects. Let's decide based on the shatter position.
   LONG onFrame=2; // default to "start of frame"
   if (nbDefKeys > 1)
      onFrame = GetRenderOptions()->m_motion_shutter_onframe;

   // arrays to get/store the informations to be cloned from the masters
   AtArray *vlist(NULL), *nlist(NULL), *vidxs(NULL), *nidxs(NULL);

   // Each element of the group to be cloned could be a model.
   // So, for each group element we'll have a vector of shapes, which will be of size 1 (so the element itself)
   // if the element is a mesh instead of a model
   vector < vector < CStrandInstance > > strandInstances;
   // The same to the AtNode*'s that we'll clone. 
   vector < vector <AtNode* > > masterNodes;

   // The size these 2 vectors is equal to the number of members of the group
   strandInstances.resize(group.GetMembers().GetCount()); 
   masterNodes.resize(group.GetMembers().GetCount());
   
   AtNode *masterNode;
   X3DObject masterObj;
   // let's loop over the group members
   for (LONG i=0; i<group.GetMembers().GetCount(); i++)
   {
      CRef masterRef = group.GetMembers().GetItem(i);
      X3DObject groupObj(masterRef);
      // The group element could be a model, so get all the polymesh and hair for this model
      // If it's not a model, the groupObj itself is returned
      CRefArray masterObjs = GetMeshesAndHairBelowMaster(groupObj, in_frame);
      // Resize the containers for the i-th group element
      strandInstances[i].resize(masterObjs.GetCount());
      masterNodes[i].resize(masterObjs.GetCount());

      // Loop over the model's shapes (or just once if the group element was a plain mesh)
      int nbValidShapes = 0;
      for (LONG shapeIndex=0; shapeIndex<masterObjs.GetCount(); shapeIndex++)
      {
         masterObj = (X3DObject)masterObjs.GetItem(shapeIndex);

         // Get the local transformation, so with respect to the object's model
         // So, all the meshes under the same model will respect the model's center, and
         // so keep their "position" with respect to the other members of the model
         CTransformation masterObjTransform, groupModelTransform;
         // if masterObj child of groupObj ?
         bool isHierarchy = CObjectUtilities().HasParent(masterObj, groupObj);

         if (groupObj.IsA(siModelID) || isHierarchy)
         {
            // We need the local transf with respect to the group element.
            // In other words, all the shapes under the group element (and those under submodels)
            // must always be expressed in local coordinates with respect to the group element
            // Get the shape transf
            masterObjTransform = masterObj.GetKinematics().GetGlobal().GetTransform();
            // Get the group elem transf and invert it
            groupModelTransform = groupObj.GetKinematics().GetGlobal().GetTransform();
            groupModelTransform.InvertInPlace();
            // And multiply
            masterObjTransform.Mul(masterObjTransform, groupModelTransform);
         }
         else // If the object is a plain object, just get its scale
         {
            masterObjTransform = masterObj.GetKinematics().GetLocal().GetTransform();
            masterObjTransform.SetTranslation(CVector3(0.0, 0.0, 0.0));
            masterObjTransform.SetRotationFromXYZAngles(CVector3(0.0, 0.0, 0.0));
         }

         // Get the master node at the appropriate frame time.
         // We don't care about the master transformation keys, since the instance def mb
         // will just depend on the hair deformation. 
         // So, we just get the master at a single time, depending on the mb position settings
         CDoubleArray masterTransfKeys, masterDefKeys;
         CRefArray masterProperties(masterObj.GetProperties());
         CSceneUtilities::GetMotionBlurData(masterRef, masterTransfKeys, masterDefKeys, in_frame);
         LONG nbMasterTransfKeys = masterTransfKeys.GetCount();
         double masterFrame;
         if (onFrame == 0) // center of frame
            masterFrame = (masterTransfKeys[0] + masterTransfKeys[nbMasterTransfKeys-1]) * 0.5f;
         else if (onFrame == 1) // end of frame
            masterFrame = masterTransfKeys[nbMasterTransfKeys-1];
         else // start of frame
            masterFrame = masterTransfKeys[0];

         // So get the master node at masterFrame time
         vector <AtNode*> nodes;
         bool isGroup(false);
         masterNode = GetRenderInstance()->NodeMap().GetExportedNode(masterObj, masterFrame);
         if (!masterNode)  
         {
            vector <AtNode*> *tempV;
            if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, masterObj, masterFrame))
            {
               nodes = *tempV;
               isGroup = true;
               // get only the first chunk, not to overcomplicate the code. 
               // The assumption is that nobody will instance
               // hair A on hair B, with A made of more than CHUNK_SIZE (300K) hairs
               masterNode = nodes[0];
            }
         }

         if (masterNode)
         {
            // Store the master node
            masterNodes[i][nbValidShapes] = masterNode;
            // Get the geo stuff from the master object.
            if (CNodeUtilities().GetEntryName(masterNode) == L"curves")
            {
               // use the vlist array to store the only curves array we care, so "points"
               // we don't bother about orientations
               vlist = AiNodeGetArray(masterNode, "points");
               nlist = vidxs = nidxs = NULL;
            }
            else
            {
               vlist = AiNodeGetArray(masterNode, "vlist");
               nlist = AiNodeGetArray(masterNode, "nlist");
               vidxs = AiNodeGetArray(masterNode, "vidxs");
               nidxs = AiNodeGetArray(masterNode, "nidxs");
            }
            // Create the instance object. It will make a local copy of the masterNode vertices,
            // and normals, and allocate the buffer for the bended version of the shape,
            // that will then be read when actually cloning the master AtNode
            // Do not compute the bounding cylinder now. In fact, if the group element is a model,
            // the bounding cylinder has to be computed for the whole model, not on the single objects
            // The instance is stored in the vector of the instances for the i-th group member
            strandInstances[i][nbValidShapes].Init(vlist, nlist, vidxs, nidxs, masterObjTransform, masterObj);
            nbValidShapes++;
         }

      }

      strandInstances[i].resize(nbValidShapes);
      masterNodes[i].resize(nbValidShapes);

      // We must now compute the cylindrical coordinates for the instances.
      // ComputeModelBoundingCylinder will loop all the members of hairInstances[i],
      // and store the bounding cylinder (so of the WHOLE model) into the bounding cylinder
      // structure of strandInstances[i][0]
      for (int shapeIndex=0; shapeIndex<nbValidShapes; shapeIndex++)
      {
         if (shapeIndex == 0) // so we do it only once for the whole model.
            strandInstances[i][0].ComputeModelBoundingCylinder(strandInstances[i]);
         // DON'T move the above call outsude if the "for", bacause masterObjs.GetCount() could be 0 if
         // one of the groups members is a null. In that case, strandInstances[i][0] would not exist and crash

         // for all the further shapes of the i-th model, copy the bounding cylinder information
         // from strandInstances[i][0]. In other words, all the shapes under the same model share
         // the same bounding cylinder.
         else
            strandInstances[i][shapeIndex].m_boundingCylinder.CopyBoundaries(strandInstances[i][0].m_boundingCylinder);
         // Using the above bounding cylinder, remap the points to cylindrical coordinates
         strandInstances[i][shapeIndex].RemapPointsToCylinder();
      }
   }

   if (masterNodes.size() < 1)
      return CStatus::OK;

   bool enableMatte = !GetRenderOptions()->m_ignore_matte;

   // Compute the total amount of strands
   int totalNumberOfStrands = 0;
   CRenderHairAccessor hairAccessor = hairPrimitive.GetRenderHairAccessor(totalHairs, CHUNK_SIZE, defKeys[0]);
   while (hairAccessor.Next())
   {
      // Getting returned chunk size (it could be less that CHUNK_SIZE)
      LONG chunkSize = hairAccessor.GetChunkHairCount(); 
      // chunkSize can be 0 even if hairAccessor.Next() is true (ticket #1004)
      if (chunkSize == 0)
         break;
      totalNumberOfStrands+= chunkSize;
   }

   // (*) After cloning, we'll store the pointer for further usage into clonedNodes
   // In fact, with mb is on, we'll need to keep setting the clones's points and normals, while looping over the mb keys.
   // The hairAccessor loop is INSIDE the mb loop, and it has to be so, since the hairAccossors have to be evaluated
   // at the key time values! But, of course, in such case we must edit the points of the clones that are created
   // ONLY at the first key time of the mb loop. So, that's why we need to store a copy of the created AtNode*'s
   // into clonedNodes. So, also in this case clonedNodes must be a vector (the strands) of vectors (the shapes of each
   // group element that is cloned on that strand)
   vector < vector < AtNode* > > clonedNodes;
   // clonedNodes's size is the total number of strands, else we would loose it while cycling the hair accessors
   clonedNodes.resize(totalNumberOfStrands);

   // Set up completed, now we'll clone the instances over the strands

   CStrandInstance *strandInstance;
   int instanceIndex;
   float wmValue;
   unsigned int seed = 666;
   float r; // random value
   CString name, masterNodeName;

   // Outest cycle, over the hair number of mb deform keys
   for (unsigned int iDefKey=0; iDefKey<nbDefKeys; iDefKey++)
   {
      hairAccessor = hairPrimitive.GetRenderHairAccessor(totalHairs, CHUNK_SIZE, defKeys[iDefKey]);
      int currentStrandIndex = 0;

      while (hairAccessor.Next())
      {
         LONG chunkSize = hairAccessor.GetChunkHairCount(); 
         // chunkSize can be 0 even if hairAccessor.Next() is true (ticket #1004)
         if (chunkSize == 0)
            break;

         CHair hair;
         // Translate the hairAccossor data into our more friendly class
         hair.BuildFromXsiHairAccessor(hairAccessor, assignmentWeightMapName, tangentMapName, instanceOrientationSpread);
         // hair.Log();

         for (int strandIndex=0; strandIndex<hair.GetNbStrands(); strandIndex++)
         {
            // let's decide the group element to use
            if (instanceGroupAssignmentType == 0) // random -> use modulo
               instanceIndex = strandIndex%masterNodes.size();
            else // weightmap
            {
               wmValue = hair.m_strands[strandIndex].GetWeightMapValue(); // 0.0-1.0 (clamped  at SetWeightMapValue time)
               if (instanceGroupAssignmentMapFuzziness > 0.0f)
               {
                  r = sfrand(&seed) * instanceGroupAssignmentMapFuzziness; // -1.0 - 1.0
                  wmValue+= r * (1.0f / (float)masterNodes.size()); // displace it by +- 1.0f / (float)masterNodes.size()
                  if (wmValue < 0.0f)
                     wmValue = 0.0f;
                  if (wmValue > 1.0f) 
                     wmValue = 1.0f;
               }

               instanceIndex = (int)(wmValue * (float)masterNodes.size()); // turn it into an int

               if (instanceIndex > (int)masterNodes.size()-1) // you never know...
                  instanceIndex = (int)masterNodes.size()-1;
            }

            // If this is the first mb loop, let's allocate the clonedNodes for the current strand
            // See (*) comment above
            if (iDefKey == 0)
               clonedNodes[currentStrandIndex].resize(strandInstances[instanceIndex].size());

            // Loop all the shapes that we need to clone on this strand
            for (unsigned int j=0; j<strandInstances[instanceIndex].size(); j++)
            {
               // Get the instancing object, and its corresponding master AtNode*
               strandInstance = &strandInstances[instanceIndex][j];
               masterNode = masterNodes[instanceIndex][j];

               AtNode *cloneNode = NULL;
               if (iDefKey == 0)
               {
                  // Clone the master node, and assign it to the clonedNodes array
                  cloneNode = AiNodeClone(masterNode);
                  clonedNodes[currentStrandIndex][j] = cloneNode;

                  if (CNodeUtilities().GetEntryName(cloneNode) == L"curves")
                  {
                     // allocate the points array for the curves node 
                     vlist = AiArrayAllocate((int)strandInstance->m_points.size(), (uint8_t)nbDefKeys, AI_TYPE_VECTOR);
                     nlist = NULL;
                     // Give the arrays to the cloned node
                     AiNodeSetArray(cloneNode, "points", vlist);
                  }
                  else
                  {
                     // Allocate as many vertices and normals as needed.
                     // We need to allocate, instead of re-using the master vectors, since the number
                     // of mb keys could differ from the master ones.
                     vlist = AiArrayAllocate((int)strandInstance->m_points.size(), (uint8_t)nbDefKeys, AI_TYPE_VECTOR);
                     nlist = AiArrayAllocate((int)strandInstance->m_normals.size(), (uint8_t)nbDefKeys, AI_TYPE_VECTOR);
                     // Give the arrays to the cloned node
                     AiNodeSetArray(cloneNode, "vlist", vlist);
                     AiNodeSetArray(cloneNode, "nlist", nlist);
                  }
               }
               else // Retrieve the clone from the currentStrandIndex-th strand, so to store the extra mb points/vectors
               {
                  cloneNode = clonedNodes[currentStrandIndex][j];
                  if (CNodeUtilities().GetEntryName(cloneNode) == L"curves")
                  {
                     // for curves, get the points array into vlist
                     vlist = AiNodeGetArray(cloneNode, "points");
                     nlist = NULL;
                  }
                  else
                  {
                     vlist = AiNodeGetArray(cloneNode, "vlist");
                     nlist = AiNodeGetArray(cloneNode, "nlist");
                  }
               }

               // Bend the instanced objects along the strand
               strandInstance->BendOnStrand(hair.m_strands[strandIndex]);
               // Assign the bended points/normals to the iDefKey-th array of vlist and nlist
               strandInstance->Get(vlist, nlist, iDefKey);

               // Set the matrices on the clone. Let's do it only once, not for every deform step
               if (iDefKey == 0)
               {
                  AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);
                  AtMatrix matrix;
                  for (unsigned int iTransfKey=0; iTransfKey<nbTransfKeys; iTransfKey++)
                  {
                     // Get the transform matrices of the hair object and assign them to the clone
                     CUtilities().S2A(in_xsiObj.GetKinematics().GetGlobal().GetTransform(transfKeys[iTransfKey]), matrix);
                     AiArraySetMtx(matrices, iTransfKey, matrix);
                  }
                  AiNodeSetArray(cloneNode, "matrix", matrices);
                  // other thing to do only once:
                  // overwrite visibility and sidedness if found on the hair object
                  // if the hair has an arnold viz prop, just use it
                  if (arnoldVisibilityOnHair)
                     CNodeSetter::SetByte(cloneNode, "visibility", hairVisibility, true);
                  else 
                  {
                     // check the master's "Instance Master Hidden" flag
                     CRefArray masterProperties = strandInstance->m_masterObject.GetProperties();
                     Property visProperty = masterProperties.GetItem(L"Visibility");
                     bool hideMaster = ParAcc_GetValue(visProperty, L"hidemaster", in_frame);
                     if (hideMaster) // the master was hidden, but we are not. So we need to retrieve the object visibility
                        CNodeSetter::SetByte(cloneNode, "visibility", GetVisibility(masterProperties, in_frame, false), true);
                     // else, the visibility stays the same as the master (because it's a clone)
                  }
                  
                  if (arnoldSidednessOnHair)
                     CNodeSetter::SetByte(cloneNode, "sidedness", hairSidedness, true);
                  // overwrite parameters if found on the hair object
                  if (arnoldParameters.IsValid())
                     LoadArnoldParameters(cloneNode, arnoldParameters.GetParameters(), in_frame);
                  // else, the sidedness stays the same as the master (because it's a clone)

                  CNodeUtilities::SetMotionStartEnd(cloneNode);
                  LoadUserOptions(cloneNode, userOptionsProperty, in_frame); // #680
                  LoadUserDataBlobs(cloneNode, in_xsiObj, in_frame); // #728

                  if (enableMatte)
                  {
                     Property matteProperty;
                     hairProperties.Find(L"arnold_matte", matteProperty);
                     LoadMatte(cloneNode, matteProperty, in_frame);      
                  }

                  // set the name. The id stays the same as the master
                  masterNodeName = CNodeUtilities().GetName(masterNode);

                  name = CStringUtilities().MakeSItoAName((SIObject)in_xsiObj, in_frame, L"", false);
                  name = name + L"." + (CString)(CValue)currentStrandIndex + L" " + masterNodeName;

                  CNodeUtilities().SetName(cloneNode, name);
               }
            }

            // Increment the current global strand index
            currentStrandIndex++;
         }
      }
   }

   // flattening down the cloned nodes, to push them as a group
   vector < AtNode* > members;

   vector < vector < AtNode* > >::iterator it1;
   vector < AtNode* >::iterator it2;
   
   for (it1 = clonedNodes.begin(); it1 != clonedNodes.end() ; it1++)
   for (it2 = it1->begin(); it2 != it1->end() ; it2++)
      members.push_back(*it2);

   GetRenderInstance()->GroupMap().PushGroup(&members, in_xsiObj, in_frame);

   members.clear();

   return CStatus::OK;
}

