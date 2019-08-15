/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "loader/ICE.h"

#include <xsi_geometryaccessor.h>
#include <xsi_polygonmesh.h>

#include <ai_nodes.h>

using namespace XSI;

/// Return the distance in bytes from key to key of an array
inline uint32_t arrayStride(const AtArray* array)
{
    return AiArrayGetNumElements(array) * AiParamGetTypeSize(AiArrayGetType(array));
}

#define INDEX_ARRAY(a) (static_cast<unsigned*>(AiArrayMap(a)))
#define VALUE_AT(a, k, i) (&(static_cast<uint8_t*>(AiArrayMap(a))[(k * AiArrayGetNumElements(a) + i) * type_size]))

/// Index-value pairs
struct IndexValue
{
    uint32_t vidx;         ///< vertex index
    uint32_t position;     ///< original index position
    uint32_t value_index;  ///< corresponding value index
    void*    value;        ///< value pointer

    IndexValue() {}

    inline void set(uint32_t position, uint32_t vidx, void* value)
    {
        this->vidx        = vidx;
        this->position    = position;
        this->value_index = 0;
        this->value       = value;
    }
};


struct ClusterIndexToNodeIndex
{
   uint32_t clusterIndex;
   uint32_t position;

   ClusterIndexToNodeIndex() {}

   inline void Set(const uint32_t in_clusterIndex, const uint32_t in_position)
   {
       clusterIndex = in_clusterIndex;
       position     = in_position;
   }

   bool operator < (const ClusterIndexToNodeIndex& arg) const
   {
      return clusterIndex < arg.clusterIndex;
   }

    void Log()
    {
        Application().LogMessage(((CValue)(int)clusterIndex).GetAsText() + L" " + ((CValue)(int)position).GetAsText());
    }
};

class CMesh
{
public:
   CMesh()
   {
      m_node_indices = NULL;
      m_hasMainUv = false;
      m_hasIceTree = false;
      m_hasIceNodeUserNormal = false;
   }

   ~CMesh()
   {
      AiArrayDestroy(m_node_indices);
   }

   // Create the polymesh node and set the base class members
   bool Create(const X3DObject &in_xsiObj, double in_frame);
   // Export nsides, the number of nodes per polygon
   void ExportPolygonVerticesCount();
   // Export vidxs, the vertices indeces. The number of vidxs equals the sum of the nsides array
   void ExportVertexIndices();
   // Export the vertices and the nidxs and nlist, ie the normals
   void ExportVerticesAndNormals(double in_frame);
   // Export the transformation matrices
   void ExportMatrices();
   // Export weight maps and CAV
   void ExportClusters();
   // Export the face visibility 
   void ExportFaceVisibility(double in_frame);
   // Export edge and vertex creases
   void ExportCreases();
   // Export the materials, ie the shaders and the displacement map
   void ExportMaterials(double in_frame);
   // Export the ICE attributes
   void ExportIceAttributes(double in_frame);
   // Export the UVs, either as the main UV set or as VECTOR2 user data
   void ExportUVs(double in_frame);
   // Export the environment shader
   void ExportEnvironment();
   // Export the light group
   void ExportLightGroup();
   // Export the subdivision attributes
   void ExportSubdivision(double in_frame);
   // Export the pref points, ie the points at the modeling stage
   void ExportPref(double in_frame);
   // Export the nref normals, ie the normals at the modeling stage
   void ExportNref(double in_frame);
   // Export the visibility, sidedness, custom parameters, user options and blob data
   void ExportVizSidednessAndOptions(double in_frame);
   // Export motion_start, motion_end
   void ExportMotionStartEnd();

private:
   // Check if the mesh has an ICE tree, and set m_hasIceTree accordingly
   void CheckIceTree();
   // Export the vertices in case they have to be mblurred by the PointVelocity attribute
   bool ExportIceVertices(AtArray *io_vlist);
   // Resize the vlist and nlist arrays to just contain one key.
   bool RemoveMotionBlur(AtArray *in_vlist, AtArray *in_nlist, bool in_exportNormals, LONG in_firstKeyPosition);
   // Check if the mesh has an ICE nodeusernormal attribute, and set m_hasIceNodeUserNormal accordingly
   void CheckIceNodeUserNormal();
   // Check if the input uvs are homogenous, ie their w is the weight
   bool AreUVsHomogenous(ClusterProperty &in_uvProperty, CDoubleArray &in_uvValues);
   // Return the normals, taking care of the user normals, if any
   void GetGeoAccessorNormals(CGeometryAccessor &in_ga, LONG in_normalIndicesSize, CFloatArray &out_nodeNormals);
   // Get the normals from the ICE node user normals attribue (>=2011 only)
   void GetIceNodeUserNormals(PolygonMesh in_polyMesh, CFloatArray &out_nodeNormals);
   // Transforms the uvs by hand, by the texture definition parameters
   bool TransformUVByTextureProjectionDefinition(ClusterProperty in_uvProperty, CDoubleArray &inout_uvValues);
   // Convert the CLongArray to a AtArray
   AtArray* LongArrayToUIntArray(const CLongArray &in_nodeIndices) const;
   // Return the Softimage node indices as an AtArray
   AtArray* NodeIndices();
   // Merge vertex indices that have the same value on the same point in place.
   void IndexMerge(AtArray*& idxs, AtArray*& values, bool canonical = false) const;
   // Export a standard Softimage projection as the main UV set
   bool ExportStandardProjectionAsUV(AtArray* in_nodeIndices, CDoubleArray &in_uvValues);
   // Export a standard Softimage projection as face varying user data
   bool ExportStandardProjectionAsFaceVaryingData(AtArray *in_nodeIndices, CDoubleArray &in_uvValues, CString &in_projectionName, bool in_areHomogenous);
   // Export a ICE projection as the main UV set or as user data
   bool ExportIceProjection(CIceAttribute &in_txtProjAttr, bool in_mainUvDone);

   AtNode* m_node;                  // the created polymesh node
   AtArray *m_node_indices;         // Softimage node indices
   Geometry m_xsiIceGeo;            // used to get the ICE materials and UVs

   X3DObject         m_xsiObj;             // the Softimage object, set by Create()
   Primitive         m_primitive;          // the active primitive, set by Create()
   PolygonMesh       m_polyMesh;           // the current frae Softimage mesh, set by Create()
   CGeometryAccessor m_geoAccessor;        // the current frame geo accessor, set by Create()
   LONG              m_nbVertexIndices;    // number of vertex indices
   LONG              m_nbVertices;         // number of vertices;
   LONG              m_nbPolygons;         // number of polygons
   LONG              m_nbMaterials;        // total number of materials, including the ICE ones  
   CRefArray         m_standardUVsArray;   // standard (non ICE) UV clusters
   LONG              m_nbStandardUVs;      // number of standard UV clusters
   double            m_materialFrame;      // frame for shifted materials (if enabled in the preferences)
   CRefArray         m_materialsArray;     // standard (non ICE) materials
   ClusterProperty   m_defaultUV;          // the default standard projection that wins the race for uvlist

   // properties
   CRefArray         m_properties;         // the properties array
   Property          m_paramProperty;      // the Arnold Parameters property

   // geometry approximation parameters
   Property          m_geoProperty;        // the geometry approximation property
   bool              m_useDiscontinuity;   // the automatic discontinuity flag
   double            m_discontinuityAngle; // the discontinuity angle
   LONG              m_subdivIterations;   // the Render Level subdivisions

   bool              m_hasMainUv;            // turned true after the first main UV (uvlist) is exported
   bool              m_hasIceTree;           // true if the mesh has an ICE tree applied
   bool              m_hasIceNodeUserNormal; // true if the nodeusernormal attribute is avaliable

   CDoubleArray      m_transfKeys, m_defKeys;     // the mb keys
   LONG              m_nbTransfKeys, m_nbDefKeys; // the number of transf/def keys
};

// Load all the polymeshes
CStatus LoadPolymeshes(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly = false);
// Load a single polymesh
CStatus LoadSinglePolymesh(X3DObject &in_xsiObj, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly = false);
