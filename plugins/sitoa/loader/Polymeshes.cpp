/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "loader/Loader.h"
#include "loader/Polymeshes.h"
#include "loader/Properties.h"
#include "loader/Shaders.h"
#include "loader/Procedurals.h"
#include "loader/Volume.h"
#include "renderer/Renderer.h"
#include "renderer/RendererOptions.h"

#include <xsi_application.h>
#include <xsi_boolarray.h>
#include <xsi_edge.h>
#include <xsi_facet.h>
#include <xsi_kinematics.h>
#include <xsi_library.h>
#include <xsi_model.h>
#include <xsi_point.h>
#include <xsi_primitive.h>
#include <xsi_project.h>
#include <xsi_sample.h>
#include <xsi_texture.h>
#include <xsi_version.h>
#include <xsi_vertex.h>

#include <algorithm>

inline int compareFloatN(const void *ptr1, const void *ptr2, int float_size)
{
    int v = 0;
    int *p1 = (int*)ptr1;
    int *p2 = (int*)ptr2;

    while (float_size-- > 0 && v == 0)
        v = *(p1++) - *(p2++);

    return v;
}

inline bool equalFloat1(const void *ptr1, const void *ptr2, int /*float_size*/)
{
    return *(float*)ptr1 == *(float*)ptr2;
}

inline bool equalFloat2(const void *ptr1, const void *ptr2, int /*float_size*/)
{
    return *(uint64_t*)ptr1 == *(uint64_t*)ptr2;
}

inline bool equalFloat3(const void *ptr1, const void *ptr2, int /*float_size*/)
{
    // compare 64 + 32 bits
    return *(uint64_t*)ptr1 == *(uint64_t*)ptr2 && *((float*)ptr1 + 2) == *((float*)ptr2 + 2);
}

inline bool equalFloat4(const void *ptr1, const void *ptr2, int /*float_size*/)
{
    // compare 64 + 64 bits
    const uint64_t *di = (uint64_t*)ptr1;
    const uint64_t *dj = (uint64_t*)ptr2;
    return *di == *dj && *(di + 1) == *(dj + 1);
}

inline bool equalFloatN(const void *ptr1, const void *ptr2, int float_size)
{
    return compareFloatN(ptr1, ptr2, float_size) == 0;
}

/// IndexValue comparison functors for std::sort()
struct IndexValueLessThanFloat1
{
    bool operator()(const IndexValue& i, const IndexValue& j)
    {
        return i.vidx == j.vidx ? *(float*)i.value < *(float*)j.value : i.vidx < j.vidx;
    }
};

struct IndexValueLessThanFloat2
{
    bool operator()(const IndexValue& i, const IndexValue& j)
    {
        // compare 64 bits
        return i.vidx == j.vidx ? *(uint64_t*)i.value < *(uint64_t*)j.value : i.vidx < j.vidx;
    }
};

struct IndexValueLessThanFloat3
{
    bool operator()(const IndexValue& i, const IndexValue& j)
    {
        if (i.vidx == j.vidx)
        {
            // compare 64 + 32 bits
            uint64_t *di = (uint64_t*)i.value;
            uint64_t *dj = (uint64_t*)j.value;
            return *di == *dj ? *((float*)i.value + 2) < *((float*)j.value + 2) : *di < *dj;
        }
        return i.vidx < j.vidx;
    }
};

struct IndexValueLessThanFloat4
{
    bool operator()(const IndexValue& i, const IndexValue& j)
    {
        if (i.vidx == j.vidx)
        {
            // compare 64 + 64 bits
            uint64_t *di = (uint64_t*)i.value;
            uint64_t *dj = (uint64_t*)j.value;
            return *di == *dj ? *(di + 1) < *(dj + 1) : *di < *dj;
        }
        return i.vidx < j.vidx;
    }
};

struct IndexValueLessThanFloatN
{
    IndexValueLessThanFloatN(int float_size) : float_size(float_size) {}

    bool operator()(const IndexValue& i, const IndexValue& j)
    {
        return i.vidx == j.vidx ? compareFloatN(i.value, j.value, float_size) < 0 : i.vidx < j.vidx;
    }

    int float_size; ///< size in 32bit floats
};


//////////////////////////////////////////////////
//////////////////////////////////////////////////
// CMesh class
//////////////////////////////////////////////////
//////////////////////////////////////////////////

// Create the polymesh node and set the base class members
//
// @param in_xsiObj         The Softimage object
// @param in_frame          The frame time
//
// @return true is creation went ok, else false
//
bool CMesh::Create(const X3DObject &in_xsiObj, double in_frame)
{
   m_xsiObj = in_xsiObj;
   m_properties = m_xsiObj.GetProperties();
   
   m_geoProperty = m_properties.GetItem(L"Geometry Approximation");
   m_properties.Find(L"arnold_parameters", m_paramProperty);

   m_useDiscontinuity   = (bool)ParAcc_GetValue(m_geoProperty, L"gapproxmoad", in_frame);
   m_discontinuityAngle = (double)ParAcc_GetValue(m_geoProperty, L"gapproxmoan", in_frame);
   m_subdivIterations = (LONG)ParAcc_GetValue(m_geoProperty, L"gapproxmordrsl", in_frame);

   // Get Motion Blur Data
   CSceneUtilities::GetMotionBlurData(m_xsiObj.GetRef(), m_transfKeys, m_defKeys, in_frame);
   m_nbTransfKeys = m_transfKeys.GetCount();
   m_nbDefKeys    = m_defKeys.GetCount();

   // to get the geo info like the number of points, evaluate the primitive at the first key time
   // that will also be used during the deformation mb loop
   CLongArray keysPosition;
   CDoubleArray defKeys = CSceneUtilities::OptimizeMbKeysOrder(m_defKeys, keysPosition, in_frame);
   double time0 = defKeys[0];

   m_primitive = CObjectUtilities().GetPrimitiveAtFrame(m_xsiObj, time0);
   m_polyMesh = CObjectUtilities().GetGeometryAtFrame(m_xsiObj, siConstructionModeSecondaryShape, time0);
   m_geoAccessor = m_polyMesh.GetGeometryAccessor(siConstructionModeSecondaryShape, siCatmullClark, 0, false, 
                                                  m_useDiscontinuity, m_discontinuityAngle);
   m_nbVertices = m_geoAccessor.GetVertexCount(); 
   m_nbPolygons = m_geoAccessor.GetPolygonCount();
   if (m_nbPolygons == 0)
      return false;

   m_node = AiNode("polymesh");
   CString name = CStringUtilities().MakeSItoAName(m_xsiObj, in_frame, L"", false);
   CNodeUtilities().SetName(m_node, name);
   CNodeSetter::SetInt(m_node, "id", CObjectUtilities().GetId(m_xsiObj));
      
   GetRenderInstance()->NodeMap().PushExportedNode(m_xsiObj, in_frame, m_node);

   CheckIceTree();
   CheckIceNodeUserNormal();

   return true;
}


// Check if the mesh has an ICE tree, and set m_hasIceTree accordingly
//
void CMesh::CheckIceTree()
{
   CRefArray nestedObjects = m_primitive.GetNestedObjects();
   for (LONG i=0; i<nestedObjects.GetCount(); i++)
   {
      if (nestedObjects[i].GetClassID() == siICETreeID)
      {
         m_hasIceTree = true;
         return;
      }
   }
}


// Check if the mesh has an ICE nodeusernormal attribute, and set m_hasIceNodeUserNormal accordingly
//
void CMesh::CheckIceNodeUserNormal()
{
   if (m_hasIceTree)
   {
      CIceAttribute nodeUserNormalAttr(m_polyMesh.GetICEAttributeFromName(L"nodeusernormal"));
      if (nodeUserNormalAttr.Update())
          m_hasIceNodeUserNormal = nodeUserNormalAttr.m_v3Data.GetCount() > 0;
   }
}


// Check if the input uvs are homogenous, ie their w is the weight. For now, this only happens
// in the case the projection was a camera projection.
//
// @param in_uvProperty   the UV cluster property
// @param in_uvValues     the raw UV values, pulled by Property::GetElements::GetArray
//
// @return true if the set is homogenous, else false
//
bool CMesh::AreUVsHomogenous(ClusterProperty &in_uvProperty, CDoubleArray &in_uvValues)
{
   LONG count = in_uvValues.GetCount();

   // first, let's check if at least one w is != 1
   bool haveW(false);
   for (LONG wIndex=2; wIndex<count; wIndex+=3)
   {
      if (in_uvValues[wIndex] != 1.0)
      {
         haveW = true;
         break;
      }
   }

   if (!haveW) // all w are 1, so return false
      return false;

   // this is not enough, we still have to be sure that w!=1 are weights, and not the z component
   // of a standard 3d projection, like "spatial". Let's confront the input values with the normalized
   // values that we get from in_uvProperty.GetValues.
   // Note that I was not able to find another way to understand if a UV set is homogenous, in particular
   // after the texture projection operator has been frozen.
   CFloatArray uvValues;
   in_uvProperty.GetValues(uvValues);
   for (LONG wIndex=2; wIndex<count; wIndex+=3)
   {
      if (fabs((float)in_uvValues[wIndex] - uvValues[wIndex]) > 0.001f)
         return true;
   }

   return false;
}


// Export nsides, the number of nodes per polygon
//
void CMesh::ExportPolygonVerticesCount()
{
   CLongArray polygonVerticesCountArray;
   m_geoAccessor.GetPolygonVerticesCount(polygonVerticesCountArray);

   AtArray *nsides = AiArrayAllocate(m_nbPolygons, 1, AI_TYPE_UINT);
   for (LONG i = 0; i < m_nbPolygons; i++)
      AiArraySetUInt(nsides, i, polygonVerticesCountArray[i]);      

   AiNodeSetArray(m_node, "nsides", nsides);
}


// Export vidxs, the vertices indeces. The number of vidxs equals the sum of the nsides array
//
void CMesh::ExportVertexIndices()
{
   CLongArray vertexIndices;
   m_geoAccessor.GetVertexIndices(vertexIndices);
   m_nbVertexIndices = vertexIndices.GetCount();

   AtArray *vidxs = LongArrayToUIntArray(vertexIndices);
   AiNodeSetArray(m_node, "vidxs", vidxs);
}


// Export the vertices in case they have to be mblurred by the PointVelocity attribute
//
// @param io_vlist          The vlist array to be filled
//
// @return true if the PointVelocity was found and the points set, else false
//
bool CMesh::ExportIceVertices(AtArray *io_vlist)
{
   CIceAttribute pointVelocityAttr(m_polyMesh.GetICEAttributeFromName(L"pointvelocity"));
   if (!pointVelocityAttr.Update())
      return false;

   AtVector point;
   CDoubleArray pointsArray;
   // then, we go with linear mb. Let's steal the time delta and the way to compute
   // the motion vectors from ICE.cpp
   float frameRate = (float)CTimeUtilities().GetFps();
   float secondsPerFrame = frameRate > 1.0 ? 1.0f / frameRate : 1.0f;

   // get the mb times at time 0
   CDoubleArray transfKeysAtTimeZero, defKeysAtTimeZero;
   CSceneUtilities::GetMotionBlurData(m_xsiObj.GetRef(), transfKeysAtTimeZero, defKeysAtTimeZero, 0, true);

   m_geoAccessor.GetVertexPositions(pointsArray);
   CVector3f p, vel;

   for (LONG key=0; key<m_nbDefKeys; key++)
   {
      float scaleFactor = secondsPerFrame * (float)defKeysAtTimeZero[key];
      for (LONG i=0; i < m_nbVertices; i++)
      {
         // point at in_frame
         p.Set((float)pointsArray[i*3], (float)pointsArray[i*3+1], (float)pointsArray[i*3+2]);
         vel = pointVelocityAttr.m_isConstant ? pointVelocityAttr.m_v3Data[0] : pointVelocityAttr.m_v3Data[i];
         vel.ScaleInPlace(scaleFactor);
         p.AddInPlace(vel);

         CUtilities().S2A(p, point);
         CUtilities().SetArrayValue(io_vlist, point, i, key); 
      }
   }
   return true;
}


// Resize the vlist and nlist arrays to just contain one key.
// Ticket #1718: Protect motion-blurred polymeshes against changing topology
// Called if the topology of the polymesh changes in the shutter interval.
// It must be called in the deformation blur loop, for key > 0
//
// @param in_vlist              the multi-key array of vertices
// @param in_nlist              the multi-key array of normals
// @param in_exportNormals      true if normals must be exported
// @param in_firstKeyPosition   the key for which the array was fisrt written into during the deformation blur loop
//
// @return false if the input arrays are null or have just one key, else true
//
bool CMesh::RemoveMotionBlur(AtArray *in_vlist, AtArray *in_nlist, bool in_exportNormals, LONG in_firstKeyPosition)
{
   // if well called (key > 1), these checks should all be passed
   if (!in_vlist)
      return false;
   if (AiArrayGetNumKeys(in_vlist) < 2)
      return false;

   if (in_exportNormals)
   {
      if (!in_nlist)
         return false;
      if (AiArrayGetNumKeys(in_nlist) < 2)
         return false;
   }

   AtVector  p;
   // allocate a new vlist array with a single key
   AtArray *vlist = AiArrayAllocate(AiArrayGetNumElements(in_vlist), 1, AI_TYPE_VECTOR);
   // copy the first key of the input array into the new array
   for (unsigned int i=0; i<AiArrayGetNumElements(in_vlist); i++)
   {
      CUtilities().GetArrayValue(in_vlist, &p, i, in_firstKeyPosition);
      CUtilities().SetArrayValue(vlist, p, i, 0);
   }
   // we can destroy the old array, so the calling function, if still referencing it, MUST return
   AiArrayDestroy(in_vlist);
   // assign the new array to the mesh
   AiNodeSetArray(m_node, "vlist", vlist);

   // the model is broken, don't export the normals and let Arnold manage that
   if (in_exportNormals)
      AiArrayDestroy(in_nlist);

   return true;
}


// Export the vertices and the nidxs and nlist, ie the normals
// For #1220 we have unified here the vertices and normals loop, to half the number of calls to GetGeometry, that before
// were done twice for the same mb times, once to get all the vertices and then once to get all the normals.
// m_vertexIndices are required, so be sure to have ExportVertexIndices called BEFORE ExportVerticesAndNormals
//
// @param in_frame          The frame time
//
void CMesh::ExportVerticesAndNormals(double in_frame)
{
   AtVector point;
   CDoubleArray pointsArray;

   AtArray* vlist = AiArrayAllocate(m_nbVertices, (uint8_t)m_nbDefKeys, AI_TYPE_VECTOR);
   bool mbDoneWithPointVelocity(false);

   // check if we want to mb the mesh with its PointVelocity attribute (#432)
   bool usePointVelocity(false);
   if (m_hasIceTree && m_paramProperty.IsValid())
      usePointVelocity = (bool)ParAcc_GetValue(m_paramProperty, L"use_pointvelocity", in_frame);

   if (usePointVelocity && m_nbDefKeys > 0)
      mbDoneWithPointVelocity = ExportIceVertices(vlist);

   CNodeSetter::SetBoolean(m_node, "smoothing", ((m_useDiscontinuity && m_discontinuityAngle>0.0f) || (!m_useDiscontinuity)));

   bool exportNormals = ( (m_subdivIterations == 0) && 
                         ((m_useDiscontinuity && m_discontinuityAngle > 0.0f) || m_geoAccessor.GetUserNormals().GetCount() > 0) ) ||
                        m_hasIceNodeUserNormal;

   if (exportNormals && m_paramProperty.IsValid()) // don't export normals if iterations > 0 in the Arnold Parameters 
   {
      uint8_t iterations = (uint8_t)ParAcc_GetValue(m_paramProperty, L"subdiv_iterations", in_frame);
      if (iterations > 0)
         exportNormals = false;
   }

   CFloatArray nodeNormals;
   AtArray *nlist = NULL, *nidxs = NULL;
   LONG normalIndicesSize(0);

   // Get the def mb keys with in_frame (if it is equal to one of the keys) moved into the first position,
   // so to save one GetGeometryAtFrame evaluation, because GetGeometryAtFrame(in_frame) has already be pulled
   // by CMesh::Create.
   // Else, defKeys stays equal to m_defKeys

   CLongArray keysPosition;
   CDoubleArray defKeys = CSceneUtilities::OptimizeMbKeysOrder(m_defKeys, keysPosition, in_frame);

   // evaluate the geo at the deform time steps. This is the standard way to do mb.
   // Skipping for points if mb was already computed by the ICE point velocity attribute above
   for (LONG key = 0; key < m_nbDefKeys; key++)
   {
      LONG keyPosition = keysPosition[key]; // where to write into the Arnold array

      PolygonMesh polygonMeshBlur = CObjectUtilities().GetGeometryAtFrame(m_xsiObj, siConstructionModeSecondaryShape, defKeys[key]);

      CGeometryAccessor geoAccessorBlur = polygonMeshBlur.GetGeometryAccessor(siConstructionModeSecondaryShape, siCatmullClark, 0, false, 
                                                                              m_useDiscontinuity, m_discontinuityAngle);
      if (!mbDoneWithPointVelocity)
      {
         geoAccessorBlur.GetVertexPositions(pointsArray);

         if (key > 0) //check topology consistence for mb (#1718)
         {
            LONG pointsCount = pointsArray.GetCount() / 3;
            // m_nbVertices is initialized using the geo at the first deformation key. 
            // We must be sure that the point count doesn't change for the other keys
            if (pointsCount != m_nbVertices)
            {
               GetMessageQueue()->LogMsg(L"[sitoa] point count mismatch for " + m_xsiObj.GetFullName() + " in the shutter interval. Disabling motion blur for the object", siWarningMsg);
               RemoveMotionBlur(vlist, nlist, exportNormals, keysPosition[0]);
               // return, RemoveMotionBlur sets vlist and nlist, and destroys the current arrays
               return;
            }
         }

         for (LONG i = 0; i < m_nbVertices; i++)
         {
            CUtilities().S2A(pointsArray[i*3], pointsArray[i*3+1], pointsArray[i*3+2], point);
            CUtilities().SetArrayValue(vlist, point, i, keyPosition);
         }
      }

      if (exportNormals)
      {
         if (key == 0) // first key, collect the normal indices only once
         {
            nidxs = NodeIndices();
            normalIndicesSize = AiArrayGetNumElements(nidxs);
            nlist = AiArrayAllocate(AiArrayGetNumElements(nidxs), (uint8_t)m_nbDefKeys, AI_TYPE_VECTOR);
         }

         if (m_hasIceNodeUserNormal)
             GetIceNodeUserNormals(polygonMeshBlur, nodeNormals);
         else // Eric Mootz for #704
             GetGeoAccessorNormals(geoAccessorBlur, normalIndicesSize, nodeNormals);

         AtVector normal;         
         for (LONG i = 0, i3 = 0; i < normalIndicesSize; i++, i3+= 3)
         {
            CUtilities().S2A(nodeNormals[i3], nodeNormals[i3 + 1], nodeNormals[i3 + 2], normal);
            CUtilities().SetArrayValue(nlist, normal, i, keyPosition); 
         }
      } // if exportNormals
   } // keys loop

   if (exportNormals)
   {
      IndexMerge(nidxs, nlist);
      AiNodeSetArray(m_node, "nidxs", nidxs);
      AiNodeSetArray(m_node, "nlist", nlist);
   }

   AiNodeSetArray(m_node, "vlist", vlist);
}


// Eric Mootz for #704
//
// Return the normals, taking care of the user normals, if any
//
// @param in_ga                  The geo accessor
// @param in_normalIndicesSize   The number normal indeces, used unless a different count is found for user normals
// @param out_nodeNormals        The returned normals array
// 
void CMesh::GetGeoAccessorNormals(CGeometryAccessor &in_ga, LONG in_normalIndicesSize, CFloatArray &out_nodeNormals)
{
   CRefArray userNormalsRefs = in_ga.GetUserNormals();
   if (userNormalsRefs.GetCount() <= 0)
      in_ga.GetNodeNormals(out_nodeNormals);
   else
   {
      // there are user normals available... we simply take the first user normals in the ref array.
      ClusterProperty clusterProp(userNormalsRefs[0]);
      // get the cluster property element array.
      CClusterPropertyElementArray clusterPropElements = clusterProp.GetElements();

      LONG clusterElementCount = clusterPropElements.GetCount();
      if (clusterElementCount != in_normalIndicesSize) // log a warning for incomplete clusters (#1889)
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Cluster size mismatch for " + clusterProp.GetFullName() + L": " + (CValue(clusterElementCount).GetAsText()) + 
                                   L" values, " + (CValue(in_normalIndicesSize).GetAsText()) + " expected.", siWarningMsg);
      }

      // do we have a matching count? If so, get the values via GetValues()
      if (clusterElementCount <= in_normalIndicesSize)
         clusterProp.GetValues(out_nodeNormals);
      else
      {
         // we do not have a matching count, so we need to get the user normals "on foot", 
         // because clusterProp.GetValues(nodeNormals) would crash Softimage.
         // resize the array of floats.
         out_nodeNormals.Resize(in_normalIndicesSize * 3L);
         // get them.
         CDoubleArray tmp;
         float *nrm = (float *)out_nodeNormals.GetArray();
         for (LONG i=0; i<in_normalIndicesSize; i++,nrm+=3)
         {
	         tmp = clusterPropElements.GetItem(i);
	         nrm[0] = (float)tmp[0];
	         nrm[1] = (float)tmp[1];
	         nrm[2] = (float)tmp[2];
         }
      }
   }
}


// #1465: Support ICE Node User Normals

// Get the normals from the ICE node user normals attribue (>=2011 only)
//
// @param in_polyMesh            The Softimage polymesh
// @param out_nodeNormals        The returned normals array
// 
void CMesh::GetIceNodeUserNormals(PolygonMesh in_polyMesh, CFloatArray &out_nodeNormals)
{
   CIceAttribute nodeUserNormalAttr(in_polyMesh.GetICEAttributeFromName(L"nodeusernormal"));
   if (!nodeUserNormalAttr.Update())
      return; // ouch

   LONG count = nodeUserNormalAttr.m_v3Data.GetCount();
   // this count should always by equal to the node indeces count
   out_nodeNormals.Resize(count*3);

   for (LONG i=0; i<count; i++)
   {
      CVector3f n = nodeUserNormalAttr.m_isConstant ? nodeUserNormalAttr.m_v3Data[0] : nodeUserNormalAttr.m_v3Data[i];
      n.Get(out_nodeNormals[i*3], out_nodeNormals[i*3+1], out_nodeNormals[i*3+2]);
   }
}


// Export the transformation matrices
//
void CMesh::ExportMatrices()
{
   AtMatrix matrix;
   AtArray* matrices = AiArrayAllocate(1, (uint8_t)m_nbTransfKeys, AI_TYPE_MATRIX);
   for (LONG key=0; key<m_nbTransfKeys; key++)
   {
      CUtilities().S2A(m_xsiObj.GetKinematics().GetGlobal().GetTransform(m_transfKeys[key]), matrix);
      AiArraySetMtx(matrices, key, matrix);
   }

   AiNodeSetArray(m_node, "matrix", matrices);
}


// Export weight maps and CAV
//
void CMesh::ExportClusters()
{
   CRefArray clusters = m_polyMesh.GetClusters();
   LONG nbClusters = clusters.GetCount();

   for (LONG clusterIndex = 0; clusterIndex < nbClusters; clusterIndex++)
   {
      Cluster cluster = clusters[clusterIndex];
      CRefArray clusterProperties = cluster.GetProperties();
      LONG nbClusterProperties = clusterProperties.GetCount();

      for (LONG propIndex = 0; propIndex < nbClusterProperties; propIndex++)
      {
         if (clusterProperties[propIndex].GetClassID() != siClusterPropertyID) 
            continue;
            
         ClusterProperty prop(clusterProperties[propIndex]);     
         CClusterPropertyElementArray elements(prop.GetElements());
         CDoubleArray values = elements.GetArray();
         CString propNameString = prop.GetName();
         const char* propName = propNameString.GetAsciiString();

         if (prop.GetPropertyType() == siClusterPropertyWeightMapType)
         {               
            LONG nbValues = elements.GetCount();
            if (nbValues != m_nbVertices) // count check
            {
                GetMessageQueue()->LogMsg(L"[sitoa] Cluster size mismatch for " + propNameString + L": " + (CValue(nbValues).GetAsText()) + 
                                          L" values, " + (CValue(m_nbVertices).GetAsText()) + " expected. Skipping.", siWarningMsg);
                continue;
            }

            if (AiNodeDeclare(m_node, propName, "varying FLOAT"))
            {
               AtArray* propArray = AiArrayAllocate(nbValues, 1, AI_TYPE_FLOAT);         
               if (!propArray)
                  continue;
               
               for (LONG i = 0; i < nbValues; i++) 
                  AiArraySetFlt(propArray, i, (float)values[i]); 

               AiNodeSetArray(m_node, propName, propArray);
            }
         }
         else if (prop.GetPropertyType() == siClusterPropertyVertexColorType)
         {
            // export as face varying user data
            if (m_nbVertexIndices != values.GetCount() / 4) // count check
            {
                GetMessageQueue()->LogMsg(L"[sitoa] Cluster size mismatch for " + propNameString + L": " + (CValue(values.GetCount()).GetAsText()) + 
                                          L" values, " + (CValue(m_nbVertexIndices*4).GetAsText()) + " expected. Skipping.", siWarningMsg);
                continue;
            }

            if (AiNodeDeclare(m_node, propName, "indexed RGBA"))
            {
               AtArray* colors  = AiArrayAllocate(m_nbVertexIndices, 1, AI_TYPE_RGBA);
               AtArray* indices = NodeIndices();

               for (LONG i = 0, i4 = 0; i < m_nbVertexIndices; i++, i4+= 4)
               {
                  AtRGBA color = AtRGBA((float)values[i4], (float)values[i4 + 1], (float)values[i4 + 2], (float)values[i4 + 3]);
                  AiArraySetRGBA(colors, i, color);
               }

               IndexMerge(indices, colors);

               AiNodeSetArray(m_node, propName, colors);
               CString idxName = CString(propName) + L"idxs";
               AiNodeSetArray(m_node, idxName.GetAsciiString(), indices);
            }
         }
         // else if (prop.GetPropertyType() == siClusterPropertyUVType)
         // that's the case for texture maps and Color_Map_Lookup (#1082)
         // We don't export user data, the main uv set should be used by the shader
      }
   }
}


// Export the face visibility 
//
// @param in_frame          The frame time
//
void CMesh::ExportFaceVisibility(double in_frame)
{
   // Create array with all facets initializated to true (all faces visible)
   AtArray* faceVisibility = AiArrayAllocate(m_nbPolygons, 1, AI_TYPE_BOOLEAN);
   for (uint32_t i = 0; i < AiArrayGetNumElements(faceVisibility) ; i++)
      AiArraySetBool(faceVisibility, i, true); // TODO: optimize by filling the array to true only is one element is (later) found false

   // Search invisible facets through clusters      
   CRefArray polygonClustersArray(m_geoAccessor.GetClusters(siClusterPolygonType));    
   LONG nbClusters = polygonClustersArray.GetCount();
   bool invFacets(false);
   for (LONG i=0; i<nbClusters; i++)
   {
      Cluster polygonCluster(polygonClustersArray[i]);
      Property visibilityProp = polygonCluster.GetLocalProperties().GetItem(L"Visibility");
      if (visibilityProp.IsValid() && !(bool)ParAcc_GetValue(visibilityProp, L"rendvis", in_frame)) 
      {
         // Hide all faces in this cluster
         CClusterElementArray elements(polygonCluster.GetElements());
         LONG nbElements = elements.GetCount();
         for (LONG j=0; j<nbElements; j++)
            AiArraySetBool(faceVisibility, (LONG)elements.GetItem(j), false);
         
         invFacets = true;
      }
   }

   // Only add the data if we have found invisible facets
   if (invFacets)
   {
      AiNodeDeclare(m_node, "face_visibility", "uniform BOOL");
      AiNodeSetArray(m_node, "face_visibility", faceVisibility);
   }
}


// Export edge and vertex creases
//
void CMesh::ExportCreases()
{
   if (!strcmp(AiNodeGetStr(m_node, "subdiv_type"), "none"))
       return;

   vector <uint32_t> crease_idxs;       // main indices array
   vector <float>    crease_sharpness;  // main softness array
   vector <uint32_t> idxs;              // per edge/vertex indeces array
   vector <float>    sharpness;         // per edge/vertex softness array
   bool   hard, allHard(true);
   float  crease=0.0f;
   int    idxsSize;

   // Edges. Looping all edges, not the clusters, because creases can be set in ICE as well
   CEdgeRefArray edges = m_polyMesh.GetEdges();
   LONG nbEdges = edges.GetCount();
   idxsSize = 0;
   CLongArray indexArray;

   const CBoolArray hardArray   = edges.GetIsHardArray();
   CDoubleArray     creaseArray = edges.GetCreaseArray();

   for (LONG edgeIndex=0; edgeIndex<nbEdges; edgeIndex++)
   {
      hard = hardArray[edgeIndex];

      if (!hard)
         crease = (float)creaseArray[edgeIndex];

      if (!(hard || crease > 0.0f))
         continue;
         
      if (idxsSize == 0) // resize this cluster's array to the max possible extent
      {
         idxs.resize(nbEdges*2);
         sharpness.resize(nbEdges);
      }

      if (hard)
         crease = 100.0f; // bigger than a huge subdiv_itarations 
      else
         allHard = false;

      Edge edge = edges.GetItem(edgeIndex);
      indexArray = edge.GetPoints().GetIndexArray();
      idxs[idxsSize]   = indexArray[0];
      idxs[idxsSize+1] = indexArray[1];

      sharpness[idxsSize/2] = crease;

      idxsSize+=2;
   }

   if (idxsSize > 0)
   {
      idxs.resize(idxsSize); // resize to the actual size, then insert into the main vector
      crease_idxs.insert(crease_idxs.end(), idxs.begin(), idxs.end());
      idxs.clear();

      sharpness.resize(idxsSize/2);
      crease_sharpness.insert(crease_sharpness.end(), sharpness.begin(), sharpness.end());
      sharpness.clear();
   }

   // Vertex creases. Also in this case, let's loop the vertices, not the clusters
   CVertexRefArray vertices = m_polyMesh.GetVertices();
   LONG nbVertices = vertices.GetCount();
   creaseArray = vertices.GetCreaseArray();

   idxsSize = 0;
   for (LONG vertexIndex=0; vertexIndex<nbVertices; vertexIndex++)
   {
      crease = (float)creaseArray[vertexIndex];
      if (crease <= 0.0f)
         continue;
      // there is no a GetIsHard for vertices. On applying a hard property, GetCrease returns 10, so
      // let's take 10 as the hard limit.
      hard = crease == 10.0f; 

      if (idxsSize == 0) // resize the array to the max possible extent
      {
         idxs.resize(nbVertices*2);
         sharpness.resize(nbVertices);
      }

      if (hard)
         crease = 100.0f; // bigger than a huge subdiv_itarations 
      else
         allHard = false;
      // in Arnold, a crease vertex is defined duplicating the vertex index in the crease_idxs array
      idxs[idxsSize] = idxs[idxsSize+1] = vertexIndex;
      sharpness[idxsSize/2] = crease;

      idxsSize+=2;
   }

   if (idxsSize > 0)
   {
      idxs.resize(idxsSize); // resize to the actual size, then insert into the main vector
      crease_idxs.insert(crease_idxs.end(), idxs.begin(), idxs.end());
      idxs.clear();

      sharpness.resize(idxsSize/2);
      crease_sharpness.insert(crease_sharpness.end(), sharpness.begin(), sharpness.end());
      sharpness.clear();
   }

   // assign the arrays to the polymesh node
   if (crease_idxs.size() > 0)
   {
      AiNodeSetArray(m_node, "crease_idxs", AiArrayConvert((int)crease_idxs.size(), 1, AI_TYPE_UINT, (unsigned int*)&crease_idxs[0]));
      if (!allHard)
         AiNodeSetArray(m_node, "crease_sharpness", AiArrayConvert((int)crease_sharpness.size(), 1, AI_TYPE_FLOAT, (float*)&crease_sharpness[0]));

      crease_idxs.clear();
      crease_sharpness.clear();
   }
}


// Export the materials, ie the shaders and the displacement map
//
// @param in_frame          The frame time
//
void CMesh::ExportMaterials(double in_frame)
{
   m_materialFrame = in_frame;

   // Getting default UV projection from main material of the object
   m_defaultUV = m_xsiObj.GetMaterial().GetCurrentUV();

   m_standardUVsArray = m_geoAccessor.GetUVs();
   m_nbStandardUVs = m_standardUVsArray.GetCount(); 

   m_materialsArray = m_geoAccessor.GetMaterials();

   bool hasICEMaterials(false);

   long nbMaterialsObjLevel = m_materialsArray.GetCount();
   // ICE materials patch by Paul Hudson
   m_xsiIceGeo = CObjectUtilities().GetGeometryAtFrame(m_xsiObj, in_frame);

   if (m_hasIceTree)
   {
      CIceAttribute matAttr(m_xsiIceGeo.GetICEAttributeFromName(L"Materials"));
      if (matAttr.Update())
      {
         hasICEMaterials = matAttr.m_strData2D.GetCount() > 0;
         for (ULONG i=0; i<matAttr.m_strData2D.GetCount(); i++)
         {
            CICEAttributeDataArrayString strData;
            // get the i-th arrays string from matAttr, which, however, should just
            // contain a single array
            matAttr.m_strData2D.GetSubArray(i, strData);
            for (ULONG j=0; j<strData.GetCount(); j++)
            {
               ULONG nbChar;
               CRef refIceMaterial;
               const CICEAttributeDataArrayString::TData* pStr;
               // get the j-th string (so, the j-th material)
               strData.GetData(j, &pStr, nbChar);
               refIceMaterial.Set(CString(pStr, nbChar));
               if (refIceMaterial.IsValid())
                  m_materialsArray.Add(refIceMaterial);
            }
         }
      }
   }

   m_nbMaterials = m_materialsArray.GetCount();

   AtArray *shaders = AiArrayAllocate(m_nbMaterials, 1, AI_TYPE_NODE);

   AtArray *displacementShaders = AiArrayAllocate(m_nbMaterials, 1, AI_TYPE_NODE);
   bool dispOk(false);

   for (int i=0; i<m_nbMaterials; i++)
   {
      Material material(m_materialsArray[i]);
      AtNode *materialNode = LoadMaterial(material, LOAD_MATERIAL_SURFACE, m_materialFrame, m_xsiObj.GetRef());
      AiArraySetPtr(shaders, i, materialNode);

      AtNode *dispMapNode = LoadMaterial(material, LOAD_MATERIAL_DISPLACEMENT, in_frame, m_xsiObj.GetRef());

      if (dispMapNode)
         dispOk=true;

      AiArraySetPtr(displacementShaders, i, dispMapNode);

      // If there is no valid default UV set yet, continue looking for it
      if (!m_defaultUV.IsValid())
         m_defaultUV = material.GetCurrentUV();
   }

   AiNodeSetArray(m_node, "shader", shaders);

   if (dispOk)
      AiNodeSetArray(m_node, "disp_map", displacementShaders);

   // There are clusters with different materials
   // Need to read shader idxs
   if (m_nbMaterials>1)
   {
      // Get Material Index per Face (clusters)
      CLongArray materialIndices;
      m_geoAccessor.GetPolygonMaterialIndices(materialIndices);
      LONG nshidxs = materialIndices.GetCount();

      AtArray* shidxs = AiArrayAllocate(nshidxs, 1, AI_TYPE_BYTE);
      for (LONG i=0; i<nshidxs; i++)
         AiArraySetByte(shidxs, i, (uint8_t)materialIndices[i]);

      // ICE material
      // If ICE Materials exist, cycle through Polys and update shidxs if a MaterialID is set
      // Offset MaterialID by number of original Materials on the polymesh
      if (hasICEMaterials)
      {
         CIceAttribute matIdAttr(m_xsiIceGeo.GetICEAttributeFromName(L"MaterialID"));
         if (matIdAttr.Update())
         {
            for (ULONG d=0; d<matIdAttr.m_lData.GetCount(); d++)
                if (matIdAttr.m_lData[d] > 0)
                    AiArraySetByte(shidxs, d, (uint8_t)(matIdAttr.m_lData[d] + nbMaterialsObjLevel-1));
         }
      }

      AiNodeSetArray(m_node, "shidxs", shidxs);
   }
}


// Export the ICE attributes
//
// @param in_frame          The frame time
//
void CMesh::ExportIceAttributes(double in_frame)
{
   if (!m_hasIceTree)
      return;

   // initialize the main attributes set
   CIceAttributesSet iceAttributes(m_xsiObj, m_xsiIceGeo);
   // collect all the required attributes
   iceAttributes.CollectRequiredAttributes(in_frame, m_materialsArray, false, false);
   // prepare iceAttributes to host the required attributes
   iceAttributes.GetRequiredAttributesSet();
   // add all necessary provided attributes to the required map
   iceAttributes.GetProvidedAttributesSet();
   // now let's query all ice attributes that need to be pushed
   iceAttributes.BuildAttributesMaps();
   // get the full chunk
   iceAttributes.UpdateChunk(0, m_nbVertices, true);
   // let's use a dummy CIceObjectBase, just to declare the attributes on the node
   CIceObjectBase iceBaseObject;
   // give it our node
   iceBaseObject.m_node = m_node;

   // check if at least one data is per node (face-varyig)
   AtArray *indices = NULL;
   for (AttrMap::iterator attribIt = iceAttributes.m_requiredAttributesMap.begin(); attribIt != iceAttributes.m_requiredAttributesMap.end(); attribIt++)
   {
      if (attribIt->second->GetContextType() == siICENodeContextComponent0D2D)
      {  // if so, we need the node indices
         indices = NodeIndices();
         break;
      }
   }

   // loop the required attributes, and push them
   for (AttrMap::iterator attribIt = iceAttributes.m_requiredAttributesMap.begin(); attribIt != iceAttributes.m_requiredAttributesMap.end(); attribIt++)
      iceBaseObject.DeclareICEAttributeOnMeshNode(attribIt->second, indices);

   AiArrayDestroy(indices);
}


// Transforms the uvs by hand, by the texture definition parameters
//
// @param in_textureProjection   the texture projection
// @param inout_uvValues         the uv values
// 
// @return true if the transformation went ok, else false
//
bool CMesh::TransformUVByTextureProjectionDefinition(ClusterProperty in_textureProjection, CDoubleArray &inout_uvValues)
{
   LONG nbUV = inout_uvValues.GetCount();
   if (nbUV % 3) // we're looping the uv values arrays by 3, let's double check the size
      return false;

   Primitive prim = GetTextureProjectionDefFromTextureProjection(in_textureProjection);
   if (!prim.IsValid())
      return false;

   // get the srt from the projection definition
   CVector3 s((double)ParAcc_GetValue(prim, L"projsclu", DBL_MAX), (double)ParAcc_GetValue(prim, L"projsclv", DBL_MAX), (double)ParAcc_GetValue(prim, L"projsclw", DBL_MAX));
   CVector3 r((double)ParAcc_GetValue(prim, L"projrotu", DBL_MAX), (double)ParAcc_GetValue(prim, L"projrotv", DBL_MAX), (double)ParAcc_GetValue(prim, L"projrotw", DBL_MAX));
   CVector3 t((double)ParAcc_GetValue(prim, L"projtrsu", DBL_MAX), (double)ParAcc_GetValue(prim, L"projtrsv", DBL_MAX), (double)ParAcc_GetValue(prim, L"projtrsw", DBL_MAX));
   // make up the matrix
   CTransformation transf;
   transf.SetScaling(s);
   transf.SetRotationFromXYZAngles(r);
   transf.SetTranslation(t);
   CMatrix4 m = transf.GetMatrix4();
   m.InvertInPlace();

   // apply the matrix to the uvw triplets.
   CVector3 uv;
   for (LONG i=0; i<nbUV; i+=3)
   {
      uv.Set(inout_uvValues[i], inout_uvValues[i+1], inout_uvValues[i+2]);
      // GetMessageQueue()->LogMessage(L"uv0 = " + (CString)uv.GetX() + L" " + (CString)uv.GetY() + L" " + (CString)uv.GetZ());
      uv.MulByMatrix4InPlace(m);
      // GetMessageQueue()->LogMessage(L"uv1 = " + (CString)uv.GetX() + L" " + (CString)uv.GetY() + L" " + (CString)uv.GetZ());
      inout_uvValues[i]   = uv.GetX();
      inout_uvValues[i+1] = uv.GetY();
      inout_uvValues[i+2] = uv.GetZ();
   }

   return true;
}


// Convert the CLongArray to a AtArray
AtArray* CMesh::LongArrayToUIntArray(const CLongArray &in_nodeIndices) const
{
   LONG indicesSize = in_nodeIndices.GetCount();
   AtArray* indices = AiArrayAllocate(indicesSize, 1, AI_TYPE_UINT);
   for (LONG i = 0; i < indicesSize; i++)
      AiArraySetUInt(indices, i, in_nodeIndices[i]);

   return indices;
}

// Return the Softimage node indices as an AtArray
AtArray* CMesh::NodeIndices()
{
   if (!m_node_indices)
   {
      CLongArray nodeIndices;
      m_geoAccessor.GetNodeIndices(nodeIndices);
      m_node_indices = LongArrayToUIntArray(nodeIndices);
   }

   return AiArrayCopy(m_node_indices);
}


// Merge vertex indices that have the same value on the same point in place.
// For a value array with multiple mb keys, it's important to call this function only after
// having collected all the keys, and NOT after each single key, because the input array
// gets destroyed and resized.
//
// @param idxs indexed data indices to merge
// @param values data values to merge
// @param canonical assume the indices and values are canonical
//
void CMesh::IndexMerge(AtArray*& idxs, AtArray*& values, bool canonical) const
{
    AtArray* vidxs = AiNodeGetArray(m_node, "vidxs");

    if (!vidxs || !idxs || !values)
        return;

    if (AiArrayGetNumElements(vidxs) != AiArrayGetNumElements(idxs))
        return;

    if (AiArrayGetNumElements(idxs) < 2 || AiArrayGetNumElements(values) < 2)
        return;

    const int type_size = AiParamGetTypeSize(AiArrayGetType(values));

    if (type_size % 4) // storage class not float or int
        return;

    const int float_size = type_size / 4; // size in 32bit floats or ints

    // create indexed values vector
    const uint32_t index_count = AiArrayGetNumElements(idxs);
    IndexValue* index_values = (IndexValue*)AiMalloc(sizeof(IndexValue) * index_count);

    // initialize the indexed values, optimize if the indices are canonical,
    // which is always the case in HtoA so far.
    // index_values is filled with the first key of the values array only
    if (canonical)
        for (uint32_t i = 0; i < index_count; ++i)
            index_values[i].set(i, INDEX_ARRAY(vidxs)[i], VALUE_AT(values, 0, i));
    else
        for (uint32_t i = 0; i < index_count; ++i)
            index_values[i].set(i, INDEX_ARRAY(vidxs)[i], VALUE_AT(values, 0, INDEX_ARRAY(idxs)[i]));

    // sort by vertex index and by data value for equal indices
    switch (float_size)
    {
    case 1:  std::sort(index_values, index_values + index_count, IndexValueLessThanFloat1()); break;
    case 2:  std::sort(index_values, index_values + index_count, IndexValueLessThanFloat2()); break;
    case 3:  std::sort(index_values, index_values + index_count, IndexValueLessThanFloat3()); break;
    case 4:  std::sort(index_values, index_values + index_count, IndexValueLessThanFloat4()); break;
    default: std::sort(index_values, index_values + index_count, IndexValueLessThanFloatN(float_size)); break;
    }

    // count the number of unique pairs, and assign to first_unique the index to the first unique pair
    IndexValue* it_prev = index_values;
    IndexValue* it = it_prev + 1;
    uint32_t value_count = 1;

    bool (*equalFloatX)(const void *, const void *, int);
    switch (float_size)
    {
    case 1:  equalFloatX = equalFloat1; break;
    case 2:  equalFloatX = equalFloat2; break;
    case 3:  equalFloatX = equalFloat3; break;
    case 4:  equalFloatX = equalFloat4; break;
    default: equalFloatX = equalFloatN; break;
    }

    for (uint32_t i = 1; i < index_count; ++it, ++i)
    {
        if (it->vidx == it_prev->vidx && equalFloatX(it->value, it_prev->value, float_size))
            it->value_index = it_prev->value_index;
        else
        {
            it->value_index = i;
            it_prev = it;
            ++value_count;
        }
    }

    // nothing to merge, the caller will keep its original arrays
    if (value_count == AiArrayGetNumElements(values))
    {
        AiFree(index_values);
        return;
    }

    const uint32_t values_stride = arrayStride(values);
    // write out compressed values array and update merged indices
    AtArray* merged_values = AiArrayAllocate(value_count, AiArrayGetNumKeys(values), AiArrayGetType(values));

    for (uint32_t i = 0, value_index = 0; i < index_count; ++i)
    {
        // unique value to be added to the returned value array, true for i == 0
        if (i == index_values[i].value_index)
        {
            // All the keys are copied: key 0 from index_values[i].value, key k from the same pointer displaced by k * array_stride bytes.
            // Note however that nkeys should be > 1 only when merging normals, other data should not be subject to motion blur.
            for (uint8_t k = 0; k < AiArrayGetNumKeys(values); ++k)
                memcpy(VALUE_AT(merged_values, k, value_index), (uint8_t*)index_values[i].value + k * values_stride, type_size);
            value_index++;
        }

        // the index to be returned for the i-th vertex index points to the last added value
        INDEX_ARRAY(idxs)[index_values[i].position] = value_index - 1;
    }

    AiFree(index_values);
    AiArrayDestroy(values);
    values = merged_values;
}


// Export the UVs, either as the main UV set or as VECTOR2 user data
//
// @param in_frame          The frame time
//
void CMesh::ExportUVs(double in_frame)
{
   // We start with the ICE texture projections, by looking for the attributes matching the tspace_id of the texture shaders.
   // So, if there is at least one attribute of per-node context-type, it will be used as the main Arnold UV set, regardless
   // of the standard (if any) main projection (the one used in the ogl viewport).
   bool mainUvDone(false);

   vector <CIceTextureProjectionAttribute> iceTextureProjectionAttributes; // to store the texture attribute names and wrapping

   if (m_hasIceTree)
   {
      CIceAttribute txtProjAttr;
      // usual routine to get the required/provided attributes.
      CIceAttributesSet iceAttributes(m_xsiObj, m_xsiIceGeo);
      // in this case, the pulling shader is txt-explicit, so the "true" tail parameter
      iceAttributes.CollectRequiredAttributes(in_frame, m_materialsArray, false, true);
      iceAttributes.GetRequiredAttributesSet();
      iceAttributes.GetProvidedAttributesSet();
      iceAttributes.BuildAttributesMaps();
      // loop the texture attributes
      for (AttrMap::iterator attribIt = iceAttributes.m_requiredAttributesMap.begin(); attribIt != iceAttributes.m_requiredAttributesMap.end(); attribIt++)
      {
         // GetMessageQueue()->LogMessage(L"--- Texture Attribute = " + attribIt->second->GetName());
         txtProjAttr = m_xsiIceGeo.GetICEAttributeFromName(attribIt->second->GetName());
         bool hasICETextureProjection = txtProjAttr.Update() && txtProjAttr.m_v3Data.GetCount() > 0;
         if (hasICETextureProjection)
         {
            // export this UV set. If the main uv is already set, or if this attribute is not per-node, then
            // ExportIceProjection return false. Instead, if the main uv set was successfully exported, it returns true
            if (ExportIceProjection(txtProjAttr, mainUvDone))
               mainUvDone = true;
            // add this attribute name to a vector, that will be used by SetWrappingSettings
            CIceTextureProjectionAttribute iceTpa(attribIt->second->GetName());
            iceTpa.EvaluateWrapping(m_xsiIceGeo); // get the projection wrapping attributes
            iceTextureProjectionAttributes.push_back(iceTpa);
         }
      }
   }

   // Set wrapping settings
   for (int i = 0; i < m_nbMaterials; i++)
      SetWrappingAndInstanceValues(m_node, m_xsiObj.GetRef(), m_materialsArray[i], m_standardUVsArray, &iceTextureProjectionAttributes, m_materialFrame);

   // regular (non ICE) texture projections
   if (m_nbStandardUVs == 0)
      return;

   AtArray *nodeIndices = NodeIndices(); // not assigned, must be destroyed
   uint32_t nbIndices = AiArrayGetNumElements(nodeIndices);

   vector <ClusterIndexToNodeIndex> clusterToNode(nbIndices);

   for (LONG i = 0; i < m_nbStandardUVs; i++)
   {
      // UV values are stored as a flat list of float values grouped in
      // triplets (i.e. UVW), first triplet being the values at node 0, etc...
      ClusterProperty uvProperty(m_standardUVsArray[i]);
      CString uvPropertyName = uvProperty.GetName();
      Cluster uvCluster = uvProperty.GetParent(); // get the cluster
      if (!uvCluster.IsValid())
         continue;

      // get the cluster indices
      CLongArray clusterIndices = uvCluster.GetElements().GetArray();
      for (uint32_t j = 0; j < nbIndices; j++)
         clusterToNode[j].Set(clusterIndices[j], j);
      // sort them, so to have in <position> the node index
      std::sort(clusterToNode.begin(), clusterToNode.end());

      // Define if the subdivided uv's will be treated smoothed or not (trac #968)
      // If "smooth when subdividing" is enabled, we will use the pin_borders mode
      // else, the linear mode Arnold 3.3.5 incorporated
      if ((bool)ParAcc_GetValue(uvProperty, L"subdsmooth", in_frame))
         CNodeSetter::SetString(m_node, "subdiv_uv_smoothing", "pin_borders");
      else
         CNodeSetter::SetString(m_node, "subdiv_uv_smoothing", "linear"); 

      CDoubleArray uvValues;
      // UV transformation from its texture
      bool transfDone(false);
      for (int j = 0; j < m_nbMaterials; j++)
      {
         Material material(m_materialsArray[j]);
         Texture texture(material.GetCurrentTexture());
         if (texture.IsValid())
         {
            Parameter tspace_id = ParAcc_GetParameter(texture, L"tspace_id");
            // Some XSI shaders like the normal map ones have a strange name parameter (tspaceid instead of tspace_id)
            if (!tspace_id.IsValid())
               tspace_id = ParAcc_GetParameter(texture, L"tspaceid");

            CString projectionName = tspace_id.GetInstanceValue(m_xsiObj.GetRef(), false).GetAsText();
            // If the projection of the Texture is the same as this UVProperty
            if (projectionName.IsEqualNoCase(uvPropertyName))
            {
               texture.GetTransformValues(siTextureComputeTransformation, uvValues);   
               transfDone = true;
               break;
            }
         }
      }

      if (uvValues.GetCount() == 0) // we didn't get the transformed uv, export the default ones
         uvValues = uvProperty.GetElements().GetArray();

      bool areUVsHomogenous = AreUVsHomogenous(uvProperty, uvValues);

      // If there is just a texture map property, used by a map_lookup_color, the texture does
      // not show up in GL, and it is not returned by material.GetCurrentTexture.
      // So, we would not get transformed uvs for map_lookup_color's.
      // Let's try again getting the texture map from the objects, and compute by hand the 
      // transformation on the uvValues.
      // Can this substitute entirely the material loop above ??
      if (!transfDone)
      {
         for (LONG polyPropIndex = 0; polyPropIndex < m_properties.GetCount(); polyPropIndex++)
         {
            Property textureMapProperty = m_properties[polyPropIndex];
            if (textureMapProperty.GetType() != L"TextureProp")
               continue;
            CString uvSpace = textureMapProperty.GetParameter(L"UVReference").GetValue();
            if (uvSpace == uvPropertyName)
               TransformUVByTextureProjectionDefinition(uvProperty, uvValues);
         }
      }

      // rebuild the indices based on the cluster indices.
      // This is needed when an op like boolean or local subdivision are in place 
      AtArray *indices = AiArrayAllocate(nbIndices, 1, AI_TYPE_UINT); // will be assigned or destoyed by ExportStandardProjection*
      for (uint32_t j = 0; j < nbIndices; j++)
      {
         uint32_t nodeIndex = AiArrayGetUInt(nodeIndices, j); 
         AiArraySetUInt(indices, j, clusterToNode[nodeIndex].position);
      }

      // Let's add the default UV set as main UV set, or the first projection if there is no defaultUV projection
      // If a valid ICE texture projection was detected, we skip this
      // Also, we skip using uvlist if the UV set is homogenous, because if so we export it as face varying points,
      // since the w is needed in the texture shader to divide u and v, for proper camera projection
      if ((!areUVsHomogenous) && (!mainUvDone) && (m_defaultUV == uvProperty || (!m_defaultUV.IsValid() && i == 0)))
         ExportStandardProjectionAsUV(indices, uvValues);
      else // For other projections, add them as face varying user data
         ExportStandardProjectionAsFaceVaryingData(indices, uvValues, uvPropertyName, areUVsHomogenous);
   }

   AiArrayDestroy(nodeIndices);
}


// Export a standard Softimage projection as the main UV set
//
// @param in_nodeIndices     the nodes indexes array
// @param in_uvValues        the uv values
// 
// @return true
//
bool CMesh::ExportStandardProjectionAsUV(AtArray *in_nodeIndices, CDoubleArray &in_uvValues)
{
   if (m_nbVertexIndices != (LONG)AiArrayGetNumElements(in_nodeIndices))
   {
      AiArrayDestroy(in_nodeIndices);
      return false;
   }

   AtArray* uvlist = AiArrayAllocate(m_nbVertexIndices, 1, AI_TYPE_VECTOR2);
   LONG i3 = 0;
   for (LONG i = 0; i < m_nbVertexIndices; i++, i3+=3)
   {
      AtVector2 uv = AtVector2((float)in_uvValues[i3], (float)in_uvValues[i3 + 1]);
      AiArraySetVec2(uvlist, i, uv);
   }
      
   IndexMerge(in_nodeIndices, uvlist);
   AiNodeSetArray(m_node, "uvlist", uvlist);
   AiNodeSetArray(m_node, "uvidxs", in_nodeIndices);

   return true;
}


// Export a standard Softimage projection as face varying user data
//
// @param in_nodeIndices       the nodes indexes array
// @param in_uvValues          the uv values
// @param in_projectionName    the texture projection name
// @param in_areUVsHomogenous  true if the w is to exported, in case of camera projection
// 
// @return true
//
bool CMesh::ExportStandardProjectionAsFaceVaryingData(AtArray* in_nodeIndices, CDoubleArray &in_uvValues, CString &in_projectionName, bool in_areUVsHomogenous)
{
   // we export the uvs are VECTOR if the set is homogenous (for camera projection), else as standard VECTOR2
   if (!AiNodeDeclare(m_node, in_projectionName.GetAsciiString(), in_areUVsHomogenous ? "indexed VECTOR" : "indexed VECTOR2"))
   {
      AiArrayDestroy(in_nodeIndices);
      return false;
   }

   if (m_nbVertexIndices != (LONG)AiArrayGetNumElements(in_nodeIndices))
   {
      AiArrayDestroy(in_nodeIndices);
      return false; // COUNT_CHECK
   }

   AtArray* uvlist = AiArrayAllocate(m_nbVertexIndices, 1, in_areUVsHomogenous ? AI_TYPE_VECTOR : AI_TYPE_VECTOR2);                  
   LONG i3 = 0;
   if (in_areUVsHomogenous) // export uvw
   {
      AtVector uvw;
      for (LONG i = 0; i < m_nbVertexIndices; i++, i3+=3)
      {
         uvw.x = (float)in_uvValues[i3];
         uvw.y = (float)in_uvValues[i3 + 1];
         uvw.z = (float)in_uvValues[i3 + 2];
         AiArraySetVec(uvlist, i, uvw);
      }
   }
   else
   {
       AtVector2 uv;
       for (LONG i = 0; i < m_nbVertexIndices; i++, i3+=3)
       {
          uv.x = (float)in_uvValues[i3];
          uv.y = (float)in_uvValues[i3 + 1];
          AiArraySetVec2(uvlist, i, uv);
       }
   }

   IndexMerge(in_nodeIndices, uvlist);

   AiNodeSetArray(m_node, in_projectionName.GetAsciiString(), uvlist);
   CString idxName = in_projectionName + L"idxs"; // no need to declare the idx array
   AiNodeSetArray(m_node, idxName.GetAsciiString(), in_nodeIndices);

   return true;
}


// Export a ICE projection as the main UV set or as user data
//
// @param in_node                The polymesh node
// @param in_txtProjAttr         The ICE attribute setting the projection
// @param in_mainUvDone          True if the main Arnold UV set has already been set, else false
//
// @return true if this attribute was successfully set as the main Arnold UV set, else false
//
bool CMesh::ExportIceProjection(CIceAttribute &in_txtProjAttr, bool in_mainUvDone)
{
   siICENodeContextType contextType = in_txtProjAttr.GetContextType();

   if (contextType == siICENodeContextComponent0D2D) // one element per node -> regular UV set
   {
      AtArray* uvlist = AiArrayAllocate(m_nbVertexIndices, 1, AI_TYPE_VECTOR2);
      AtArray* uvidxs = NodeIndices();

      AtVector2 uv;
      for (LONG i = 0; i < m_nbVertexIndices; i++)
      {
         uv.x = in_txtProjAttr.m_v3Data[i].GetX();
         uv.y = in_txtProjAttr.m_v3Data[i].GetY();
         AiArraySetVec2(uvlist, i, uv);
      }

      IndexMerge(uvidxs, uvlist);

      if (!in_mainUvDone) // export the node set as the main uv
      {
         AiNodeSetArray(m_node, "uvlist", uvlist);
         AiNodeSetArray(m_node, "uvidxs", uvidxs);
         return true; // return true to mean that the main uv set is now set
      }
      else // export the node set as face varying user data
      {
         CString attributeName = in_txtProjAttr.GetName();

         if (AiNodeDeclare(m_node, attributeName.GetAsciiString(), "indexed VECTOR2"))
         {
            AiNodeSetArray(m_node, attributeName.GetAsciiString(), uvlist);
            CString idxName = attributeName + L"idxs"; // no need to declare the idx array
            AiNodeSetArray(m_node, idxName.GetAsciiString(), uvidxs);
         }
         return false;
      }
   }
   else if (contextType == siICENodeContextComponent0D) // one element per point -> varying user data
   {
      LONG elementCount = (LONG)in_txtProjAttr.GetElementCount();
      CString attributeName = in_txtProjAttr.GetName();
      if (AiNodeDeclare(m_node, attributeName.GetAsciiString(), "varying VECTOR2"))
      {
         AtArray* uvs = AiArrayAllocate(elementCount, 1, AI_TYPE_VECTOR2);
         for (LONG i=0; i<elementCount; i++) // loop the points
         {
            AtVector2 uv = AtVector2(in_txtProjAttr.m_v3Data[i].GetX(), in_txtProjAttr.m_v3Data[i].GetY());
            AiArraySetVec2(uvs, i, uv); 
         }
         AiNodeSetArray(m_node, attributeName.GetAsciiString(), uvs);
      }
      return false;
   }

   return false;
}


// Export the environment shader
void CMesh::ExportEnvironment()
{
   AtNode *environmentNode = LoadMaterial(m_xsiObj.GetMaterial(), LOAD_MATERIAL_ENVIRONMENT, m_materialFrame, m_xsiObj.GetRef());

   // If we have found a shader attached to the material's environment we will
   // declare it as constant pointer on the mesh (shaders that implement environment lookup
   // will have to get it from the object)
   if (environmentNode)
      if (AiNodeDeclare(m_node, "environment", "constant NODE"))
         CNodeSetter::SetPointer(m_node, "environment", environmentNode);
}


// Export the light group
//
void CMesh::ExportLightGroup()
{
   AtArray* light_group = GetRenderInstance()->LightMap().GetLightGroup(m_xsiObj);
   if (light_group)
   {
      CNodeSetter::SetBoolean(m_node, "use_light_group", true);
      if (AiArrayGetNumElements(light_group) > 0)
         AiNodeSetArray(m_node, "light_group", light_group);
   }
}  


// Export the subdivision attributes
//
// @param in_frame          The frame time
//
void CMesh::ExportSubdivision(double in_frame)
{
   uint8_t subdiv_iterations = (uint8_t)ParAcc_GetValue(m_geoProperty, L"gapproxmordrsl", in_frame);

   float adaptive_error = GetRenderOptions()->m_adaptive_error;
   bool prop_adaptive_subdivision(false);
   CString adaptive_metric(L"auto"), adaptive_space(L"raster");

   if (m_paramProperty.IsValid())
   {
      prop_adaptive_subdivision = ParAcc_GetValue(m_paramProperty, L"adaptive_subdivision", in_frame);
      if (prop_adaptive_subdivision) // else adaptive_error stays == global_pixel_error
      {
         adaptive_error = (float)ParAcc_GetValue(m_paramProperty, L"subdiv_adaptive_error", in_frame);
         adaptive_metric = ParAcc_GetValue(m_paramProperty, L"subdiv_adaptive_metric", in_frame).GetAsText();
         adaptive_space = ParAcc_GetValue(m_paramProperty, L"subdiv_adaptive_space", in_frame).GetAsText();
      }
      uint8_t prop_subdiv_iterations = (uint8_t)ParAcc_GetValue(m_paramProperty, L"subdiv_iterations", in_frame);
      subdiv_iterations+= prop_subdiv_iterations;
   }

   if (subdiv_iterations > 0)
   {
      Parameter subdruleParameter = m_primitive.GetParameter(L"subdrule");
      if (subdruleParameter.IsValid())
      {
         CValue subdruleType = subdruleParameter.GetValue(in_frame);
         // We support 0 and 3 (0: Catmull Clark, 2: XSI-Doo-Sabin, 3: Linear)
         if (subdruleType == 3)
            CNodeSetter::SetString(m_node, "subdiv_type", "linear");
         else
            CNodeSetter::SetString(m_node, "subdiv_type", "catclark");
      }

      CNodeSetter::SetByte  (m_node, "subdiv_iterations",      subdiv_iterations);
      CNodeSetter::SetFloat (m_node, "subdiv_adaptive_error",  adaptive_error);
      CNodeSetter::SetString(m_node, "subdiv_adaptive_metric", adaptive_metric.GetAsciiString());
      CNodeSetter::SetString(m_node, "subdiv_adaptive_space",  adaptive_space.GetAsciiString());
   }
}


// Export the pref points, ie the points at the modeling stage
//
// @param in_frame          The frame time
//
void CMesh::ExportPref(double in_frame)
{
  // Export the Pref data if checked
   if (!(bool)ParAcc_GetValue(m_paramProperty, L"export_pref", in_frame))
      return;

   AiNodeDeclare(m_node, "Pref", "varying VECTOR");

   PolygonMesh polyMeshBindPose = CObjectUtilities().GetGeometryAtFrame(m_xsiObj, siConstructionModeModeling, in_frame);
   CGeometryAccessor geoAccessorBindPose = polyMeshBindPose.GetGeometryAccessor(siConstructionModeModeling, siCatmullClark, 0, false, 
                                                                                m_useDiscontinuity, m_discontinuityAngle);

   LONG vertexCountBindPose = geoAccessorBindPose.GetVertexCount();
   AtVector* vlistBindPose = new AtVector[vertexCountBindPose];
   CDoubleArray pointsArrayBindPose;

   geoAccessorBindPose.GetVertexPositions(pointsArrayBindPose);

   for (LONG i=0; i < vertexCountBindPose; ++i)
   {
      vlistBindPose[i].x = (float)pointsArrayBindPose[3*i];
      vlistBindPose[i].y = (float)pointsArrayBindPose[3*i + 1];
      vlistBindPose[i].z = (float)pointsArrayBindPose[3*i + 2];
   }

   AiNodeSetArray(m_node, "Pref", AiArrayConvert(vertexCountBindPose, 1, AI_TYPE_VECTOR, vlistBindPose));
   delete [] vlistBindPose;
}


// Export the visibility, sidedness, custom parameters, user options and blob data
//
// @param in_frame          The frame time
//
void CMesh::ExportVizSidednessAndOptions(double in_frame)
{
   CNodeSetter::SetByte(m_node, "visibility", GetVisibility(m_properties, in_frame), true);

   uint8_t sidedness;
   if (GetSidedness(m_properties, in_frame, sidedness))
      CNodeSetter::SetByte(m_node, "sidedness", sidedness, true);

   if (m_paramProperty.IsValid())
      LoadArnoldParameters(m_node, m_paramProperty.GetParameters(), in_frame);

   CustomProperty userOptionsProperty;
   m_properties.Find(L"arnold_user_options", userOptionsProperty); /// #680

   LoadUserOptions(m_node, userOptionsProperty, in_frame); // #680
   LoadUserDataBlobs(m_node, m_xsiObj, in_frame); // #728
   
   if (!GetRenderOptions()->m_ignore_matte)
   {
      Property matteProperty;
      m_properties.Find(L"arnold_matte", matteProperty);
      LoadMatte(m_node, matteProperty, in_frame);
   }
}


void CMesh::ExportMotionStartEnd()
{
   CNodeUtilities::SetMotionStartEnd(m_node);
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// Load all the polymeshes
//
// @param in_frame               The frame time
// @param in_selectedObjs        The selected objs to render (if in_selectionOnly==true)
// @param in_selectionOnly       True is only in_selectedObjs must be rendered
//
// @return CStatus:OK if all went well, else the error CStatus
//
CStatus LoadPolymeshes(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{ 
   CStatus status;

   CRefArray polysArray = Application().GetActiveSceneRoot().FindChildren(L"", siPolyMeshType, CStringArray(), true);

   for (LONG i=0; i<polysArray.GetCount(); i++)
   {
      // check if this mesh is selected
      if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, polysArray[i]))
         continue;

      X3DObject mesh(polysArray[i]);
      status = LoadSinglePolymesh(mesh, in_frame, in_selectedObjs, in_selectionOnly);
      if (status != CStatus::OK)
         break;
   }

   return status;
}


// Load a single polymesh
//
// @param in_xsiObj              The Softimage mesh
// @param in_frame               The frame time
// @param in_selectedObjs        The selected objs to render (if in_selectionOnly==true)
// @param in_selectionOnly       True is only in_selectedObjs must be rendered
//
// @return CStatus:OK if all went well, else the error CStatus
//
CStatus LoadSinglePolymesh(X3DObject &in_xsiObj, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   // already exported ? 
   if (GetRenderInstance()->NodeMap().GetExportedNode(in_xsiObj, in_frame) != NULL)
      return CStatus::OK;

   // if the mesh invisible ?
   Property visProperty;
   in_xsiObj.GetPropertyFromName(L"Visibility", visProperty);
   if (!ParAcc_GetValue(visProperty,L"rendvis", in_frame))
      return CStatus::OK;
   
   CRefArray properties = in_xsiObj.GetProperties();

   // is this a procedural ?
   if (properties.GetItem(L"arnold_procedural").IsValid())
      return LoadSingleProcedural(in_xsiObj, in_frame, in_selectedObjs, in_selectionOnly);
   // is this a volume ?
   if (properties.GetItem(L"arnold_volume").IsValid())
      return LoadSingleVolume(in_xsiObj, in_frame, in_selectedObjs, in_selectionOnly);

   CMesh mesh;
   if (!mesh.Create(in_xsiObj, in_frame))
      return CStatus::OK;

   mesh.ExportPolygonVerticesCount();
   mesh.ExportVertexIndices();
   mesh.ExportVerticesAndNormals(in_frame);
   mesh.ExportMatrices();
   mesh.ExportFaceVisibility(in_frame);
   mesh.ExportSubdivision(in_frame);
   mesh.ExportCreases();
   mesh.ExportMaterials(in_frame);
   mesh.ExportClusters();
   mesh.ExportIceAttributes(in_frame);
   mesh.ExportUVs(in_frame);
   mesh.ExportEnvironment();
   mesh.ExportLightGroup();
   mesh.ExportPref(in_frame);
   mesh.ExportMotionStartEnd();
   mesh.ExportVizSidednessAndOptions(in_frame);
 
   return CStatus::OK;
}


