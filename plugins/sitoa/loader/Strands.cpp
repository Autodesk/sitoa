/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "loader/Strands.h"
#include "renderer/Renderer.h"

// Return the signed angle from in_v0 to in_v1 around in_axis.
//
// @param in_v0     The first vector
// @param in_v1     The second vector
// @param in_axis   The axis of rotation, defining the positive (counter clock wise) orientation
// @return the angle
//
float VectorsSignedAngle(const CVector3f &in_v0, const CVector3f &in_v1, const CVector3f &in_axis)
{
   // make a copy
	CVector3f v0(in_v0), v1(in_v1);

	if (v0.NormalizeInPlace()!=CStatus::OK || v1.NormalizeInPlace()!=CStatus::OK)
		return 0.0f;	//whatever
	float dot = v0.Dot(v1);

	if (dot >= 1.0f)
		return 0.0f;

	float result = acos(dot);

	CVector3f cross;
	cross.Cross(in_v0, v1);
	dot = cross.Dot(in_axis);
	if (dot < 0.0f)
		result = (float)AI_PITIMES2 - result;
	return result;
}


// Compute the axes from an input rotation
//
// @param in_rot    The input crotation
// @param out_x     The returned x axis
// @param out_y     The returned y axis if argument not null
// @param out_z     The returned z axis if argument not null
//
// @return void
//
void GetAxesFromRotation(CRotation in_rot, CVector3f *out_x, CVector3f *out_y, CVector3f *out_z)
{
   CMatrix3 m = in_rot.GetMatrix();
   CVector3 v(1.0, 0.0, 0.0);
   v.MulByMatrix3InPlace(m);
   //GetMessageQueue()->LogMsg(L"X = " + (CString)v.GetX() + L" " + (CString)v.GetY() + L" " + (CString)v.GetZ());
   out_x->Set((float)v.GetX(), (float)v.GetY(), (float)v.GetZ());

   // out_y can be a null pointer , meaning that only the x axis is required by the caller
   if (!out_y)
      return;

   v.Set(0.0, 1.0, 0.0);
   v.MulByMatrix3InPlace(m);
   // GetMessageQueue()->LogMsg(L"Y = " + (CString)v.GetX() + L" " + (CString)v.GetY() + L" " + (CString)v.GetZ());
   out_y->Set((float)v.GetX(), (float)v.GetY(), (float)v.GetZ());

   v.Set(0.0, 0.0, 1.0);
   v.MulByMatrix3InPlace(m);
   //GetMessageQueue()->LogMsg(L"Z = " + (CString)v.GetX() + L" " + (CString)v.GetY() + L" " + (CString)v.GetZ());
   out_z->Set((float)v.GetX(), (float)v.GetY(), (float)v.GetZ());
}


// Intersection of the ray(in_org, in_dir) and the plane(in_p, in_n).
// Negative intersections returned as well. N must be normalized.
//
// @param in_p         A point on the plane
// @param in_n         The plane normal
// @param in_org       The ray origin
// @param in_dir       The ray direction
// @param out_result   The output intersection
//
// @return true if an intersection was found, else false
//
static bool RayPlaneIntersection(CVector3f &in_p, CVector3f &in_n, CVector3f &in_org, CVector3f &in_dir, CVector3f *out_result)
{
   float denom = in_n.Dot(in_dir);
   if (fabs(denom) == 0.0)
	   return false;

   float dot = in_n.Dot(in_org);
   float D = -in_p.Dot(in_n);
   float t = - (dot + D) / denom;

   out_result->Scale(t, in_dir);
   out_result->AddInPlace(in_org);
   return true;
}



///////////////////////////////////////////////
///////////////////////////////////////////////

// Copy the bounding cylinder boundaries
//
// @param in_arg   The source bounding cylinder
// @return void
//
void CBoundingCylinder::CopyBoundaries(const CBoundingCylinder &in_arg)
{
   m_y_min = in_arg.m_y_min;
   m_y_max = in_arg.m_y_max;
   m_height = in_arg.m_height;
   m_radius = in_arg.m_radius;
}


// Refine the bounding cylinder by in_v
//
// @param in_v   The point to be "added" to the bounding cylinder computation
// @return void
//
void CBoundingCylinder::Adjust(const CVector3f &in_v)
{
   if (in_v.GetY() < m_y_min) 
      m_y_min = in_v.GetY();
   if (in_v.GetY() > m_y_max) 
      m_y_max = in_v.GetY();
   m_height = m_y_max - m_y_min;
   float r = sqrtf(in_v.GetX()*in_v.GetX() + in_v.GetZ()*in_v.GetZ());
   if (r > m_radius) 
      m_radius = r;
}


// Remap point in_v from xyz coords to cylindrical ones, and store it as the in_index-th point of the cylinder
//
// @param in_v   The point to be "added" to the bounding cylinder computation
// @return void
//
void CBoundingCylinder::RemapPoint(const CVector3f &in_v, const int in_index)
{
   // Compute the radius and the 0..1 height
   m_points[in_index].m_radius = sqrtf(in_v.GetX()*in_v.GetX() + in_v.GetZ()*in_v.GetZ());
   // protect against flat masters (height==0), #1151
   m_points[in_index].m_height = m_height > 0.001f ? (in_v.GetY() - m_y_min) / m_height : 0.0f;

   if (m_points[in_index].m_radius == 0.0f)
   {
      m_points[in_index].m_angle = 0.0f;
      return;
   }

   // Compute the angle around the cylinder axis
   CVector3f x(1.0f, 0.0f, 0.0f);
   CVector3f p(in_v.GetX(), 0.0f, in_v.GetZ()); //cut off y coord to compute angle around y
   CVector3f y(0.0f, 1.0f, 0.0f);
   m_points[in_index].m_angle = VectorsSignedAngle(x, p, y);
}


// Get the index-th point of the bounding cylinder. Store the result (radius, haight, angle) into out_result
//
// @param out_result   The cyl-mapped point result
// @param index        The index of the point to be returned
// @return false if in_index is out of range 
//
bool CBoundingCylinder::GetRemappedPoint(CCylMappedPoint *out_result, const int in_index)
{
   if (in_index >= (int)m_points.size())
      return false;
   *out_result = m_points[in_index];
   return true;
}



//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

// Initialize the strand
//
// @param in_nbPoints       Points count for this strand
// @param in_nbRadii        Radii count 
// @param in_nbVel          Velocities count
// @param in_nbOrientation  Orientation vectors count
// @param in_exactMb        True if this strand will be exact-mblurred
// @param in_nbDeformKeys   Count of the deform mb keys
//
// @return void
//
void CStrand::Init(int in_nbPoints, int in_nbRadii, int in_nbVel, int in_nbOrientation, bool in_exactMb, int in_nbDeformKeys)
{ 
   if (in_nbPoints < 2)
      in_nbPoints = 2;
   m_points.resize(in_nbPoints);
   if (in_nbRadii > 0)
      m_radii.resize(in_nbRadii);
   if (in_nbVel > 0)
      m_vel.resize(in_nbVel);
   if (in_nbOrientation > 0)
      m_orientation.resize(in_nbOrientation);
   // m_uvs.resize(nbUvs);
   m_X.resize(in_nbPoints);

   if (in_exactMb)
   {
      m_mbPoints.resize(in_nbDeformKeys);
      for (int i=0; i<in_nbDeformKeys; i++)
         m_mbPoints[i].resize(in_nbPoints);
   }
}


// Set the in_index-th point of the strand by a CVector3f
//
// @param in_p      The point to be set
// @param in_index  The index of the point along the strand (must not exceed the strand's numner of points)
// @return false if in_index is out of range 
//
bool CStrand::SetPoint(const CVector3f &in_p, const int in_index)
{
   if (in_index >= (int)m_points.size()) 
      return false;
   m_points[in_index] = in_p;
   return true;
}


// Set the in_index-th point of the strand by x,y,z
//
// @param in_p      The point to be set
// @param in_index  The index of the point along the strand (must not exceed the strand's numner of points)
// @return false if in_index is out of range 
//
bool CStrand::SetPoint(const float in_x, const float in_y, const float in_z, const int in_index)
{
   if (in_index >= (int)m_points.size()) 
      return false;
   m_points[in_index].Set(in_x, in_y, in_z);
   return true;
}


// Get the in_index-th point of the strand
//
// @param out_result  The returned point
// @param in_index    The index of the point along the strand (must not exceed the strand's numner of points)
// @return false if in_index is out of range 
//
bool CStrand::GetPoint(CVector3f *out_result, const int in_index)
{
   if (in_index >= (int)m_points.size()) 
      return false;
   *out_result = m_points[in_index];
   return true;
}


// Set the in_index-th point of the strand for the in_key-th mb key
//
// @param in_p      The point to be set
// @param in_index  The index of the point along the strand (must not exceed the strand's numner of points)
// @param in_key    The mb key (must not exceed the strand's number of mb arrays)
//
// @return false if in_index is out of range 
//
bool CStrand::SetMbPoint(const CVector3f &in_p, const int in_index, const int in_key)
{
   if (in_key >= (int)m_mbPoints.size())
      return false;
   if (in_index >= (int)m_mbPoints[in_key].size()) 
      return false;
   m_mbPoints[in_key][in_index] = in_p;
   return true;
}


// Get the in_index-th point of the strand for the in_key-th mb key
//
// @param out_result  The returned point
// @param in_index    The index of the point along the strand (must not exceed the strand's number of points)
// @param in_key      The mb key (must not exceed the strand's number of mb arrays)
//
// @return false if in_index is out of range 
//
bool CStrand::GetMbPoint(CVector3f *out_result, const int in_index, const int in_key)
{
   if (in_key >= (int)m_mbPoints.size())
      return false;
   if (in_index >= (int)m_mbPoints[in_key].size()) 
      return false;
   *out_result = m_mbPoints[in_key][in_index];
   return true;
}


// Set the in_index-th velocity of the strand by a CVector3f
//
// @param in_p      The velocity to be set
// @param in_index  The index of the point along the strand (must not exceed the strand's numner of points)
// @return false if in_index is out of range 
//
bool CStrand::SetVelocity(const CVector3f &in_v, const int in_index)
{
   if (in_index >= (int)m_vel.size()) 
      return false;
   m_vel[in_index] = in_v;
   // test
   // float x, y, z;
   // in_v.Get(x, y, z);
   return true;
}


// Get the in_index-th velocity of the strand
//
// @param out_result  The returned velocity
// @param in_index    The index of the point along the strand (must not exceed the strand's numner of points)
// @return false if in_index is out of range 
//
bool CStrand::GetVelocity(CVector3f *out_result, const int in_index)
{
   if (in_index >= (int)m_vel.size()) 
      return false;
   *out_result = m_vel[in_index];
   return true;
}


//set index-th radius
bool CStrand::SetRadius(float in_r, int index)
{
   if (index >= (int)m_radii.size()) 
      return false;
   m_radii[index] = in_r;
   return true;
}


//get index-th radius
bool CStrand::GetRadius(float *result, int index)
{
   if (index >= (int)m_radii.size()) 
      return false;
   *result = m_radii[index];
   return true;
}


//set index-th orientation
bool CStrand::SetOrientation(CRotationf in_r, int index)
{
   if (index >= (int)m_orientation.size()) 
      return false;
   m_orientation[index] = in_r;
   return true;
}


//get index-th orientation
bool CStrand::GetOrientation(CRotation *out_result, int index)
{
   if (index >= (int)m_orientation.size()) 
      return false;

   CRotationf rotf = m_orientation[index];
   *out_result = CIceUtilities().RotationfToRotation(rotf);
   return true;
}

/*
//set index-th uv
bool CStrand::SetUv(CVector3f *pV, int index)
{
   if (index >= (int)uvs.size()) 
      return false;
   uvs[index] = *pV;
   return true;
}


//set index-th uv by xyz
bool CStrand::SetUv(float x, float y, float z, int index)
{
   if (index >= (int)uvs.size()) 
      return false;
   uvs[index].Set(x, y, z);
   return true;
}


//get index-th uv
bool CStrand::GetUv(CVector3f *result, int index)
{
   if (index >= (int)uvs.size()) 
      return false;
   *result = uvs[index];
   return true;
}
*/


// Set the 0..1 clamped weighmap value for this strand (used for weightmap assignment)
//
// @param in_w  The input wm value
// @return void 
//
void CStrand::SetWeightMapValue(const float in_w)
{
   m_weightMapValue = in_w;
   // Let's clamp between 0 and 1, so the assigmnent routine is simplified
   if (m_weightMapValue < 0.0f)
      m_weightMapValue = 0.0f;
   else if (m_weightMapValue > 1.0f)
      m_weightMapValue = 1.0f;
}

// Get the weighmap value for this strand (used for weightmap assignment)
//
// @return the weightmap value 
//
float CStrand::GetWeightMapValue()
{
   return m_weightMapValue;
}


// Set the tangentmap value for this strand (used for orientation) by CVector3f
//
// @param in_v  The input tm value
// @return void 
//
void CStrand::SetTangentMapValue(const CVector3f &in_v)
{
   m_tangentMapValue = in_v;
}


// Set the tangentmap value for this strand (used for orientation) by rgb
//
// @param in_r  The input red value
// @param in_g  The input green value
// @param in_b  The input blu value
// @return void 
//
void CStrand::SetTangentMapValue(const float in_r, const float in_g, const float in_b)
{
   // 0..1 to -1..1
   CVector3f n((in_r - 0.5f) * 2.0f, (in_g - 0.5f) * 2.0f, (in_b - 0.5f) * 2.0f);
   m_tangentMapValue.Normalize(n);
}


// Get the tangentmap value for this strand (used for orientation)
//
// @return the tangentmap value 
//
CVector3f CStrand::GetTangentMapValue()
{
   return m_tangentMapValue;
}


#if USE_SURFACE_NORMALS
//set surface normal value
bool CStrand::SetSurfaceNormal(CVector3f n)
{
   m_surfaceNormal = n;
   return true;
}

//set surface normal value by xyz
bool CStrand::SetSurfaceNormal(float x, float y, float z)
{
   m_surfaceNormal.Set(x, y, z);
   return true;
}


//get surface normal value
bool CStrand::GetSurfaceNormal(CVector3f *result)
{
   *result = m_surfaceNormal;
   return true;
}
#endif


// Get the direction of the in_index-th segment
//
// @param out_result  The normalized direction
// @param in_index    The index of the segment
// @return false if in_index is out of range 
//
bool CStrand::GetSegmentDirection(CVector3f *out_result, const int in_index)
{
   int nbPoints = (int)m_points.size();
   if (in_index >= nbPoints) 
      return false;
   if (in_index == nbPoints-1) //special case, equals index-1
      out_result->Sub(m_points[in_index], m_points[in_index-1]);
   else
      out_result->Sub(m_points[in_index+1], m_points[in_index]);

   out_result->NormalizeInPlace();
   return true;
}


// Get the tangent of the in_index-th point. With tangent (nothing to do with the tangent map)
// we mean the average of the directions of the two segments sharing the point
//
// @param out_result  The normalized direction
// @param in_index    The index of the point
// @return false if in_index is out of range 
//
bool CStrand::GetSegmentTangent(CVector3f *out_result, const int in_index)
{
   int nbPoints = (int)m_points.size();
   if (in_index >= nbPoints) 
      return false;
   if ((in_index == 0) || (in_index == nbPoints-1))
      return GetSegmentDirection(out_result, in_index);

   out_result->Sub(m_points[in_index+1], m_points[in_index-1]);
   out_result->NormalizeInPlace();
   return true;
}


// Compute the length of the strand
void CStrand::ComputeLength()
{
   CVector3f v;
   m_length = 0.0f;
   for (int i=0; i<(int)m_points.size()-1; i++)
   {
      v.Sub(m_points[i+1], m_points[i]);
      m_length+= v.GetLength();
   }
}


// Return the normalized length-wise height of the in_index-th point
//
// @param in_index    The index of the point
// @return the normalized (0..1) length-wise height of the in_index-th point  
//
float CStrand::ComputeLengthRatio(int in_index)
{
   CVector3f v;
   if (in_index > (int)m_points.size())
      in_index = (int)m_points.size();
   float l = 0.0f;
   for (int i=0; i<in_index; i++)
   {
      v.Sub(m_points[i+1], m_points[i]);
      l+= v.GetLength();
   }
   return l/m_length;
}


// Given a in_t between 0 and 1, return the corresponding point index, and the t
// difference from the in_t the the returned point's t
// in_t is a continous value, so in general it will always point between to points
// of the strand. So, we return the index of the point "below" t, and tha ratio between
// the remains of t (past the point's t) and the next segment's length
// 
// Say for instance we have 4 points (3 segments): 
// in_t == 0.5 -> out_index = 1, out_remain = 0.5 
// because in_t is between point 1 and 2, and exactly in the middle of them
//
// @param in_t           The length-wise t where to evaluate the strand
// @param out_index      The index of the corresponding point of the strand
// @param out_remain     The difference between in_t and returned point's t, norlalized against the segment's length
// @return void  
// 
void CStrand::GetPointindexAlongLength(const float in_t, int *out_index, float *out_remain)
{
   if (in_t <= 0.0f) 
   {
      *out_index = 0; //root
      *out_remain = 0.0f;
      return;
   }
   if (in_t >= 1.0f)
   {
      *out_index = (int)m_points.size() - 1; //last
      *out_remain = 0.0f;
      return;
   }

   float l, accL=0.0f;
   CVector3f v;

   for (int i=0; i<(int)m_points.size()-1; i++)
   {
      v.Sub(m_points[i+1], m_points[i]);
      l = v.GetLength() / m_length; //dist to next point
      if ( (accL + l) > in_t) //distance to (i+1)-th point would overtake t
      {
         *out_index = i;
         float remainingT = in_t - accL; //remaining t
         *out_remain = remainingT / l; //ratio against segment
         //should always be 0 < remain < 1
         // if (*out_remain > 1.0f) //test
         //    *out_remain = *out_remain;
         break;
      }
      accL+=l;
   }
}


// Get the position on the strand at in_t (0 <= in_t <=1)
// @param in_t           The length-wise t where to evaluate the strand
// @param out_result     The returned position
// @return the index of the segment where out_result lies
// 
int CStrand::GetPositionByT(CVector3f *out_result, const float in_t)
{
   int index;
   float  remain;
   GetPointindexAlongLength(in_t, &index, &remain);
   if ((unsigned)index == m_points.size() - 1)
   {
      *out_result = m_points[m_points.size() - 1];
      return index;
   }
   CVector3f d;
   d.Sub(m_points[index+1], m_points[index]);
   d*=remain;
   out_result->Add(m_points[index], d);
   return index;
}


// Get the radius on the strand at in_t (0 <= in_t <=1)
// @param in_t           The length-wise t where to evaluate the strand
// @param out_result     The returned radius
// @return the index of the segment where out_result lies
// 
int CStrand::GetRadiusByT(float *result, float in_t)
{
   int index;
   float  remain;
   GetPointindexAlongLength(in_t, &index, &remain);
   if (index == (int(m_radii.size()) - 1))
   {
      *result = m_radii[m_points.size() - 1];
      return index;
   }
   float d = m_radii[index+1] - m_radii[index];
   d*=remain;
   *result = m_radii[index] + d;
   return index;
}


// Get the tangent on the strand at in_t (0 <= in_t <=1)
// @param in_t           The length-wise t where to evaluate the strand
// @param out_result     The returned tangent
// @return the index of the segment where in_t lies
// 
/*
int CStrand::GetTangentByT(CVector3f *out_result, const float in_t)
{
   int index;
   float  remain;
   GetPointindexAlongLength(in_t, &index, &remain);
   if ((index == 0) || ((unsigned)index == points.size() - 1)) //root or tip
   {
      GetSegmentTangent(out_result, index);
      return index;
   }
   CVector3f r0, r1;
   GetSegmentTangent(&r0, index);
   GetSegmentTangent(&r1, index+1);

   // isn't this wrong? in_t is along the full strand, not between index and index+1
   // However, this method is unused
   out_result->LinearlyInterpolate(r0, r1, in_t);
   out_result->NormalizeInPlace();
   return index;
}
*/


// Get the X at in_t (0 <= in_t <=1). X is not the bended axis, it's a point along such a direction
// @param in_t           The length-wise t where to evaluate the strand
// @param out_result     The returned point
// @return the index of the segment where in_t lies
// 
int CStrand::GetXByT(CVector3f *out_result, const float in_t)
{
   int index;
   float  remain;
   GetPointindexAlongLength(in_t, &index, &remain);
   if ((unsigned)index == m_points.size() - 1)
   {
      *out_result = m_X[m_points.size() - 1];
      return index;
   }
   CVector3f d;
   d.LinearlyInterpolate(m_X[index], m_X[index+1], remain);
   *out_result = d;
   return index;
}


// Take a point on the the x axis side at the strand root and walk it up and up along the strand points.
// The safer, although surely not fast, way, is to start taking a point along the x axis at the
// root of the strand, and trace a ray from it along the first segment direction, until it
// crosses the plane defined by the next segment start point and its tangent.
// This will give us the x axis bended at the height of the beginning of the second segment.
// Then repeat for the third segment etc, until we reach the index-th segment.
// All the x axes are stored together with the strand
//
// @param in_useTangentMap   If to use the tangent map
// @param in_oriSpread       The orientation spread angle, if in_useTangentMap is true;
//
// @return void
// 
void CStrand::ComputeBendedX(const bool in_useTangentMap, const float in_oriSpread)
{
	CVector3f dir, v, n, newV, hDir;
   CVector3f rotX, rotY, rotZ, previousRotX; // the axes representation of the orientations
   CRotation rot;

#if USE_SURFACE_NORMALS
   dir = surfaceNormal; surface normals are broken
#else
	GetSegmentTangent(&dir, 0); 
#endif

   if (in_useTangentMap)
   {
      // rotate tangentMapValue by origSpread around surfaceNormal
      if (in_oriSpread!=0.0f && in_oriSpread!=360.0f)
      {
         CVector3 x3;
         CTransformation trans;
         CVector3 hy3(dir.GetX(), dir.GetY(), dir.GetZ());
         CVector3 hx3(m_tangentMapValue.GetX(), m_tangentMapValue.GetY(), m_tangentMapValue.GetZ());
         trans.SetRotationFromAxisAngle(hy3, in_oriSpread);   
         CMatrix3 m3 = trans.GetRotationMatrix3();
         x3.MulByMatrix3(hx3, m3);
         v.Set((float)x3.GetX(), (float)x3.GetY(), (float)x3.GetZ());
      }
      else
         v = m_tangentMapValue;
   }
   else
   {
      if (m_orientation.size() > 0)
      {
         GetOrientation(&rot, 0);
         GetAxesFromRotation(rot, &rotX, &rotY, &rotZ);
         previousRotX = rotX;
         v = rotX;
      }
      else // compute the x axis as the cross between the strand y and the global z
      {
         CVector3f Z(0.0f, 0.0f, 1.0f);
         v.Cross(dir, Z);
      }
   }

	v.SetLength(0.01f); // keep it side by side with the strand
	v+= m_points[0];
   m_X[0] = v; // initial point along the x axis
   
   // now repeat until the index-th point of the strand
	for (int i=1; i<(int)m_points.size(); i++)
	{
		GetSegmentTangent(&dir, i); // averaged direction of the i-th segment
		GetSegmentDirection(&hDir, i-1); // direction of the previous segment
      // points[i] and dir define the plane
      // find the intersection of ray leaving from v along the hDir dir with the above plane
		if (RayPlaneIntersection(m_points[i], dir, v, hDir, &newV))
      {
         // set the length to 0.01
		   n.Sub(newV, m_points[i]);
		   n.SetLength(0.01f);

         if (m_orientation.size() > 0)
         {
            // get the orientation at this point of the strand
            GetOrientation(&rot, i);
            // and the corresponding X axis
            GetAxesFromRotation(rot, &rotX);
            // difference with the previous x axis, so the rotation increment
            float angleDiff = VectorsSignedAngle(previousRotX, rotX, rotY);
            // save the current axis for the next point of the strand
            previousRotX = rotX;

            if (fabs(angleDiff) > 0.001f)
            {
               // now rotate n, so our local x axis of angleDiff around the strand direction
               // we have to go double to use CTransformation
               CTransformation trans;
               CVector3 dir3((double)dir.GetX(), (double)dir.GetY(), (double)dir.GetZ());
               trans.SetRotationFromAxisAngle(dir3, angleDiff);   
               // get the matrix to use
               CMatrix3 m3 = trans.GetRotationMatrix3();
               // vector to be rotated
               CVector3 n3((double)n.GetX(), (double)n.GetY(), (double)n.GetZ());
               n3.MulByMatrix3InPlace(m3);               
               // done, give it back to n
               n.Set((float)n3.GetX(), (float)n3.GetY(), (float)n3.GetZ());
            }
         }

         v.Add(m_points[i], n);
         m_X[i] = v;
      }
      else // ray intersection went wrong for some reason
      {
         // then copy the X from the previous point
         n.Sub(m_X[i-1], m_points[i-1]);
		   v.Add(m_points[i], n);
         m_X[i] = v;
      }
	}
}


// Return the x and the tangent directions along the strand, bended at in_pos
// 
// @param out_x      The bended x direction
// @param io_tangent The bended tangent direction (orthogonal to out_x, and along the strand direction)
// @param in_pos     The point on the strand
// @param in_index   The segment index of in_pos
// @return void
// 
/*
void CStrand::ComputeBendedXDirection(CVector3f *out_x, CVector3f *io_tangent, const CVector3f &in_pos, const int in_index)
{
	CVector3f x, z;

   x.Sub(X[in_index], in_pos);
	// recompute the proper coords system
	z.Cross(x, *io_tangent);
	x.Cross(*io_tangent, z);
   out_x->Normalize(x);
}
*/

// Return the x and the tangent directions along the strand at in_t (0 <= in_t <=1)
// 
// @param out_x      The bended x direction
// @param io_tangent The bended tangent direction (orthogonal to out_x, and along the strand direction)
// @param in_pos     The point on the strand
// @param in_t       The length-wise t where to evaluate the strand
// @return void
// 
void CStrand::ComputeBendedXDirectionByT(CVector3f *out_x, CVector3f *io_tangent, const CVector3f &in_pos, const float in_t)
{
	CVector3f x, z, tX;

   this->GetXByT(&tX, in_t);
   x.Sub(tX, in_pos);
	// recompute the proper coords system
	z.Cross(x, *io_tangent);
	x.Cross(*io_tangent, z);
   out_x->Normalize(x);
}


// Log the strand info
void CStrand::Log()
{
   int nbPoints = (int)m_points.size();
   GetMessageQueue()->LogMsg(L" NbPoints = " + CValue(nbPoints).GetAsText());
   for (int i=0; i<nbPoints; i++)
   {
      GetMessageQueue()->LogMsg("  p[" + CValue(i).GetAsText() + L"]=" + CValue(m_points[i].GetX()).GetAsText() + L" " + CValue(m_points[i].GetY()).GetAsText() + L" " + CValue(m_points[i].GetZ()).GetAsText());
      // GetMessageQueue()->LogMsg("  r[" + CValue(i).GetAsText() + L"]=" + CValue(radii[i]).GetAsText());
   }
   int nbOrientation = (int)m_orientation.size();
   GetMessageQueue()->LogMsg(L" NbOrientation = " + CValue(nbOrientation).GetAsText());
   CRotation rot;
   for (int i=0; i<nbOrientation; i++)
   {
      float x, y, z;
      m_orientation[i].GetXYZAngles(x, y, z);
      GetMessageQueue()->LogMsg("  ori[" + CValue(i).GetAsText() + L"]=" + CValue(x).GetAsText() + 
                               L" " + CValue(y).GetAsText() + 
                               L" " + CValue(z).GetAsText());
   }
}



////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
// CHair class: a set of strands
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

// Allocate the strands vector
// 
// @param in_nbStrands   The number of strands
// @return void
// 
void CHair::Init(const int in_nbStrands)
{
   m_strands.resize(in_nbStrands);
}


// Get the number of strands
// 
// @return the number of strands
// 
int CHair::GetNbStrands()
{ 
   return (int)m_strands.size(); 
}


// Build from an hair accessor object.
// 
// @param in_hairAccessor              The hair accessor
// @param in_assignmentWeightMapName   The assignement weightmap name
// @param in_tangentMapName            The orientation tangentmap name
// @param in_oriSpread                 The orientation spread angle
// @return if successfull or not
// 
bool CHair::BuildFromXsiHairAccessor(const CRenderHairAccessor &in_hairAccessor, 
                                     const CString in_assignmentWeightMapName, 
                                     const CString in_tangentMapName, const float in_oriSpread)
{
   int nbStrands = in_hairAccessor.GetChunkHairCount();
	this->Init(nbStrands);

   CLongArray verticesCountArray;
   in_hairAccessor.GetVerticesCount(verticesCountArray);
   int nbVerticesPerStrand = (int)verticesCountArray[0];

   CFloatArray verticesPositions;
   in_hairAccessor.GetVertexPositions(verticesPositions);

   // Get the render hair radius
   /*
   CFloatArray verticesRadii;
   hairAccessor.GetVertexRadiusValues(verticesRadii);
   int nbRadii = (int)verticesRadii.GetCount();

   if (nbVerticesPerStrand != nbRadii/nbStrands)
   {
      GetMessageQueue()->LogMsg(L"Mismatch: nbVerticesPerStrand = " + (CString)nbVerticesPerStrand + L", nbRadii per strand = " + (CString)(nbRadii/nbStrands), siErrorMsg);
      return false;
   }
   */

   // int nbUv = hairAccessor.GetUVCount() == -1 ? 0 : hairAccessor.GetUVCount();

   int nbWeightMaps = (int)in_hairAccessor.GetWeightMapCount();
   int nbVertexColorMaps = (int)in_hairAccessor.GetVertexColorCount();;

   int index;
   for (int i=0; i<nbStrands; i++)
   {
      // just 1 or 0 weight and tangent map for assignment
      m_strands[i].Init(nbVerticesPerStrand); 
      for (int j=0; j<nbVerticesPerStrand; j++)
      {
         // position
         index = (i * nbVerticesPerStrand + j) * 3;
         m_strands[i].SetPoint(verticesPositions[index], verticesPositions[index+1], verticesPositions[index+2], j);
         // radius
         index = (i * nbVerticesPerStrand + j);
         // strands[i].SetRadius(verticesRadii[index], j);
      }
   }

   /*
   for (int uvIndex=0; uvIndex<nbUv; uvIndex++)
   {
      CFloatArray uvValues;
      hairAccessor.GetUVValues(uvIndex, uvValues);
      int uvCount = uvValues.GetCount();
      if (nbStrands != uvCount*3)
      {
         GetMessageQueue()->LogMsg(L"Mismatch: strands = " + (CString)nbStrands + L", uvs = " + (CString)uvCount, siErrorMsg);
         return false;
      }
      for (int i=0; i<nbStrands; i++)
         strands[i].SetUv(uvValues[i*3], uvValues[i*3+1], uvValues[i*3+2], uvIndex);
   }
   */

   // set the weightmap (if any) for the object assignment
   if (nbWeightMaps > 0 && in_assignmentWeightMapName != L"")
   {
      // loop all the hair wm
      for (int wmIndex=0; wmIndex<nbWeightMaps; wmIndex++)
      {
         if (in_hairAccessor.GetWeightMapName(wmIndex) != in_assignmentWeightMapName)
            continue;
         // GetMessageQueue()->LogMsg("weighMapName = " + hairAccessor.GetWeightMapName(wmIndex));
         CFloatArray weightMapValues;
         in_hairAccessor.GetWeightMapValues(wmIndex, weightMapValues);
         for (int i=0; i<nbStrands; i++)
         {
            // GetMessageQueue()->LogMsg(L"wm at " + (CString)i + " = " + (CString)weightMapValues[i]);
            m_strands[i].SetWeightMapValue(weightMapValues[i]);
         }
         break;
      }
   }

   // set the tangentmap (if any) for the object orientation
   bool useTangentMap = false;
   if (nbVertexColorMaps > 0 && in_tangentMapName != L"")
   {
      // loop all the hair wm
      for (int tgIndex=0; tgIndex<nbVertexColorMaps; tgIndex++)
      {
         if (in_hairAccessor.GetVertexColorName(tgIndex) != in_tangentMapName)
            continue;
         // GetMessageQueue()->LogMsg("weighMapName = " + hairAccessor.GetWeightMapName(wmIndex));
         CFloatArray tangentMapValues;
         in_hairAccessor.GetVertexColorValues(tgIndex, tangentMapValues);
         for (int i=0; i<nbStrands; i++)
         {
            float r = tangentMapValues[i*4];
            float g = tangentMapValues[i*4+1];
            float b = tangentMapValues[i*4+2];
            // GetMessageQueue()->LogMsg(L"tg at " + (CString)i + " = " + (CString)tangentMapValues[i*4]);
            // the vertex map should return rgba, we just care of rgb (right?)
            m_strands[i].SetTangentMapValue(r, g, b);
         }
         useTangentMap = true;
         break;
      }
   }

#if USE_SURFACE_NORMALS
   // surface normals
   CFloatArray surfaceNormalValues;
   hairAccessor.GetHairSurfaceNormalValues(surfaceNormalValues);
   /* test
   GetMessageQueue()->LogMsg("Count = " + (CString)surfaceNormalValues.GetCount());
   GetMessageQueue()->LogMsg("Normal[0] = " + (CString)surfaceNormalValues[0] + 
                                          L" " + (CString)surfaceNormalValues[1] + 
                                          L" " + (CString)surfaceNormalValues[2]);
   */
   for (int i=0; i<nbStrands; i++)
      strands[i].SetSurfaceNormal(surfaceNormalValues[i*3], surfaceNormalValues[i*3+1], surfaceNormalValues[i*3+2]);
#endif

   for (int i=0; i<GetNbStrands(); i++)
   {
      m_strands[i].ComputeLength();
      m_strands[i].ComputeBendedX(useTangentMap, in_oriSpread);
   }

   return true;
}



// Log all the strands
void CHair::Log()
{
   for (int i=0; i<GetNbStrands(); i++)
   {
      GetMessageQueue()->LogMsg(L"Strand #" + CValue(i).GetAsText());
      m_strands[i].Log();
   }
}


/*
bool CHair::BuildTest()
{
   int nbStrands = 1;
	this->Init(nbStrands);
   strands[0].Init(2, 0, 0, 0);
   strands[0].SetPoint(0.0f, 0.0f, 0.0f, 0);
   strands[0].SetPoint(0.0f, 1.0f, 0.0f, 1);
   return true;
}
*/


////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
// CStrandInstance class: a copy of the shape to be cloned, and its bended version 
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

// Init by an arnol polymesh
// 
// @param in_vlist               The vertices array
// @param in_nlist               The normals array
// @param in_vidxs               The vertices indeces
// @param in_nidxs               The normals indeces
// @param in_masterObjTransform  The transformation to apply to the vertices
// @param in_masterObj           The master object
// @return void
// 
void CStrandInstance::Init(AtArray *in_vlist, AtArray *in_nlist, AtArray *in_vidxs, AtArray *in_nidxs, 
                           const CTransformation in_masterObjTransform, const X3DObject in_masterObject)
{
   m_masterObject = in_masterObject;
   m_points.resize(AiArrayGetNumElements(in_vlist));
   m_bendedPoints.resize(AiArrayGetNumElements(in_vlist));
   if (in_nlist)
   {
      m_normals.resize(AiArrayGetNumElements(in_nlist));
      m_pointAtNormals.resize(AiArrayGetNumElements(in_nlist));
      m_bendedNormals.resize(AiArrayGetNumElements(in_nlist));
   }

   AtVector point;
   AtVector normal;
   // set the master points

   for (unsigned int i=0; i<AiArrayGetNumElements(in_vlist); i++)
   {
      point = AiArrayGetVec(in_vlist, i);
      CVector3 p(point.x, point.y, point.z);
      p.MulByTransformationInPlace(in_masterObjTransform);
      m_points[i].Set((float)p.GetX(), (float)p.GetY(), (float)p.GetZ());
   }

   if (in_nidxs)
   {
      unsigned int vlistIndex, nlistIndex;
      for (unsigned int nidxsIndex=0; nidxsIndex<AiArrayGetNumElements(in_nidxs); nidxsIndex++)
      {
         nlistIndex = AiArrayGetUInt(in_nidxs, nidxsIndex);
         // nlistIndex is the index of the normal that we need to store.
         // it may have been set already, in such case it will be overwritten
         vlistIndex = AiArrayGetUInt(in_vidxs, nidxsIndex);
         // vlistIndex is the index of the corresponding point;
         normal = AiArrayGetVec(in_nlist, nlistIndex);
         point = AiArrayGetVec(in_vlist, vlistIndex);

         m_normals[nlistIndex].Set(normal.x, normal.y, normal.z);
         m_pointAtNormals[nlistIndex] = vlistIndex; // nlistIndex, NOT vlistIndex
         // temp test
         // bendedNormals[nlistIndex].Set(normal.x, normal.y, normal.z);
      }
   }
}


// Return the vertices and normals TO an arnold shape
//
// @param io_vlist      The vertices array
// @param io_nlist      The normals array
// @param in_defKey     The deformation mb key
// @return void
// 
void CStrandInstance::Get(AtArray *io_vlist, AtArray *io_nlist, const unsigned int in_defKey)
{
   AtVector current;

   // get the bended points
   if (io_vlist)
   for (unsigned int i=0; i<AiArrayGetNumElements(io_vlist); i++)
   {
      m_bendedPoints[i].Get(current.x, current.y, current.z);
      AiArraySetVec(io_vlist, in_defKey * AiArrayGetNumElements(io_vlist) + i, current);
   }
   // get the bended normals
   if (io_nlist)
   for (unsigned int i=0; i<AiArrayGetNumElements(io_nlist); i++)
   {
      m_bendedNormals[i].Get(current.x, current.y, current.z);
      AiArraySetVec(io_nlist, in_defKey * AiArrayGetNumElements(io_nlist) + i, current);
   }
}


// Compute the bounding cylinder for this object
// @return void
// 
void CStrandInstance::ComputeBoundingCylinder()
{
   for (int i=0; i<(int)m_points.size(); i++)
      m_boundingCylinder.Adjust(m_points[i]);
}


// Compute and store the bounding cylinder of an array of CStrandInstances
//
// @param in_strandInstances      The array of CStrandInstances
// @return void
// 
void CStrandInstance::ComputeModelBoundingCylinder(vector <CStrandInstance> in_strandInstances)
{
   for (int i=0; i<(int)in_strandInstances.size(); i++)
      for (int j=0; j<(int)in_strandInstances[i].m_points.size(); j++)
         m_boundingCylinder.Adjust(in_strandInstances[i].m_points[j]);
}


// Remap the points to cylindrical coordinates
//
// @return void
// 
void CStrandInstance::RemapPointsToCylinder()
{
   // init for the cylinder the same points size
   m_boundingCylinder.m_points.resize(m_points.size());
   for (int i=0; i<(int)m_points.size(); i++)
      m_boundingCylinder.RemapPoint(m_points[i], i);
}


// Bend the vertices and normals along a strand.
// Store the bended points into this->bendedPoints
// Store the bended normals into this->bendedNOrmals
//
// @param in_strand      The strand
// @return void
// 
void CStrandInstance::BendOnStrand(CStrand &in_strand)
{
   CVector3f pos;
   CVector3f hx, hy, hz; //bending coordinate system
   CVector3f x, center, v, dir, hDir, n, newV;
   CVector3  x3, axis, hx3, hy3, hz3;
   CTransformation trans;
   int index;
   CCylMappedPoint mappedPoint;
   float stretch;

   // points
   for (int i=0; i<(int)m_points.size(); i++)
   {
      // get cylindrically mapped point
      m_boundingCylinder.GetRemappedPoint(&mappedPoint, i);
      // pos == point on in_strand at mappedPoint.m_height 
      index = in_strand.GetPositionByT(&pos, mappedPoint.m_height); //returning index of point "below" height

      in_strand.GetSegmentDirection(&hy, index);
      // in_strand.ComputeBendedXDirection(&hx, &hy, pos, index);
      in_strand.ComputeBendedXDirectionByT(&hx, &hy, pos, mappedPoint.m_height);
      // hx is the x axis, bended along the strand at mappedPoint.height
      // hy is the bended y, ie the strand tangent at mappedPoint.height

      // GetMessageQueue()->LogMsg(L"hx = " + (CString)hx.GetX() + L" " + (CString)hx.GetY() + L" " + (CString)hx.GetZ());

      // so, now we must rotate hx around hy by mappedPoint.angle
      // this will give us the point correctly bended.
      // ........ in mental ray this was
      // ........ mi_matrix_rotate_axis(m, &hy, mappedPoint.angle);
      // ........ mi_vector_transform(&x, &hx, m);

      // in Soft we must use a longer path, because CVector3f is not powerful enough
      hx3.Set((double)hx.GetX(), (double)hx.GetY(), (double)hx.GetZ());
      hy3.Set((double)hy.GetX(), (double)hy.GetY(), (double)hy.GetZ());

      trans.SetRotationFromAxisAngle(hy3, mappedPoint.m_angle);   
      CMatrix3 m3 = trans.GetRotationMatrix3();
      x3.MulByMatrix3(hx3, m3);
      x.Set((float)x3.GetX(), (float)x3.GetY(), (float)x3.GetZ());

      // so, x is the unit vector from the strand to the bended point.
      // we still have to set its length
      // Get the ratio between the in_strand length and the master object height
      // If the master was flat (a grid), set an arbitrary value of 1. Else, the instances would stretch way too much (#1151)
      stretch = m_boundingCylinder.m_height > 0.001f ? in_strand.m_length / m_boundingCylinder.m_height : 1.0f;
      // scale the point's radius accordingly, so we don get FAT instances

      // #1252. Scale (not set) the radius by the strand radius at this height. 
      // I think it's more elegant to scale than to set, as long as it's documented
      if (in_strand.m_radii.size() > 0)
      {
         float strandRadius;
         in_strand.GetRadiusByT(&strandRadius, mappedPoint.m_height);
         stretch*= strandRadius;
      }

      x.SetLength(mappedPoint.m_radius * stretch);

      // and finally add it to the current strand position
      pos+=x;
      // give it to the arnold buffer
      m_bendedPoints[i] = pos;
   }
   //normals
   for (int i=0; i<(int)m_normals.size(); i++)
   {
      m_boundingCylinder.GetRemappedPoint(&mappedPoint, m_pointAtNormals[i]);
      index = in_strand.GetPositionByT(&pos, mappedPoint.m_height);

      in_strand.GetSegmentDirection(&hy, index);
      // in_strand.ComputeBendedXDirection(&hx, &hy, pos, index);
      in_strand.ComputeBendedXDirectionByT(&hx, &hy, pos, mappedPoint.m_height);
      hz.Cross(hx, hy); //got the full reference axes
      hz.NormalizeInPlace();

      // get the i-th normal
      v = m_normals[i];

      // now transform the normal (v) to the current strand segment coords sys (hx, hy, hz)
      n.PutX(hx.GetX() * v.GetX() + hy.GetX() * v.GetY() + hz.GetX() * v.GetZ());
      n.PutY(hx.GetY() * v.GetX() + hy.GetY() * v.GetY() + hz.GetY() * v.GetZ());
      n.PutZ(hx.GetZ() * v.GetX() + hy.GetZ() * v.GetY() + hz.GetZ() * v.GetZ());
      // give it to the arnold buffer
      m_bendedNormals[i].Normalize(n);
   }
}


