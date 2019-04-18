/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "loader/Shaders.h"
#include "loader/Strands.h"
#include "renderer/AtNodeLookup.h"

#include <xsi_geometry.h>
#include <xsi_icetree.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_iceattributedataarray2D.h>
#include <xsi_imageclip.h>
#include <xsi_kinematics.h>
#include <xsi_model.h>
#include <xsi_property.h>
#include <xsi_status.h>
#include <xsi_vector3f.h>
#include <xsi_version.h> // DON'T REMOVE, FOR #1366

#include <map>
#include <set>

using namespace XSI;
using namespace XSI::MATH;
using namespace std;


// the prefixes for the attribute names that are allowed to drive Arnold parameters
// directly, and that won't be exported as user data.
static CString g_ArnoldLightAttributePrefix(L"arnoldlight");
static CString g_ArnoldProceduralAttributePrefix(L"arnoldprocedural");
// this map to store, for each shape instance node, if it's in object only or in hierarhy mode
// for #1728. Tha map is built by SearchAllTreesForShapeNodes, because the solo/hierarchy information
// is not available as attribute, so, for this only time, we have to explore the node that generated the shapes
typedef map<CString, bool> ShapeHierarchyModeMap;
typedef pair<CString, bool> ShapeHierarchyModePair;

class CIceUtilities
{
public:
   // Convert a CRotationf to a CRotation
   CRotation RotationfToRotation(const CRotationf &in_r);
   // Check if in_attributeName begins by "ArnoldLight" (case insensitive). 
   bool GetArnoldLightParameterFromAttribute(CString in_attributeName, CString &out_parameterName);
   // Check if an object has a procedural property AND any of the string parameters are "ArnoldProcedural"
   bool ObjectHasArnoldProceduralProceduralProperty(X3DObject in_xsiObj, double in_frame, CString &out_path);
   // Check if if any object under a model has a procedural property AND any of the string parameters are "ArnoldProcedural"
   bool ModelHasArnoldProceduralProceduralProperty(const Model &in_model, double in_frame);
   // Check if in_attributeName begins by "ArnoldProcedural" (case insensitive).
   bool GetTrimmedArnoldProceduralAttributeName(CString in_attributeName, CString &out_parameterName, bool in_lower = true);
   // check if the pointcloud has the same number of points at all the mb times
   void IsPointCountTheSameAtAllMbSteps(const X3DObject &in_xsiObj, LONG in_pointCount, CDoubleArray in_transfKeys, CDoubleArray in_defKeys, 
                                        double in_frame, bool &out_doExactTransformMb, bool &out_doExactDeformMb);
   // Traverse all the ice trees of an object to fill the hyerarchy mode map, for shape instancing
   void SearchAllTreesForShapeNodes(const X3DObject &in_xsiObj, ShapeHierarchyModeMap &out_map);
private:
   // Traverse all the nodes of an ice trees to fill the hyerarchy mode map, for shape instancing
   void SearchShapeNodes(const ICENode& in_node, ShapeHierarchyModeMap &out_map);
};


// forward declaration
class CIceAttribute;
class CIceObjects;

// Type of attributes to set
enum eDeclICEAttr
{
   eDeclICEAttr_Constant,
   eDeclICEAttr_Uniform,
   eDeclICEAttr_Varying,
   eDeclICEAttr_Indexed,
   eDeclICEAttr_NbElements
};

// chunk size used to read the points
// basically give up using chunks in 2013 because of #1366
#if XSISDK_VERSION == 11000
const LONG ICE_CHUNK_SIZE = 100000000; // 100M
#else
const LONG ICE_CHUNK_SIZE = 100000;
#endif 


// small extension of the base ICEAttribute
class CIceAttribute : public ICEAttribute
{
public:
   CString m_name;
   bool m_isDefined;
   bool m_isArray;
   bool m_isConstant;
   siICENodeDataType m_eType;
   LONG m_storedOffset;
   // be ready to host any kind of attribute
   CICEAttributeDataArrayBool          m_bData;
   CICEAttributeDataArrayFloat         m_fData;
   CICEAttributeDataArrayLong          m_lData;
   CICEAttributeDataArrayVector2f      m_v2Data;
   CICEAttributeDataArrayVector3f      m_v3Data;
   CICEAttributeDataArrayVector4f      m_v4Data;
   CICEAttributeDataArrayQuaternionf   m_qData;
   CICEAttributeDataArrayMatrix3f      m_m3Data;
   CICEAttributeDataArrayMatrix4f      m_m4Data;
   CICEAttributeDataArrayColor4f       m_cData;
   CICEAttributeDataArrayRotationf     m_rData;
   CICEAttributeDataArrayShape         m_sData;

   CICEAttributeDataArray2DBool        m_bData2D;
   CICEAttributeDataArray2DFloat       m_fData2D;
   CICEAttributeDataArray2DLong        m_lData2D;
   CICEAttributeDataArray2DVector2f    m_v2Data2D;
   CICEAttributeDataArray2DVector3f    m_v3Data2D;
   CICEAttributeDataArray2DVector4f    m_v4Data2D;
   CICEAttributeDataArray2DQuaternionf m_qData2D;
   CICEAttributeDataArray2DMatrix3f    m_m3Data2D;
   CICEAttributeDataArray2DMatrix4f    m_m4Data2D;
   CICEAttributeDataArray2DColor4f     m_cData2D;
   CICEAttributeDataArray2DRotationf   m_rData2D;
   CICEAttributeDataArray2DShape       m_sData2D;

   CICEAttributeDataArrayString        m_strData;
   CICEAttributeDataArray2DString      m_strData2D;

   CIceAttribute()
   {
   }

   CIceAttribute(const CIceAttribute &in_arg) : ICEAttribute(in_arg)
   {
      m_name       = in_arg.m_name;
      m_eType      = in_arg.m_eType;
      m_isArray    = in_arg.m_isArray;
      m_isConstant = in_arg.m_isConstant;
      m_isDefined  = in_arg.m_isDefined;
      m_storedOffset = in_arg.m_storedOffset;
   }

   CIceAttribute(const ICEAttribute &in_arg) : ICEAttribute(in_arg)
   {
      m_name       = this->GetName();
      m_eType      = this->GetDataType();
      m_isArray    = this->GetStructureType() == siICENodeStructureArray;
      m_isConstant = this->IsConstant();
      m_isDefined  = this->IsValid() && this->IsDefined();
      m_storedOffset = -1;
   }

   ~CIceAttribute()
   {
   }

   // accessors for the data array to be use in place of the [] operator
   bool         GetBool(LONG in_index);
   float        GetFloat(LONG in_index);
   int          GetInt(LONG in_index);
   CVector3f    GetVector3f(LONG in_index);
   CQuaternionf GetQuaternionf(LONG in_index);
   CMatrix3f    GetMatrix3f(LONG in_index);
   CMatrix4f    GetMatrix4f(LONG in_index);
   CColor4f     GetColor4f(LONG in_index);
   CRotationf   GetRotationf(LONG in_index);
   CShape       GetShape(LONG in_index);

   // Get the attributes for the in_Count points, starting from in_Offset
   bool Update(LONG in_Offset, LONG in_Count);
   bool Update();
};


class CompareCStrings
{
   public:
   bool operator() (const CString& lhs, const CString& rhs) const
   {
      return strcmp(lhs.GetAsciiString(), rhs.GetAsciiString()) > 0;
   }
};


// The map to store the attributes. It's just the attribute name and the CIceAttribute pointer
// key == the string
typedef map<CString, CIceAttribute*, CompareCStrings> AttrMap;
typedef pair<CString, CIceAttribute*> AttrPair;

// The attributes center
class CIceAttributesSet
{
private:
   X3DObject m_xsiObj;
   Geometry  m_xsiGeo;

   CIceAttribute *m_pointPosition;
   CIceAttribute *m_orientation;
   CIceAttribute *m_scale;
   CIceAttribute *m_size;
   CIceAttribute *m_shape;
   CIceAttribute *m_shapeTime;
   CIceAttribute *m_color;
   CIceAttribute *m_pointVelocity;
   CIceAttribute *m_angularVelocity;
   CIceAttribute *m_strandPosition;
   CIceAttribute *m_strandScale;
   CIceAttribute *m_strandVelocity;
   CIceAttribute *m_strandSize;
   CIceAttribute *m_strandOrientation;

public:
   // plain set of required attributes
   set<CString> m_reqAttrNames;

   set<CString> m_requiredAttributesSet;
   set<CString> m_providedAttributesSet;
   AttrMap  m_requiredAttributesMap;
   AttrMap  m_providedAttributesMap;

   // construct by object and geometry
   CIceAttributesSet(X3DObject in_xsiObj, Geometry in_xsiGeo)
   {
      this->m_xsiObj = in_xsiObj;
      this->m_xsiGeo = in_xsiGeo;
      m_pointPosition = m_orientation = m_scale = m_size = 
      m_shape = m_color = m_pointVelocity = m_angularVelocity = 
      m_strandPosition = m_strandScale = m_strandOrientation = 
      m_strandVelocity = m_strandSize = m_shapeTime = NULL;
   }

   CIceAttributesSet()
   {
      m_pointPosition = m_orientation = m_scale = m_size = 
      m_shape = m_color = m_pointVelocity = m_angularVelocity = 
      m_strandPosition = m_strandScale = m_strandOrientation = 
      m_strandVelocity = m_strandSize = m_shapeTime = NULL;
   }

   // initialize by object and geometry
   void Init(X3DObject in_xsiObj, Geometry in_xsiGeo)
   {
      this->m_xsiObj = in_xsiObj;
      this->m_xsiGeo = in_xsiGeo;
      m_pointPosition = m_orientation = m_scale = m_size = 
      m_shape = m_color = m_pointVelocity = m_angularVelocity = 
      m_strandPosition = m_strandScale = m_strandOrientation = 
      m_strandVelocity = m_strandSize = m_shapeTime = NULL;
   }

   ~CIceAttributesSet();
   // Collect all the attributes required by the shaders connected to this ice object
   void CollectRequiredAttributes(double in_frame, CRefArray in_iceMaterials, bool in_isVolume, bool in_iceTextures);
   // Store the required attributes names into the requiredAttributesSet set
   void GetRequiredAttributesSet();
   // Copy the required attributes set into the providedAttributesSet set, and add the standard provided attributes
   void GetProvidedAttributesSet(bool in_onlyMbAttributes = false);
   // Build the required and provided attribute maps (name + CIceAttribute pointer)
   void BuildAttributesMaps(bool in_addArnoldLightAttributes=false, bool in_addArnoldProceduralAttributes = false);
   // Connect the map attribute (->second) to a simpler pointer, just to have a better handler to it
   void ConnectAttributeHandlers();
   // Read all the attribute
   void UpdateChunk(const LONG in_pointOffset, const LONG in_nbPoints, const bool in_isMesh = false);
   // Attributes existence checkers
   bool HasPointPosition();
   bool HasOrientation();
   bool HasScale();
   bool HasSize();
   bool HasShape();
   bool HasColor();
   bool HasPointVelocity();
   bool HasAngularVelocity();
   bool HasShapeTime();
   bool HasStrandPosition();
   bool HasStrandScale();
   bool HasStrandVelocity();
   bool HasStrandSize();
   bool HasStrandOrientation();
   // Attribute getters
   // Get the position of a point
   CVector3f  GetPointPosition(LONG in_pointIndex);
   // Get the orientation of a point as CRotationf
   CRotationf GetOrientationf(LONG in_pointIndex);
   // Get the orientation of a point
   CRotation  GetOrientation(LONG in_pointIndex);
   // Get the scale of a point
   CVector3f  GetScale(LONG in_pointIndex);
   // Get the size of a point
   float      GetSize(LONG in_pointIndex, float in_default);
   // Get the shape of a point
   CShape     GetShape(LONG in_pointIndex);
   // Get the color of a point
   CColor4f   GetColor(LONG in_pointIndex);
   // Get the velocity of a point
   CVector3f  GetPointVelocity(LONG in_pointIndex);
   // Get the angular velocity of a point
   CRotation  GetAngularVelocity(LONG in_pointIndex);
   // Get the shape time of a point
   float      GetShapeTime(LONG in_pointIndex, float in_default);
   // strands getters return arrays (one entry for each strand trail point)
   // Get the points position of a strand as a data array vector attribute
   bool GetStrandPosition(LONG in_pointIndex, CICEAttributeDataArrayVector3f &out_data);
   // Get the points position of a strand as a vector of points
   bool GetStrandPosition(LONG in_pointIndex, CICEAttributeDataArrayVector3f &in_dav, vector < CVector3f > &out_data);

   // Get the points scale of a strand
   bool GetStrandScale(LONG in_pointIndex, CICEAttributeDataArrayVector3f &out_data);
   // Get the points velocity of a strand
   bool GetStrandVelocity(LONG in_pointIndex, CICEAttributeDataArrayVector3f &out_data);
   // Get the points size of a strand
   bool GetStrandSize(LONG in_pointIndex, CICEAttributeDataArrayFloat &out_data);
   // Get the points orientation of a strand
   bool GetStrandOrientation(LONG in_pointIndex, CICEAttributeDataArrayRotationf &out_data);

private:
   // Collects in out_attr the set of attributes requested by a shader
   void ParseAttributesShader(Shader in_shader, double in_frame, bool in_isVolume, bool in_iceTextures, set<CString> &out_attr);
   // Collects in out_attr the set of attributes requested by the in_xsiObj materials
   void ParseMaterialAttributeShaders(const X3DObject &in_xsiObj, CRefArray in_iceMaterials, double in_frame, bool in_isVolume, bool in_iceTextures, set<CString> &out_attr);
   // Cycle ALL the objects instanced by this pointcloud, in order to get the full list of them
   void CollectInstancedObjects(set<ULONG> &out_id);
   // Cycle the in_id id set, and for each item, check if it's a model. If so, add the children to out_id. Else, add the object itself to out_id
   void RefineInstancedObjectsSet(set<ULONG> &in_id, set<ULONG> &out_id);
};


// Load all the pointcloud objects
CStatus LoadPointClouds(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly = false);
// Load a pointcloud object
CStatus LoadSinglePointCloud(const X3DObject &in_xsiObj, double in_frame, 
                             CRefArray &in_selectedObjs, bool in_selectionOnly, vector <AtNode*> &out_postLoadedNodesToHide);
// Load a pointcloud object as a points node, regardless of the points shape, for volume rendering in "points" mode
CStatus LoadVolumePointCloud(const X3DObject &in_xsiObj, double in_frame);


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Classes used to store data for the nodes, build and export them
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// CIceObjectBase class
// This is very base class (could also be used for nodes other than those related with ice.
// Id, node type, name, matrix, visibility, sidedness, shader array, plus the AtNode that 
// will eventually be created
///////////////////////////////////////////////////////////////////////
class CIceObjectBase
{
public:
   AtString     m_name;       // unique name
   AtString     m_type;       // "points", "box", "ginstance", etc
   int          m_id;
   uint8_t      m_visibility;
   uint8_t      m_sidedness;
   AtNode*      m_node;       // the node that will be created
   AtArray*     m_matrix;     // the matrices
   AtNode*      m_shader;     // the shaders
   bool         m_isLight;    // is this a light?
   bool         m_isProcedural;    // is this a procedural? (set only for >= 2011

   CIceObjectBase() 
   {
      m_id         = 0;
      m_node       = NULL;
      m_matrix     = NULL;
      m_visibility = AI_RAY_ALL;
      m_sidedness  = AI_RAY_ALL;
      m_shader     = NULL;
      m_isLight    = false;
      m_isProcedural = false;
   }

   ~CIceObjectBase() 
   {
      // if this matrix was never exported to Arnold, destroy it, else #1335
      if (m_node==NULL && m_matrix)
         AiArrayDestroy(m_matrix);
   }

   CIceObjectBase(const CIceObjectBase &in_arg) :
      m_name(in_arg.m_name), m_type(in_arg.m_type), m_id(in_arg.m_id), m_visibility(in_arg.m_visibility), 
      m_sidedness(in_arg.m_sidedness), m_isLight(in_arg.m_isLight), m_isProcedural(in_arg.m_isProcedural)
   {
      m_node = in_arg.m_node;
      if (in_arg.m_matrix)
         m_matrix = AiArrayCopy(in_arg.m_matrix);
      m_shader = in_arg.m_shader;
   }

   // Creates the node
   bool    CreateNode();
   // Get the node pointer
   AtNode* GetNode();
   // Set the matrix array at the given mb key
   bool    SetMatrix(AtMatrix in_value, const int in_key);
   // Set the matrix array at the given mb key
   bool    SetMatrix(CMatrix4 in_value, const int in_key);
   // Set the matrix array at the given mb key
   bool    SetMatrix(CTransformation in_value, const int in_key);
   // Set the object visibility (not yet the node's one)
   void    SetVisibility(uint8_t in_viz);
   // Set the object sidedness (not yet the node's one)
   void    SetSidedness(uint8_t in_sid);
   // Set the object shaders (not yet the node's one)
   void    SetShader(AtNode *in_shader);
   // Set the object vary basic attributes (not yet the node's one)
   void    SetNodeBaseAttributes(int in_id, const char* in_type, const char* in_name);
   // Alloc the object'a matrices and set them to identity
   void    AllocMatrixArray(int in_nbTransfKeys);
   // Give the node the object's attributes. This is where things start to get pushed
   bool    SetNodeData();
   // Assign the light group to the node
   bool    SetLightGroup(AtArray* in_lightGroup);

   // Assign the input boolean attribute value to the parameter with the same name of this (->m_node) light or procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, bool in_value);
   // Assign the input LONG attribute value to the parameter with the same name of this (->m_node) light or procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, int in_value);
   // Assign the input float attribute value to the parameter with the same name of this (->m_node) light or procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, float in_value);
   // Assign the input color attribute value to the parameter with the same name of this (->m_node) light or procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CColor4f in_value);
   // Assign the input vector attribute value to the parameter with the same name of this (->m_node) light or procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CVector3f in_value);
   // Assign the input string attribute value to the parameter with the same name of this (->m_node) procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CString in_value);
   // Assign the input matrix attribute value to the parameter with the same name of this (->m_node) procedural node.
   bool    SetICEAttributeAsNodeParameter(CIceAttribute *in_attr, CMatrix4f in_value);
   // Attach a given attributes to this node
   void    DeclareICEAttributeOnNode(LONG in_index, LONG in_dataArrayIndex, CIceAttribute* in_attribute, double in_frame, 
                                     eDeclICEAttr in_declareType = eDeclICEAttr_Constant, LONG in_count=0, LONG in_offset=0, 
                                     int in_strandCount=0, int in_nbStrandPoints=0);
   // Attach a given attributes to a polymesh node
   void    DeclareICEAttributeOnMeshNode(CIceAttribute* in_attribute, const AtArray *in_indices);
   // Attach a given attributes to a volume node
   void    DeclareICEAttributeOnVolumeNode(CIceAttribute* in_attribute);
   // Set motion_start, motion_end
   void    SetMotionStartEnd();
   // Set the Arnold user options (#680)
   void    SetArnoldUserOptions(CustomProperty in_property, double in_frame);
   // Set the blob data (#728)
   void    SetUserDataBlobs(CRefArray &in_blobProperties, double in_frame);
   // Export the matte data for this node
   void    SetMatte(Property in_property, double in_frame);
};


///////////////////////////////////////////////////////////////////////
// Base class for all the point objects (disk, sphere)
// Derives from CIceObjectBase
///////////////////////////////////////////////////////////////////////
class CIceObjectPoints : public CIceObjectBase
{
private:
   AtArray *m_points;
   AtArray *m_radius;

public:
   CIceObjectPoints() : CIceObjectBase() 
   {
      m_points = NULL;
      m_radius = NULL;
   }

   ~CIceObjectPoints() {}

   CIceObjectPoints(const CIceObjectPoints &in_arg)
   : CIceObjectBase(in_arg)
   {
      if (in_arg.m_points)
         m_points = AiArrayCopy(in_arg.m_points);
      if (in_arg.m_radius)
         m_radius = AiArrayCopy(in_arg.m_radius);
   }

   // Resize the points and radius arrays
   void    Resize(const int in_nbElements, const int nb_keys);
   // Set the in_index/in_key-th point value by a AtVector
   bool    SetPoint(AtVector &in_value, const int in_index, const int in_key);
   // Set the in_index/in_key-th point value by a CVector3f
   bool    SetPoint(CVector3f &in_value, const int in_index, const int in_key);
   // Get the in_index/in_key-th point value
   AtVector GetPoint(const int in_index, const int in_key);
   // Set the in_index/in_key-th radius value
   bool    SetRadius(float in_value, const int in_index, const int in_key);
   // Get the in_index/in_key-th radius value
   float   GetRadius(const int in_index, const int in_key);
   // Create the node
   bool    CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Compute all the positions and radii for mb for a given point
   void    ComputeMotionBlur(CDoubleArray in_keysTime, float in_secondsPerFrame, CVector3f in_velocity, bool in_exactMb, 
                             const vector <CVector3f> &in_mbPos, const vector <float> &in_mbSize, const int in_index, const int in_pointIndex);
   // Give the node the object's attributes.
   bool    SetNodeData();
   // Attach all the required attributes to this node
   void    DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex, int in_dataArrayIndex, int in_nbPoints);
   // Set the Arnold Parameters set 
   void    SetArnoldParameters(CustomProperty in_property, double in_frame);
};


///////////////////////////////////////////////////////////////////////
// Class for rectangular shapes
///////////////////////////////////////////////////////////////////////
class CIceObjectRectangle : public CIceObjectBase
{
private:
   vector < vector < CVector3f > >  m_points;   // points position
   vector < vector < CVector3f > >  m_scale;    // points scale
   vector < vector < CRotation > >  m_rotation; // points orientation

   AtArray *m_nsides, *m_vidxs, *m_vlist;
   // seems there is no need to export the normals
   // AtArray *m_nidxs, *m_nlist;
   AtArray *m_uvlist, *m_uvidxs;

public:
   CIceObjectRectangle() : CIceObjectBase(), m_nsides(NULL), m_vidxs(NULL), m_vlist(NULL),
                           m_uvlist(NULL), m_uvidxs(NULL)
   {}

   ~CIceObjectRectangle() 
   {
      m_points.clear();
      m_scale.clear();
      m_rotation.clear();
   }

   CIceObjectRectangle(const CIceObjectRectangle &in_arg)
      : CIceObjectBase(in_arg), m_points(in_arg.m_points), m_scale(in_arg.m_scale), m_rotation(in_arg.m_rotation)
   {}

   // Resize the points, scale, rotation arrays
   void Resize(const int in_nbElements, const int in_nbKeys, bool in_exactMb);
   // Set the in_index/in_key-th point value by a CVector3f
   bool SetPoint(CVector3f &in_value, const int in_index, const int in_key);
   // Set the in_index/in_key-th scale value by a CVector3f
   bool SetScale(CVector3f &in_value, const int in_index, const int in_key);
   // Set the in_index/in_key-th rotation value
   bool SetRotation(CRotation &in_value, const int in_index, const int in_key);
   // Get the in_index/in_key-th point value
   CVector3f GetPoint(const int in_index, const int in_key);
   // Get the in_index/in_key-th point scale
   CVector3f GetScale(const int in_index, const int in_key);
   // Get the in_index/in_key-th point rotation
   CRotation GetRotation(const int in_index, const int in_key);
   // Create the node
   bool CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Compute all the positions for mb for a given point
   void ComputeMotionBlur(CDoubleArray in_keysTime, float in_secondsPerFrame, CVector3f in_velocity, bool in_exactMb, 
                          const vector <CVector3f> &in_mbPos, const vector <CVector3f> &in_mbScale, const vector <CRotation> &in_mbOri, 
                          const int in_index, const int in_pointIndex);
   // Build the quand polymesh
   bool MakeQuad(bool in_doExactDeformMb);
   // Give the node the object's attributes
   bool SetNodeData();
   // Attach all the required attributes to this node
   void DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex, int in_dataArrayIndex, int in_nbPoints);
   // Set the Arnold Parameters set 
   void SetArnoldParameters(CustomProperty in_property, double in_frame);
};


///////////////////////////////////////////////////////////////////////
// Point Disk class. Derives from CIceObjectPoints, with mode=="disk"
///////////////////////////////////////////////////////////////////////
class CIceObjectPointsDisk : public CIceObjectPoints
{
public:
   CIceObjectPointsDisk() : CIceObjectPoints() {}
   ~CIceObjectPointsDisk() {}
   CIceObjectPointsDisk(const CIceObjectPointsDisk &in_arg) :
      CIceObjectPoints(in_arg)
   {
   }

   bool CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
};


///////////////////////////////////////////////////////////////////////
// Point Sphere class. Derives from CIceObjectPoints, with mode=="sphere"
///////////////////////////////////////////////////////////////////////
class CIceObjectPointsSphere : public CIceObjectPoints
{
public:
   CIceObjectPointsSphere() : CIceObjectPoints() {}
   ~CIceObjectPointsSphere() {}
   CIceObjectPointsSphere(const CIceObjectPointsSphere &in_arg) :
      CIceObjectPoints(in_arg)
   {
   }

   bool CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
};


///////////////////////////////////////////////////////////////////////
// Disc/Box/Cylinder/Cone base class. There will be one class per each Disc, box, etc
///////////////////////////////////////////////////////////////////////

class CIceObjectBaseShape : public CIceObjectBase
{
public:
   CTransformation m_transf; // the point transformation (not yet the node matrix)

   CIceObjectBaseShape() : CIceObjectBase()
   {
      m_transf.SetIdentity();
   }

   ~CIceObjectBaseShape() {}

   CIceObjectBaseShape(const CIceObjectBaseShape &in_arg)
   : CIceObjectBase(in_arg), m_transf(in_arg.m_transf)
   {}

   // Stuff all of the three components into a regular XSI transform
   void SetTransf(const CVector3f in_pos, const CVector3f in_scale, const CRotation in_rot);
   // Compute the motion blur matrices.
   void ComputeMotionBlur(CDoubleArray in_keysTime, float in_secondsPerFrame, CVector3f in_velocity, CRotation in_angVel,
                          bool in_exactMb, const vector <CVector3f> &in_mbPos, const vector <CVector3f> &in_mbScale, 
                          const vector <CRotation> &in_mbOri, const int in_pointIndex);
   // Attach all the required attributes to this node
   void DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex);
   // Set the Arnold Parameters set 
   void SetArnoldParameters(CustomProperty in_property, double in_frame);
};


///////////////////////////////////////////////////////////////////////
// Disc class. Derives from CIceObjectBaseShape
///////////////////////////////////////////////////////////////////////
class CIceObjectDisc : public CIceObjectBaseShape
{
private:
   float     m_radius;
   AtVector  m_normal;

public:
   CIceObjectDisc() : CIceObjectBaseShape() 
   {
      m_radius   = 1.0f;
      m_normal.x = m_normal.z = 0.0f;
      m_normal.y = 1.0f;
   }

   ~CIceObjectDisc() 
   {}

   CIceObjectDisc(const CIceObjectDisc &in_arg) 
   : CIceObjectBaseShape(in_arg), m_radius(in_arg.m_radius)
   {
      m_normal = in_arg.m_normal;
   }

   // Create the node
   bool    CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Set the normal
   void    SetNormal(const AtVector in_value);
   // Give the node the object's attributes.
   bool    SetNodeData();
};



///////////////////////////////////////////////////////////////////////
// Box class. Same as above, except the node is a "box"
///////////////////////////////////////////////////////////////////////
class CIceObjectBox : public CIceObjectBaseShape
{
private:
   AtVector  m_min, m_max;

public:
   CIceObjectBox() : CIceObjectBaseShape() 
   {
      m_min.x = m_min.y = m_min.z = -1.0f;
      m_max.x = m_max.y = m_max.z = 1.0f;
   }

   ~CIceObjectBox() 
   {}

   CIceObjectBox(const CIceObjectBox &in_arg) : 
      CIceObjectBaseShape(in_arg)
   {
      m_min = in_arg.m_min;
      m_max = in_arg.m_max;
   }

   bool    CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Set the box size
   void    SetMinMax(const AtVector in_min, const AtVector in_max);
   // Cut the y, so the box becomes a rectangle
   void    SetFlat();
   // Give the node the object's attributes.
   bool    SetNodeData();
};

///////////////////////////////////////////////////////////////////////
// Cylinder class. Same as above, except the node is a "cylinder"
///////////////////////////////////////////////////////////////////////
class CIceObjectCylinder : public CIceObjectBaseShape
{
private:
   float    m_radius;
   AtVector m_top, m_bottom;

public:
   CIceObjectCylinder() : CIceObjectBaseShape() 
   {
      m_top.x = m_top.z = 0.0f;
      m_top.y = 1.0f;
      m_bottom.x = m_bottom.z = 0.0f;
      m_bottom.y = -1.0f;
      m_radius = 1.0f;
   }

   ~CIceObjectCylinder() 
   {}

   CIceObjectCylinder(const CIceObjectCylinder &in_arg)
   : CIceObjectBaseShape(in_arg), m_radius(in_arg.m_radius)
   {
      m_top    = in_arg.m_top;
      m_bottom = in_arg.m_bottom;
   }

   // Create the node
   bool    CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Set the cylinder top and bottom
   void    SetTopBottom(const AtVector in_top, const AtVector in_bottom);
   // Set the cylinder radius
   void    SetRadius(const float in_radius);
   // Give the node the object's attributes.
   bool    SetNodeData();
};

///////////////////////////////////////////////////////////////////////
// Cone class
///////////////////////////////////////////////////////////////////////
class CIceObjectCone : public CIceObjectBaseShape
{
private:
   float    m_bottom_radius;
   AtVector m_top, m_bottom;

public:
   CIceObjectCone() : CIceObjectBaseShape() 
   {
      m_top.x = m_top.z = 0.0f;
      m_top.y = 1.0f;
      m_bottom.x = m_bottom.z = 0.0f;
      m_bottom.y = -1.0f;
      m_bottom_radius = 1.0f;
   }

   ~CIceObjectCone() 
   {}

   CIceObjectCone(const CIceObjectCone &in_arg) : 
      CIceObjectBaseShape(in_arg), m_bottom_radius(in_arg.m_bottom_radius)
   {
      m_top    = in_arg.m_top;
      m_bottom = in_arg.m_bottom;
   }

   // Create the node
   bool    CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Set the cone top and bottom
   void    SetTopBottom(const AtVector in_top, const AtVector in_bottom);
   // Set the cone bottom radius
   void    SetBottomRadius(const float in_radius);
   // Give the node the object's attributes.
   bool    SetNodeData();
};


///////////////////////////////////////////////////////////////////////
// Strand class. Derives from CIceObjectBase and from CHair
///////////////////////////////////////////////////////////////////////
class CIceObjectStrand : public CIceObjectBase, public CHair
{
private:
   AtArray* m_num_points;
   AtArray* m_points;
   AtArray* m_radius;
   AtArray* m_orientations;
   AtString m_mode; // thick...
   float    m_minPixelWidth;
   int      m_nbPoints; // total number of points of the strand object, NOT of the Arnold node

public:
   CIceObjectStrand() : CIceObjectBase() 
   {
      m_num_points = NULL;
      m_points = NULL;
      m_radius = NULL;
      m_orientations = NULL;
      m_mode = AtString("thick");
      m_minPixelWidth = 0.0f;
      m_nbPoints = 0;
   }

   ~CIceObjectStrand() 
   {
   }

   CIceObjectStrand(const CIceObjectStrand &in_arg) : 
      CIceObjectBase(in_arg), CHair(in_arg), m_mode(in_arg.m_mode), m_minPixelWidth(in_arg.m_minPixelWidth), m_nbPoints(in_arg.m_nbPoints)
   {
      m_num_points   = in_arg.m_num_points   ? AiArrayCopy(in_arg.m_num_points)   : NULL;
      m_points       = in_arg.m_points       ? AiArrayCopy(in_arg.m_points)       : NULL;
      m_radius       = in_arg.m_radius       ? AiArrayCopy(in_arg.m_radius)       : NULL;
      m_orientations = in_arg.m_orientations ? AiArrayCopy(in_arg.m_orientations) : NULL;
   }

   // Create the node
   bool CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Give the node the object's attributes.
   bool SetNodeData();
   // Build all the curves stuff
   bool MakeCurve(CustomProperty in_arnoldParameters, double in_frame, CDoubleArray in_defKeys, float in_secondsPerFrame, bool in_exactMb);
   // Attach all the required attributes for a strand
   void DeclareAttributes(CIceAttributesSet *in_attributes, double in_frame, int in_pointIndex, int in_dataArrayIndex, int in_offset, int nbStrandPoints);
   // Set the Arnold Parameters set 
   void SetArnoldParameters(CustomProperty in_property, double in_frame);
};


///////////////////////////////////////////////////////////////////////
// Instance class. Derives from CIceObjectBaseShape
///////////////////////////////////////////////////////////////////////
class CIceObjectInstance : public CIceObjectBaseShape
{
public:
   ULONG     m_masterId; // id of the instanced object (object or model)
   // the objects instanced on a point. It's a vector, since the master object can be a model,
   // so we push here all the objects under the model
   vector <CIceObjectBaseShape> m_members; 
   
   CIceObjectInstance()
   {
      m_masterId = 0;
   }

   ~CIceObjectInstance() 
   {
      m_members.clear();
   }

   CIceObjectInstance(const CIceObjectInstance &in_arg) : 
      CIceObjectBaseShape(in_arg), m_masterId(in_arg.m_masterId), m_members(in_arg.m_members)
   {
   }

   // set the id, so the object (or model) id to be ginstanced
   bool SetMasterId(ULONG in_masterId);
   // Create the node
   bool CreateNode(int in_id, const char* in_name, int in_nbTransfKeys);
   // Give the nodes the objects' attributes.
   bool SetNodeData(bool in_setInheritTransform);
   // Call the instance loader
   bool AddShapes(CDoubleArray in_keyFramesTransform, double in_frame, 
                  bool in_hasShapeTime, ShapeHierarchyModeMap &in_shapeHierarchyMap, CRefArray &in_selectedObjs, bool in_selectionOnly, 
                  CIceObjects *in_iceObjects, int in_index, vector <AtNode*> &out_postLoadedNodes);

   // Find the objects to be ginstanced on the point and push them into the members vector
   bool LoadInstance(Model in_modelMaster, X3DObject in_objMaster, CRefArray in_shapeArray, CDoubleArray in_keyFramesTransform, 
                     double in_frame, bool in_hasShapeTime, ShapeHierarchyModeMap &in_shapeHierarchyMap, 
                     CRefArray &in_selectedObjs, bool in_selectionOnly, vector <AtNode*> &out_postLoadedNodes);

   CIceObjectBaseShape LoadProcedural(X3DObject &in_xsiObj, double in_frame, CString in_proceduralPath);
};


///////////////////////////////////////////////////////////////////////
// Strand Instance class. Derives from CIceObjectInstance
///////////////////////////////////////////////////////////////////////
class CIceObjectStrandInstance : public CIceObjectInstance
{
public:
   // The strand. We keep two versions of the strand:
   // strand   == the strand at the frame time, so a "master copy"
   // mbStrand == the same strand modified by the mb for a given time (so modified by StrandVelocity),
   //             so the one over which to bend shapes if def mb is on
   CStrand m_strand, m_mbStrand; 
   vector <CStrandInstance> m_strandInstances; // the bended objects
   vector <AtNode* > m_masterNodes; // the master nodes to be bended
   vector <bool>     m_postLoaded;  // true if the master was post loaded because of time shift

   CIceObjectStrandInstance()
   {}

   ~CIceObjectStrandInstance()
   {
      m_strandInstances.clear();
      m_masterNodes.clear();
      m_postLoaded.clear();
   }

   CIceObjectStrandInstance(const CIceObjectStrandInstance &in_arg) : 
      CIceObjectInstance(in_arg), m_strand(in_arg.m_strand), m_mbStrand(in_arg.m_mbStrand), 
      m_strandInstances(in_arg.m_strandInstances), m_masterNodes(in_arg.m_masterNodes), m_postLoaded(in_arg.m_postLoaded)
   {
   }

   // Get the list of objects to be cloned on the strand
   bool AddStrandShapes(CDoubleArray in_defKeys, double in_frame, 
                        CIceObjects *in_iceObjects, int in_index, float in_secondsPerFrame);
   // Clone the objects on the strand
   bool LoadStrandInstance(Model in_modelMaster, CRefArray in_shapeArray, CDoubleArray in_defKeys, 
                           double in_frame, float in_secondsPerFrame, CIceObjectStrandInstance* in_brotherObjectStrandInstance=NULL);
};


///////////////////////////////////////////////////////////////////////
// CIceObjects class, the home of all the objects built for the ice tree
///////////////////////////////////////////////////////////////////////

// These are two lookup maps to be used to speed up the exporter when the same objects
// must be ginstanced or cloned on strands.
// instances:
typedef map<AtShaderLookupKey, int> InstanceLookupMap;
typedef pair<AtShaderLookupKey, int> InstanceLookupPair;
typedef InstanceLookupMap::iterator InstanceLookupIt;
// strand instances:
typedef map<AtNodeLookupKey, int> StrandInstanceLookupMap;
typedef pair<AtNodeLookupKey, int> StrandInstanceLookupPair;
typedef StrandInstanceLookupMap::iterator StrandInstanceLookupIt;

class CIceObjects
{
public:
   // number of points of the unique pointsSphere node, NOT m_pointsSphere.size, which is always 1 or 0
   int m_pointsSphereNbPoints; 
   // number of points of the unique pointsDisk node, NOT m_pointsDisk.size, which is always 1 or 0
   int m_pointsDiskNbPoints;
   // number of points of the unique rectangle mesh node, NOT m_rectangles.size, which is always 1 or 0
   int m_nbRectangles;
   // the pointsSphere node. Use a vector, although it should be a single element
   vector <CIceObjectPointsSphere> m_pointsSphere;
   // the pointsDisk node. Use a vector, although it should be a single element
   vector <CIceObjectPointsDisk>   m_pointsDisk;
   // the rectangle mesh node. Use a vector, although it should be a single element
   vector <CIceObjectRectangle>    m_rectangles;
   int    m_nbDiscs; // size of discs. Used to resize discs
   // the discs nodes. A node is created for each disc
   vector <CIceObjectDisc> m_discs; 

   int    m_nbBoxes; // size of boxes. Used to resize boxes
   // the boxes nodes. A node is created for each box
   vector <CIceObjectBox> m_boxes; 

   int    m_nbCylinders; // size of cylinders. Used to resize cylinders
   // the cylinders nodes. A node is created for each cylinder
   vector <CIceObjectCylinder> m_cylinders; 

   int    m_nbCones; // size of cones. Used to resize cones
   // the cones nodes. A node is created for each cone
   vector <CIceObjectCone> m_cones; 

   int    m_nbStrands; // number of strands
   vector <CIceObjectStrand> m_strands;

   int    m_nbInstances; // number of instances
   vector <CIceObjectInstance> m_instances;
   // key == the object id. the int is the index of the first instance of that id in instances
   InstanceLookupMap m_instanceMap;

   set <int> m_uncachebleIds;

   // key == the object id. the int is the index of the first strand instance of that id in strandInstances
   StrandInstanceLookupMap m_strandInstanceMap;

   int    m_nbStrandInstances; // number of instanced strands
   vector <CIceObjectStrandInstance> m_strandInstances;

   CIceObjects() 
   {
      m_pointsSphereNbPoints = m_pointsDiskNbPoints = m_nbRectangles =
      m_nbDiscs = m_nbBoxes = m_nbCylinders = m_nbCones = m_nbStrands = 
      m_nbInstances = m_nbStrandInstances = 0;
   }

   CIceObjects(const CIceObjects &in_arg) : 
   	m_pointsSphereNbPoints(in_arg.m_pointsSphereNbPoints), m_pointsDiskNbPoints(in_arg.m_pointsDiskNbPoints),
      m_nbRectangles(in_arg.m_nbRectangles), m_nbDiscs(in_arg.m_nbDiscs), m_nbBoxes(in_arg.m_nbBoxes),
      m_nbCylinders(in_arg.m_nbCylinders), m_nbCones(in_arg.m_nbCones), m_nbStrands(in_arg.m_nbStrands),
      m_nbInstances(in_arg.m_nbInstances), m_nbStrandInstances(in_arg.m_nbStrandInstances)
   {
      // copy the vectors
      m_pointsSphere      = in_arg.m_pointsSphere;
      m_pointsDisk        = in_arg.m_pointsDisk;
      m_rectangles        = in_arg.m_rectangles;
      m_discs             = in_arg.m_discs;
      m_boxes             = in_arg.m_boxes;
      m_cylinders         = in_arg.m_cylinders;
      m_cones             = in_arg.m_cones;
      m_strands           = in_arg.m_strands;
      m_instances         = in_arg.m_instances;
      m_strandInstances   = in_arg.m_strandInstances;
      m_instanceMap       = in_arg.m_instanceMap;
      m_uncachebleIds     = in_arg.m_uncachebleIds;
      m_strandInstanceMap = in_arg.m_strandInstanceMap;
   }

   ~CIceObjects() 
   {
      m_pointsSphere.clear();
      m_pointsDisk.clear();
      m_rectangles.clear();
      m_discs.clear();
      m_boxes.clear();
      m_cylinders.clear();
      m_cones.clear();
      m_strands.clear();
      m_instances.clear();
      m_strandInstances.clear();
      m_instanceMap.clear();
      m_uncachebleIds.clear();
      m_strandInstanceMap.clear();
   }

   // Set the visibility for all the objects
   void SetNodesVisibility(uint8_t in_viz, bool in_arnoldVizExists);
   // Set the sidedness for all the objects
   void SetNodesSidedness(uint8_t in_sid);
   // Set the shaders for all the objects
   void SetNodesShader(AtNode *in_shader);
   // Set the attributes for all the nodes
   void SetNodesData();
   // Get all the nodes into a vector
   vector <AtNode*> GetAllNodes();
   // Multiply all the matrices by the pointcloud matrices
   void MultiplyInstancesByPointCloudMatrices(AtArray* in_pointCloudMatrices);
   // Set the arnold parameters for all the nodes
   void SetArnoldParameters(CustomProperty in_property, double in_frame);
   // Set motion_start, motion_end
   void SetMotionStartEnd();
   // Set the Arnold user options (#680)
   void SetArnoldUserOptions(CustomProperty in_property, double in_frame);
   // Set the blob data (#728)
   void SetUserDataBlobs(const X3DObject &in_xsiObj, double in_frame);
   // Set the matte data for all the nodes
   void SetMatte(Property in_property, double in_frame);
   // Assign the light group to all the nodes
   void SetLightGroup(AtArray* in_lightGroup);
   // return true if at least one of the instanced objects is a light
   bool HasAtLeastOneInstancedLight();
   // Log the number of objects for each type
   void Log();
};


