/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "common/Tools.h"
#include "loader/ICE.h"
#include "loader/Instances.h"
#include "loader/Lights.h"
#include "loader/Loader.h"
#include "loader/Properties.h"
#include "loader/Shaders.h"
#include "loader/Procedurals.h"
#include "renderer/Renderer.h"
#include "renderer/RenderTree.h"

#include <vector>
#include <cstdio>


///////////////////////////////////////////////////////////////////////
// CIceUtilities class
///////////////////////////////////////////////////////////////////////

// Convert a CRotationf to a CRotation
// Rotation attributes come and we store them in float, but we need to go to long
// because of the methods not supported by CRotatoinf, and because of #806
//
// @param in_r  The input CRotationf rotation
// @return the rotation as a CRotation 
//
CRotation CIceUtilities::RotationfToRotation(const CRotationf &in_r)
{
   CRotation result;
   switch (in_r.GetRepresentation())
   {
      case CRotationf::siEulerRot:
      {
         CVector3f tmpAngles = in_r.GetXYZAngles();
         result.SetFromXYZAngles(tmpAngles.GetX(), tmpAngles.GetY(), tmpAngles.GetZ());
         break;
      }
      case CRotationf::siAxisAngleRot:
      case CRotationf::siQuaternionRot:
      {
         // Forcing AxisAngle in siQuaternionRot because quat was giving different results in different versions of Softimage (trac #806)
         float tmpAngle;
         CVector3f tmpAxis = in_r.GetAxisAngle(tmpAngle);
         result.SetFromAxisAngle(CVector3(tmpAxis.GetX(), tmpAxis.GetY(), tmpAxis.GetZ()), tmpAngle);
         break;
      }                     
   }
   return result;
}


// Check if in_attributeName begins by "ArnoldLight" (case insensitive). 
// If so, return the string following "ArnoldLight" in lower case.
// For instance, if in_attributeName=="ArnoldLightIntensity", true is returned, and out_parameterName set to "intensity"
//
// @param in_attributeName   The input attribute name
// @param out_parameterName  The returned correspond Arnold parameter name
//
// @return true if in_attributeName begins with "ArnoldLight", else false
//
bool CIceUtilities::GetArnoldLightParameterFromAttribute(CString in_attributeName, CString &out_parameterName)
{
   CString attributeName = CStringUtilities().ToLower(in_attributeName);
   if (attributeName.FindString(g_ArnoldLightAttributePrefix) == 0)
   {
      out_parameterName = attributeName.GetSubString(g_ArnoldLightAttributePrefix.Length());
      return true;
   }
   return false;
}


// Check if an object has a procedural property AND any of the string parameters are "ArnoldProcedural"
//
// @param in_xsiObj           The input object
// @param in_frame            The frame time
// @param out_path            The returned procedural path parameter
// @param out_scale           The returned scale value
//
// @return true is found, else false
//
bool CIceUtilities::ObjectHasArnoldProceduralProceduralProperty(X3DObject in_xsiObj, double in_frame, CString &out_path)
{
   Property proceduralProperty = in_xsiObj.GetProperties().GetItem(L"arnold_procedural");
   if (!proceduralProperty.IsValid())
      return false;

   bool result(false);

   out_path = ParAcc_GetValue(proceduralProperty, L"filename", in_frame).GetAsText();
   if (out_path.IsEqualNoCase(g_ArnoldProceduralAttributePrefix))
      result = true;

   return result;
}


// Check if if any object under a model has a procedural property AND any of the string parameters are "ArnoldProcedural"
//
// @param in_model           The input model
// @param in_frame           The frame time
//
// @return true is found, else false
bool CIceUtilities::ModelHasArnoldProceduralProceduralProperty(const Model &in_model, double in_frame)
{
   CString proceduralPath;
   CString objType;

   // all the mesh objects under this model
   CRefArray objArray = in_model.FindChildren(L"" ,L"", CStringArray(), true);
   for (LONG i = 0; i < objArray.GetCount(); i++)
   {
      X3DObject obj(objArray[i]);
      if (!obj.IsValid())
         continue;
      CString objType = obj.GetType();
      // check if this is an allowed procedural placeholder
      if ( objType.IsEqualNoCase(L"polymsh") || 
           objType.IsEqualNoCase(L"hair") || 
           objType.IsEqualNoCase(L"pointcloud") )
      {
         if (ObjectHasArnoldProceduralProceduralProperty(obj, in_frame, proceduralPath))
            return true;
      }
   }

   return false;
}


// Check if in_attributeName begins by "ArnoldProcedural" (case insensitive). 
// If so, return the string following "ArnoldProcedural" in lower case.
//
// @param in_attributeName   The input attribute name
// @param out_parameterName  The returned correspond Arnold parameter name
// @param in_lower           If true, return the param name all lower case (Arnold style)
//
// @return true if in_attributeName begins with "ArnoldProcedural", else false
//
bool CIceUtilities::GetTrimmedArnoldProceduralAttributeName(CString in_attributeName, CString &out_parameterName, bool in_lower)
{
   CString attributeName = CStringUtilities().ToLower(in_attributeName);
   if (attributeName.FindString(g_ArnoldProceduralAttributePrefix) == 0)
   {
      
      if (in_lower) 
         out_parameterName = attributeName.GetSubString(g_ArnoldProceduralAttributePrefix.Length());
      else // use in_attributeName instead of attributeName, se we also get the letters case
         out_parameterName = in_attributeName.GetSubString(g_ArnoldProceduralAttributePrefix.Length());
      return true;
   }
   return false;
}


// Check if the pointcloud has the same number of points at all the mb times.
// In case of dynamic emission, the count is very likely to change, and this will not 
// allow us to do exact mb.
//
// @param in_xsiObj                 The input object
// @param in_pointCount             The point count at in_frame
// @param in_transfKeys             The transformation mb keys
// @param in_defKeys                The deformation mb keys
// @param in_frame                  The current (before mb) rendering frame
// @param out_doExactTransformMb    The returned value for transformation mb: true if all the keys have the same number of points, else false
// @param out_doExactDeformMb       The returned value for deformation mb: true if all the keys have the same number of points, else false
//
void CIceUtilities::IsPointCountTheSameAtAllMbSteps(const X3DObject &in_xsiObj, LONG in_pointCount, CDoubleArray in_transfKeys, CDoubleArray in_defKeys, 
                                                    double in_frame, bool &out_doExactTransformMb, bool &out_doExactDeformMb)
{
   double keyTime;
   LONG   iKey, pointCount;
   set <double> okTransfKeys;    // to store all the transf keys for which the test was successfull
   double failingTransfKey(0.0); // the first (if any) transformation key for which the test failed, initialized to 0 just not to make the compiler complain

   // Get the transf mb keys with in_frame (if it is equal to one of the keys) moved into the first position,
   // so to save one GetGeometryAtFrame evaluation, because GetGeometry(in_frame) has already been pulled
   // by LoadSinglePointCloud.
   // Else, transfKeys stays equal to in_transfKeys
   CLongArray keysPosition; // not used, since we're not pushing any data, just checking the points count
   CDoubleArray transfKeys = CSceneUtilities::OptimizeMbKeysOrder(in_transfKeys, keysPosition, in_frame);

   // transformation
   out_doExactTransformMb = true;
   for (iKey = 0; iKey < transfKeys.GetCount(); iKey++)
   {
      keyTime = transfKeys[iKey];

      Geometry mbXsiGeo = CObjectUtilities().GetGeometryAtFrame(in_xsiObj, keyTime);

      ICEAttribute mbPointPositionAttribute = mbXsiGeo.GetICEAttributeFromName(L"PointPosition");
      pointCount = mbPointPositionAttribute.GetElementCount();
      if (pointCount != in_pointCount) // for this transf key, the number of points differs
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Different point count for " + in_xsiObj.GetFullName() + L", disabling exact transformation motion blur", siWarningMsg);
         GetMessageQueue()->LogMsg(L"[sitoa] Count at frame time " + (CString)in_frame + L" : " + (CString)in_pointCount, siWarningMsg);
         GetMessageQueue()->LogMsg(L"[sitoa] Count at frame time " + (CString)keyTime + L" : " + (CString)pointCount, siWarningMsg);
         out_doExactTransformMb = false;
         failingTransfKey = keyTime; // store the first failing key time
         break;
      }

      okTransfKeys.insert(keyTime); // for this frame time, the geo was ok
   }

   // deformation
   if (!out_doExactTransformMb) // if the transf test failed, it will also fail for deformation, if we have the same key in the deform array
   {
      for (iKey = 0; iKey < in_defKeys.GetCount(); iKey++)
      {
         keyTime = transfKeys[iKey];
         if (keyTime == failingTransfKey)
         {
            // we can avoid pulling the geo, we must also disable exact mb for deformation
            out_doExactDeformMb = false;
            okTransfKeys.clear();
            return;
         }
      }
   }

   // else, let's loop the deformation keys
   out_doExactDeformMb = true;
   for (iKey = 0; iKey < in_defKeys.GetCount(); iKey++)
   {
      keyTime = in_defKeys[iKey];
      // if the geomatry was pulled already in the transformation loop and we found it was ok, then we can skip checking this deform key again
      if (okTransfKeys.find(keyTime) != okTransfKeys.end())
         continue;

      Geometry mbXsiGeo = CObjectUtilities().GetGeometryAtFrame(in_xsiObj, keyTime);

      ICEAttribute mbPointPositionAttribute = mbXsiGeo.GetICEAttributeFromName(L"PointPosition");
      pointCount = mbPointPositionAttribute.GetElementCount();
      if (pointCount != in_pointCount)
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Different point count for " + in_xsiObj.GetFullName() + L", disabling exact deformation motion blur", siWarningMsg);
         GetMessageQueue()->LogMsg(L"[sitoa] Count at frame time " + (CString)in_frame + L" : " + (CString)in_pointCount, siWarningMsg);
         GetMessageQueue()->LogMsg(L"[sitoa] Count at frame time " + (CString)keyTime + L" : " + (CString)pointCount, siWarningMsg);
         out_doExactDeformMb = false;
         break;
      }
   }

   okTransfKeys.clear();
}


// Traverse all the nodes of an ice trees to fill the hyerarchy mode map, for shape instancing
//
// @param in_node       the tree root or any node container when called recursively
// @param out_map       the returned map. This has one entry for every Instance Shape node:
//                      first: the reference string, ie the name of the object or hierarchy shape
//                      second: true if inhierarchy mode, false if in only-object mode
//
void CIceUtilities::SearchShapeNodes(const ICENode& in_node, ShapeHierarchyModeMap &out_map)
{
   if (in_node.IsConnected() && in_node.GetType() == L"ShapeInstancingNode")
   {
      int hm = in_node.GetParameterValue(L"hierarchymode");
      CString reference = in_node.GetParameterValue(L"reference");
      out_map.insert(ShapeHierarchyModePair(reference, hm==1));
   }

   if (in_node.IsA(siICENodeContainerID))
   {   // The input node might be a ICETree or ICECompoundNode, let's get their ICENodes
      CRefArray nodes;
      ICENodeContainer container(in_node.GetRef());
      nodes = container.GetNodes();
      // Recursively traverse the graph
      for (LONG i=0; i<nodes.GetCount(); i++)
         SearchShapeNodes(nodes[i], out_map);
   }
}


// Traverse all the ice trees of an object to fill the hyerarchy mode map, for shape instancing
//
// @param in_xsiObj     the Softimage object
// @param out_map       the returned map. This has one entry for every Instance Shape node:
//                      first: the reference string, ie the name of the object or hierarchy shape
//                      second: true if inhierarchy mode, false if in only-object mode
//
void CIceUtilities::SearchAllTreesForShapeNodes(const X3DObject &in_xsiObj, ShapeHierarchyModeMap &out_map)
{
   CRefArray trees = in_xsiObj.GetActivePrimitive().GetICETrees();
   for (LONG i=0; i<trees.GetCount(); i++)
   {
      ICETree tree(trees[i]);
      SearchShapeNodes(tree, out_map);
   }
   // GetMessageQueue()->LogMsg(L"Logging map:");
   // for (ShapeHierarchyModeMap::iterator it=out_map.begin(); it!=out_map.end(); it++)
   //    GetMessageQueue()->LogMsg(it->first + L", " + ((CValue)it->second).GetAsText());
}


///////////////////////////////////////////////////////////////////////
// CIceAttribute class
///////////////////////////////////////////////////////////////////////

// Evaluate the attribute. There are more attribute cases than the ones we need, that's ok
// 
// @param in_Offset The offset (so, the chunk start index)
// @param in_Count  The number of points
//
// @return true if all went ok, else false
//
bool CIceAttribute::Update(LONG in_Offset, LONG in_Count)
{
   if (!m_isDefined)
      return false;
   if (m_storedOffset == in_Offset)
      return true;
   if (m_storedOffset == 0 && m_isConstant)
      return true;

   m_storedOffset = in_Offset;

   if (m_isArray)
   {
      switch (m_eType)
      {
         case siICENodeDataBool:
            GetDataArray2DChunk(in_Offset, in_Count, m_bData2D);
            break;
         case siICENodeDataLong:
            GetDataArray2DChunk(in_Offset, in_Count, m_lData2D);
            break;
         case siICENodeDataFloat:
            GetDataArray2DChunk(in_Offset, in_Count, m_fData2D);
            break;
         case siICENodeDataVector2:
            GetDataArray2DChunk(in_Offset, in_Count, m_v2Data2D);
            break;
         case siICENodeDataVector3:
            GetDataArray2DChunk(in_Offset, in_Count, m_v3Data2D);
            break;
         case siICENodeDataVector4:
            GetDataArray2DChunk(in_Offset, in_Count, m_v4Data2D);
            break;
         case siICENodeDataColor4:
            GetDataArray2DChunk(in_Offset, in_Count, m_cData2D);
            break;
         case siICENodeDataQuaternion:
            GetDataArray2DChunk(in_Offset, in_Count, m_qData2D);
            break;
         case siICENodeDataMatrix33:
            GetDataArray2DChunk(in_Offset, in_Count, m_m3Data2D);
            break;
         case siICENodeDataMatrix44:
            GetDataArray2DChunk(in_Offset, in_Count, m_m4Data2D);
            break;
         case siICENodeDataRotation:
            GetDataArray2DChunk(in_Offset, in_Count, m_rData2D);
            break;
         case siICENodeDataShape:
            GetDataArray2DChunk(in_Offset, in_Count, m_sData2D);
            break;
         case siICENodeDataString:
            GetDataArray2DChunk(in_Offset, in_Count, m_strData2D);
            break;
         default:
            return false;
      }
   }
   else
   {
      switch (m_eType)
      {
         case siICENodeDataBool:
            GetDataArrayChunk(in_Offset, in_Count, m_bData);
            break;
         case siICENodeDataLong:
            GetDataArrayChunk(in_Offset, in_Count, m_lData);
            break;
         case siICENodeDataFloat:
            GetDataArrayChunk(in_Offset, in_Count, m_fData);
            break;
         case siICENodeDataVector2:
            GetDataArrayChunk(in_Offset, in_Count, m_v2Data);
            break;
         case siICENodeDataVector3:
            GetDataArrayChunk(in_Offset, in_Count, m_v3Data);
            break;
         case siICENodeDataVector4:
            GetDataArrayChunk(in_Offset, in_Count, m_v4Data);
            break;
         case siICENodeDataColor4:
            GetDataArrayChunk(in_Offset, in_Count, m_cData);
            break;
         case siICENodeDataQuaternion:
            GetDataArrayChunk(in_Offset, in_Count, m_qData);
            break;
         case siICENodeDataMatrix33:
            GetDataArrayChunk(in_Offset, in_Count, m_m3Data);
            break;
         case siICENodeDataMatrix44:
            GetDataArrayChunk(in_Offset, in_Count, m_m4Data);
            break;
         case siICENodeDataRotation:
            GetDataArrayChunk(in_Offset, in_Count, m_rData);
            break;
         case siICENodeDataShape:
            GetDataArrayChunk(in_Offset, in_Count, m_sData);
            break;
         case siICENodeDataString:
            GetDataArrayChunk(in_Offset, in_Count, m_strData);
            break;
         default:
            return false;
      }
   }

   return true;
}


bool CIceAttribute::Update()
{
   if (!m_isDefined)
      return false;

   if (m_isArray)
   {
      switch (m_eType)
      {
         case siICENodeDataBool:
            GetDataArray2D(m_bData2D);
            break;
         case siICENodeDataLong:
            GetDataArray2D(m_lData2D);
            break;
         case siICENodeDataFloat:
            GetDataArray2D(m_fData2D);
            break;
         case siICENodeDataVector2:
            GetDataArray2D(m_v2Data2D);
            break;
         case siICENodeDataVector3:
            GetDataArray2D(m_v3Data2D);
            break;
         case siICENodeDataVector4:
            GetDataArray2D(m_v4Data2D);
            break;
         case siICENodeDataColor4:
            GetDataArray2D(m_cData2D);
            break;
         case siICENodeDataQuaternion:
            GetDataArray2D(m_qData2D);
            break;
         case siICENodeDataMatrix33:
            GetDataArray2D(m_m3Data2D);
            break;
         case siICENodeDataMatrix44:
            GetDataArray2D(m_m4Data2D);
            break;
         case siICENodeDataRotation:
            GetDataArray2D(m_rData2D);
            break;
         case siICENodeDataShape:
            GetDataArray2D(m_sData2D);
            break;
         case siICENodeDataString:
            GetDataArray2D(m_strData2D);
            break;
         default:
            return false;
      }
   }
   else
   {
      switch (m_eType)
      {
         case siICENodeDataBool:
            GetDataArray(m_bData);
            break;
         case siICENodeDataLong:
            GetDataArray(m_lData);
            break;
         case siICENodeDataFloat:
            GetDataArray(m_fData);
            break;
         case siICENodeDataVector2:
            GetDataArray(m_v2Data);
            break;
         case siICENodeDataVector3:
            GetDataArray(m_v3Data);
            break;
         case siICENodeDataVector4:
            GetDataArray(m_v4Data);
            break;
         case siICENodeDataColor4:
            GetDataArray(m_cData);
            break;
         case siICENodeDataQuaternion:
            GetDataArray(m_qData);
            break;
         case siICENodeDataMatrix33:
            GetDataArray(m_m3Data);
            break;
         case siICENodeDataMatrix44:
            GetDataArray(m_m4Data);
            break;
         case siICENodeDataRotation:
            GetDataArray(m_rData);
            break;
         case siICENodeDataShape:
            GetDataArray(m_sData);
            break;
         case siICENodeDataString:
            GetDataArray(m_strData);
            break;
         default:
            return false;
      }
   }

   return true;
}



// Handles to be used in place of the [], originally because of #1621

//
// bool arrays can't use direct access, because they are a bitset
bool CIceAttribute::GetBool(LONG in_index)
{
   return m_bData[in_index];
}


float CIceAttribute::GetFloat(LONG in_index)
{
   return m_fData[in_index];
}


int CIceAttribute::GetInt(LONG in_index)
{
   return m_lData[in_index];
}


CVector3f CIceAttribute::GetVector3f(LONG in_index)
{
   return m_v3Data[in_index];
}


CQuaternionf CIceAttribute::GetQuaternionf(LONG in_index)
{
   return m_qData[in_index];
}


CMatrix3f CIceAttribute::GetMatrix3f(LONG in_index)
{
   return m_m3Data[in_index];
}


CMatrix4f CIceAttribute::GetMatrix4f(LONG in_index)
{
   return m_m4Data[in_index];
}


CColor4f CIceAttribute::GetColor4f(LONG in_index)
{
   return m_cData[in_index];
}


CRotationf CIceAttribute::GetRotationf(LONG in_index)
{
   return m_rData[in_index];
}


CShape CIceAttribute::GetShape(LONG in_index)
{
   return m_sData[in_index];
}

///////////////////////////////////////////////////////////////////////
// CIceAttributesSet class
///////////////////////////////////////////////////////////////////////

// Collects in out_attr the set of attributes requested by a shader
//
// @param in_shader         The shader
// @param in_frame          The evaluation frame (quite useless here)
// @param in_isVolume       True if being called by a volume object
// @param out_attr          The result set
//
void CIceAttributesSet::ParseAttributesShader(Shader in_shader, double in_frame, bool in_isVolume, bool in_iceTextures, set<CString> &out_attr)
{
   if (CRenderTree().IsCompound(in_shader))
   {
      // collect all the input shaders and recurse
      CRefArray inputShaders = in_shader.GetAllShaders();
      for (LONG i=0; i<inputShaders.GetCount(); i++)
      {
         Shader shader(inputShaders.GetItem(i));
         ParseAttributesShader(shader, in_frame, in_isVolume, in_iceTextures, out_attr);
      }
   }

   CString attributeName;

   // if we're being called because collecting the ICE texture attributes, by now we detect the standard texture shader
   if (in_iceTextures)
   {
      if (in_shader.GetProgID().Split(L".txt2d-image-explicit").GetCount() >= 2)
      {
         attributeName = ParAcc_GetValue(in_shader, L"tspace_id", in_frame).GetAsText();
         if (!attributeName.IsEmpty())
            out_attr.insert(attributeName);
      }
      return;
   }
   // The image clip node can retrieve the time from an attribute.
   if (in_shader.GetProgID().Split(L".sib_image_clip").GetCount() >= 2)
   {
      attributeName = ParAcc_GetValue(in_shader, L"TimeSource", in_frame).GetAsText();
      if (!attributeName.IsEmpty())
         out_attr.insert(attributeName);
      return;
   }
   // It's the only exception to regular sib_attribute nodes
   if (in_shader.GetProgID().Split(L".sib_attribute_").GetCount() >= 2)
   {
      // get the name of the attribute which is used
      attributeName = ParAcc_GetValue(in_shader, L"attribute", in_frame).GetAsText();
      if (!(attributeName.IsEqualNoCase(L"default") || attributeName.IsEmpty()))
         out_attr.insert(attributeName);
      return;
   }
   if (in_isVolume) // the attributes exported as user data of the volume object
   {
      if (in_shader.GetProgID().Split(L".BA_volume_cloud").GetCount() >= 2)
         out_attr.insert(L"VolumeCloud");
      else if (in_shader.GetProgID().Split(L".BA_particle_density").GetCount() >= 2)
      {
         out_attr.insert(L"Color");
         out_attr.insert(L"Id");
         out_attr.insert(L"PointVelocity");
         out_attr.insert(L"Orientation");
         out_attr.insert(L"Size");
         out_attr.insert(L"PointPosition");
         out_attr.insert(L"Age");
         out_attr.insert(L"AgeLimit");
         out_attr.insert(L"StrandPosition");
         out_attr.insert(L"StrandVelocity");
         out_attr.insert(L"StrandSize");
         out_attr.insert(L"StrandOrientation");
         out_attr.insert(L"StrandColor");
      }
      else 
      {
         // let's allow for a large set of atttributes (a subset of the particle_density case)
         // In any case, the attributes will not be available if the user does not pull it in the rendertree, either by
         // 1. Use a attribute shader
         // 2. Have the attributes declared in the shader spdl, as in the BA ones
         out_attr.insert(L"Color");
         out_attr.insert(L"Id");
         out_attr.insert(L"PointVelocity");
         out_attr.insert(L"Size");
         out_attr.insert(L"PointPosition");
         out_attr.insert(L"Age");
         out_attr.insert(L"AgeLimit");
         out_attr.insert(L"StrandPosition");
         out_attr.insert(L"StrandVelocity");
         out_attr.insert(L"StrandSize");
         out_attr.insert(L"StrandColor");
         // add some aux ones, so the shader writer can declare them in the spdl and push them out in ICE
         out_attr.insert(L"ArnoldVolume0");
         out_attr.insert(L"ArnoldVolume1");
         out_attr.insert(L"ArnoldVolume2");
         out_attr.insert(L"ArnoldVolume3");
         out_attr.insert(L"ArnoldVolume4");
         out_attr.insert(L"ArnoldVolume5");
         out_attr.insert(L"ArnoldVolume6");
         out_attr.insert(L"ArnoldVolume7");
         out_attr.insert(L"ArnoldVolume8");
         out_attr.insert(L"ArnoldVolume9");
      }
   }
}



// Collects in out_attr the set of attributes requested by the in_xsiObj materials
//
// @param in_xsiObj         The object (typically a point cloud)
// @param in_iceMaterials   The ice materials of a mesh, if any, else void 
// @param in_frame          The evaluation frame (quite useless here)
// @param in_isVolume       True if being called by a volume object
// @param out_attr          The result set
//
void CIceAttributesSet::ParseMaterialAttributeShaders(const X3DObject &in_xsiObj, CRefArray in_iceMaterials, double in_frame, bool in_isVolume, bool in_iceTextures, set<CString> &out_attr)
{
   // We parse the materials of either
   // 1. A mesh. Then, we get in_iceMaterials from the geo accessor of the polymesh, which works well also for crowds (#1337)
   // 2. A pointcloud, then in_iceMaterials is void, and we pull the materials from the cloud
   CRefArray materials = in_iceMaterials.GetCount() > 0 ? in_iceMaterials : in_xsiObj.GetMaterials();

   for (int i=0; i<materials.GetCount(); i++)
   {
      Material mat = (Material)materials[i];
      if (!mat.IsValid())
         continue;

      CRefArray shaders = mat.GetAllShaders();
      CRefArray clips = mat.GetAllImageClips();

      for (LONG j=0; j<shaders.GetCount(); j++)
      {
         Shader shader = (Shader)shaders[j];
         if (!shader.IsValid())
            continue;
         ParseAttributesShader(shader, in_frame, in_isVolume, in_iceTextures, out_attr);
      }
      // The image clip node can retrieve the time from an attribute.
      // It's the only exception to regular sib_attribute nodes
      for (LONG j=0; j<clips.GetCount(); j++)
      {
         ImageClip clip(clips[j]);
         CString attributeName = ParAcc_GetValue(clip, L"TimeSource", in_frame).GetAsText();
         if (!attributeName.IsEmpty())
            out_attr.insert(attributeName);
      }
   }

   // for (set<CString>::iterator it=out_attr.begin(); it!=out_attr.end(); it++)
   //    GetMessageQueue()->LogMsg(L"  attr = " + *it);
}


// Cycle ALL the objects instanced by this pointcloud, in order to get the full list of them
// and so be able to know which attributes are requested by their materials.
//
// @param out_id       The returned set of the ids of the objects
//
void CIceAttributesSet::CollectInstancedObjects(set<ULONG> &out_id)
{
   // check the pointPosition attribute, if is invalid or empty skip this cloud
   ICEAttribute pointPositionAttribute = m_xsiGeo.GetICEAttributeFromName(L"PointPosition");
   LONG pointCount = pointPositionAttribute.GetElementCount();
   
   if (!pointPositionAttribute.IsDefined() || !pointPositionAttribute.IsValid() || pointCount == 0)
      return;

   // get the shape attribute
   CIceAttribute shapeAttr = m_xsiGeo.GetICEAttributeFromName(L"shape");
   CShape shape;
   siICEShapeType shapeType;
   for (LONG pointOffset=0; pointOffset < pointCount; pointOffset+=ICE_CHUNK_SIZE)
   {
      LONG nbPoints = pointCount - pointOffset < ICE_CHUNK_SIZE ? pointCount - pointOffset : ICE_CHUNK_SIZE;
      shapeAttr.Update(pointOffset, nbPoints);
      if (!shapeAttr.IsDefined())
         continue;

      for (LONG pointIndex = 0; pointIndex < nbPoints; pointIndex++)
      {
         shape = shapeAttr.IsConstant() ? shapeAttr.GetShape(0) : shapeAttr.GetShape(pointIndex);
         shapeType = shape.GetType();
         if (shapeType == siICEShapeInstance || shapeType == siICEShapeReference)
            out_id.insert(shape.GetReferenceID());
      }
   }
}


// Cycle the in_id id set, and for each item, check if it's a model.
// If so, add the children to out_id. Else, add the object itself to out_id
//
// @param in_id    The input set of id to filter
// @param out_id   The output (filtered) set
//
void CIceAttributesSet::RefineInstancedObjectsSet(set<ULONG> &in_id, set<ULONG> &out_id)
{
   set<ULONG>::iterator it;
   for (it=in_id.begin(); it!=in_id.end(); it++)
   {
      X3DObject obj = (X3DObject)Application().GetObjectFromID(*it);
      if (!obj.IsValid())
         continue;
      Model model(obj);
      if (model.IsValid())
      {
         CRefArray shapes = GetObjectsAndLightsUnderMaster(model);
         for (int i=0; i<shapes.GetCount(); i++)
         {
            X3DObject subObj(shapes[i]); 
            // insert without checking the existance, set does it for us
            out_id.insert(CObjectUtilities().GetId(subObj));
         }
      }
      else
         out_id.insert(*it);
   }
}


// Collect all the attributes required by the shaders connected to this ice object
// The attributes can be requested either by the material of the pointcloud itself,
// or by any material owned by a shape instanced by the pointcloud
//
// @param in_frame          The evaluation frame
// @param in_iceMaterials   The ice materials of a mesh, if any, else void 
// @param in_isVolume       True if being called by a volume object
//
void CIceAttributesSet::CollectRequiredAttributes(double in_frame, CRefArray in_iceMaterials, bool in_isVolume, bool in_iceTextures)
{
   set<ULONG> ids, ids2;

   // Start getting the attributes required by the icetree materials themselves.
   ParseMaterialAttributeShaders(m_xsiObj, in_iceMaterials, in_frame, in_isVolume, in_iceTextures, m_reqAttrNames);
   // Then, we must look at the materials of the instanced objects, if any.
   // In fact, they can query the attributes as well.
   // Get all the objects instanced by this pointcloud
   CollectInstancedObjects(ids);
   // If some objects in ids are models, extract their children.
   // The final list is stored in ids2
   RefineInstancedObjectsSet(ids, ids2);

   set<ULONG>::iterator idIt;
   for (idIt=ids2.begin(); idIt!=ids2.end(); idIt++)
   {
      X3DObject obj = (X3DObject)Application().GetObjectFromID(*idIt);
      // GetMessageQueue()->LogMsg(L"Found object: " + obj.GetName());
      // For each (unique) object found, traverse the shaders looking for attribute requirement
      // The final list of attributes ends up in m_reqAttrNames
      ParseMaterialAttributeShaders(obj, in_iceMaterials, in_frame, in_isVolume, in_iceTextures, m_reqAttrNames);
   }
   // printer:
   // set<CString>::iterator attrIt;
   // for (attrIt=m_reqAttrNames.begin(); attrIt!=m_reqAttrNames.end(); attrIt++)
   //    GetMessageQueue()->LogMsg(L"Found required attribute: " + *attrIt);
}


CIceAttributesSet::~CIceAttributesSet()
{
   AttrMap::iterator attribIt;
   for (attribIt = m_providedAttributesMap.begin(); attribIt != m_providedAttributesMap.end(); attribIt++)
      delete(attribIt->second);

   m_providedAttributesMap.clear();
   m_requiredAttributesMap.clear();
}


// Store the required attributes names into the m_requiredAttributesSet set
void CIceAttributesSet::GetRequiredAttributesSet()
{
   set<CString>::iterator it;
   for (it=m_reqAttrNames.begin(); it!=m_reqAttrNames.end(); it++)
   {
      // get the name of the attribute queried by some shader
      CString attribName = CStringUtilities().ToLower(*it); 
      m_requiredAttributesSet.insert(attribName);
   }
}


// Copy the required attributes set into the providedAttributesSet set, and add the standard provided attributes
void CIceAttributesSet::GetProvidedAttributesSet(bool in_onlyMbAttributes)
{
   if (!in_onlyMbAttributes)
      m_providedAttributesSet = m_requiredAttributesSet; // copy from the required attributes
   // there is no need to check the existance of the pair, the insert will fail if the key exists already
   
   // the attributes that are needed in any case
   m_providedAttributesSet.insert(L"pointposition");
   m_providedAttributesSet.insert(L"size");
   m_providedAttributesSet.insert(L"orientation");
   m_providedAttributesSet.insert(L"scale");
   m_providedAttributesSet.insert(L"strandposition");
   // the attributes for this very frame, not needed for exact mb 
   if (!in_onlyMbAttributes)
   {
      m_providedAttributesSet.insert(L"color");
      m_providedAttributesSet.insert(L"shape");
      m_providedAttributesSet.insert(L"shapeinstancetime");
      m_providedAttributesSet.insert(L"pointvelocity");
      m_providedAttributesSet.insert(L"angularvelocity");
      m_providedAttributesSet.insert(L"strandsize");
      m_providedAttributesSet.insert(L"strandscale");
      m_providedAttributesSet.insert(L"strandvelocity");
      m_providedAttributesSet.insert(L"strandorientation");
   }
}


// Build the required and provided attribute maps (name + CIceAttribute pointer)
void CIceAttributesSet::BuildAttributesMaps(bool in_addArnoldLightAttributes, bool in_addArnoldProceduralAttributes)
{
   CRefArray attributeArray = m_xsiGeo.GetICEAttributes();
   CString unusedS;

   for (LONG i=0; i<attributeArray.GetCount(); i++)
   {
      ICEAttribute attrib(attributeArray[i]);

      if (!attrib.IsDefined() || !attrib.IsValid()) // skip undefined attributes
         continue;
      if (attrib.IsReadonly()) // skip readonly attributes
         continue;

      // skip all attributes of unsupported type!
      switch (attrib.GetDataType())
      {
         case siICENodeDataVector2:
         case siICENodeDataVector4:
         case siICENodeDataMatrix33:
         case siICENodeDataGeometry:
         case siICENodeDataLocation:
         case siICENodeDataExecute:
         case siICENodeDataValue:
         case siICENodeDataMultiComp:
         case siICENodeDataArithmeticSupport:
         case siICENodeDataAny:
            continue;
         default:
            break;
      }

      CString attribName = CStringUtilities().ToLower(attrib.GetName());

      // #1219: attributes starting by "ArnoldLight" are forced in, regardless is they are required
      bool isArnoldLightAttribute(false);
      if (in_addArnoldLightAttributes)
         isArnoldLightAttribute = CIceUtilities().GetArnoldLightParameterFromAttribute(attribName, unusedS);

      // attributes starting by "ArnoldProcedural" are forced in, regardless is they are required
      bool isArnoldProceduralAttribute(false);
      if (in_addArnoldProceduralAttributes)
         isArnoldProceduralAttribute = CIceUtilities().GetTrimmedArnoldProceduralAttributeName(attribName, unusedS);

      // skip all attributes that are not provided!
      if (!isArnoldLightAttribute && !isArnoldProceduralAttribute)
      if (m_providedAttributesSet.find(attribName) == m_providedAttributesSet.end())
         continue;

      CIceAttribute *sAttrib = new CIceAttribute(attrib);
      // insert this attribute
      m_providedAttributesMap.insert(AttrPair(attribName, sAttrib));

      // if this attribute is required, also add it to the required map
      if (m_requiredAttributesSet.find(attribName) != m_requiredAttributesSet.end())
         m_requiredAttributesMap.insert(AttrPair(attribName, sAttrib));
      // #1219: also insert it if this is an ArnoldLight attribute
      if (isArnoldLightAttribute || isArnoldProceduralAttribute)
         m_requiredAttributesMap.insert(AttrPair(attribName, sAttrib));
   }
}



// Connect the map attribute (->second) to a simpler pointer, just to have a better handler to it
void CIceAttributesSet::ConnectAttributeHandlers()
{
   AttrMap::iterator attribIt;
   if ((attribIt = m_providedAttributesMap.find(L"pointposition")) != m_providedAttributesMap.end())
      m_pointPosition = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"orientation")) != m_providedAttributesMap.end())
      m_orientation = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"scale")) != m_providedAttributesMap.end())
      m_scale = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"size")) != m_providedAttributesMap.end())
      m_size = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"shape")) != m_providedAttributesMap.end())
      m_shape = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"shapeinstancetime")) != m_providedAttributesMap.end())
      m_shapeTime = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"color")) != m_providedAttributesMap.end())
      m_color = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"pointvelocity")) != m_providedAttributesMap.end())
      m_pointVelocity = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"angularvelocity")) != m_providedAttributesMap.end())
      m_angularVelocity = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"strandposition")) != m_providedAttributesMap.end())
      m_strandPosition = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"strandscale")) != m_providedAttributesMap.end())
      m_strandScale = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"strandvelocity")) != m_providedAttributesMap.end())
      m_strandVelocity = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"strandsize")) != m_providedAttributesMap.end())
      m_strandSize = attribIt->second;
   if ((attribIt = m_providedAttributesMap.find(L"strandorientation")) != m_providedAttributesMap.end())
      m_strandOrientation = attribIt->second;
}


// Read all the attribute
//
// @param in_pointOffset The chunk offset
// @param in_nbPoints    The number of points for this chunk
// @param in_nbPolygons  The number of polygons for this chunk, used for per-poly attr on polymeshes
//
void CIceAttributesSet::UpdateChunk(const LONG in_pointOffset, const LONG in_nbPoints, const bool in_isMesh)
{
   if (m_pointPosition) 
      m_pointPosition->Update(in_pointOffset, in_nbPoints);
   if (m_pointVelocity) 
      m_pointVelocity->Update(in_pointOffset, in_nbPoints);
   if (m_orientation) 
      m_orientation->Update(in_pointOffset, in_nbPoints);
   if (m_angularVelocity) 
      m_angularVelocity->Update(in_pointOffset, in_nbPoints);
   if (m_scale) 
      m_scale->Update(in_pointOffset, in_nbPoints);
   if (m_shapeTime)
      m_shapeTime->Update(in_pointOffset, in_nbPoints);
   if (m_size) 
      m_size->Update(in_pointOffset, in_nbPoints);
   if (m_shape) 
      m_shape->Update(in_pointOffset, in_nbPoints);
   if (m_color) 
      m_color->Update(in_pointOffset, in_nbPoints);
   if (m_strandPosition)
      m_strandPosition->Update(in_pointOffset, in_nbPoints);
   if (m_strandVelocity)
      m_strandVelocity->Update(in_pointOffset, in_nbPoints);
   if (m_strandSize)
      m_strandSize->Update(in_pointOffset, in_nbPoints);
   if (m_strandOrientation)
      m_strandOrientation->Update(in_pointOffset, in_nbPoints);

   // update the required attributes
   AttrMap::iterator attribIt;
   for (attribIt = m_requiredAttributesMap.begin(); attribIt != m_requiredAttributesMap.end(); attribIt++)
   {
      if (in_isMesh) // mesh case
         attribIt->second->Update();
      else
         attribIt->second->Update(in_pointOffset, in_nbPoints);
   }
}


//////////////////////////
// Attributes existence checkers
//////////////////////////

bool CIceAttributesSet::HasPointPosition()
{
   return m_pointPosition && m_pointPosition->m_isDefined;
}

bool CIceAttributesSet::HasOrientation()
{
   return m_orientation && m_orientation->m_isDefined;
}

bool CIceAttributesSet::HasScale()
{
   return m_scale && m_scale->m_isDefined;
}

bool CIceAttributesSet::HasSize()
{
   return m_size && m_size->m_isDefined;
}

bool CIceAttributesSet::HasShape()
{
   return m_shape && m_shape->m_isDefined;
}

bool CIceAttributesSet::HasColor()
{
   return m_color && m_color->m_isDefined;
}

bool CIceAttributesSet::HasPointVelocity()
{
   return m_pointVelocity && m_pointVelocity->m_isDefined;
}

bool CIceAttributesSet::HasAngularVelocity()
{
   return m_angularVelocity && m_angularVelocity->m_isDefined;
}

bool CIceAttributesSet::HasStrandPosition()
{
   return m_strandPosition && m_strandPosition->m_isDefined;
}

bool CIceAttributesSet::HasStrandScale()
{
   return m_strandScale && m_strandScale->m_isDefined;
}

bool CIceAttributesSet::HasStrandVelocity()
{
   return m_strandVelocity && m_strandVelocity->m_isDefined;
}

bool CIceAttributesSet::HasStrandSize()
{
   return m_strandSize && m_strandSize->m_isDefined;
}

bool CIceAttributesSet::HasStrandOrientation()
{
   return m_strandOrientation && m_strandOrientation->m_isDefined;
}

bool CIceAttributesSet::HasShapeTime()
{
   return m_shapeTime && m_shapeTime->m_isDefined;
}


//////////////////////////
// Getters
//////////////////////////

// Get the position of a point
//
// @param in_pointIndex The point index
// @return the point position, or 0,0,0 if the attribute does not exist
//
CVector3f CIceAttributesSet::GetPointPosition(LONG in_pointIndex)
{
   if (HasPointPosition())
      return m_pointPosition->m_isConstant ? m_pointPosition->GetVector3f(0) : m_pointPosition->GetVector3f(in_pointIndex);
   return CVector3f(0.0f, 0.0f, 0.0f);
}


// Get the orientation of a point as CRotationf. 
//
// @param in_pointIndex The point index
// @return the point CRotation, or 0,0,0 if the attribute does not exist
//
CRotationf CIceAttributesSet::GetOrientationf(LONG in_pointIndex)
{
   CRotationf result;
   if (HasOrientation())
      result = m_orientation->m_isConstant ? m_orientation->GetRotationf(0) : m_orientation->GetRotationf(in_pointIndex);
   else
      result.SetIdentity();

   return result;
}


// Get the orientation of a point as CRotation
//
// @param in_pointIndex The point index
// @return the point CRotation, or 0,0,0 if the attribute does not exist
//
CRotation CIceAttributesSet::GetOrientation(LONG in_pointIndex)
{
   CRotation result;
   if (HasOrientation())
   {
      CRotationf rotationf = GetOrientationf(in_pointIndex);
      result = CIceUtilities().RotationfToRotation(rotationf);
   }
   else
      result.SetIdentity();

   return result;
}


// Get the scale of a point
//
// @param in_pointIndex The point index
// @return the point scale, or 1,1,1 if the attribute does not exist
//
CVector3f CIceAttributesSet::GetScale(LONG in_pointIndex)
{
   if (HasScale())
      return m_scale->m_isConstant ? m_scale->GetVector3f(0) : m_scale->GetVector3f(in_pointIndex);
   return CVector3f(1.0f, 1.0f, 1.0f);
}


// Get the size of a point
//
// @param in_pointIndex The point index
// @param in_default    The returned value if the attribute does not exist
// @return the point scale, or in_default if the attribute does not exist
//
float CIceAttributesSet::GetSize(LONG in_pointIndex, float in_default)
{
   if (HasSize())
      return m_size->m_isConstant ? m_size->GetFloat(0) : m_size->GetFloat(in_pointIndex);
   return in_default;
}


// Get the shape of a point
//
// @param in_pointIndex The point index
// @return the point shape, or siICEShapeUnknown if the attribute does not exist
//
CShape CIceAttributesSet::GetShape(LONG in_pointIndex)
{
   if (HasShape())
      return m_shape->m_isConstant ? m_shape->GetShape(0) : m_shape->GetShape(in_pointIndex);
   return siICEShapeUnknown;
}


// Get the color of a point
//
// @param in_pointIndex The point index
// @return the point color, or black if the attribute does not exist
//
CColor4f CIceAttributesSet::GetColor(LONG in_pointIndex)
{
   if (HasColor())
      return m_color->m_isConstant ? m_color->GetColor4f(0) : m_color->GetColor4f(in_pointIndex);
   return CColor4f(0.0f, 0.0f, 0.0f, 0.0f);
}


// Get the velocity of a point
//
// @param in_pointIndex The point index
// @return the point velocity, or 0,0,0 if the attribute does not exist
//
CVector3f CIceAttributesSet::GetPointVelocity(LONG in_pointIndex)
{
   if (HasPointVelocity())
      return m_pointVelocity->m_isConstant ? m_pointVelocity->GetVector3f(0) : m_pointVelocity->GetVector3f(in_pointIndex);
   return CVector3f(0.0f, 0.0f, 0.0f);
}


// Get the angular velocity of a point
//
// @param in_pointIndex The point index
// @return the point angular velocity, or 0,0,0 if the attribute does not exist
//
CRotation CIceAttributesSet::GetAngularVelocity(LONG in_pointIndex)
{
   // angular velocity can be rotation or quaternion...!
   CRotation result;
   if (HasAngularVelocity())
   {
      if (m_angularVelocity->m_eType == siICENodeDataRotation)
      {
         CRotationf tmpRot = m_angularVelocity->m_isConstant ? m_angularVelocity->GetRotationf(0) : m_angularVelocity->GetRotationf(in_pointIndex);
         result = CIceUtilities().RotationfToRotation(tmpRot);
      }
      else if (m_angularVelocity->m_eType == siICENodeDataQuaternion)
      {
         CQuaternionf tmpQuat = m_angularVelocity->m_isConstant ? m_angularVelocity->GetQuaternionf(0) : m_angularVelocity->GetQuaternionf(in_pointIndex);
         result.SetFromQuaternion(CQuaternion(tmpQuat.GetW(),tmpQuat.GetX(), tmpQuat.GetY(), tmpQuat.GetZ()));
      }
   }
   else
      result.SetIdentity();

   return result;
}


// Get the shape time of a point ('s instance)
//
// @param in_pointIndex The point index
// @param in_default    The returned value if the attribute does not exist
// @return the point shape time, or  in_default if the attribute does not exist
//
float CIceAttributesSet::GetShapeTime(LONG in_pointIndex, float in_default)
{
   if (HasShapeTime())
     return m_shapeTime->m_isConstant ? m_shapeTime->GetFloat(0) : m_shapeTime->GetFloat(in_pointIndex);
   return in_default;
}

//////////////////////////////////////
// strands getters return arrays (one entry for each strand trail point)
//////////////////////////////////////

// Get the points position of a strand as a data array vector attribute
//
// @param in_pointIndex The strand index
// @param out_data      The returned point array
// @return false if the attribute does not exist, else true
//
bool CIceAttributesSet::GetStrandPosition(LONG in_pointIndex, CICEAttributeDataArrayVector3f &out_data)
{
   if (HasStrandPosition())
   {
      m_strandPosition->m_v3Data2D.GetSubArray(in_pointIndex, out_data);
      return out_data.GetCount() > 0;
   }
   return false;
}


// Get the points position of a strand as a vector of points
// Note that the data array vector (in_dav) that is used to access the points before copying them into 
// the output argument must be passed from the outside. If declaring it into the method scope, it fails
// for further calls (it just works the first time it gets called)
//
// @param in_pointIndex The strand index
// @param in_dav        The external data vector array needed to make it work
// @param out_data      The returned point array
//
// @return false if the attribute does not exist, else true
bool CIceAttributesSet::GetStrandPosition(LONG in_pointIndex, CICEAttributeDataArrayVector3f &in_dav, vector < CVector3f > &out_data)
{
   if (GetStrandPosition(in_pointIndex, in_dav))
   {
      out_data.resize(in_dav.GetCount());
      for (ULONG i=0; i<in_dav.GetCount(); i++)
      {
         out_data[i] = in_dav[i];
      }
      return true;
   }
   return false;
}


// Get the points scale of a strand
//
// @param in_pointIndex The strand index
// @param out_data      The returned vector array
// @return false if the attribute does not exist, else true
//
bool CIceAttributesSet::GetStrandScale(LONG in_pointIndex, CICEAttributeDataArrayVector3f &out_data)
{
   if (HasStrandScale())
   {
      m_strandScale->m_v3Data2D.GetSubArray(in_pointIndex, out_data);
      return out_data.GetCount() > 0;
   }
   return false;
}


// Get the points velocity of a strand
//
// @param in_pointIndex The strand index
// @param out_data      The returned vector array
// @return false if the attribute does not exist, else true
//
bool CIceAttributesSet::GetStrandVelocity(LONG in_pointIndex, CICEAttributeDataArrayVector3f &out_data)
{
   if (HasStrandVelocity())
   {
      m_strandVelocity->m_v3Data2D.GetSubArray(in_pointIndex, out_data);
      return out_data.GetCount() > 0;
   }
   return false;
}


// Get the points scale of a strand
//
// @param in_pointIndex The strand index
// @param out_data      The returned float array
// @return false if the attribute does not exist, else true
//
bool CIceAttributesSet::GetStrandSize(LONG in_pointIndex, CICEAttributeDataArrayFloat &out_data)
{
   if (HasStrandSize())
   {
      m_strandSize->m_fData2D.GetSubArray(in_pointIndex, out_data);
      return out_data.GetCount() > 0;
   }
   return false;
}


// Get the points orientation of a strand
//
// @param in_pointIndex The strand index
// @param out_data      The returned vector array
// @return false if the attribute does not exist, else true
//
bool CIceAttributesSet::GetStrandOrientation(LONG in_pointIndex, CICEAttributeDataArrayRotationf &out_data)
{
   if (HasStrandOrientation())
   {
      m_strandOrientation->m_rData2D.GetSubArray(in_pointIndex, out_data);
      return out_data.GetCount() > 0;
   }
   return false;
}


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Classes used to store data for the nodes, build and export them
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// CIceObjectBase class
///////////////////////////////////////////////////////////////////////

// Create the node
//
// @return false if the node cration failed, else true
bool CIceObjectBase::CreateNode()
{
    if (m_id == 0 || m_type.empty() || m_name.empty())
   {
      // GetMessageQueue()->LogMsg(L"CIceObjectBase::CreateNode, undefined base attributes", siErrorMsg);
      return false;
   }

   m_node = AiNode(m_type);
   if (!m_node)
      return false;

   if (m_id != 0)
      CNodeSetter::SetInt(m_node, "id", m_id);

   CNodeUtilities().SetName(m_node, m_name);

   return true;
}


// Get the node pointer
//
// @return the node pointer
AtNode* CIceObjectBase::GetNode()
{
   return m_node;
}


// Set the matrix array at the given mb key
//
// @param in_value   The input AtMatrix matrix
// @param in_key     The mb key
// @return false if the assignment failed, else true
//
bool CIceObjectBase::SetMatrix(AtMatrix in_value, const int in_key)
{
   return CUtilities().SetArrayValue(m_matrix, in_value, in_key);
}


// Set the matrix array at the given mb key
//
// @param in_value   The input CMatrix4 matrix
// @param in_key     The mb key
// @return false if the assignment failed, else true
//
bool CIceObjectBase::SetMatrix(CMatrix4 in_value, const int in_key)
{
   AtMatrix m;
   CUtilities().S2A(in_value, m);
   return CUtilities().SetArrayValue(m_matrix, m, in_key);
}


// Set the matrix array at the given mb key
//
// @param in_value   The input CTransformation matrix
// @param in_key     The mb key
// @return false if the assignment failed, else true
//
bool CIceObjectBase::SetMatrix(CTransformation in_value, const int in_key)
{
   AtMatrix m;
   CUtilities().S2A(in_value, m);
   return CUtilities().SetArrayValue(m_matrix, m, in_key);
}


// Set the object visibility (not yet the node's one)
//
// @param in_viz   The input visibility
void  CIceObjectBase::SetVisibility(uint8_t in_viz)
{
   m_visibility = in_viz;
}


// Set the object sidedness (not yet the node's one)
//
// @param in_sid   The input sidedness
void CIceObjectBase::SetSidedness(uint8_t in_sid)
{
   m_sidedness  = in_sid;
}


// Set the object shaders (not yet the node's one)
//
// @param in_shader   The input shaders
void CIceObjectBase::SetShader(AtNode *in_shader)
{
   m_shader = in_shader;
}


// Set the object vary basic attributes (not yet the node's one)
//
// @param in_id     The input id
// @param in_type   The input type
// @param in_name   The input name
void CIceObjectBase::SetNodeBaseAttributes(int in_id, const char* in_type, const char* in_name)
{
   m_id   = in_id;
   m_type = AtString(in_type);
   m_name = AtString(in_name);
}


// Alloc the object'a matrices and set them to identity
//
// @param in_nbTransfKeys     Number of motion blur transf keys
void CIceObjectBase::AllocMatrixArray(int in_nbTransfKeys)
{
   m_matrix = AiArrayAllocate(1, (uint8_t)in_nbTransfKeys, AI_TYPE_MATRIX);
   AtMatrix idM = AiM4Identity();
   for (int i=0; i<in_nbTransfKeys; i++)
      CUtilities().SetArrayValue(m_matrix, idM, i);
}


// Give the node the object's attributes. This is where things start to get pushed
//
// @return false if the node does not exist, else true
bool CIceObjectBase::SetNodeData()
{
   if (!m_node)
      return false;
   if (m_matrix)
      AiNodeSetArray(m_node, "matrix", m_matrix);
   if (!m_isLight)
   {
      CNodeSetter::SetByte(m_node, "visibility", m_visibility, true);
      CNodeSetter::SetByte(m_node, "sidedness",  m_sidedness, true);
   }
   if (m_shader)
      AiNodeSetArray(m_node, "shader", AiArray(1, 1, AI_TYPE_NODE, m_shader));

   return true;
}


// Assign the light group to the node
//
// @param in_lightGroup:  The light group array
//
bool CIceObjectBase::SetLightGroup(AtArray* in_lightGroup)
{
   if (!m_node)
      return false;

   if (m_isLight) // don't do light association for an instanced light (#1722)
      return false;
      
   CNodeSetter::SetBoolean(m_node, "use_light_group", true);
   if (AiArrayGetNumElements(in_lightGroup) > 0)
      AiNodeSetArray(m_node, "light_group", AiArrayCopy(in_lightGroup));

   return true;
}


// Assign the input boolean attribute value to the parameter with the same name of this (->m_node) light or procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldLight' or 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldLightDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set. If the type of the parameter does not match the attribute type, 
// the best possible conversion applies.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, bool in_value)
{
   // go float
   return SetICEAttributeAsNodeParameter(in_attr, in_value ? 1.0f : 0.0f);
}



// Assign the input LONG attribute value to the parameter with the same name of this (->m_node) light or procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldLight' or 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldLightDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set. If the type of the parameter does not match the attribute type, 
// the best possible conversion applies.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, int in_value)
{
   // go float
   return SetICEAttributeAsNodeParameter(in_attr, (float)in_value);
}


// Assign the input float attribute value to the parameter with the same name of this (->m_node) light or procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldLight' or 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldLightDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set. If the type of the parameter does not match the attribute type, 
// the best possible conversion applies.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, float in_value)
{
   if (!(m_isLight || m_isProcedural))
      return false; // continue only for lights and procedurals

   CString arnoldParamName;
   if (m_isLight)
   {
      if (!CIceUtilities().GetArnoldLightParameterFromAttribute(in_attr->m_name, arnoldParamName))
         return false;
   }
   else // procedural
   {
      if (!CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, arnoldParamName))
         return false;
   }

   const char *pName = arnoldParamName.GetAsciiString();
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(m_node), pName);
   if (!paramEntry)
      return false; // no such parameter exists for m_node

   int paramType = AiParamGetType(paramEntry);
   switch (paramType)
   {
      case AI_TYPE_INT:
         CNodeSetter::SetInt(m_node, pName, (int)in_value);
         return true;
      case AI_TYPE_UINT:
         CNodeSetter::SetUInt(m_node, pName, (unsigned int)in_value);
         return true;
      case AI_TYPE_BOOLEAN:
         CNodeSetter::SetBoolean(m_node, pName, in_value > 0.0f);
         return true;
      case AI_TYPE_FLOAT:
         CNodeSetter::SetFloat(m_node, pName, in_value);
         return true;
      case AI_TYPE_RGB:
         CNodeSetter::SetRGB(m_node, pName, in_value, in_value, in_value);
         return true;
      case AI_TYPE_RGBA:
         CNodeSetter::SetRGBA(m_node, pName, in_value, in_value, in_value, in_value);
         return true;
      case AI_TYPE_VECTOR:
         CNodeSetter::SetVector(m_node, pName, in_value, in_value, in_value);
         return true;
      case AI_TYPE_VECTOR2:
         CNodeSetter::SetVector2(m_node, pName, in_value, in_value);
         return true;
      default:
         return false;
   }
}


// Assign the input color attribute value to the parameter with the same name of this (->m_node) light or procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldLight' or 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldLightDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set. If the type of the parameter does not match the attribute type, 
// the best possible conversion applies.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CColor4f in_value)
{
   if (!(m_isLight || m_isProcedural))
      return false; // continue only for lights and procedurals

   CString arnoldParamName;
   if (m_isLight)
   {
      if (!CIceUtilities().GetArnoldLightParameterFromAttribute(in_attr->m_name, arnoldParamName))
         return false;
   }
   else // procedural
   {
      if (!CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, arnoldParamName))
         return false;
   }

   const char *pName = arnoldParamName.GetAsciiString();
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(m_node), pName);
   if (!paramEntry)
      return false; // no such parameter exists for m_node

   int paramType = AiParamGetType(paramEntry);
   switch (paramType)
   {
      case AI_TYPE_INT:
         CNodeSetter::SetInt(m_node, pName, (int) ((in_value.GetR() + in_value.GetG() + in_value.GetB()) / 3.0f) );
         return true;
      case AI_TYPE_UINT:
         CNodeSetter::SetUInt(m_node, pName, (unsigned int) ((in_value.GetR() + in_value.GetG() + in_value.GetB()) / 3.0f) );
         return true;
      case AI_TYPE_BOOLEAN:
         CNodeSetter::SetBoolean(m_node, pName, in_value.GetR() > 0.0f || in_value.GetG() > 0.0f || in_value.GetB() > 0.0f);
         return true;
      case AI_TYPE_FLOAT:
         CNodeSetter::SetFloat(m_node, pName, (in_value.GetR() + in_value.GetG() + in_value.GetB()) / 3.0f);
         return true;
      case AI_TYPE_RGB:
         CNodeSetter::SetRGB(m_node, pName, in_value.GetR(), in_value.GetG(), in_value.GetB());
         return true;
      case AI_TYPE_RGBA:
         CNodeSetter::SetRGBA(m_node, pName, in_value.GetR(), in_value.GetG(), in_value.GetB(), in_value.GetA());
         return true;
      case AI_TYPE_VECTOR:
         CNodeSetter::SetVector(m_node, pName, in_value.GetR(), in_value.GetG(), in_value.GetB());
         return true;
      case AI_TYPE_VECTOR2:
         CNodeSetter::SetVector2(m_node, pName, in_value.GetR(), in_value.GetG());
         return true;
      default:
         return false;
   }
}


// Assign the input vector attribute value to the parameter with the same name of this (->m_node) light or procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldLight' or 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldLightDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set. If the type of the parameter does not match the attribute type, 
// the best possible conversion applies.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CVector3f in_value)
{
   if (!(m_isLight || m_isProcedural))
      return false; // continue only for lights and procedurals

   CString arnoldParamName;
   if (m_isLight)
   {
      if (!CIceUtilities().GetArnoldLightParameterFromAttribute(in_attr->m_name, arnoldParamName))
         return false;
   }
   else // procedural
   {
      if (!CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, arnoldParamName))
         return false;
   }

   const char *pName = arnoldParamName.GetAsciiString();
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(m_node), pName);
   if (!paramEntry)
      return false; // no such parameter exists for m_node

   int paramType = AiParamGetType(paramEntry);
   switch (paramType)
   {
      case AI_TYPE_INT:
         CNodeSetter::SetInt(m_node, pName, (int) ((in_value.GetX() + in_value.GetY() + in_value.GetZ()) / 3.0f) );
         return true;
      case AI_TYPE_UINT:
         CNodeSetter::SetUInt(m_node, pName, (unsigned int) ((in_value.GetX() + in_value.GetY() + in_value.GetZ()) / 3.0f) );
         return true;
      case AI_TYPE_BOOLEAN:
         CNodeSetter::SetBoolean(m_node, pName, in_value.GetX() > 0.0f || in_value.GetY() > 0.0f || in_value.GetZ() > 0.0f);
         return true;
      case AI_TYPE_FLOAT:
         CNodeSetter::SetFloat(m_node, pName, (in_value.GetX() + in_value.GetY() + in_value.GetZ()) / 3.0f);
         return true;
      case AI_TYPE_RGB:
         CNodeSetter::SetRGB(m_node, pName, in_value.GetX(), in_value.GetY(), in_value.GetZ());
         return true;
      case AI_TYPE_RGBA:
         CNodeSetter::SetRGBA(m_node, pName, in_value.GetX(), in_value.GetY(), in_value.GetZ(), 1.0f);
         return true;
      case AI_TYPE_VECTOR:
         CNodeSetter::SetVector(m_node, pName, in_value.GetX(), in_value.GetY(), in_value.GetZ());
         return true;
      case AI_TYPE_VECTOR2:
         CNodeSetter::SetVector2(m_node, pName, in_value.GetX(), in_value.GetY());
         return true;
      default:
         return false;
   }
}


// Assign the input string attribute value to the parameter with the same name of this (->m_node) procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldProceduralDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CString in_value)
{
   if (!m_isProcedural)
      return false; // continue only for procedurals

   CString arnoldParamName;
   if (!CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, arnoldParamName))
      return false;

   const char *pName = arnoldParamName.GetAsciiString();
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(m_node), pName);
   if (!paramEntry)
      return false; // no such parameter exists for m_node

   int paramType = AiParamGetType(paramEntry);
   switch (paramType)
   {
      case AI_TYPE_STRING:
         CNodeSetter::SetString(m_node, pName, in_value.GetAsciiString());
         return true;
      // don't know how to convert to types other than string
      default:
         return false;
   }
}


// Assign the input matrix attribute value to the parameter with the same name of this (->m_node) procedural node.
// "Same Name" means "except the mandatory prefix 'ArnoldProcedural'
// So, for instance, if the attribute name is "ArnoldProceduralDummy", a parameter called "dummy" is searched in m_node.
// If found, the parameter value is set.
//
// @param in_attr:  The ice attribute
// @param in_value: The value to set
//
// @return true if the assignment succeeded, else false
//
bool CIceObjectBase::SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CMatrix4f in_value)
{
   if (!m_isProcedural)
      return false; // continue only for procedurals

   CString arnoldParamName;
   if (!CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, arnoldParamName))
      return false;

   const char *pName = arnoldParamName.GetAsciiString();
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(m_node), pName);
   if (!paramEntry)
      return false; // no such parameter exists for m_node

   int paramType = AiParamGetType(paramEntry);
   switch (paramType)
   {
      case AI_TYPE_MATRIX:
         AtMatrix matrix;
         CUtilities().S2A(in_value, matrix);
         CNodeSetter::SetMatrix(m_node, pName, matrix);
         return true;
      // don't know how to convert to types other than matrix
      default:
         return false;
   }
}


// Attach a given attributes to this node
//
// @param in_index:          The index of the attribute to be read (the point index)
// @param in_dataArrayIndex: The index of the object if in multipoint mode. For instance, say particles 3 and 5 are of type sphere.
//                           This function will be called with in_dataArrayIndex== 0 and 1, while instead in_index will be 3 and 5
// @param in_attr:           The ice attribute
// @param in_declareType:    The type of data (constant, uniform, varying)
// @param in_count:          The size of the array to be allocated
// @param in_offset:         Used only for varying type, ie for strands. It's the offset where to start writing data into the array
// @param in_strandCount:    For strands only: the total number of strands
// @param in_nbStrandPoints: For strands only: The number of points on this single strand
//
void CIceObjectBase::DeclareICEAttributeOnNode(LONG in_index, LONG in_dataArrayIndex, CIceAttribute *in_attr, 
                                               double in_frame, eDeclICEAttr in_declareType, LONG in_count, 
                                               LONG in_offset, int in_strandCount, int in_nbStrandPoints)
{
   // check if pointers are valid
   if (m_node == NULL || in_attr == NULL || !in_attr->m_isDefined)
      return;
   // Skip "private" attributes. The doc tells
   // Attributes whose names begin with two underscore characters are hidden and not 
   // shown in attribute explorers. This can be useful if you have internal bookkeeping
   // attributes inside compounds that you don't want users to fiddle with.
   // However, i am not sure we should do this, at least I am not getting the real reason
   if (in_attr->m_name.Length() > 1)
      if (in_attr->m_name[0] == '_' && in_attr->m_name[1] == '_')
         return;

   // Special check for strands, which are the only case getting here with in_declareType == eDeclICEAttr_Varying
   // The strands come here with in_declareType == eDeclICEAttr_Varying, but we should also check that 
   // the attribute is a 2d one, and if it's constant.
   if (in_declareType == eDeclICEAttr_Varying)
   {
      // Fixing #1175 also, because I was not considering the Uniform possibility
      if (!in_attr->m_isArray) // it's NOT a 2d array. For instance, the Color (NOT the StrandColor) 
      {
         if (in_attr->m_isConstant)
            in_declareType = eDeclICEAttr_Constant;
         else
         {
            in_declareType = eDeclICEAttr_Uniform;
            in_count = in_strandCount; // instead of the total number of points, the attribute size is then equal to the number of strands
         }
      }
      // For a constant 2d array case, we stick with varying data
   }

   CString attrTrimmedName; // not lower case

   // if we declare a uniform value we need to a declare the param 
   // and then allocate the array. For constant values it is enough to
   // just declare the param and then write the value (without allocating an array)
   switch (in_declareType)
   {
      case eDeclICEAttr_Constant:
      {
         if (in_attr->m_isConstant)
            in_index = 0;

         if (in_attr->m_isArray) // constant arrays, original patch by Steven Caron
         {
            if ( m_isProcedural && CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, attrTrimmedName, false) )
            {
               switch (in_attr->m_eType)
               {
                  case siICENodeDataBool:
                     in_attr->m_bData2D.GetSubArray(in_index, in_attr->m_bData);
                     if (in_attr->m_bData.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_bData.GetCount(), 1, AI_TYPE_BOOLEAN);
                        for (int i=0; i<(int)in_attr->m_bData.GetCount(); i++)
                           AiArraySetBool(dataArray, i, in_attr->GetBool(i));
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY BOOL");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }
                     break;

                  case siICENodeDataLong:
                     in_attr->m_lData2D.GetSubArray(in_index, in_attr->m_lData);
                     if (in_attr->m_lData.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_lData.GetCount(), 1, AI_TYPE_INT);
                        for (int i=0; i<(int)in_attr->m_lData.GetCount(); i++)
                           AiArraySetInt(dataArray, i, in_attr->GetInt(i));
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY INT");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }
                     break;

                  case siICENodeDataFloat:
                     in_attr->m_fData2D.GetSubArray(in_index, in_attr->m_fData);
                     if (in_attr->m_fData.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_fData.GetCount(), 1, AI_TYPE_FLOAT);
                        for (int i=0; i<(int)in_attr->m_fData.GetCount(); i++)
                           AiArraySetFlt(dataArray, i, in_attr->GetFloat(i));
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY FLOAT");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }
                     break;

                  case siICENodeDataVector3:
                     in_attr->m_v3Data2D.GetSubArray(in_index, in_attr->m_v3Data);
                     if (in_attr->m_v3Data.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_v3Data.GetCount(), 1, AI_TYPE_VECTOR);
                        AtVector vec;
                        for (int i=0; i<(int)in_attr->m_v3Data.GetCount(); i++)
                        {
                           CUtilities().S2A(in_attr->GetVector3f(i), vec);
                           AiArraySetVec(dataArray, i, vec);
                        }
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY VECTOR");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }
                     break;

                  case siICENodeDataColor4:
                     in_attr->m_cData2D.GetSubArray(in_index, in_attr->m_cData);
                     if (in_attr->m_cData.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_cData.GetCount(), 1, AI_TYPE_RGBA);
                        AtRGBA rgba;
                        for (int i=0; i<(int)in_attr->m_cData.GetCount(); i++)
                        {
                           CUtilities().S2A(in_attr->GetColor4f(i), rgba);
                           AiArraySetRGBA(dataArray, i, rgba);
                        }
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY RGBA");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }
                     break;

                  case siICENodeDataString:
                  {
                     // Unfortunately this does not work with 2d arrays of string.
                     // It look all correct to me, but in_attr->m_strData.GetData(i, &pStr, nCount)
                     // always return the strings of the first subarray.
                     in_attr->m_strData2D.GetSubArray(in_index, in_attr->m_strData);
                     if (in_attr->m_strData.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_strData.GetCount(), 1, AI_TYPE_STRING);
                        for (int i=0; i < (int)in_attr->m_strData.GetCount(); i++)
                        {
                           ULONG nCount;
                           const CICEAttributeDataArrayString::TData* pStr;
                           in_attr->m_strData.GetData(i, &pStr, nCount);
                           AiArraySetStr(dataArray, i, CString(pStr, nCount).GetAsciiString());
                        }
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY STRING");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }  
                     break;
                  }

                  case siICENodeDataMatrix44:
                     in_attr->m_m4Data2D.GetSubArray(in_index, in_attr->m_m4Data);
                     if (in_attr->m_m4Data.GetCount() > 0)
                     {
                        AtArray *dataArray = AiArrayAllocate(in_attr->m_m4Data.GetCount(), 1, AI_TYPE_MATRIX);
                        AtMatrix matrix;
                        for (int i=0; i<(int)in_attr->m_m4Data.GetCount(); i++)
                        {
                           CUtilities().S2A(in_attr->GetMatrix4f(i), matrix);
                           AiArraySetMtx(dataArray, i, matrix);
                        }
                        AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant ARRAY MATRIX");
                        AiNodeSetArray(m_node, attrTrimmedName.GetAsciiString(), dataArray);
                     }
                     break;

                  default:
                     return;
               }
            }
         }
         else // NOT in_attr->m_isArray
         {
            attrTrimmedName = in_attr->m_name;
            if (m_isProcedural) // if procedural, try cutting the "ArnoldProcedural" prefix. Else, the attr name stays the same
               CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, attrTrimmedName, false);

            switch (in_attr->m_eType)
            {
               case siICENodeDataBool:
               {
                  // try setting the parameter directly for light or procedurals (#1219 and #1248).
                  // if failed, export the attribute as user data
                  if (!SetICEAttributeAsNodeParameter(in_attr, in_attr->GetBool(in_index)))
                     if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant BOOL"))
                        CNodeSetter::SetBoolean(m_node, attrTrimmedName.GetAsciiString(), in_attr->GetBool(in_index));
                  break;
               }
               case siICENodeDataLong:
               {
                  if (!SetICEAttributeAsNodeParameter(in_attr, in_attr->GetInt(in_index)))
                     if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant INT"))
                        CNodeSetter::SetInt(m_node, attrTrimmedName.GetAsciiString(), in_attr->GetInt(in_index));
                  break;
               }
               case siICENodeDataFloat:
               {
                  if (!SetICEAttributeAsNodeParameter(in_attr, in_attr->GetFloat(in_index)))
                     if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant FLOAT"))
                        CNodeSetter::SetFloat(m_node, attrTrimmedName.GetAsciiString(), in_attr->GetFloat(in_index));
                  break;
               }
               case siICENodeDataVector3:
               {
                  if (!SetICEAttributeAsNodeParameter(in_attr, in_attr->GetVector3f(in_index)))
                     if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant VECTOR"))
                        CNodeSetter::SetVector(m_node, attrTrimmedName.GetAsciiString(), 
                        in_attr->GetVector3f(in_index).GetX(), in_attr->GetVector3f(in_index).GetY(), in_attr->GetVector3f(in_index).GetZ());
                  break;
               }
               case siICENodeDataColor4:
               {
                  if (!SetICEAttributeAsNodeParameter(in_attr, in_attr->GetColor4f(in_index)))
                     if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant RGBA"))
                        CNodeSetter::SetRGBA(m_node, attrTrimmedName.GetAsciiString(), 
                        in_attr->GetColor4f(in_index).GetR(), in_attr->GetColor4f(in_index).GetG(), in_attr->GetColor4f(in_index).GetB(), in_attr->GetColor4f(in_index).GetA());
                  break;
               }
               case siICENodeDataString:
               {
                  // we export string user data only for procedurals by now, other types of node would
                  // not know what to do with them, since there is no string shader to pull them
                  if ( m_isProcedural && CIceUtilities().GetTrimmedArnoldProceduralAttributeName(in_attr->m_name, attrTrimmedName, false) )
                  {
                     ULONG nCount;
                     const CICEAttributeDataArrayString::TData* pStr;         
                     in_attr->m_strData.GetData(in_index, &pStr, nCount);
                     CPathString stringAttribute = CString(pStr, nCount);
                     // Resolve the tokens, for strings like [Project Path] etc.
                     // This is the only editing we do to the strings. Since this is a general
                     // parameters settings, let's not give for granted that we can apply all the
                     // path translation steps we do in other places, like in LoadSingleProcedural.
                     stringAttribute.ResolveTokensInPlace(in_frame);

                     // also in this case, try setting the string parameter directly for procedurals (#1248).
                     // If failed, add it as user data
                     if (!SetICEAttributeAsNodeParameter(in_attr, (CString)stringAttribute))
                        if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant STRING"))
                           CNodeSetter::SetString(m_node, attrTrimmedName.GetAsciiString(), stringAttribute.GetAsciiString());
                  }
                  break;
               }
               case siICENodeDataMatrix44:
                  if (!SetICEAttributeAsNodeParameter(in_attr, in_attr->GetMatrix4f(in_index)))
                     if (AiNodeDeclare(m_node, attrTrimmedName.GetAsciiString(), "constant MATRIX"))
                     {
                        AtMatrix matrix;
                        CUtilities().S2A(in_attr->GetMatrix4f(in_index), matrix);
                        CNodeSetter::SetMatrix(m_node, attrTrimmedName.GetAsciiString(), matrix);
                     }
                  break;
               default:
                  return;
            }
         }

         break; // done with case eDeclICEAttr_Constant (ginstances, procedurals, etc.)
      }
      case eDeclICEAttr_Uniform:
      {
         LONG attrIndex = in_attr->m_isConstant ? 0 : in_index;

         if (!in_attr->m_isArray) // uniform arrays are not supported at this point...!
         {
            switch (in_attr->m_eType)
            {
               case siICENodeDataBool:
               {
                  if (in_dataArrayIndex == 0) // the array does not exist yet, let's create it
                  {
                     if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform BOOL"))
                        AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_BOOLEAN));
                  }
                  AtArray* dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());
                  AiArraySetBool(dataArray, in_dataArrayIndex, in_attr->GetBool(attrIndex));
                  break;
               }
               case siICENodeDataLong:
               {
                  if (in_dataArrayIndex == 0)
                  {
                     if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform INT"))
                        AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_INT));
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());
                  AiArraySetInt(dataArray, in_dataArrayIndex, in_attr->GetInt(attrIndex));
                  break;
               }
               case siICENodeDataFloat:
               {
                  if (in_dataArrayIndex == 0)
                  {
                     if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform FLOAT"))
                        AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_FLOAT));
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());
                  AiArraySetFlt(dataArray, in_dataArrayIndex, in_attr->GetFloat(attrIndex));
                  break;
               }
               case siICENodeDataVector3:
               {
                  if (in_dataArrayIndex == 0)
                  {
                     if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform VECTOR"))
                        AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_VECTOR));
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());
                  AtVector vec;
                  CUtilities().S2A(in_attr->GetVector3f(attrIndex), vec);
                  AiArraySetVec(dataArray, in_dataArrayIndex, vec);
                  break;
               }
               case siICENodeDataColor4:
               {
                  if (in_dataArrayIndex == 0)
                  {
                     if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform RGBA"))
                        AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_RGBA));
                  }
                  AtArray *dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());
                  AtRGBA rgba;
                  CUtilities().S2A(in_attr->GetColor4f(attrIndex), rgba);
                  AiArraySetRGBA(dataArray, in_dataArrayIndex, rgba);
                  break;
               }
               case siICENodeDataMatrix44:
               {
                  if (in_dataArrayIndex == 0)
                  {
                     if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform MATRIX"))
                        AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_MATRIX));
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());
                  AtMatrix matrix;
                  CUtilities().S2A(in_attr->GetMatrix4f(attrIndex), matrix);
                  AiArraySetMtx(dataArray, in_dataArrayIndex, matrix);
                  break;
               }
               default:
                  return;
            }
         }
         break;
      }

      // we get here only for strands
      case eDeclICEAttr_Varying:
      {
         LONG attrIndex = in_attr->m_isConstant ? 0 : in_index;

         if (in_attr->m_isArray)
         {
            switch (in_attr->m_eType)
            {
               case siICENodeDataBool:
               {
                  // get the subarray
                  in_attr->m_bData2D.GetSubArray(attrIndex, in_attr->m_bData);
                  int nbAttributeValues = in_attr->m_bData.GetCount();
                  if (in_index == 0)
                  {
                     // The first time we try to write into the array, we must also allocate it.
                     // However, the first thing to do is check if the subarray has more than one data.
                     // Else, it means that we have one single data (for instance a bool) for each strand, and
                     // so we must default to "uniform", which in the case of curves means "one data per curve strand".
                     // For instance, one could use "Set Particle Color" on a strand, to give the same color to the entire strand.
                     // In this case, the size of the array to be allocated is, of course, equal to the number of strands
                     if (nbAttributeValues == 1)
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform BOOL"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_strandCount, 1, AI_TYPE_BOOLEAN));
                     }
                     else
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "varying BOOL"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_BOOLEAN));
                     }
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());

                  int nbArrayValues = nbAttributeValues;
                  if (nbAttributeValues == 1) // the offset, so WHERE to write, is then equal to incoming strand index
                     in_offset = in_index;
                  else
                  {
                     if (nbAttributeValues != in_nbStrandPoints)
                     {
                        nbArrayValues = in_nbStrandPoints;  // override nbArrayValues if it's mismatch so that we always set the right amount ov values to the array, Github #70
                        GetMessageQueue()->LogMsg(L"[sitoa] Strand #" + CString(in_index) + L": " + in_attr->m_name + L" array count mismatch. ("+ in_attr->m_name + L": " + CString(nbAttributeValues) + L", StrandPosition: " + CString(in_nbStrandPoints) + L")", siWarningMsg);
                     }
                  }

                  for (ULONG i=0; i<nbArrayValues; i++, in_offset++)
                  {
                     if (i < nbAttributeValues)
                        AiArraySetBool(dataArray, in_offset, in_attr->GetBool(i));
                     else
                        AiArraySetBool(dataArray, in_offset, false);
                  }
                  break;
               } // all the other cases work the same way, except for the data type
               case siICENodeDataLong:
               {
                  // get the sub array
                  in_attr->m_lData2D.GetSubArray(attrIndex, in_attr->m_lData);
                  int nbAttributeValues = in_attr->m_lData.GetCount();
                  if (in_index == 0)
                  {
                     if (nbAttributeValues == 1)
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform INT"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_strandCount, 1, AI_TYPE_INT));
                     }
                     else
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "varying INT"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_INT));
                     }
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());

                  int nbArrayValues = nbAttributeValues;
                  if (nbAttributeValues == 1)
                     in_offset = in_index;
                  else
                  {
                     if (nbAttributeValues != in_nbStrandPoints)
                     {
                        nbArrayValues = in_nbStrandPoints;  // override nbArrayValues if it's mismatch so that we always set the right amount ov values to the array, Github #70
                        GetMessageQueue()->LogMsg(L"[sitoa] Strand #" + CString(in_index) + L": " + in_attr->m_name + L" array count mismatch. ("+ in_attr->m_name + L": " + CString(nbAttributeValues) + L", StrandPosition: " + CString(in_nbStrandPoints) + L")", siWarningMsg);
                     }
                  }

                  for (ULONG i=0; i<nbArrayValues; i++, in_offset++)
                  {
                     if (i < nbAttributeValues)
                        AiArraySetInt(dataArray, in_offset, in_attr->GetInt(i));
                     else
                        AiArraySetInt(dataArray, in_offset, 0);
                  }
                  break;
               }
               case siICENodeDataFloat:
               {
                  // get the sub array
                  in_attr->m_fData2D.GetSubArray(attrIndex, in_attr->m_fData);
                  int nbAttributeValues = in_attr->m_fData.GetCount();
                  if (in_index == 0)
                  {
                     if (nbAttributeValues == 1)
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform FLOAT"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_strandCount, 1, AI_TYPE_FLOAT));
                     }
                     else
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "varying FLOAT"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_FLOAT));
                     }
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());

                  int nbArrayValues = nbAttributeValues;
                  if (nbAttributeValues == 1)
                     in_offset = in_index;
                  else
                  {
                     if (nbAttributeValues != in_nbStrandPoints)
                     {
                        nbArrayValues = in_nbStrandPoints;  // override nbArrayValues if it's mismatch so that we always set the right amount ov values to the array, Github #70
                        GetMessageQueue()->LogMsg(L"[sitoa] Strand #" + CString(in_index) + L": " + in_attr->m_name + L" array count mismatch. ("+ in_attr->m_name + L": " + CString(nbAttributeValues) + L", StrandPosition: " + CString(in_nbStrandPoints) + L")", siWarningMsg);
                     }
                  }

                  for (ULONG i=0; i<nbArrayValues; i++, in_offset++)
                  {
                     if (i < nbAttributeValues)
                        AiArraySetFlt(dataArray, in_offset, in_attr->GetFloat(i));
                     else
                        AiArraySetFlt(dataArray, in_offset, 0.0f);
                  }
                  break;
               }
               case siICENodeDataVector3:
               {
                  // get the sub array
                  in_attr->m_v3Data2D.GetSubArray(attrIndex, in_attr->m_v3Data);
                  int nbAttributeValues = in_attr->m_v3Data.GetCount();
                  if (in_index == 0)
                  {
                     if (nbAttributeValues == 1)
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform VECTOR"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_strandCount, 1, AI_TYPE_VECTOR));
                     }
                     else
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "varying VECTOR"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_VECTOR));
                     }
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());

                  int nbArrayValues = nbAttributeValues;
                  if (nbAttributeValues == 1)
                     in_offset = in_index;
                  else
                  {
                     if (nbAttributeValues != in_nbStrandPoints)
                     {
                        nbArrayValues = in_nbStrandPoints;  // override nbArrayValues if it's mismatch so that we always set the right amount ov values to the array, Github #70
                        GetMessageQueue()->LogMsg(L"[sitoa] Strand #" + CString(in_index) + L": " + in_attr->m_name + L" array count mismatch. ("+ in_attr->m_name + L": " + CString(nbAttributeValues) + L", StrandPosition: " + CString(in_nbStrandPoints) + L")", siWarningMsg);
                     }
                  }

                  AtVector vec;
                  for (ULONG i=0; i<nbArrayValues; i++, in_offset++)
                  {
                     if (i < nbAttributeValues)
                        CUtilities().S2A(in_attr->GetVector3f(i), vec);
                     else
                        vec = AtVector(0.0f, 0.0f, 0.0f);
                     AiArraySetVec(dataArray, in_offset, vec);
                  }
                  break;
               }
               case siICENodeDataColor4:
               {
                  // get the sub array
                  in_attr->m_cData2D.GetSubArray(attrIndex, in_attr->m_cData);
                  int nbAttributeValues = in_attr->m_cData.GetCount();
                  if (in_index == 0)
                  {
                     if (nbAttributeValues == 1)
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform RGBA"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_strandCount, 1, AI_TYPE_RGBA));
                     }
                     else
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "varying RGBA"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_RGBA));
                     }
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());

                  int nbArrayValues = nbAttributeValues;
                  if (nbAttributeValues == 1)
                     in_offset = in_index;
                  else
                  {
                     if (nbAttributeValues != in_nbStrandPoints)
                     {
                        nbArrayValues = in_nbStrandPoints;  // override nbArrayValues if it's mismatch so that we always set the right amount ov values to the array, Github #70
                        GetMessageQueue()->LogMsg(L"[sitoa] Strand #" + CString(in_index) + L": " + in_attr->m_name + L" array count mismatch. ("+ in_attr->m_name + L": " + CString(nbAttributeValues) + L", StrandPosition: " + CString(in_nbStrandPoints) + L")", siWarningMsg);
                     }
                  }

                  AtRGBA rgba;
                  for (ULONG i=0; i<nbArrayValues; i++, in_offset++)
                  {
                     if (i < nbAttributeValues)
                        CUtilities().S2A(in_attr->GetColor4f(i), rgba);
                     else
                        rgba = AI_RGBA_ZERO;
                     AiArraySetRGBA(dataArray, in_offset, rgba);
                  }
                  break;
               }
               case siICENodeDataMatrix44:
               {
                  // get the sub array
                  in_attr->m_m4Data2D.GetSubArray(attrIndex, in_attr->m_m4Data);
                  int nbAttributeValues = in_attr->m_m4Data.GetCount();
                  if (in_index == 0)
                  {
                     if (nbAttributeValues == 1)
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "uniform MATRIX"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_strandCount, 1, AI_TYPE_MATRIX));
                     }
                     else
                     {
                        if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), "varying MATRIX"))
                           AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), AiArrayAllocate(in_count, 1, AI_TYPE_MATRIX));
                     }
                  }
                  AtArray * dataArray = AiNodeGetArray(m_node, in_attr->m_name.GetAsciiString());

                  int nbArrayValues = nbAttributeValues;
                  if (nbAttributeValues == 1)
                     in_offset = in_index;
                  else
                  {
                     if (nbAttributeValues != in_nbStrandPoints)
                     {
                        nbArrayValues = in_nbStrandPoints;  // override nbArrayValues if it's mismatch so that we always set the right amount ov values to the array, Github #70
                        GetMessageQueue()->LogMsg(L"[sitoa] Strand #" + CString(in_index) + L": " + in_attr->m_name + L" array count mismatch. ("+ in_attr->m_name + L": " + CString(nbAttributeValues) + L", StrandPosition: " + CString(in_nbStrandPoints) + L")", siWarningMsg);
                     }
                  }

                  AtMatrix matrix;
                  for (ULONG i=0; i<nbArrayValues; i++, in_offset++)
                  {
                     if (i < nbAttributeValues)
                        CUtilities().S2A(in_attr->GetMatrix4f(i), matrix);
                     else
                        matrix = AiM4Identity();
                     AiArraySetMtx(dataArray, in_offset, matrix);
                  }
                  break;
               }
               default:
                  return;
            }
         }

         break;
      }
      default:
         break;
   }
}


// Attach a given attribute to a mesh node.
//
// @param in_attr:           The ice attribute
// @param in_indices:        Indices to be used for face-varying data
//
void CIceObjectBase::DeclareICEAttributeOnMeshNode(CIceAttribute *in_attr, const AtArray *in_indices)
{
   // check if pointers are valid
   if (m_node == NULL || in_attr == NULL || !in_attr->m_isDefined)
      return;
   // not managing array attributes yet, we would need first to support the array index
   // of the attribute shaders
   if (in_attr->m_isArray)
      return;

   // skipping "private" attributes.
   if (in_attr->m_name.Length() > 1)
      if (in_attr->m_name[0] == '_' && in_attr->m_name[1] == '_')
         return;

   // skipping attributes such as per-edge, or (sadly) per-polynode like texture uvs
   siICENodeContextType contextType = in_attr->GetContextType();

   if (!in_attr->IsConstant())
   {
      bool isValidContextType = contextType == siICENodeContextComponent0D ||            // per point attribute
                                contextType == siICENodeContextSingletonOrComponent0D || // per point attribute
                                contextType == siICENodeContextComponent2D ||            // per polygon attribute
                                contextType == siICENodeContextSingletonOrComponent2D || // per polygon attribute
                                in_indices && contextType == siICENodeContextComponent0D2D; // per node (aka indexed)

      if (!isValidContextType) 
         return;
   }

   eDeclICEAttr declareType;
   string declaration;

   // make up the declaration string
   if (in_attr->IsConstant())
   {
      declareType = eDeclICEAttr_Constant;
      declaration = "constant";
   }
   else if (contextType == siICENodeContextComponent0D ||contextType == siICENodeContextSingletonOrComponent0D)
   {
      declareType = eDeclICEAttr_Varying;
      declaration = "varying";
   }
   else if (contextType == siICENodeContextComponent2D ||contextType == siICENodeContextSingletonOrComponent2D)
   {
      declareType = eDeclICEAttr_Uniform;
      declaration = "uniform";
   }
   else // siICENodeContextComponent0D2D
   {
      declareType = eDeclICEAttr_Indexed;
      declaration = "indexed";
   }

   switch (in_attr->m_eType)
   {
      case siICENodeDataBool:
         declaration+= " BOOL";
         break;
      case siICENodeDataLong:
         declaration+= " INT";
         break;
      case siICENodeDataFloat:
         declaration+= " FLOAT";
         break;
      case siICENodeDataVector3:
         declaration+= " VECTOR";
         break;
      case siICENodeDataColor4:
         declaration+= " RGBA";
         break;
      case siICENodeDataMatrix44:
         declaration+= " MATRIX";
         break;
      default:
         return;
   }

   // ready, go
   if (declareType == eDeclICEAttr_Constant)
   {
      switch (in_attr->m_eType)
      {
         case siICENodeDataBool:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
               CNodeSetter::SetBoolean(m_node, in_attr->m_name.GetAsciiString(), in_attr->GetBool(0));
            break;
         case siICENodeDataLong:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
               CNodeSetter::SetInt(m_node, in_attr->m_name.GetAsciiString(), in_attr->GetInt(0));
            break;
         case siICENodeDataFloat:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
               CNodeSetter::SetFloat(m_node, in_attr->m_name.GetAsciiString(), in_attr->GetFloat(0));
            break;
         case siICENodeDataVector3:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
               CNodeSetter::SetVector(m_node, in_attr->m_name.GetAsciiString(), 
                  in_attr->GetVector3f(0).GetX(), in_attr->GetVector3f(0).GetY(), in_attr->GetVector3f(0).GetZ());
            break;
         case siICENodeDataColor4:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               CNodeSetter::SetRGBA(m_node, in_attr->m_name.GetAsciiString(), 
                  in_attr->GetColor4f(0).GetR(), in_attr->GetColor4f(0).GetG(), in_attr->GetColor4f(0).GetB(), in_attr->GetColor4f(0).GetA());
            }
            break;
         case siICENodeDataMatrix44:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               AtMatrix m;
               CUtilities().S2A(in_attr->GetMatrix4f(0), m);
               CNodeSetter::SetMatrix(m_node, in_attr->m_name.GetAsciiString(), m);
            }
            break;
         default:
            return;
      }
   }
   else 
   {
      AtArray* dataArray = NULL;
      ULONG count;
      switch (in_attr->m_eType)
      {
         case siICENodeDataBool:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               count = in_attr->m_bData.GetCount();
               dataArray = AiArrayAllocate(count, 1, AI_TYPE_BOOLEAN);
               for (ULONG i = 0; i < count; i++)
                  AiArraySetBool(dataArray, i, in_attr->GetBool(i));
            }
            break;

         case siICENodeDataLong:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               count = in_attr->m_lData.GetCount();
               dataArray = AiArrayAllocate(count, 1, AI_TYPE_INT);
               for (ULONG i = 0; i < count; i++)
                  AiArraySetInt(dataArray, i, in_attr->GetInt(i));
            }
            break;

         case siICENodeDataFloat:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               count = in_attr->m_fData.GetCount();
               dataArray = AiArrayAllocate(count, 1, AI_TYPE_FLOAT);
               for (ULONG i = 0; i < count; i++)
                  AiArraySetFlt(dataArray, i, in_attr->GetFloat(i));
            }
            break;

         case siICENodeDataVector3:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               count = in_attr->m_v3Data.GetCount();
               dataArray = AiArrayAllocate(count, 1, AI_TYPE_VECTOR);
               AtVector vec;
               for (ULONG i = 0; i < count; i++)
               {
                  CUtilities().S2A(in_attr->GetVector3f(i), vec);
                  AiArraySetVec(dataArray, i, vec);
               }
            }
            break;

         case siICENodeDataColor4:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               count = in_attr->m_cData.GetCount();
               dataArray = AiArrayAllocate(count, 1, AI_TYPE_RGBA);
               AtRGBA rgba;
               for (ULONG i = 0; i < count; i++)
               {
                  CUtilities().S2A(in_attr->GetColor4f(i), rgba);
                  AiArraySetRGBA(dataArray, i, rgba);
               }
            }
            break;

         case siICENodeDataMatrix44:
            if (AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str()))
            {
               count = in_attr->m_m4Data.GetCount();
               dataArray = AiArrayAllocate(count, 1, AI_TYPE_MATRIX);
               AtMatrix m;
               for (ULONG i = 0; i < count; i++)
               {
                  CUtilities().S2A(in_attr->GetMatrix4f(i), m);
                  AiArraySetMtx(dataArray, i, m);
               }
            }
            break;

         default:
            return;
      }

      if (dataArray)
      {
         AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), dataArray);
         if (declareType == eDeclICEAttr_Indexed)
         {
            CString idxName = in_attr->m_name + L"idxs";
            AiNodeSetArray(m_node, idxName.GetAsciiString(), AiArrayCopy(in_indices));
         }
      }
   }
}


// Attach a given attribute to a volume node.
//
// @param in_attr:           The ice attribute
//
void CIceObjectBase::DeclareICEAttributeOnVolumeNode(CIceAttribute *in_attr)
{
   // check if pointers are valid
   if (m_node == NULL || in_attr == NULL || !in_attr->m_isDefined)
      return;

   // skipping "private" attributes.
   if (in_attr->m_name.Length() > 1)
      if (in_attr->m_name[0] == '_' && in_attr->m_name[1] == '_')
         return;

   string declaration = "constant ARRAY";
   unsigned char arnoldType = AI_TYPE_NONE;

   if (in_attr->m_isArray)
   {
      declaration+= " ARRAY";
      arnoldType = AI_TYPE_ARRAY;
   }
   else
   {
      switch (in_attr->m_eType)
      {
         case siICENodeDataBool:
            declaration+= " BOOL";
            arnoldType = AI_TYPE_BOOLEAN;
            break;
         case siICENodeDataLong:
            declaration+= " INT";
            arnoldType = AI_TYPE_INT;
            break;
         case siICENodeDataFloat:
            declaration+= " FLOAT";
            arnoldType = AI_TYPE_FLOAT;
            break;
         case siICENodeDataVector3:
            declaration+= " VECTOR";
            arnoldType = AI_TYPE_VECTOR;
            break;
         case siICENodeDataColor4:
            declaration+= " RGBA";
            arnoldType = AI_TYPE_RGBA;
            break;
         case siICENodeDataMatrix44:
         case siICENodeDataRotation: // rotations are axported as matrices
            declaration+= " MATRIX";
            arnoldType = AI_TYPE_MATRIX;
            break;
         case siICENodeDataString:
            declaration+= " STRING";
            arnoldType = AI_TYPE_STRING;
            break;
         default:
            return;
      }
   }

   // we export constant data as arrays of size 1, so not to have too many cases to deal with while shader-writing
   ULONG count = in_attr->m_isConstant ? 1 : in_attr->GetElementCount();

   AiNodeDeclare(m_node, in_attr->m_name.GetAsciiString(), declaration.c_str());
   AtArray *dataArray = AiArrayAllocate(count, 1, arnoldType);

   switch (in_attr->m_eType)
   {
      case siICENodeDataBool:
         for (ULONG i=0; i<count; i++)
         {
            if (!in_attr->m_isArray)
               AiArraySetBool(dataArray, i, in_attr->GetBool(i));
            else
            {
               in_attr->m_bData2D.GetSubArray(i, in_attr->m_bData);
               ULONG subCount = in_attr->m_bData.IsConstant() ? 1 : in_attr->m_bData.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_BOOLEAN);
               for (ULONG j=0; j<subCount; j++)
                  AiArraySetBool(subArray, j, in_attr->GetBool(j));
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataLong:
         for (ULONG i=0; i<count; i++)
         {
            if (!in_attr->m_isArray)
               AiArraySetInt(dataArray, i, in_attr->GetInt(i));
            else
            {
               in_attr->m_lData2D.GetSubArray(i, in_attr->m_lData);
               ULONG subCount = in_attr->m_lData.IsConstant() ? 1 : in_attr->m_lData.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_INT);
               for (ULONG j=0; j<subCount; j++)
                  AiArraySetInt(subArray, j, in_attr->GetInt(j));
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataFloat:
         for (ULONG i=0; i<count; i++)
         {
            if (!in_attr->m_isArray)
               AiArraySetFlt(dataArray, i, in_attr->GetFloat(i));
            else
            {
               in_attr->m_fData2D.GetSubArray(i, in_attr->m_fData);
               ULONG subCount = in_attr->m_fData.IsConstant() ? 1 : in_attr->m_fData.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_FLOAT);
               for (ULONG j=0; j<subCount; j++)
                  AiArraySetFlt(subArray, j, in_attr->GetFloat(j));
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataVector3:
         for (ULONG i=0; i<count; i++)
         {
            AtVector vec;
            if (!in_attr->m_isArray)
            {
               CUtilities().S2A(in_attr->GetVector3f(i), vec);
               AiArraySetVec(dataArray, i, vec);
            }
            else
            {
               in_attr->m_v3Data2D.GetSubArray(i, in_attr->m_v3Data);
               ULONG subCount = in_attr->m_v3Data.IsConstant() ? 1 : in_attr->m_v3Data.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_VECTOR);
               for (ULONG j=0; j<subCount; j++)
               {
                  CUtilities().S2A(in_attr->GetVector3f(j), vec);
                  AiArraySetVec(subArray, j, vec);
               }
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataColor4:
         for (ULONG i=0; i<count; i++)
         {
            AtRGBA rgba;
            if (!in_attr->m_isArray)
            {
               CUtilities().S2A(in_attr->GetColor4f(i), rgba);
               AiArraySetRGBA(dataArray, i, rgba);
            }
            else
            {
               in_attr->m_cData2D.GetSubArray(i, in_attr->m_cData);
               ULONG subCount = in_attr->m_cData.IsConstant() ? 1 : in_attr->m_cData.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_RGBA);
               for (ULONG j=0; j<subCount; j++)
               {
                  CUtilities().S2A(in_attr->GetColor4f(j), rgba);
                  AiArraySetRGBA(subArray, j, rgba);
               }
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataMatrix44:
         for (ULONG i=0; i<count; i++)
         {
            AtMatrix matrix;
            if (!in_attr->m_isArray)
            {
               CUtilities().S2A(in_attr->GetMatrix4f(i), matrix);
               AiArraySetMtx(dataArray, i, matrix);
            }
            else
            {
               in_attr->m_m4Data2D.GetSubArray(i, in_attr->m_m4Data);
               ULONG subCount = in_attr->m_m4Data.IsConstant() ? 1 : in_attr->m_m4Data.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_MATRIX);
               for (ULONG j=0; j<subCount; j++)
               {
                  CUtilities().S2A(in_attr->GetMatrix4f(j), matrix);
                  AiArraySetMtx(subArray, j, matrix);
               }
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataRotation:
         // orientations are stored as matrices
         for (ULONG i=0; i<count; i++)
         {
            AtMatrix matrix;
            if (!in_attr->m_isArray)
            {
               CUtilities().S2A(in_attr->GetRotationf(i), matrix);
               AiArraySetMtx(dataArray, i, matrix);
            }
            else
            {
               in_attr->m_rData2D.GetSubArray(i, in_attr->m_rData);
               ULONG subCount = in_attr->m_rData.IsConstant() ? 1 : in_attr->m_rData.GetCount();
               AtArray *subArray = AiArrayAllocate(subCount, 1, AI_TYPE_MATRIX);
               for (ULONG j=0; j<subCount; j++)
               {
                  CUtilities().S2A(in_attr->GetRotationf(j), matrix);
                  AiArraySetMtx(subArray, j, matrix);
               }
               AiArraySetArray(dataArray, i, subArray);
            }
         }
         break;

      case siICENodeDataString:
         for (ULONG i=0; i<count; i++)
         {
            if (!in_attr->m_isArray)
            {
               ULONG nCount;
               const CICEAttributeDataArrayString::TData* pStr;         
               in_attr->m_strData.GetData(i, &pStr, nCount);
               CString s(pStr, nCount);
               AiArraySetStr(dataArray, i, s.GetAsciiString());
            }
         }
         break;

      default:
         return;
   }

   AiNodeSetArray(m_node, in_attr->m_name.GetAsciiString(), dataArray);
}

void CIceObjectBase::SetMotionStartEnd()
{
   CNodeUtilities::SetMotionStartEnd(m_node);
}

// Set the arnold user options (#680) for this node
//
// @param in_property        The Arnold User Options custom property
// @param in_frame           The frame time
//
void CIceObjectBase::SetArnoldUserOptions(CustomProperty in_property, double in_frame)
{
   LoadUserOptions(m_node, in_property, in_frame); 
}

// Set the user data blobs (#728) for this node
//
// @param in_blobProperties    The blob property
// @param in_frame             The frame time
//
void CIceObjectBase::SetUserDataBlobs(CRefArray &in_blobProperties, double in_frame)
{
   ExportUserDataBlobProperties(m_node, in_blobProperties, in_frame);
}


// Export the matte data for this node
//
// @param in_property     The matte property
// @param in_frame        The frame time
//
void CIceObjectBase::SetMatte(Property in_property, double in_frame)
{
   LoadMatte(m_node, in_property, in_frame);
}


///////////////////////////////////////////////////////////////////////
// CIceObjectPoints class
// Base class for all the point objects (disk, sphere, quad (quad not implemented yet))
///////////////////////////////////////////////////////////////////////

// Resize the points and radius arrays
//
// @param in_nbElements The number of points
// @param nb_keys       The number of motion keys
//
void CIceObjectPoints::Resize(const int in_nbElements, const int nb_keys)
{
   m_points = AiArrayAllocate(in_nbElements, (uint8_t)nb_keys, AI_TYPE_VECTOR);
   m_radius = AiArrayAllocate(in_nbElements, (uint8_t)nb_keys, AI_TYPE_FLOAT);
}


// Set the in_index/in_key-th point value by a AtVector
//
// @param in_value  The value to set
// @param in_index  The point index
// @param in_key    The mb key
// @return false if the assignment failed, else true
bool CIceObjectPoints::SetPoint(AtVector &in_value, const int in_index, const int in_key)
{
   return CUtilities().SetArrayValue(m_points, in_value, in_index, in_key);
}


// Set the in_index/in_key-th point value by a CVector3f
//
// @param in_value  The value to set
// @param in_index  The point index
// @param in_key    The mb key
// @return false if the assignment failed, else true
bool CIceObjectPoints::SetPoint(CVector3f &in_value, const int in_index, const int in_key)
{
   AtVector point;
   CUtilities().S2A(in_value, point);
   return CUtilities().SetArrayValue(m_points, point, in_index, in_key);
}


// Get the in_index/in_key-th point value
//
// @param in_index  The point index
// @param in_key    The mb key
// @return the point
//
AtVector CIceObjectPoints::GetPoint(const int in_index, const int in_key)
{
   AtVector result = AtVector(0.0f, 0.0f, 0.0f);
   CUtilities().GetArrayValue(m_points, &result, in_index, in_key);
   return result;
}


// Set the in_index/in_key-th radius value
//
// @param in_value  The value to set
// @param in_index  The point index
// @param in_key    The mb key
// @return false if the assignment failed, else true
//
bool CIceObjectPoints::SetRadius(float in_value, const int in_index, const int in_key)
{
   return CUtilities().SetArrayValue(m_radius, in_value, in_index, in_key);
}


// Get the in_index/in_key-th radius value
//
// @param in_index  The point index
// @param in_key    The mb key
// @return the radius
//
float CIceObjectPoints::GetRadius(const int in_index, const int in_key)
{
   float result = 0.0f;
   CUtilities().GetArrayValue(m_radius, &result, in_index, in_key);
   return result;
}


// Create the node
// 
// @param in_id                The input id
// @param in_type              The input type
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectPoints::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   SetNodeBaseAttributes(in_id, "points", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}


// Compute all the positions and radii for mb for a given point
// 
// @param in_keysTime          The mb key times
// @param in_secondsPerFrame   The seconds per frame
// @param in_velocity          The velocity of the point
// @param in_exactMb           True is exact motion blur is required (not based on velocity)
// @param in_mbPos             Array of positions at each mb key for exact mb
// @param in_mbSize            Array of sizes at each mb key for exact mb
// @param in_index             The point index
// @param in_pointIndex        The particle index
//
void CIceObjectPoints::ComputeMotionBlur(CDoubleArray in_keysTime, float in_secondsPerFrame, CVector3f in_velocity, 
                                         bool in_exactMb, const vector <CVector3f> &in_mbPos, const vector <float> &in_mbSize, 
                                         const int in_index, const int in_pointIndex)
{
   AtVector p;
   CVector3f vel;
   float scaleFactor;
   int nbKeys = (int)in_keysTime.GetCount();

   if (in_exactMb)
   {
      for (int iKey = 0; iKey < nbKeys; iKey++)
      {
         int index = in_pointIndex * nbKeys + iKey;
         in_mbPos[index].Get(p.x, p.y, p.z);
         SetPoint(p, in_index, iKey);
         SetRadius(in_mbSize[index], in_index, iKey);
      }
   }
   else
   {
      AtVector p0 = GetPoint(in_index, 0);
      float   r  = GetRadius(in_index, 0);
      for (int iKey = 0; iKey < nbKeys; iKey++)
      {
         scaleFactor = in_secondsPerFrame * (float)in_keysTime[iKey];
         vel.Scale(scaleFactor, in_velocity);
         p.x = p0.x + vel.GetX();
         p.y = p0.y + vel.GetY();
         p.z = p0.z + vel.GetZ();
         SetPoint(p, in_index, iKey);
         // There is not a right way to mb the radii, we would need a size velocity
         SetRadius(r, in_index, iKey);
      }
   }
}


// Give the node the object's attributes.
//
// @return false if the node does not exist, else true
//
bool CIceObjectPoints::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();
   AiNodeSetArray(m_node, "points", m_points);
   AiNodeSetArray(m_node, "radius", m_radius);
   return true;
}


// Attach all the required attributes to this node
//
// @param in_attributes:     The ice attributes
// @param in_frame:          The frame time
// @param in_pointIndex:     The index of the ice point
// @param in_dataArrayIndex: The index of the point.
// @param in_nbPoints:       The total number of points (not the total number of the ice points!)
//
void CIceObjectPoints::DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex, int in_dataArrayIndex, int in_nbPoints)
{
   AttrMap::iterator attribIt;
   // loop the required attributes, and push them as uniform (so, one attribute per point)
   for (attribIt = in_attributes->m_requiredAttributesMap.begin(); attribIt != in_attributes->m_requiredAttributesMap.end(); attribIt++)
      DeclareICEAttributeOnNode(in_pointIndex, in_dataArrayIndex, attribIt->second, in_frame, eDeclICEAttr_Uniform, in_nbPoints);
}


// Set the arnold parameters for all the nodes
//
// @param in_property   The Arnold Parameters custom property
// @param in_frame      The evaluation frame
//
void CIceObjectPoints::SetArnoldParameters(CustomProperty in_property, double in_frame)
{
   LoadArnoldParameters(m_node, in_property.GetParameters(), in_frame, true); 
}


///////////////////////////////////////////////////////////////////////
// CIceObjectRectangle class
///////////////////////////////////////////////////////////////////////

// Resize the points, scale, rotation arrays
//
// @param in_nbElements The number of points
// @param nb_keys       The number of motion keys
// @param in_exactMb    true if exact mb is enabled
//
void CIceObjectRectangle::Resize(const int in_nbElements, const int in_nbKeys, bool in_exactMb)
{
   int scaleAndRotationKeys = in_exactMb ? in_nbKeys : 1;

   m_points.resize(in_nbKeys);
   // if exact mb if off, we don't need to store the scale and rotation for the extra keys,
   // because when computing the mblurred position only the velocity will be used
   m_scale.resize(scaleAndRotationKeys);
   m_rotation.resize(scaleAndRotationKeys);

   for (int i=0; i<in_nbKeys; i++)
      m_points[i].resize(in_nbElements);
   for (int i=0; i<scaleAndRotationKeys; i++)
   {
      m_scale[i].resize(in_nbElements);
      m_rotation[i].resize(in_nbElements);
   }
}


// Set the in_index/in_key-th point value
//
// @param in_value  The value to set
// @param in_index  The point index
// @param in_key    The mb key
//
// @return false if the assignment failed, else true
//
bool CIceObjectRectangle::SetPoint(CVector3f &in_value, const int in_index, const int in_key)
{
   if ((int)m_points.size() <= in_key)
      return false;
   if ((int)m_points[in_key].size() <= in_index)
      return false;
   m_points[in_key][in_index] = in_value;
   return true;
}


// Set the in_index/in_key-th scale value
//
// @param in_value  The value to set
// @param in_index  The point index
// @param in_key    The mb key
//
// @return false if the assignment failed, else true
//
bool CIceObjectRectangle::SetScale(CVector3f &in_value, const int in_index, const int in_key)
{
   if ((int)m_scale.size() <= in_key)
      return false;
   if ((int)m_scale[in_key].size() <= in_index)
      return false;
   m_scale[in_key][in_index] = in_value;
   return true;
}


// Set the in_index/in_key-th rotation value
//
// @param in_value  The value to set
// @param in_index  The point index
// @param in_key    The mb key
//
// @return false if the assignment failed, else true
//
bool CIceObjectRectangle::SetRotation(CRotation &in_value, const int in_index, const int in_key)
{
   if ((int)m_rotation.size() <= in_key)
      return false;
   if ((int)m_rotation[in_key].size() <= in_index)
      return false;
   m_rotation[in_key][in_index] = in_value;
   return true;
}


// Set the in_index/in_key-th point value
//
// @param in_index  The point index
// @param in_key    The mb key
//
// @return the value
//
CVector3f CIceObjectRectangle::GetPoint(const int in_index, const int in_key)
{
   return m_points[in_key][in_index];
}


// Set the in_index/in_key-th scale value
//
// @param in_index  The point index
// @param in_key    The mb key
//
// @return the value
//
CVector3f CIceObjectRectangle::GetScale(const int in_index, const int in_key)
{
   return m_scale[in_key][in_index];
}


// Set the in_index/in_key-th rotation value
//
// @param in_index  The point index
// @param in_key    The mb key
//
// @return the value
//
CRotation CIceObjectRectangle::GetRotation(const int in_index, const int in_key)
{
   return m_rotation[in_key][in_index];
}


// Create the node
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectRectangle::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   SetNodeBaseAttributes(in_id, "polymesh", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}



// Compute all the mb positions for a given point
// 
// @param in_keysTime          The mb key times
// @param in_secondsPerFrame   The seconds per frame
// @param in_velocity          The velocity of the point
// @param in_exactMb           True is exact motion blur is required (not based on velocity)
// @param in_mbPos             Array of positions at each mb key for exact mb
// @param in_mbScale           Array of scales at each mb key for exact mb
// @param in_mbOri             Array of rotationss at each mb key for exact mb
// @param in_index             The rectangle index
// @param in_pointIndex        The particle index
//
void CIceObjectRectangle::ComputeMotionBlur(CDoubleArray in_keysTime, float in_secondsPerFrame, CVector3f in_velocity, 
                                         bool in_exactMb, const vector <CVector3f> &in_mbPos, const vector <CVector3f> &in_mbScale, 
                                         const vector <CRotation> &in_mbOri, const int in_index, const int in_pointIndex)
{
   CVector3f p, vel, scale;
   CRotation orientation;
   float scaleFactor;
   int nbKeys = (int)in_keysTime.GetCount();

   if (in_exactMb)
   {
      for (int iKey = 0; iKey < nbKeys; iKey++)
      {
         int index = in_pointIndex * nbKeys + iKey;
         p = in_mbPos[index];
         SetPoint(p, in_index, iKey);
         scale = in_mbScale[index];
         SetScale(scale, in_index, iKey);
         orientation = in_mbOri[index];
         SetRotation(orientation, in_index, iKey);
      }
   }
   else
   {
      CVector3f p0 = GetPoint(in_index, 0);
      for (int iKey = 0; iKey < nbKeys; iKey++)
      {
         scaleFactor = in_secondsPerFrame * (float)in_keysTime[iKey];
         vel.Scale(scaleFactor, in_velocity);
         p.Add(p0, vel);
         SetPoint(p, in_index, iKey);
      }
   }
}


// Give the polymesh node the object's attributes.
//
// @return false if the node does not exist, else true
//
bool CIceObjectRectangle::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();

   AiNodeSetArray(m_node, "nsides", m_nsides);
   AiNodeSetArray(m_node, "vidxs", m_vidxs);
   AiNodeSetArray(m_node, "vlist", m_vlist);
   // normals not required
   // AiNodeSetArray(m_node, "nidxs", m_nidxs);
   // AiNodeSetArray(m_node, "nlist", m_nlist);
   AiNodeSetArray(m_node, "uvlist", m_uvlist);
   AiNodeSetArray(m_node, "uvidxs", m_uvidxs);
   return true;
}


// Build the quand polymesh
//
// @param in_doExactDeformMb  true if exact mb is enabled
//
bool CIceObjectRectangle::MakeQuad(bool in_doExactDeformMb)
{
   if (!m_node)
      return false;

   uint8_t nbKeys = (uint8_t)m_points.size();
   int pointsCount = (int)m_points[0].size();
   // all the polygons have 4 vertices
   m_nsides = AiArrayAllocate(pointsCount, 1, AI_TYPE_UINT);
   for (int i=0; i<pointsCount; i++)
      AiArraySetUInt(m_nsides, i, 4);
   
   // natural order for the vertex indices
   m_vidxs = AiArrayAllocate(pointsCount*4, 1, AI_TYPE_UINT);
   for (int i=0; i<pointsCount*4; i++)
      AiArraySetUInt(m_vidxs, i, i);
   // same natural order for normals
   // AtArray *m_nidxs = AiArrayCopy(vidxs);

   m_vlist = AiArrayAllocate(pointsCount*4, nbKeys, AI_TYPE_VECTOR);
   // m_nlist = AiArrayAllocate(pointsCount*4, nbKeys, AI_TYPE_VECTOR);

   CVector3Array unitQuad;
   unitQuad.Add(CVector3(-1, 0,  1));
   unitQuad.Add(CVector3( 1, 0,  1));
   unitQuad.Add(CVector3( 1, 0, -1));
   unitQuad.Add(CVector3(-1, 0, -1));

   AtVector vertex;
   // AtVector normal;

   for (int keyIndex=0; keyIndex<nbKeys; keyIndex++)
   for (int i=0; i<pointsCount; i++)
   {
      int scaleAndRotationIndex = in_doExactDeformMb ? keyIndex : 0;
      CVector3f centerf = GetPoint(i, keyIndex);
      CVector3f scalef  = GetScale(i, scaleAndRotationIndex);
      // go to double vectors
      CVector3 center((double)centerf.GetX(), (double)centerf.GetY(), (double)centerf.GetZ());
      CVector3 scale ((double)scalef.GetX(),  (double)scalef.GetY(),  (double)scalef.GetZ());
      // transform the up vector by the rotation matrix to get the quad normal
      CRotation rot = GetRotation(i, scaleAndRotationIndex);
      CMatrix3 rotM = rot.GetMatrix();
      CVector3 upVector(0.0f, 1.0f, 0.0f);
      upVector.MulByMatrix3InPlace(rotM);
      // Now the 4 points. First, reset to unit (2x2 units) quad
      CVector3Array quad(unitQuad); 
      // srt transform
      CTransformation transf;
      transf.SetScaling(scale);
      transf.SetRotation(rotM);
      transf.SetTranslation(center);
      for (int j=0; j<4; j++)
      {
         quad[j].MulByTransformationInPlace(transf);
         CUtilities().S2A(quad[j], vertex);
         CUtilities().SetArrayValue(m_vlist, vertex, i*4+j, keyIndex);
      }
   }

   // UVs (#1765)
   AtVector2 p[4];
   p[0].x = 0.0f; p[0].y = 0.0f;
   p[1].x = 1.0f; p[1].y = 0.0f;
   p[2].x = 1.0f; p[2].y = 1.0f;
   p[3].x = 0.0f; p[3].y = 1.0f;
   // just an array of 4 values
   m_uvlist = AiArrayAllocate(4, 1, AI_TYPE_VECTOR2);
   for (int i=0; i<4; i++)
      AiArraySetVec2(m_uvlist, i, p[i]);
   // 0 1 2 3 0 1 2 3 ...
   m_uvidxs = AiArrayAllocate(pointsCount*4, 1, AI_TYPE_UINT);
   for (int i=0; i<pointsCount*4; i++)
      AiArraySetUInt(m_uvidxs, i, i%4);

   return true;
}


// Attach all the required attributes to this node
//
// @param in_attributes:     The ice attributes
// @param in_frame:          The frame time
// @param in_pointIndex:     The index of the ice point
// @param in_dataArrayIndex: The index of the point.
// @param in_nbPoints:       The total number of points (not the total number of the ice points!)
//
void CIceObjectRectangle::DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex, 
                                            int in_dataArrayIndex, int in_nbPoints)
{
   AttrMap::iterator attribIt;
   // loop the required attributes, and push them as uniform (so, one attribute per point)
   for (attribIt = in_attributes->m_requiredAttributesMap.begin(); attribIt != in_attributes->m_requiredAttributesMap.end(); attribIt++)
      DeclareICEAttributeOnNode(in_pointIndex, in_dataArrayIndex, attribIt->second, in_frame, eDeclICEAttr_Uniform, in_nbPoints);
}


// Set the arnold parameters
//
// @param in_property   The Arnold Parameters custom property
// @param in_frame      The frame time
//
void CIceObjectRectangle::SetArnoldParameters(CustomProperty in_property, double in_frame)
{
   LoadArnoldParameters(m_node, in_property.GetParameters(), in_frame, true); 
}


///////////////////////////////////////////////////////////////////////
// CIceObjectPointsDisk class
///////////////////////////////////////////////////////////////////////
bool CIceObjectPointsDisk::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   if (!CIceObjectPoints::CreateNode(in_id, in_name, in_nbTransfKeys))
      return false;
   CNodeSetter::SetString(m_node, "mode", "disk");
   return true;
}


///////////////////////////////////////////////////////////////////////
// CIceObjectPointsSphere class
///////////////////////////////////////////////////////////////////////
bool CIceObjectPointsSphere::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   if (!CIceObjectPoints::CreateNode(in_id, in_name, in_nbTransfKeys))
      return false;
   CNodeSetter::SetString(m_node, "mode", "sphere");
   return true;
}


///////////////////////////////////////////////////////////////////////
// CIceObjectBaseShape class
///////////////////////////////////////////////////////////////////////

// Stuff all of the three components into a regular XSI transform
// This is done to be able to retrieve the 4x4 matrix later
//
// @param in_pos    The translation
// @param in_scale  The scale
// @param in_rot    The rotation

void CIceObjectBaseShape::SetTransf(const CVector3f in_pos, const CVector3f in_scale, const CRotation in_rot)
{
   m_transf.SetTranslationFromValues(in_pos.GetX(), in_pos.GetY(), in_pos.GetZ());
   m_transf.SetRotation(in_rot);
   m_transf.SetScalingFromValues(in_scale.GetX(), in_scale.GetY(), in_scale.GetZ());
}


// Compute the motion blur matrices.
// Since the basic shapes are unit nodex (for instance a unit cube), their actual shape and mb
// depends on the matrices
//
// @param in_keysTime         The array of mb times
// @param in_secondsPerFrame  Seconds per frame
// @param in_velocity         The linear velocity
// @param in_angVel           The angular velocity
// @param in_exactMb          True is exact motion blur is required (not based on velocity)
// @param in_mbPos            Array of positions at each mb key for exact mb
// @param in_mbScale          Array of scales at each mb key for exact mb
// @param in_mbOri            Array of orientations at each mb key for exact mb
// @param in_pointIndex       The particle index
//
void CIceObjectBaseShape::ComputeMotionBlur(CDoubleArray in_keysTime, float in_secondsPerFrame, CVector3f in_velocity, CRotation in_angVel,
                                            bool in_exactMb, const vector <CVector3f> &in_mbPos, const vector <CVector3f> &in_mbScale, 
                                            const vector <CRotation> &in_mbOri, const int in_pointIndex)
{
   CTransformation transf;
   int nbKeys = (int)in_keysTime.GetCount();
   if (in_exactMb)
   {
      for (int iKey = 0; iKey < nbKeys; iKey++)
      {
         int index = in_pointIndex * nbKeys + iKey;
         transf.SetIdentity();
         transf.SetTranslationFromValues(in_mbPos[index].GetX(), in_mbPos[index].GetY(), in_mbPos[index].GetZ());
         transf.SetRotation(in_mbOri[index]);
         transf.SetScalingFromValues(in_mbScale[index].GetX(), in_mbScale[index].GetY(), in_mbScale[index].GetZ());
         this->SetMatrix(transf, iKey);
      }
   }
   else
   {
      CVector3  in_angles = in_angVel.GetXYZAngles();
      CVector3  in_v(in_velocity.GetX(), in_velocity.GetY(), in_velocity.GetZ());
      CVector3  vel, rot;
      float     scaleFactor;
      // Get the base transformation, ie the base on which to apply the velocities.
      CTransformation transf0 = m_transf;
      CVector3  r, t;
      CVector3  t0 = transf0.GetTranslation();
      CVector3  r0 = transf0.GetRotation().GetXYZAngles();

      for (int iKey = 0; iKey < nbKeys; iKey++)
      {
         transf = transf0;
         scaleFactor = in_secondsPerFrame * (float)in_keysTime[iKey];
         vel.Scale(scaleFactor, in_v);
         t.Add(t0, vel);
         transf.SetTranslation(t);
         // angular velocity:
         rot.Scale(scaleFactor, in_angles);
         r.Add(r0, rot);
         transf.SetRotation(r);
         // done, set the iKey-th matrix
         this->SetMatrix(transf, iKey);
      }
   }
}


// Attach all the required attributes to this node (it will be a constant one)
//
// @param in_attributes:     The ice attributes
// @param in_frame:          The frame time
// @param in_pointIndex:     The index of the ice point
//
void CIceObjectBaseShape::DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex)
{
   AttrMap::iterator attribIt;
   for (attribIt = in_attributes->m_requiredAttributesMap.begin(); attribIt != in_attributes->m_requiredAttributesMap.end(); attribIt++)
      DeclareICEAttributeOnNode(in_pointIndex, in_pointIndex, attribIt->second, in_frame);
}


void CIceObjectBaseShape::SetArnoldParameters(CustomProperty in_property, double in_frame)
{
   LoadArnoldParameters(m_node, in_property.GetParameters(), in_frame, false); 
}


///////////////////////////////////////////////////////////////////////
// CIceObjectDisc class. Derives from CIceObjectBaseShape
///////////////////////////////////////////////////////////////////////

// Create the node
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectDisc::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   // was "disc" in 3.3.10
   SetNodeBaseAttributes(in_id, "disk", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}


// Set the normal
//
// @param in_value The input normal
//
void CIceObjectDisc::SetNormal(const AtVector in_value)
{
   m_normal = in_value;
}


// Give the node the object's attributes.
//
// @return false if the node does not exist, else true
//
bool CIceObjectDisc::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();
   CNodeSetter::SetFloat(m_node, "radius", m_radius);
   CNodeSetter::SetVector(m_node, "normal", m_normal.x, m_normal.y, m_normal.z);
   return true;

}


///////////////////////////////////////////////////////////////////////
// CIceObjectBox class
///////////////////////////////////////////////////////////////////////

// Create the node
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectBox::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   SetNodeBaseAttributes(in_id, "box", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}

// Set the box size
//
// @param in_min   The box min corner
// @param in_max   The box max corner
//
void CIceObjectBox::SetMinMax(const AtVector in_min, const AtVector in_max)
{
   m_min = in_min;
   m_max = in_max;
}


// Cut the y, so the box becomes a rectangle
void CIceObjectBox::SetFlat()
{
   m_min.y = -0.0001f; // not 0, for #1757
   m_max.y = -m_min.y;
}


// Give the node the object's attributes.
//
// @return false if the node does not exist, else true
//
bool CIceObjectBox::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();
   CNodeSetter::SetVector(m_node, "min", m_min.x, m_min.y, m_min.z);
   CNodeSetter::SetVector(m_node, "max", m_max.x, m_max.y, m_max.z);
   return true;

}


///////////////////////////////////////////////////////////////////////
// CIceObjectCylinder class
///////////////////////////////////////////////////////////////////////

// Create the node
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectCylinder::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   SetNodeBaseAttributes(in_id, "cylinder", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}


// Set the cylinder top and bottom
// 
// @param in_top       The top point
// @param in_bottom    The bottom point
//
void CIceObjectCylinder::SetTopBottom(const AtVector in_top, const AtVector in_bottom)
{
   m_top = in_top;
   m_bottom = in_bottom;
}


// Set the cylinder radius
// 
// @param in_radius       The radius
//
void CIceObjectCylinder::SetRadius(const float in_radius)
{
   m_radius = in_radius;
}


// Give the node the object's attributes.
bool CIceObjectCylinder::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();
   CNodeSetter::SetVector(m_node, "top", m_top.x, m_top.y, m_top.z);
   CNodeSetter::SetVector(m_node, "bottom", m_bottom.x, m_bottom.y, m_bottom.z);
   CNodeSetter::SetFloat(m_node, "radius", m_radius);
   return true;
}


///////////////////////////////////////////////////////////////////////
// CIceObjectCone class
///////////////////////////////////////////////////////////////////////

// Create the node
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectCone::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   SetNodeBaseAttributes(in_id, "cone", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}


// Set the cone top and bottom
// 
// @param in_top       The top point
// @param in_bottom    The bottom point
//
void CIceObjectCone::SetTopBottom(const AtVector in_top, const AtVector in_bottom)
{
   m_top    = in_top;
   m_bottom = in_bottom;
}


// Set the cone bottom radius
// 
// @param in_radius       The radius
//
void CIceObjectCone::SetBottomRadius(const float in_radius)
{
   m_bottom_radius = in_radius;
}


// Give the node the object's attributes.
bool CIceObjectCone::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();
   CNodeSetter::SetVector(m_node, "top", m_top.x, m_top.y, m_top.z);
   CNodeSetter::SetVector(m_node, "bottom", m_bottom.x, m_bottom.y, m_bottom.z);
   CNodeSetter::SetFloat(m_node, "bottom_radius", m_bottom_radius);
   return true;
}


///////////////////////////////////////////////////////////////////////
// Strand class
///////////////////////////////////////////////////////////////////////

// Create the node
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectStrand::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   SetNodeBaseAttributes(in_id, "curves", in_name);
   AllocMatrixArray(in_nbTransfKeys);
   return CIceObjectBase::CreateNode();
}


// Give the node the object's attributes.
bool CIceObjectStrand::SetNodeData()
{
   if (!m_node)
      return false;
   CIceObjectBase::SetNodeData();
   CNodeSetter::SetString(m_node, "basis",           "catmull-rom");
   AiNodeSetArray(m_node, "num_points",      m_num_points);
   AiNodeSetArray(m_node, "points",          m_points);
   AiNodeSetArray(m_node, "radius",          m_radius);
   CNodeSetter::SetString(m_node, "mode",            m_mode.c_str());
   CNodeSetter::SetFloat(m_node, "min_pixel_width", m_minPixelWidth);
   if (m_orientations && AiArrayGetNumElements(m_orientations))
      AiNodeSetArray(m_node, "orientations",    m_orientations);
   return true;
}

// Build all the curves stuff
//
// @param in_arnoldParameters  The arnold parameters property for hair objects
// @param in_frame             The frame
// @param in_defKeys           The deformation mb keys time
// @param in_secondsPerFrame   The seconds per frame
//
bool CIceObjectStrand::MakeCurve(CustomProperty in_arnoldParameters, double in_frame, CDoubleArray in_defKeys, float in_secondsPerFrame, bool in_exactMb)
{
   int nbKeys = (int)in_defKeys.GetCount();
   // set the mode, else the defaults stands
   if (in_arnoldParameters.IsValid())
   {
      m_mode = AtString(((CString)ParAcc_GetValue(in_arnoldParameters, L"mode", in_frame)).GetAsciiString());
      m_minPixelWidth = (float)ParAcc_GetValue(in_arnoldParameters, L"min_pixel_width", in_frame);
   }

   int nbStrands = GetNbStrands();
   // array telling how many points there are for each strand (usually the same for all the strands)
   m_num_points = AiArrayAllocate(nbStrands, 1, AI_TYPE_UINT);

   bool exportOrientation(false); // #1249
   for (int i=0; i<nbStrands; i++) // slow
   {
      AiArraySetUInt(m_num_points, i, (int)m_strands[i].m_points.size()+2); // +2 for the 2 extra points needed at root and tip
      m_nbPoints+= (int)m_strands[i].m_points.size(); // computing here the total number of points
      // test the orientation existance only on the first strand
      if (i==0 && m_mode == AtString("oriented") && (int)m_strands[0].m_orientation.size() > 0)
         exportOrientation = true;
   }

   // allocate the arrays
   m_points = AiArrayAllocate(m_nbPoints + 2*nbStrands, (uint8_t)nbKeys, AI_TYPE_VECTOR);
   m_radius = AiArrayAllocate(m_nbPoints, 1, AI_TYPE_FLOAT);
   if (exportOrientation)
      m_orientations = AiArrayAllocate(m_nbPoints + 2*nbStrands, (uint8_t)nbKeys, AI_TYPE_VECTOR);
      
   CVector3f v3, vel, vel0;
   AtVector  p0, p;
   float     r, scaleFactor;
   int       pointIndex(0), radiusIndex(0), orientationIndex(0);
   AtVector  v;
   CRotation rot;

   for (int i=0; i<nbStrands; i++)
   {
      CStrand *s = &m_strands[i];
      for (int j=0; j<(int)s->m_points.size(); j++)
      {
         s->GetPoint(&v3, j);
         CUtilities().S2A(v3, p0);

         if (nbKeys == 1)
         {
            AiArraySetVec(m_points, pointIndex, p0);
            pointIndex++;
            if (j==0 || j==(int)s->m_points.size()-1) // clone first and last points
            {
               AiArraySetVec(m_points, pointIndex, p0);
               pointIndex++;
            }
         }
         else
         {
            if (in_exactMb)
            {
               for (int iKey = 0; iKey < nbKeys; iKey++)
               {
                  // get the j-th point at the iKey-th mb key time
                  s->GetMbPoint(&v3, j, iKey);
                  CUtilities().S2A(v3, p);
                  CUtilities().SetArrayValue(m_points, p, pointIndex, iKey);
               }
               pointIndex++;
               if (j==0 || j==(int)s->m_points.size()-1) // clone first and last points
               {
                  for (int iKey = 0; iKey < nbKeys; iKey++)
                  {
                     s->GetMbPoint(&v3, j, iKey);
                     CUtilities().S2A(v3, p);
                     CUtilities().SetArrayValue(m_points, p, pointIndex, iKey);
                  }
                  pointIndex++;
               }
            }
            else
            {
               s->GetVelocity(&vel0, j);
               for (int iKey = 0; iKey < nbKeys; iKey++)
               {
                  scaleFactor = in_secondsPerFrame * (float)in_defKeys[iKey];
                  vel.Scale(scaleFactor, vel0);
                  p.x = p0.x + vel.GetX();
                  p.y = p0.y + vel.GetY();
                  p.z = p0.z + vel.GetZ();
                  CUtilities().SetArrayValue(m_points, p, pointIndex, iKey);
               }
               pointIndex++;
               if (j==0 || j==(int)s->m_points.size()-1) // clone first and last points
               {
                  for (int iKey = 0; iKey < nbKeys; iKey++)
                  {
                     scaleFactor = in_secondsPerFrame * (float)in_defKeys[iKey];
                     vel.Scale(scaleFactor, vel0);
                     p.x = p0.x + vel.GetX();
                     p.y = p0.y + vel.GetY();
                     p.z = p0.z + vel.GetZ();
                     CUtilities().SetArrayValue(m_points, p, pointIndex, iKey);
                  }
                  pointIndex++;
               }
            }
         }

         if (s->GetRadius(&r, j))
            AiArraySetFlt(m_radius, radiusIndex, r);
         radiusIndex++;

         // for orientation (#1249), it's probably not worth to support motion blur, since
         // ICE does not provide a strand orientation velocity attribute.
         // Let's just set one (duplicated) key for it
         if (exportOrientation && s->GetOrientation(&rot, j))
         {
            CMatrix3 rotM = rot.GetMatrix();
            // multiply 1,0,0 by the matrix to get the oriented vector
            CVector3 axis(1.0f, 0.0f, 0.0f);
            axis.MulByMatrix3InPlace(rotM);
            CUtilities().S2A(axis, v);

            // since orientations are expected for each key, let's just duplicate them.
            for (int iKey = 0; iKey < nbKeys; iKey++)
               CUtilities().SetArrayValue(m_orientations, v, orientationIndex, iKey);

            orientationIndex++;
            if (j==0 || j==(int)s->m_points.size()-1) // clone first and last point also for the orientation array
            {
               for (int iKey = 0; iKey < nbKeys; iKey++)
                  CUtilities().SetArrayValue(m_orientations, v, orientationIndex, iKey);
               orientationIndex++;
            }
         }
      }
   }

   return true;
}


// Attach all the required attributes for a strand
//
// @param in_attributes:     The ice attributes
// @param in_frame:          The frame time
// @param in_pointIndex:     The index of the ice point 
// @param in_dataArrayIndex: The index of the point.
// @param in_offset:         Where the strand points begin in the points array
// @param in_nbStrandPoints: Number of strand points on this strand
//
void CIceObjectStrand::DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex, int in_dataArrayIndex, int in_offset, int in_nbStrandPoints)
{
   AttrMap::iterator attribIt;
   for (attribIt = in_attributes->m_requiredAttributesMap.begin(); attribIt != in_attributes->m_requiredAttributesMap.end(); attribIt++)
      DeclareICEAttributeOnNode(in_pointIndex, in_dataArrayIndex, attribIt->second, in_frame, eDeclICEAttr_Varying, m_nbPoints, in_offset, this->GetNbStrands(), in_nbStrandPoints);
}


// Set the arnold parameters for all the nodes
//
// @param in_property   The Arnold Parameters custom property
// @param in_frame      The evaluation frame
//
void CIceObjectStrand::SetArnoldParameters(CustomProperty in_property, double in_frame)
{
   // This is the only case
   LoadArnoldParameters(m_node, in_property.GetParameters(), in_frame, true); 
}


///////////////////////////////////////////////////////////////////////
// Instance class
///////////////////////////////////////////////////////////////////////
// set the id, so the object (or model) id to be ginstanced
bool CIceObjectInstance::SetMasterId(ULONG in_masterId)
{
   m_masterId = in_masterId;
   return true;
}


// Create a container for the instances
// 
// @param in_id                The input id
// @param in_name              The input name
// @param in_nbTransfKeys      The number of mb keys
//
bool CIceObjectInstance::CreateNode(int in_id, const char* in_name, int in_nbTransfKeys)
{
   // leave the type string void, we're not creating any node out of it
   SetNodeBaseAttributes(in_id, "", in_name);
   // we just need the matrices to be propagated to the instances.
   AllocMatrixArray(in_nbTransfKeys);
   return true;
}


// Give the nodes the objects' attributes.
//
// @param in_setInheritTransform  Whether to set the inherit_xform flag off
//
bool CIceObjectInstance::SetNodeData(bool in_setInheritTransform)
{
   // members could be void, for instance if the instance object was hidden
   if (m_members.size() > 0)
   {
      // set the node data (matrix, etc.) for all the ginstances
      for (int i=0; i<(int)m_members.size(); i++)
      {
         m_members[i].SetNodeData();
         // set this flag only if the node is a valid ginstance.
         // For strand instances, no type is set, since they are clones
         if (in_setInheritTransform && (!m_members[i].m_isLight) && (!m_members[i].m_isProcedural))
            CNodeSetter::SetBoolean(m_members[i].GetNode(), "inherit_xform", false);
      }
   }
   return true;
}


// The shape instancer
//
// @param in_keyFramesTransform  The mb key times
// @param in_frame               The frame
// @param in_hasShapeTime        Is this a time shifted instance ?
// @param in_shapeHierarchyMap   The map with one entry for every Instance Shape node, and its hierarchy/object-only flag
// @param in_selectedObjs        To be passed for postloading (if any)
// @param in_selectionOnly       To be passed for postloading (if any)
// @param in_iceObjects          The global container of all the ice nodes. Used for caching data
// @param in_index               The index of the instanced point (not the point index)
// @param out_postLoadedNodes    Returned vector of nodes that must be later hidden
//
// @return false if something went wrong, else true
//
bool CIceObjectInstance::AddShapes(CDoubleArray in_keyFramesTransform, double in_frame, 
                                   bool in_hasShapeTime, ShapeHierarchyModeMap &in_shapeHierarchyMap, 
                                   CRefArray &in_selectedObjs, bool in_selectionOnly, 
                                   CIceObjects *in_iceObjects, int in_index, vector <AtNode*> &out_postLoadedNodes)
{
   InstanceLookupIt it = in_iceObjects->m_instanceMap.find(AtShaderLookupKey(m_masterId, in_frame));

   if (it != in_iceObjects->m_instanceMap.end())
   {
      // this object (or model) has already been instanced by instanceMap->find(masterId)->second !!
      // it->second is the index of the instancing point for which the object was already cloned the first time.
      CIceObjectInstance *masterInstance = &in_iceObjects->m_instances[it->second];
      // so here the term "master" refers to a previous CIceObjectInstance object, NOT to an actual master node or shape

      // matrices of the point, copy them before we overwrite *this
      AtArray* pointMatrices = AiArrayCopy(m_matrix);
      // also copy the name
      AtString name = m_name;

      // copy everything from the master shape, except the matrix, whose array is already allocated
      m_id           = masterInstance->m_id;
      m_isLight      = masterInstance->m_isLight;
      m_isProcedural = masterInstance->m_isProcedural;
      m_masterId     = masterInstance->m_masterId;
      m_members      = masterInstance->m_members;
      m_name         = masterInstance->m_name;
      m_node         = masterInstance->m_node;
      m_shader       = masterInstance->m_shader;
      m_sidedness    = masterInstance->m_sidedness;
      m_transf       = masterInstance->m_transf;
      m_type         = masterInstance->m_type;
      m_visibility   = masterInstance->m_visibility;
      
      // yet we must overwrite some stuff, so we cycle the members
      AtMatrix pointMatrix, masterPointMatrix, invMasterPointMatrix, masterShapeMatrix, pointToPointMatrix, resultMatrix;
      for (int mIndex=0; mIndex < (int)masterInstance->m_members.size(); mIndex++)
      {
         // this is the mIndex-th master ginstance to copy from
         CIceObjectBaseShape* masterShape = &masterInstance->m_members[mIndex];
         // clones for this point
         CIceObjectBaseShape* member = &m_members[mIndex];
         // The name. We must replace the first token of the master ginstance
         // We say "first", since the names are made of strings and spaces.
         // See in LoadInstance below how the names are made up
         size_t memberNameLength = member->m_name.length();
         const char *firstSpace  = member->m_name.c_str();
         for (size_t k = 0; k < memberNameLength; k++, firstSpace++)
         {
             if (*firstSpace == ' ')
                 break;
         }
         size_t instanceNameLength = memberNameLength + name.length();
         char* instanceName = (char*)alloca(instanceNameLength);
         sprintf(instanceName, "%s%s", name.c_str(), firstSpace);
         member->m_name = AtString(instanceName);

         // Create the ginstance node
         member->CreateNode();
         AtNode* gNode = member->GetNode();
         // set the "node" to the same "node" of the masterInstance
         CNodeSetter::SetPointer(gNode, "node", (AtNode*)AiNodeGetPtr(masterShape->GetNode(), "node"));

         int nbTransfKeys = AiArrayGetNumKeys(masterShape->m_matrix) < in_keyFramesTransform.GetCount() ? 
                            AiArrayGetNumKeys(masterShape->m_matrix) : in_keyFramesTransform.GetCount();
         // and we must set the matrices for this point
         for (int ikey = 0; ikey < nbTransfKeys; ikey++)
         {
            // matrix of this point
            pointMatrix = AiArrayGetMtx(pointMatrices, ikey);
            // matrix of the point we are cloning from, and its inverse
            masterPointMatrix = AiArrayGetMtx(masterInstance->m_matrix, ikey);
            invMasterPointMatrix = AiM4Invert(masterPointMatrix);
            // matrix of the shape we are cloning from
            masterShapeMatrix = AiArrayGetMtx(masterShape->m_matrix, ikey);
            // multiplying the 2, we get the matrix that takes the master point to our point
            pointToPointMatrix = AiM4Mult(invMasterPointMatrix, pointMatrix);
            // and we apply it to the matrix of the shape to be cloned
            resultMatrix = AiM4Mult(masterShapeMatrix, pointToPointMatrix);
            // this is the final matrix of the shape
            AiArraySetMtx(member->m_matrix, ikey, resultMatrix);
         }
      }
      // done, return
      AiArrayDestroy(pointMatrices);
      return true;
   }

   X3DObject obj = (X3DObject)Application().GetObjectFromID(m_masterId);
   if (!obj.IsValid())
      return false;

   Model model(obj);
   // if this is not been labelled as an object which can't be cached (because is a light or procedural, 
   // or a model with at least one light or procedural, try to insert it into the cachable map in_iceObjects->m_instanceMap.
   // If instead is later discovered to be un-cachable, we insert it into the in_iceObjects->m_uncachebleIds set (easier)
   if (in_iceObjects->m_uncachebleIds.find(m_masterId) == in_iceObjects->m_uncachebleIds.end())
   {
      // insert the current master object (or model) into the map.
      // Don't do it for lights, since lights are always duplicated and not ginstanced
      // Also, don't do it for models with lights, to fix #1256. 
      // This is a time waster for models with shapes and lights, since they won't be cached in instanceMap
      // Let's just suggest not to use mixed models, but have models with geometry AND models with lights.
      // Also, for >= 2011, don't insert procedurals with "ArnoldProcedural" in their path or data, 
      // which are also NOT to be ginstanced for #1248
      if (model.IsValid())
      {
         CRefArray modelLights = GetLightsUnderMaster(model);
         if (modelLights.GetCount() == 0 && (!CIceUtilities().ModelHasArnoldProceduralProceduralProperty(model, in_frame)))
            in_iceObjects->m_instanceMap.insert( InstanceLookupPair( AtShaderLookupKey(m_masterId, in_frame), in_index ) );
         else
            in_iceObjects->m_uncachebleIds.insert(m_masterId);
      }
      else
      {
         CString proceduralPath;
         if ( ! ( GetInstanceType(obj) == eInstanceType_Light ||
                  CIceUtilities().ObjectHasArnoldProceduralProceduralProperty(obj, in_frame, proceduralPath))
             )
            in_iceObjects->m_instanceMap.insert( InstanceLookupPair( AtShaderLookupKey(m_masterId, in_frame), in_index ) );
         else
            in_iceObjects->m_uncachebleIds.insert(m_masterId);
      }
   }


   // now go instance
   CRefArray shapeArray;
   if (!model.IsValid()) // a ref array with just the object
      shapeArray.Add(obj.GetRef());
   // else, if model is a model, shapeArray stays void. It will be filled by LoadInstance

   LoadInstance(model, obj, shapeArray, in_keyFramesTransform, in_frame, in_hasShapeTime, in_shapeHierarchyMap, in_selectedObjs, in_selectionOnly, out_postLoadedNodes);

   return true;
}


// Find the objects to be ginstanced on the point and push them into the members vector
//
// @param in_modelMaster         The master model, if the master object is a model
// @param in_objMaster           The master object. It is the X3DObject equal to in_modelMaster if in_modelMaster is a valid model
// @param in_shapeArray          The object, if the master object is NOT a model
// @param in_keyFramesTransform  The mb key times
// @param in_frame               The frame
// @param in_hasShapeTime        Is this a time shifted instance ?
// @param in_shapeHierarchyMap   The map with one entry for every Instance Shape node, and its hierarchy/object-only flag
// @param in_selectedObjs        Used in case of a PostLoadSingleObject
// @param in_selectionOnly       Used in case of a PostLoadSingleObject
// @param out_postLoadedNodes    Returned vector of nodes that must be later hidden
//
// @return true
//
bool CIceObjectInstance::LoadInstance(Model in_modelMaster, X3DObject in_objMaster, CRefArray in_shapeArray, 
                                      CDoubleArray in_keyFramesTransform, double in_frame, 
                                      bool in_hasShapeTime, ShapeHierarchyModeMap &in_shapeHierarchyMap, 
                                      CRefArray &in_selectedObjs, bool in_selectionOnly, vector <AtNode*> &out_postLoadedNodes)
{
   bool isHierarchy(false);
   Property visProperty;

   if (in_modelMaster.IsValid())
   {
      // Getting Shapes below Instance Master
      in_shapeArray = GetObjectsAndLightsUnderMaster(in_modelMaster);
      // Getting the instanced models under this instanced models (nested instances, see trac#437)
      CRefArray instancesArray = GetInstancedModelsUnderMaster(in_modelMaster);
      // merge under in_shapeArray
      in_shapeArray+= instancesArray;
   }
   else // hierarchy ?
   {
      ShapeHierarchyModeMap::iterator it = in_shapeHierarchyMap.find(in_objMaster.GetFullName());
      if (it == in_shapeHierarchyMap.end()) // no Instance Shape found, let's default to hierarchy mode for #1808
         isHierarchy = true;
      else if (it->second)
         isHierarchy = true;
      
      if (isHierarchy) // it's a hierarchy, so get the children
         in_shapeArray = GetObjectsAndLightsUnderMaster(in_objMaster);
   }

   LONG nbShapes = in_shapeArray.GetCount();

   // This map to store only once the informations of a give Softimage master object
   CMasterDataMap masterDataMap;

   // loop the objects under the model or hierarchy
   for (LONG ishape=0; ishape < nbShapes; ishape++)
   {
      X3DObject xsiObj(in_shapeArray[ishape]);
      CString baseSoftObjectName = xsiObj.GetFullName();
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

      // GetMessageQueue()->LogMsg(L"  LoadSingleInstance: shape = " + xsiObj.GetName());
      AtNode* masterNode = NULL;
      // Special Cases: 
      // hair and instances: we should find a group for them
      // lights : duplicate the light and apply the new matrix

      vector <AtNode*> nodes;
      vector <AtNode*> ::iterator nodeIter;
      bool isGroup(false); // will be set on if the master object is a group (for instance, hair chunks or if master object is another pointcloud )

      // If a single object, nodes will contain a unique element (the node itself)
      // Else, nodes contains all the nodes that originated from the object
      masterNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);
      if (masterNode)
         nodes.push_back(masterNode);
      else
      {
         vector <AtNode*> *tempV;
         if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, xsiObj, in_frame))
         {
            nodes = *tempV;
            isGroup = true;
         }
      }

      // before trying to postload, let's check if this is an "instance" of a procedural
      // marked as "ArnoldProcedural", and so never exported, since it's meant
      // to be created here, by a pointcloud, and its parameters set by ArnoldProcedural* ICE attributes
      if (nodes.empty())
      {
         CString proceduralPath;
         if (CIceUtilities().ObjectHasArnoldProceduralProceduralProperty(xsiObj, in_frame, proceduralPath))
         {
            CIceObjectBaseShape shape = LoadProcedural(xsiObj, in_frame, proceduralPath);
            m_members.push_back(shape);
            continue; // ok, done
         }
      }

      // Time shifted instances, or #1199: if the master object(s) can't be found yet, export it now
      bool postLoaded(false);
      if (nodes.empty())
      {
         if (PostLoadSingleObject(xsiObj, in_frame, in_selectedObjs, in_selectionOnly) == CStatus::OK)
         {
            postLoaded = true;
            masterNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);

            if (masterNode)
               nodes.push_back(masterNode);
            else
            {
               vector <AtNode*> *tempV;
               if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, xsiObj, in_frame))
               {
                  nodes = *tempV;
                  isGroup = true;
               }
            }
         }
      }

      // #1269: collect all the visibilities, since we may need to overwrite on the clones the one read from the masters
      // We use a plain vector, sized 1 in case the master is a simple shape.
      vector <uint8_t> visibilities;
      int vizCounter = 0;
      if (!nodes.empty())
      {
         if (isGroup)
         {
            visibilities.resize(nodes.size());
            int i=0;
            for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++,i++)
            {
               AtNode *n = *nodeIter;
               // get the viz only if available (for instance, it does not exist for lights)
               const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(n), "visibility");
               if (paramEntry)
               {
                  visibilities[i] = AiNodeGetByte(n, "visibility");
                  // If we are instancing a time-shifted shape, we must set the master invisible (#1269)
                  // So, we do this if the shapetime attribute exists, and if we postloaded the master.
                  // There is one case where the postload happens and there is no timeshift, and that is #1199,
                  // that's why we need to have the double check.
                  // However, for #1369, we can't hide the postloaded master now. If further points instance that
                  // master, it would be found with viz=0. So, we must keep the master viz as it is, and hide all
                  // the postloaded nodes way later, after finishing looping ALL the shape-instanced points
                  if (postLoaded && in_hasShapeTime)
                     out_postLoadedNodes.push_back(n);
               }
            }
         }
         else
         {
            visibilities.resize(1);
            const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(nodes[0]), "visibility");
            if (paramEntry)
            {
               visibilities[0] = AiNodeGetByte(nodes[0], "visibility");
               if (postLoaded && in_hasShapeTime)
                  out_postLoadedNodes.push_back(nodes[0]);
            }
         }
      }

      // loop over all the master nodes
      for (nodeIter=nodes.begin(); nodeIter!=nodes.end(); nodeIter++)
      {
         masterNode = *nodeIter;
         // check if this is a ginstance
         bool masterIsGInstance = CNodeUtilities().GetEntryName(masterNode) == L"ginstance";
         // get the matrices of the masterNode
         AtArray* masterMatrices = AiNodeGetArray(masterNode, "matrix");

         int nbTransfKeys = AiArrayGetNumKeys(masterMatrices) < in_keyFramesTransform.GetCount() ? 
                            AiArrayGetNumKeys(masterMatrices) : in_keyFramesTransform.GetCount();
         AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);

         for (int ikey=0; ikey<nbTransfKeys; ikey++)
         {
            AtMatrix matrixModel, matrixModelInv, matrixChild, matrixChildInv, matrixOutput, resultMatrix;
            // the condition below has been changed for #1441, adding the || isGroup
            // For objects belonging a model, it's obvious: we need to bring the objects in the master model space frame
            // before applying the current point's matrix.
            // However, there is another case, for groups:
            // Say the master object is a grid pointcloud pc1 of 4 points, with shape set to cone. 
            // Once created, they are pushed into a group (because a unique Softimage object generated 4 Arnold nodes).
            // If pc1 is instanced by another pointcloud pc2, we must treat pc1 as if it was a model, so bring the
            // 4 cones in the coordinate space of pc1 before we can apply the tranfrorm to be given to the ginstances of the cones
            if (in_modelMaster.IsValid() || isGroup || isHierarchy)
            {
               // get the matrix of the master model or object
               CUtilities().S2A(in_objMaster.GetKinematics().GetGlobal().GetTransform(in_keyFramesTransform[ikey]), matrixModel);
               // invert it
               matrixModelInv = AiM4Invert(matrixModel);
               // matrix of the master node
               matrixChild = AiArrayGetMtx(masterMatrices, ikey);
               // matrix taking the node to the master model local space
               matrixChildInv = AiM4Mult(matrixChild, matrixModelInv);
               // output:
               // matrix of the point
               matrixOutput = AiArrayGetMtx(m_matrix, ikey); 
               // take the node to the local space of the point
               resultMatrix = AiM4Mult(matrixChildInv, matrixOutput);
               // save to array. This is the final matrix for this ginstance
            }
            else
               resultMatrix = AiArrayGetMtx(m_matrix, ikey);

            AiArraySetMtx(matrices, ikey, resultMatrix);
         } 

         if (CNodeUtilities().GetEntryType(masterNode) == L"light")
         {
            CString masterNodeName = CNodeUtilities().GetName(masterNode);

            Light xsiLight = masterData->m_ref;

            if (!xsiLight.IsValid())
            {
               // #1793: Crash on instancing of a pointcloud of lights
               // In this case, we have a pointcloud A instancing a model, this model
               // made of another pc B instancing a light L. masterData then points to B, so
               // we don't have a direct handle to the light L. Let's get the Softimage light
               // using the master light node name.
               CString masterBaseNodeName = CStringUtilities().GetMasterBaseNodeName(masterNodeName);
               CString softimageLightName = CStringUtilities().GetSoftimageNameFromSItoAName(masterBaseNodeName);
               CRef baseRef;
               baseRef.Set(softimageLightName);
               xsiLight = baseRef;
            }

            CIceObjectBaseShape shape;
            CString gName((CString)m_name.c_str() + L" " + masterNodeName);
            // #1339. If this is a time shifted light instance, we don't want to duplicate a light.
            // We just get the node that was created by the postload
            if (postLoaded)
            {
               shape.m_node = masterNode;
               CNodeUtilities().SetName(shape.m_node, gName);
            }
            else
               shape.m_node = DuplicateLightNode(xsiLight, gName, in_frame);

            shape.m_isLight = true;
            // set the matrices
            shape.m_matrix = AiArrayCopy(matrices);
            m_members.push_back(shape);
         }
         else
         {
            // An object instance is going to be created, and pushed into memberVector for the group node
            CString masterNodeName = CNodeUtilities().GetName(masterNode);
            // For the " " in naming, see Instances.cpp
            CString gName((CString)m_name.c_str() + L" " + masterNodeName);

            CIceObjectBaseShape shape;
            // Same ID as its master (like XSI/mray does)
            int id = masterData->m_id;

            shape.SetNodeBaseAttributes(id, "ginstance", gName.GetAsciiString());
            // create the ginstance node
            if (shape.CreateNode())
            {
               AtNode* gNode = shape.GetNode();
               // either copy the master node over or create a new instance
               if (masterIsGInstance)
               {
                  CNodeSetter::SetPointer(gNode, "node", (AtNode*)AiNodeGetPtr(masterNode, "node"));
                  // Override the id (trac#437). For coherence, power instances inherit the id of the base object.
                  // If we comment this line, the ginstances that inherited the members from other
                  // ginstances get the instanced model id, instead of the instanced polymesh id
                  shape.m_id = AiNodeGetInt(masterNode, "id");
                  // copy the visibility
                  shape.SetVisibility(visibilities[vizCounter++]);
               }
               else
               {
                  CNodeSetter::SetPointer(gNode, "node", masterNode);

                  if (masterData->m_hideMaster) // the master was hidden, but we are not. So we need to retrieve the object visibility
                     shape.SetVisibility(masterData->m_visibility);
                  else
                     shape.SetVisibility(visibilities[vizCounter++]);
               }

               // copy the sidedness
               shape.SetSidedness(AiNodeGetByte(masterNode, "sidedness"));
               // copy the matrices
               shape.m_matrix = AiArrayCopy(matrices);
               // push the shape into the members
               m_members.push_back(shape); 
            }
            // else something went wrong creating the node
         } // master not a light
      } // master shapes loop

      nodes.clear();
   } // for ishapes

   masterDataMap.clear();

   return true;
}


CIceObjectBaseShape CIceObjectInstance::LoadProcedural(X3DObject &in_xsiObj, double in_frame, CString in_proceduralPath)
{
   CIceObjectBaseShape shape;
   shape.m_node = AiNode("procedural");
   shape.m_isProcedural = true;

   CNodeSetter::SetString(shape.m_node, "filename", in_proceduralPath.GetAsciiString());

   CNodeUtilities().SetName(shape.m_node, m_name);
   // get the matrices
   shape.m_matrix = AiArrayCopy(m_matrix);
   // copy the material from the placeholder
   Material material(in_xsiObj.GetMaterial());
   CString matName = material.GetName();

   if (UseProceduralMaterial(matName))
   {
      AtNode* shaderNode = LoadMaterial(material, LOAD_MATERIAL_SURFACE, in_frame, in_xsiObj.GetRef());
      if (shaderNode != NULL)
         AiNodeSetArray(shape.m_node, "shader", AiArray(1, 1, AI_TYPE_NODE, shaderNode));
   }

   return shape;
}


///////////////////////////////////////////////////////////////////////
// Strand Instance class. Derives from CIceObjectInstance
///////////////////////////////////////////////////////////////////////

// Get the list of objects to be cloned on the strand
//
// @param in_defKeys                      The deformation mb key times
// @param in_frame                        The frame
// @param in_iceObjects                   The global container of all the ice nodes. Used for caching data
// @param in_index                        The index of the strand (not the point index)
// @param in_secondsPerFrame              The seconds per frame
//
bool CIceObjectStrandInstance::AddStrandShapes(CDoubleArray in_defKeys , double in_frame, CIceObjects *in_iceObjects, 
                                               int in_index, float in_secondsPerFrame)
{
   CRefArray shapeArray;

   X3DObject obj = (X3DObject)Application().GetObjectFromID(m_masterId);
   if (!obj.IsValid())
      return false;

   CIceObjectStrandInstance *brother=NULL;
   CString name = obj.GetFullName();
   StrandInstanceLookupIt it = in_iceObjects->m_strandInstanceMap.find(AtNodeLookupKey(name, in_frame));

   if (it != in_iceObjects->m_strandInstanceMap.end())
      // so, a strand has already bended obj. We will reuse its data, which are expensive to compute (bounding cylinder, etc.)
      brother = &in_iceObjects->m_strandInstances[it->second];
   else // insert the current master object (or model) into the map
   {
      CString name = obj.GetFullName();
      in_iceObjects->m_strandInstanceMap.insert( StrandInstanceLookupPair( AtNodeLookupKey(name, in_frame), in_index ) );
   }

   Model model(obj);
   if (!model.IsValid()) // a ref array with just the object
      shapeArray.Add(obj.GetRef());
   // else, if model is a model, shapeArray stays void. It will be filled by LoadStrandInstance

   LoadStrandInstance(model, shapeArray, in_defKeys, in_frame, in_secondsPerFrame, brother);

   return true;
}


// Clone the objects on the strand
//
// @param in_modelMaster                  The model, if the master object is a model
// @param in_shapeArray                   The object, if the master object is NOT a model
// @param in_defKeys                      The deformation mb key times
// @param in_frame                        The frame
// @param in_secondsPerFrame              The seconds per frame
// @param in_brotherObjectStrandInstance  The first CIceObjectStrandInstance (if any) sharing the same objects to be bended
//
bool CIceObjectStrandInstance::LoadStrandInstance(Model in_modelMaster, CRefArray in_shapeArray, CDoubleArray in_defKeys, 
                                                  double in_frame, float in_secondsPerFrame, CIceObjectStrandInstance* in_brotherObjectStrandInstance)
{
   if (in_modelMaster.IsValid())
   {
      // Getting Shapes below Instance Master
      in_shapeArray = GetObjectsAndLightsUnderMaster(in_modelMaster);
      // Getting the instanced models under this instanced models (nested instances, see trac#437)
      CRefArray instancesArray = GetInstancedModelsUnderMaster(in_modelMaster);
      // merge under in_shapeArray
      in_shapeArray+= instancesArray;
   }

   int nbDefKeys = (int)in_defKeys.GetCount(); // number of deform mb keys of the pountcloud
   LONG nshapes = in_shapeArray.GetCount();

   // arrays to get/store the informations to be cloned from the masters
   AtArray *vlist = NULL;
   AtArray *nlist = NULL;
   AtArray *vidxs = NULL;
   AtArray *nidxs = NULL;

   vector <CStrandInstance> *pStrandInstances; // the bended objects

   // if we cloned the same objects already for another strand, in_brotherObjectStrandInstance points to it
   // So, in this case, we just need access to that strand's data, so, the bounding cylinder, etc.
   if (in_brotherObjectStrandInstance)
      pStrandInstances = &in_brotherObjectStrandInstance->m_strandInstances;
   else
   {
      // else, this is the first time we're bending these objects, so we'll need to cache them
      m_strandInstances.resize(nshapes);
      m_masterNodes.resize(nshapes);
      m_postLoaded.resize(nshapes);
      // and make pStrandInstances point this this->strandInstances
      pStrandInstances = &m_strandInstances;
   }

   
   int nbValidShapes=0;
   // loop for the objects under the model
   for (LONG shapeIndex=0; shapeIndex < nshapes; shapeIndex++)
   {
      X3DObject masterObj(in_shapeArray[shapeIndex]);
      CTransformation masterObjTransform, modelTransform;

      if (in_modelMaster.IsValid())
      {
         // Get the local transf (so with respect to the parent model)
         masterObjTransform = masterObj.GetKinematics().GetGlobal().GetTransform();
         // Get the group elem transf and invert it
         modelTransform = in_modelMaster.GetKinematics().GetGlobal().GetTransform();
         modelTransform.InvertInPlace();
         // And multiply
         masterObjTransform.Mul(masterObjTransform, modelTransform);
      }
      else
      {
         masterObjTransform = masterObj.GetKinematics().GetLocal().GetTransform();
         masterObjTransform.SetTranslation(CVector3(0.0, 0.0, 0.0));
         masterObjTransform.SetRotationFromXYZAngles(CVector3(0.0, 0.0, 0.0));
      }

      // Get the master node at the appropriate frame time.
      AtNode* masterNode = GetRenderInstance()->NodeMap().GetExportedNode(masterObj, in_frame);

      bool postLoaded(false);
      if ((!masterNode) && (masterObj.GetType() == siPolyMeshType)) // time shifted polymesh shape?
      {
         CRefArray dummyArray;
         if (PostLoadSingleObject(masterObj, in_frame, dummyArray, false) == CStatus::OK)
         {
            postLoaded = true;
            masterNode = GetRenderInstance()->NodeMap().GetExportedNode(masterObj, in_frame);
         }
      }

      if (masterNode && AiNodeIs(masterNode, ATSTRING::polymesh)) // #1319: we only bend polymesh nodes
      {
         // new objects (never bended before on other strands)
         // Get the shape vertices, normals, etc, and store them into this->strandInstances
         if (!in_brotherObjectStrandInstance)
         {
            // Store the master node
            m_masterNodes[nbValidShapes] = masterNode;
            m_postLoaded[nbValidShapes]  = postLoaded;
            // Get the geo stuff from the master object.
            vlist = AiNodeGetArray(masterNode, "vlist");
            nlist = AiNodeGetArray(masterNode, "nlist");
            vidxs = AiNodeGetArray(masterNode, "vidxs");
            nidxs = AiNodeGetArray(masterNode, "nidxs");
            // Create the instance object. It will make a local copy of the masterNode vertices,
            // and normals, and allocate the buffer for the bended version of the shape,
            // that will then be read when actually cloning the master AtNode
            // Do not compute the bounding cylinder now. In fact, if the group element is a model,
            // the bounding cylinder has to be computed for the whole model, not on the single objects
            // store the instance
            m_strandInstances[nbValidShapes].Init(vlist, nlist, vidxs, nidxs, masterObjTransform, masterObj);
         }
         nbValidShapes++;
      }
   }

   // we use nbValidShapes from now on (not nshapes), because some of the nshapes shapes could not
   // be found. In particular, this is the case of meshes in Soft with visibility off.
   if (nbValidShapes < 1)
      return false;

   // new objects (never bended before on other strands): compute the bounding cylinder and the cylindrical
   // coordinates of each point of the meshes
   if (!in_brotherObjectStrandInstance)
   {
      m_masterNodes.resize(nbValidShapes);
      m_strandInstances.resize(nbValidShapes);
      m_postLoaded.resize(nbValidShapes);

      // We must now compute the cylindrical coordinates for the instances.
      // ComputeModelBoundingCylinder will loop all the members,
      // and store the bounding cylinder (so of the WHOLE model) into the bounding cylinder
      // structure of strandInstances[0]
      // This is expensive
      for (int shapeIndex=0; shapeIndex<nbValidShapes; shapeIndex++)
      {
         if (shapeIndex == 0) // so we do it only once for the whole model.
            m_strandInstances[0].ComputeModelBoundingCylinder(m_strandInstances);
         // for all the further shapes of the i-th model, copy the bounding cylinder information
         // from strandInstances[0]. In other words, all the shapes under the same model share
         // the same bounding cylinder.
         else
            m_strandInstances[shapeIndex].m_boundingCylinder.CopyBoundaries(m_strandInstances[0].m_boundingCylinder);
         // Using the above bounding cylinder, remap the points to cylindrical coordinates
         m_strandInstances[shapeIndex].RemapPointsToCylinder();
      }
   }

   // Loop all the shapes that we need to clone on this strand. We can safely use pStrandInstances
   // in either case (new objects or already cached one)
   for (unsigned int i=0; i<pStrandInstances->size(); i++)
   {
      CStrandInstance* strandInstance;
      AtNode* masterNode;
      bool postLoaded(false);

      // depending on the case, point to the strand instances and to the master nodes
      if (in_brotherObjectStrandInstance)
      {
         strandInstance = &in_brotherObjectStrandInstance->m_strandInstances[i];
         masterNode = in_brotherObjectStrandInstance->m_masterNodes[i];
         // don't turn postLoaded on, because the brother object must be cloned
      }
      else
      {
         // Get the instancing object, and its corresponding master AtNode*
         strandInstance = &m_strandInstances[i];
         masterNode = m_masterNodes[i];
         postLoaded = m_postLoaded[i];
      }

      CIceObjectBaseShape shape;
      // if the master was just post loaded because of time shift, use the object itself.
      // else, clone the master
      shape.m_node = postLoaded ? masterNode : AiNodeClone(masterNode);

      CString shapeName = CString(m_name.c_str()) + L" " + CNodeUtilities().GetName(masterNode);
      CNodeUtilities().SetName(shape.m_node, shapeName);

      // Allocate as many vertices and normals as needed.
      // We need to allocate, instead of re-using the master vectors, since the number
      // of mb keys could differ from the master ones.
      vlist = AiArrayAllocate((int)strandInstance->m_points.size(), (uint8_t)nbDefKeys, AI_TYPE_VECTOR);
      nlist = AiArrayAllocate((int)strandInstance->m_normals.size(), (uint8_t)nbDefKeys, AI_TYPE_VECTOR);

      if (nbDefKeys == 1)
      {
         // Bend the instanced objects along the strand
         strandInstance->BendOnStrand(m_strand);
         // Assign the bended points/normals to the iDefKey-th array of vlist and nlist
         strandInstance->Get(vlist, nlist, 0);
      }
      else // we have def mb, and so we must compute the actual position of the strand, before bending on it
      {
         CVector3f p, p0, vel, vel0;
         float     scaleFactor;

         for (int iDefKey=0; iDefKey<nbDefKeys; iDefKey++)
         {
            // compute the time shifted strand (mbStrand) given the base strand and the velocity
            scaleFactor = in_secondsPerFrame * (float)in_defKeys[iDefKey];
            for (int pIndex=0; pIndex<(int)m_strand.m_points.size(); pIndex++)
            {
               m_strand.GetPoint(&p0, pIndex);
               m_strand.GetVelocity(&vel0, pIndex);
               vel.Scale(scaleFactor, vel0);
               p.Set(p0.GetX() + vel.GetX(), p0.GetY() + vel.GetY(), p0.GetZ() + vel.GetZ());
               m_mbStrand.SetPoint(p, pIndex);
            }
            // copy also the orientations and the radii
            for (int pIndex=0; pIndex<(int)m_strand.m_orientation.size(); pIndex++)
               m_mbStrand.m_orientation[pIndex] = m_strand.m_orientation[pIndex];
            for (int pIndex=0; pIndex<(int)m_strand.m_radii.size(); pIndex++)
               m_mbStrand.m_radii[pIndex] = m_strand.m_radii[pIndex];

            // recompute length and main axis of the motion displaced strand
            m_mbStrand.ComputeLength();
            m_mbStrand.ComputeBendedX(false, 0.0f);
            // Bend the instanced objects along the motion displaced strand
            strandInstance->BendOnStrand(m_mbStrand);
            // Assign the bended points/normals to the iDefKey-th array of vlist and nlist
            strandInstance->Get(vlist, nlist, iDefKey);
         }
      }

      // Give the arrays to the cloned node
      AiNodeSetArray(shape.m_node, "vlist", vlist);
      AiNodeSetArray(shape.m_node, "nlist", nlist);

      // get the matrices of the masterNode, just to get their count
      AtArray* masterNodeMatrices = AiNodeGetArray(masterNode, "matrix");
      // allocs the same number matrices for the cloned shape, and set them to identity.
      // The bended shapes are written in global space, yet we need the matrices to be allocated, 
      // since they will be further multiplied by the pointcloud 
      // matrices in CIceObjects::MultiplyInstancesByPointCloudMatrices
      shape.AllocMatrixArray(AiArrayGetNumKeys(masterNodeMatrices));
      // push the shape into the members. This allows to have the cloned objects as part of the members of
      // the exported groupnode, and so to have instances of instanced strands
      m_members.push_back(shape); 
   }

   return true;
}


///////////////////////////////////////////////////////////////////////
// CIceObjects class, the home of all the objects built for the ice tree
///////////////////////////////////////////////////////////////////////

// Set the visibility for all the objects
//
// @param in_viz              The visibility
// @param in_arnoldVizExists  True if the viz value comes from an arnold_viz property
//
void CIceObjects::SetNodesVisibility(uint8_t in_viz, bool in_arnoldVizExists)
{
   int i;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetVisibility(in_viz);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetVisibility(in_viz);

   // if the viz is not an arnold one (but an xsi one), do not set the visibility
   // of the instances, since they will inherit the masters' one
   if (!in_arnoldVizExists)
      return;

   for (i=0; i < (int)m_instances.size(); i++)
   {
      // set the viz for the ginstances
      for (int j=0; j<(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetVisibility(in_viz);
      m_instances[i].SetVisibility(in_viz);
   }

   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      // set the viz for the strand instances
      for (int j=0; j<(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetVisibility(in_viz);
      m_strandInstances[i].SetVisibility(in_viz);
   }
}

// Set the sidedness for all the objects
//
// @param in_sid       The sidedness
//
void CIceObjects::SetNodesSidedness(uint8_t in_sid)
{
   int i;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetSidedness(in_sid);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetSidedness(in_sid);

   for (i=0; i < (int)m_instances.size(); i++)
   {
      // set the sid for the ginstances
      for (int j=0; j<(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetSidedness(in_sid);
      m_instances[i].SetSidedness(in_sid);
   }
   
   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      // set the sid for the strand instances
      for (int j=0; j<(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetSidedness(in_sid);
      m_strandInstances[i].SetSidedness(in_sid);
   }
}

// Set the shaders for all the objects
//
// @param in_sid       The sidedness
//
void CIceObjects::SetNodesShader(AtNode *in_shader)
{
   int i;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetShader(in_shader);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetShader(in_shader);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetShader(in_shader);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetShader(in_shader);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetShader(in_shader);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetShader(in_shader);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetShader(in_shader);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetShader(in_shader);

   // for instances and strandInstances, the shaders are taken from the masters
}


// set the attributes for all the nodes
void CIceObjects::SetNodesData()
{
   int i;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetNodeData();
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetNodeData();
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetNodeData();
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetNodeData();
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetNodeData();
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetNodeData();
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetNodeData();
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetNodeData();
   for (i=0; i < (int)m_instances.size(); i++) //set the data for the ginstances
      m_instances[i].SetNodeData(true);
   for (i=0; i < (int)m_strandInstances.size(); i++)
      m_strandInstances[i].SetNodeData(false);
}


// Set the arnold parameters for all the nodes
//
// @param in_property   The Arnold Parameters custom property
// @param in_frame      The evaluation frame
//
void CIceObjects::SetArnoldParameters(CustomProperty in_property, double in_frame)
{
   if (!in_property.IsValid())
      return;

   int i, j;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetArnoldParameters(in_property, in_frame);
   for (i=0; i < (int)m_instances.size(); i++)
   {
      for (j=0; j <(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetArnoldParameters(in_property, in_frame);
   }
   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetArnoldParameters(in_property, in_frame);
   }
}


// Set the motion_start, end for all the nodes
//
void CIceObjects::SetMotionStartEnd()
{
   int i, j;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetMotionStartEnd();
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetMotionStartEnd();
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetMotionStartEnd();
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetMotionStartEnd();
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetMotionStartEnd();
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetMotionStartEnd();
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetMotionStartEnd();
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetMotionStartEnd();
   for (i=0; i < (int)m_instances.size(); i++)
   {
      for (j=0; j <(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetMotionStartEnd();
   }
   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetMotionStartEnd();
   }
}


// Set the arnold user options for all the nodes (#680)
//
// @param in_property        The Arnold User Options custom property
// @param in_frame           The frame time
//
void CIceObjects::SetArnoldUserOptions(CustomProperty in_property,  double in_frame)
{
   if (!in_property.IsValid())
      return;

   int i, j;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetArnoldUserOptions(in_property, in_frame);
   for (i=0; i < (int)m_instances.size(); i++)
   {
      for (j=0; j <(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetArnoldUserOptions(in_property, in_frame);
   }
   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetArnoldUserOptions(in_property, in_frame);
   }
}


// Set the user data blobs for all the nodes (#728)
//
// @param in_xsiObj          The ICE object
// @param in_frame           The frame time
//
void CIceObjects::SetUserDataBlobs(const X3DObject &in_xsiObj, double in_frame)
{
   CRefArray blobProperties = CollectUserDataBlobProperties(in_xsiObj, in_frame);
   if (blobProperties.GetCount() > 0)
   {
      int i, j;
      for (i=0; i < (int)m_pointsSphere.size(); i++)
         m_pointsSphere[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_pointsDisk.size(); i++)
         m_pointsDisk[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_rectangles.size(); i++)
         m_rectangles[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_discs.size(); i++)
         m_discs[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_boxes.size(); i++)
         m_boxes[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_cylinders.size(); i++)
         m_cylinders[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_cones.size(); i++)
         m_cones[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_strands.size(); i++)
         m_strands[i].SetUserDataBlobs(blobProperties, in_frame);
      for (i=0; i < (int)m_instances.size(); i++)
      {
         for (j=0; j <(int)m_instances[i].m_members.size(); j++)
            m_instances[i].m_members[j].SetUserDataBlobs(blobProperties, in_frame);
      }
      for (i=0; i < (int)m_strandInstances.size(); i++)
      {
         for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
            m_strandInstances[i].m_members[j].SetUserDataBlobs(blobProperties, in_frame);
      }
   }
}


// Set the matte data for all the nodes
//
// @param in_property        The matte property
// @param in_frame           The frame time
//
void CIceObjects::SetMatte(Property in_property, double in_frame)
{
   int i, j;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetMatte(in_property, in_frame);
   for (i=0; i < (int)m_instances.size(); i++)
   {
      for (j=0; j <(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetMatte(in_property, in_frame);
   }
   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetMatte(in_property, in_frame);
   }
}

// get all the nodes into a vector
//
// @return the vector ov modes
//
vector <AtNode*> CIceObjects::GetAllNodes()
{
   int i, j;
   vector <AtNode*> result;
   int size = (int)m_pointsSphere.size() + (int)m_pointsDisk.size() + (int)m_rectangles.size() + (int)m_discs.size() + 
              (int)m_boxes.size() + (int)m_cylinders.size() + (int)m_cones.size() + (int)m_strands.size(); 
   // for instances, we take the members sizes
   for (i=0; i < (int)m_instances.size(); i++)
      size+= (int)m_instances[i].m_members.size();

   for (i=0; i < (int)m_strandInstances.size(); i++)
      size+= (int)m_strandInstances[i].m_members.size();

   result.resize(size);

   int index=0;

   for (i=0; i < (int)m_pointsSphere.size(); i++)
   {
      result[index] = m_pointsSphere[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_pointsDisk.size(); i++)
   {
      result[index] = m_pointsDisk[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_rectangles.size(); i++)
   {
      result[index] = m_rectangles[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_discs.size(); i++)
   {
      result[index] = m_discs[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_boxes.size(); i++)
   {
      result[index] = m_boxes[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_cylinders.size(); i++)
   {
      result[index] = m_cylinders[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_cones.size(); i++)
   {
      result[index] = m_cones[i].GetNode();
      index++;
   }
   for (i=0; i < (int)m_strands.size(); i++)
   {
      result[index] = m_strands[i].GetNode();
      index++;
   }

   for (i=0; i < (int)m_instances.size(); i++)
   for (j=0; j <(int)m_instances[i].m_members.size(); j++)
   {
      result[index] = m_instances[i].m_members[j].GetNode();
      index++;
   }

   for (i=0; i < (int)m_strandInstances.size(); i++)
   for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
   {
      result[index] = m_strandInstances[i].m_members[j].GetNode();
      index++;
   }

   return result;
}


// Multiply all the matrices by the pointcloud matrices
//
// @param in_pointCloudMatrices   The pointcloud matrices
//
void CIceObjects::MultiplyInstancesByPointCloudMatrices(AtArray* in_pointCloudMatrices)
{
   AtMatrix matrix, pM, sM;
   CIceObjectBaseShape *shape;

   for (int i=0; i<(int)m_discs.size(); i++)
   {
      shape = &m_discs[i];
      for (int iKey=0; iKey<AiArrayGetNumKeys(in_pointCloudMatrices); iKey++)
      {
         sM = AiArrayGetMtx(shape->m_matrix, iKey);
         pM = AiArrayGetMtx(in_pointCloudMatrices, iKey);
         matrix = AiM4Mult(sM, pM);
         AiArraySetMtx(shape->m_matrix, iKey, matrix);
      }
   }

   for (int i=0; i<(int)m_boxes.size(); i++)
   {
      shape = &m_boxes[i];
      for (int iKey=0; iKey<AiArrayGetNumKeys(in_pointCloudMatrices); iKey++)
      {
         sM = AiArrayGetMtx(shape->m_matrix, iKey);
         pM = AiArrayGetMtx(in_pointCloudMatrices, iKey);
         matrix = AiM4Mult(sM, pM);
         AiArraySetMtx(shape->m_matrix, iKey, matrix);
      }
   }

   for (int i=0; i<(int)m_cylinders.size(); i++)
   {
      shape = &m_cylinders[i];
      for (int iKey=0; iKey<AiArrayGetNumKeys(in_pointCloudMatrices); iKey++)
      {
         sM = AiArrayGetMtx(shape->m_matrix, iKey);
         pM = AiArrayGetMtx(in_pointCloudMatrices, iKey);
         matrix = AiM4Mult(sM, pM);
         AiArraySetMtx(shape->m_matrix, iKey, matrix);
      }
   }

   for (int i=0; i<(int)m_cones.size(); i++)
   {
      shape = &m_cones[i];
      for (int iKey=0; iKey<AiArrayGetNumKeys(in_pointCloudMatrices); iKey++)
      {
         sM = AiArrayGetMtx(shape->m_matrix, iKey);
         pM = AiArrayGetMtx(in_pointCloudMatrices, iKey);
         matrix = AiM4Mult(sM, pM);
         AiArraySetMtx(shape->m_matrix, iKey, matrix);
      }
   }

   for (int instanceIndex=0; instanceIndex<(int)m_instances.size(); instanceIndex++)
   {
      for (int i=0; i<(int)m_instances[instanceIndex].m_members.size(); i++)
      {
         shape = &m_instances[instanceIndex].m_members[i];
         for (int iKey=0; iKey<AiArrayGetNumKeys(in_pointCloudMatrices); iKey++)
         {
            sM = AiArrayGetMtx(shape->m_matrix, iKey);
            pM = AiArrayGetMtx(in_pointCloudMatrices, iKey);
            matrix = AiM4Mult(sM, pM);
            AiArraySetMtx(shape->m_matrix, iKey, matrix);
         }
      }
   }

   for (int strandInstanceIndex=0; strandInstanceIndex<(int)m_strandInstances.size(); strandInstanceIndex++)
   {
      for (int i=0; i<(int)m_strandInstances[strandInstanceIndex].m_members.size(); i++)
      {
         shape = &m_strandInstances[strandInstanceIndex].m_members[i];
         for (int iKey=0; iKey<AiArrayGetNumKeys(in_pointCloudMatrices); iKey++)
         {
            sM = AiArrayGetMtx(shape->m_matrix, iKey);
            pM = AiArrayGetMtx(in_pointCloudMatrices, iKey);
            matrix = AiM4Mult(sM, pM);
            AiArraySetMtx(shape->m_matrix, iKey, matrix);
         }
      }
   }
}


// Assign the light group to all the nodes
//
// @param in_lightGroup:  The light group array
//
void CIceObjects::SetLightGroup(AtArray* in_lightGroup)
{
   int i, j;
   for (i=0; i < (int)m_pointsSphere.size(); i++)
      m_pointsSphere[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_pointsDisk.size(); i++)
      m_pointsDisk[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_rectangles.size(); i++)
      m_rectangles[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_discs.size(); i++)
      m_discs[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_boxes.size(); i++)
      m_boxes[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_cylinders.size(); i++)
      m_cylinders[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_cones.size(); i++)
      m_cones[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_strands.size(); i++)
      m_strands[i].SetLightGroup(in_lightGroup);
   for (i=0; i < (int)m_instances.size(); i++)
   {
      for (j=0; j <(int)m_instances[i].m_members.size(); j++)
         m_instances[i].m_members[j].SetLightGroup(in_lightGroup);
   }
   for (i=0; i < (int)m_strandInstances.size(); i++)
   {
      for (j=0; j <(int)m_strandInstances[i].m_members.size(); j++)
         m_strandInstances[i].m_members[j].SetLightGroup(in_lightGroup);
   }
}


// Return true if at least one of the instanced objects is a light
//
bool CIceObjects::HasAtLeastOneInstancedLight()
{
   for (int i=0; i < (int)m_instances.size(); i++)
   {
      if (m_instances[i].m_isLight)
         return true;
   }

   return false;
}


// Log the number of objects for each type
void CIceObjects::Log()
{
   GetMessageQueue()->LogMsg(L"nbPointsSphere     = " + (CString)m_pointsSphereNbPoints);
   GetMessageQueue()->LogMsg(L"nbPointsDisk       = " + (CString)m_pointsDiskNbPoints);
   GetMessageQueue()->LogMsg(L"nbDiscs            = " + (CString)m_nbDiscs);
   GetMessageQueue()->LogMsg(L"nbBoxes            = " + (CString)m_nbBoxes);
   GetMessageQueue()->LogMsg(L"nbCylinders        = " + (CString)m_nbCylinders);
   GetMessageQueue()->LogMsg(L"nbCones            = " + (CString)m_nbCones);
   GetMessageQueue()->LogMsg(L"nbStrands          = " + (CString)m_nbStrands);
   GetMessageQueue()->LogMsg(L"nbInstances        = " + (CString)m_nbInstances);
   GetMessageQueue()->LogMsg(L"nbStrandInstances  = " + (CString)m_nbStrandInstances);
}

