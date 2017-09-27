/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once
#include "xsi_application.h"
#include <xsi_floatarray.h>
#include <xsi_longarray.h>
#include <xsi_hairprimitive.h>
#include "xsi_math.h"
#include <xsi_renderhairaccessor.h>
#include <xsi_rotationf.h>
#include "xsi_vector3f.h"
#include "xsi_x3dobject.h"

#include <math.h>

#include <ai.h>  

#include <vector>

using namespace std;
using namespace XSI;
using namespace XSI::MATH;

/////////////
// Utilities:
/////////////

// Signed angle from in_v0 to in_v1 around in_axis.
float VectorsSignedAngle(const CVector3f &in_v0, const CVector3f &in_v1, const CVector3f &in_axis);
// Compute the axes from an input rotation
void GetAxesFromRotation(CRotation in_rot, CVector3f *out_x, CVector3f *out_y=NULL, CVector3f *out_z=NULL);
/////////////
// Classes:
/////////////


// CCylMappedPoint class. Represents a point in cylindrical coordinates
//
// @param radius   the radius
// @param height   the normalized height from the cylinder base
// @param angle    the angle around the cylinder axis
//
class CCylMappedPoint
{
public:
   float m_radius, m_height, m_angle;
   CCylMappedPoint() : m_radius(0.f), m_height(0.f), m_angle(0.f)
   {}

   CCylMappedPoint(const CCylMappedPoint &in_arg)
      : m_radius(in_arg.m_radius), m_height(in_arg.m_height), m_angle(in_arg.m_angle)
   {}

   ~CCylMappedPoint()
   {}
};


// CBoundingCylinder class. The bounding cylinder
//
// @param y_min   the minima y
// @param y_max   the maxima y
// @param angle   the height
// @param radius  the radius
//
class CBoundingCylinder
{
public:
   float m_y_min, m_y_max, m_height;
   float m_radius;

   vector<CCylMappedPoint> m_points;

   CBoundingCylinder()
   {
      m_y_min = 100000.0f;
      m_y_max = -100000.0f;
      m_height = m_radius = 0;
   }

   CBoundingCylinder(const CBoundingCylinder& arg)
      : m_y_min(arg.m_y_min), m_y_max(arg.m_y_max), m_height(arg.m_height), 
        m_radius(arg.m_radius), m_points(arg.m_points)
   {}

   ~CBoundingCylinder()
   {
      m_points.clear();
   }

   // Copy the bounding cylinder boundaries
   void CopyBoundaries(const CBoundingCylinder &in_arg);
   // Refine the bounding cylinder by in_v
   void Adjust(const CVector3f &in_v);
   // Remap point in_v from xyz coords to cylindrical ones, and store it as the in_index-th point of the cylinder
   void RemapPoint(const CVector3f &in_v, const int in_index);
   // Get the index-th point of the bounding cylinder. Store the result (radius, haight, angle) into out_result
   bool GetRemappedPoint(CCylMappedPoint *out_result, const int in_index);
};


// if on, the surface normals will be used instead of the first segnemt
// of a strand for orientation. It look like thei are not retured correctly
// so we'll stick with the strand's first segmant
#define USE_SURFACE_NORMALS 0


// CStrand class. Represents a strand (or hair)
//
// @param points            the array of points
// @param lenth             the length of the strand
// @param X                 a point along the x axis, bending along the strand, and stored for each point
// @param weightMapValue    the weighmap value for the strand
// @param tangentMapValue   the tangentmap value for the strand
//
class CStrand
{
public:
   vector<CVector3f>  m_points;
   vector<float>      m_radii;
   vector<CVector3f>  m_vel;
   vector<CRotationf> m_orientation;

   // #1002. the array of mb points, one set per deform mb key;
   vector < vector < CVector3f > > m_mbPoints;

   // vector<CVector3f> m_uvs; // list of uv per strand
   float                  m_length;
   vector<CVector3f>      m_X;
   float                  m_weightMapValue;
   CVector3f              m_tangentMapValue;
#if USE_SURFACE_NORMALS
   CVector3f              m_surfaceNormal;
#endif

   CStrand()
   {
      m_length=0.0f;
   }

   CStrand(const CStrand& arg)
   : m_points(arg.m_points), m_radii(arg.m_radii), m_vel(arg.m_vel), m_orientation(arg.m_orientation), m_mbPoints(arg.m_mbPoints), 
     m_length(arg.m_length), m_X(arg.m_X), m_weightMapValue(arg.m_weightMapValue), m_tangentMapValue(arg.m_tangentMapValue)
   {
      // uvs = arg.uvs;
#if USE_SURFACE_NORMALS
      m_surfaceNormal = arg.m_surfaceNormal;
#endif
   }

   ~CStrand()
   { 
      m_points.clear(); 
      m_radii.clear();
      m_vel.clear();
      m_orientation.clear();
      m_mbPoints.clear();
      // m_uvs.clear(); 
      m_X.clear();
   }

   // Initialize the strand
   void Init(int in_nbPoints, int in_nbRadii=0, int in_nbVel=0, int in_nbOrientation=0, bool in_exactMb=false, int in_nbDeformKeys=0);
	// Set the in_index-th point of the strand by a CVector3f
   bool SetPoint(const CVector3f &in_p, const int in_index);
   // Set the in_index-th point of the strand by x,y,z
   bool SetPoint(const float in_x, const float in_y, const float in_z, const int in_index);
   // Get the in_index-th point of the strand
   bool GetPoint(CVector3f *out_result, const int in_index);
   // #1002:
   // Set the in_index-th point of the strand for the in_key-th mb key
   bool SetMbPoint(const CVector3f &in_p, const int in_index, const int in_key);
   // Get the in_index-th point of the strand for the in_key-th mb key
   bool GetMbPoint(CVector3f *out_result, const int in_index,const int in_key);

   bool SetRadius(float in_r, int index);
   bool GetRadius(float *result, int index);

   bool SetVelocity(const CVector3f &in_v, const int in_index);
   bool GetVelocity(CVector3f *out_result, const int in_index);

   bool SetOrientation(CRotationf in_r, int index);
   bool GetOrientation(CRotation *result, int index);

   // bool SetUv(CVector3f *pV, int index);
   // bool SetUv(float x, float y, float z, int index);
   // bool GetUv(CVector3f *result, int index);

   // Set the 0..1 clamped weighmap value for this strand (used for weightmap assignment)
   void SetWeightMapValue(const float in_w);
   // Get the weighmap value for this strand (used for weightmap assignment)
   float GetWeightMapValue();
   // Set the tangentmap value for this strand (used for orientation) by CVector3f
   void SetTangentMapValue(const CVector3f &in_v);
   // Set the tangentmap value for this strand (used for orientation) by rgb
   void SetTangentMapValue(const float in_r, const float in_g, const float in_b);
   // Get the tangentmap value for this strand (used for orientation)
   CVector3f GetTangentMapValue();
#if USE_SURFACE_NORMALS
   //set surface normal value
   bool SetSurfaceNormal(CVector3f n);
   //set surface normal by x y z
   bool SetSurfaceNormal(float x, float y, float z);
   //get surface normal value
   bool GetSurfaceNormal(CVector3f *result);
#endif
   // Get the direction of the in_index-th segment
   bool GetSegmentDirection(CVector3f *out_result, const int in_index);
   // Get the tangent of the in_index-th point
   bool GetSegmentTangent(CVector3f *out_result, const int in_index);
   // Compute the length of the strand
   void ComputeLength();
   // Return the normalized length-wise height of the in_index-th point
   float ComputeLengthRatio(int in_index);
	// Get the point index on the strand at in_t (0 <= in_t <=1)
   void GetPointindexAlongLength(const float in_t, int *out_index, float *out_remain);
   // Get the position on the strand at in_t (0 <= in_t <=1)
   int  GetPositionByT(CVector3f *out_result, const float in_t);
   // Get the radius on the strand at in_t (0 <= in_t <=1)
   int  GetRadiusByT(float *result, float in_t);
   // Get the tangent on the strand at in_t (0 <= in_t <=1)
   // int  GetTangentByT(CVector3f *out_result, const float in_t);
   // Get the X at in_t (0 <= in_t <=1)
   int  GetXByT(CVector3f *out_result, const float in_t);
   // Compute the bended X axis
   void ComputeBendedX(const bool in_useTangentMap, const float in_oriSpread);
   // Return the x and the tangent directions along the strand, bended at in_pos
   // void ComputeBendedXDirection(CVector3f *out_x, CVector3f *io_tangent, const CVector3f &in_pos, const int in_index);
   // Return the x and the tangent directions along the strand at in_t (0 <= in_t <=1)
   void ComputeBendedXDirectionByT(CVector3f *out_x, CVector3f *io_tangent, const CVector3f &in_pos, const float in_t);
   // Log the strand info
   void Log();
};


// CHair class. A set of strands
//
// @param strands           the array of strands
//
class CHair
{
private:
public:
   vector<CStrand> m_strands;

   CHair()
   {
   }

   CHair(const CHair& arg)
      : m_strands(arg.m_strands)
   {}

   ~CHair()
   {
      m_strands.clear();
   }

   // Allocate the strands vector
   void Init(const int in_nbStrands);
   // Get the number of strands
   int  GetNbStrands();
   // Build from an hair accessor object
   bool BuildFromXsiHairAccessor(const CRenderHairAccessor &in_hairAccessor, 
                                 const CString in_assignmentWeightMapName, 
                                 const CString in_tangentMapName, const float in_oriSpread);
   // Log all the strands
   void Log();
   // bool CHair::BuildTest();
};



// CStrandInstance class. A copy of the shape to be cloned, and its bended version
//
// @param points           the input points of the master object
// @param bendedPoints     the points after bending around a strand
// @param normals          the input normals of the master object
// @param pointAtNormals   index of position of the point of the normal
// @param bendedNormals    the normals after bending around a strand
// @param boundingCylinder the bounding cylinder
// @param masterObject     the xsi object
//
class CStrandInstance
{
private:
public:
   vector<CVector3f>    m_points;
   vector<CVector3f>    m_bendedPoints;
   vector<CVector3f>    m_normals;
   vector<unsigned int> m_pointAtNormals;
   vector<CVector3f>    m_bendedNormals;

   CBoundingCylinder      m_boundingCylinder;
   X3DObject              m_masterObject;

   CStrandInstance()
   {}

   CStrandInstance(const CStrandInstance& arg)
      : m_points(arg.m_points), m_bendedPoints(arg.m_bendedPoints), m_normals(arg.m_normals), 
        m_pointAtNormals(arg.m_pointAtNormals), m_bendedNormals(arg.m_bendedNormals), 
        m_boundingCylinder(arg.m_boundingCylinder), m_masterObject(arg.m_masterObject)
   {
   }

   // Construct by an arnol polymesh
   // CStrandInstance(AtArray *in_vlist, AtArray *in_nlist, AtArray *in_vidxs, AtArray *in_nidxs, 
   //                 const CTransformation in_masterObjTransform, const X3DObject in_masterObj);

   ~CStrandInstance()
   {
      m_points.clear();
      m_bendedPoints.clear();
      m_normals.clear();
      m_pointAtNormals.clear();
      m_bendedNormals.clear();
   }

   // Init by an arnol polymesh
   void Init(AtArray *in_vlist, AtArray *in_nlist, AtArray *in_vidxs, AtArray *in_nidxs, 
             const CTransformation in_masterObjTransform, const X3DObject in_masterObj);

   // Return the vertices and normals TO an arnold shape
   void Get(AtArray *io_vlist, AtArray *io_nlist, const unsigned int in_defKey);
   // Compute the bounding cylinder for this object
	void ComputeBoundingCylinder();
   // Compute and store the bounding cylinder of an array of CStrandInstances
	void ComputeModelBoundingCylinder(vector <CStrandInstance> in_strandInstances);
   // Remap the points to cylindrical coordinates
	void RemapPointsToCylinder();
   // Bend the vertices and normals along a strand.
   void BendOnStrand(CStrand &in_strand);
};




