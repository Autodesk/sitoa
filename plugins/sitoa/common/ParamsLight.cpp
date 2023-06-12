/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsLight.h"
#include "common/ParamsShader.h"

#include "loader/Loader.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"

#include <xsi_kinematics.h>

// Transform a spot light matrix to the one to be set if the light type is photometric_light
//
// @param in_lightMatrix       the input light matrix
//
// @return                     the matrix to be set on the photometric_light
//
CMatrix4 TransformToPhotometricLight(const CMatrix4 &in_lightMatrix)
{
   CMatrix4 result;

   // Get a spot light, relax the constrain and set its rotation to 0
   // The spot points toward negative Z, so to make it point downward we must rotate by -90
   // around the X axis. Let build this inverted matrix
   CTransformation x90Transform;
   x90Transform.SetRotationFromXYZAnglesValues(AI_PIOVER2, 0, 0); // rotate by 90 around X
   result = x90Transform.GetMatrix4();
   // And multiply it by the light matrix.
   result.MulInPlace(in_lightMatrix);
   return result;
}


// Load a light's parameters
//
// @param in_lightNode       the Arnold light node
// @param in_xsiLight        the Softimage light object
// @param in_xsiShader       the Softimage light shader
// @param in_isMaster        true if in_lightNode is a master node, false if it's a duplicate 
// @param in_frame           frame time
// @param in_ipr             true is called during IPR, false if the light node is being built
//
// @return                   CStatus::OK success / CStatus::Fail is returned in case of failure.
//
CStatus LoadLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, const Shader &in_xsiShader, bool in_isMaster, double in_frame, bool in_ipr)
{
   CRefArray lightProperties = in_xsiLight.GetProperties();

   // Get the transformation from the xsi light and copy on the light node.
   // Of course, we should do this only for the master lights, and not for the light
   // duplicates == xsi instances of the light
   CDoubleArray keyFramesTransform, keyFramesDeform;
   CSceneUtilities::GetMotionBlurData(in_xsiLight.GetRef(), keyFramesTransform, keyFramesDeform, in_frame);

   if (in_isMaster)
   {
      int nkeys = (int)keyFramesTransform.GetCount();
          
      AtArray* matrices = AiArrayAllocate(1, (uint8_t)nkeys, AI_TYPE_MATRIX);

      for (int ikey=0; ikey<nkeys; ikey++)
      {
         AtMatrix nodeMatrix;
         CMatrix4 lightMatrix = in_xsiLight.GetKinematics().GetGlobal().GetTransform(keyFramesTransform[ikey]).GetMatrix4();

         if (AiNodeIs(in_lightNode, ATSTRING::photometric_light))
         {
            CMatrix4 photometricLightmatrix = TransformToPhotometricLight(lightMatrix);
            CUtilities().S2A(photometricLightmatrix, nodeMatrix);
         }
         else
            CUtilities().S2A(lightMatrix, nodeMatrix);

         AiArraySetMtx(matrices, ikey, nodeMatrix);
      }

      AiNodeSetArray(in_lightNode, "matrix", matrices);
   }

   // These type of light have special properties
   if (AiNodeIs(in_lightNode, ATSTRING::quad_light))
      LoadQuadLightParameters(in_lightNode, in_xsiLight, in_frame, keyFramesTransform);
   else if (AiNodeIs(in_lightNode, ATSTRING::cylinder_light)) 
      LoadCylinderLightParameters(in_lightNode, in_xsiLight, in_frame, keyFramesTransform);
   else if (AiNodeIs(in_lightNode, ATSTRING::disk_light))
      LoadDiskLightParameters(in_lightNode, in_xsiLight, in_frame);
   else if (AiNodeIs(in_lightNode, ATSTRING::mesh_light))
      LoadMeshLightParameters(in_lightNode, in_xsiLight, in_frame);

   CShaderBranchDump beforeBranch, afterBranch;
   // pack into beforeBranch all the parameters of the shading branch connected to the skydome color
   if (in_ipr && AiNodeIs(in_lightNode, ATSTRING::skydome_light))
      beforeBranch.Fill(in_lightNode, "color");

   // Setting all light shader parameters into Arnold
   LoadShaderParameters(in_lightNode, in_xsiShader.GetParameters(), in_frame, in_xsiLight.GetRef(), true);   

   if (in_ipr && AiNodeIs(in_lightNode, ATSTRING::skydome_light))
   {
      // pack into afterBranch all the parameters of the shading branch connected to the skydome color,
      // after the branch has been updated by LoadShaderParameters
      afterBranch.Fill(in_lightNode, "color");
      // if the two branches are different, then we must flush the backgroud cache for #1631
      if (!(beforeBranch == afterBranch))
         AiUniverseCacheFlush(NULL, AI_CACHE_BACKGROUND);
   }

   // Github #86 - If light is render invisible in interactive mode we will disable the light instead of dynamically destroying the light node.
   AiNodeSetDisabled(in_lightNode, !ParAcc_GetValue(Property(lightProperties.GetItem(L"Visibility")), L"rendvis", in_frame));
 
   return CStatus::OK;
}


// Load the parameters for a quad light
// Note that for animated area lights (#1125), we use the transformation deform step, since
// a change in the area light size is equivalent to a change in scale.
//
// @param *in_lightNode          Arnold Quad Light Node where to load the parameters.
// @param &in_xsiLight           XSI Light object to get the matrix(s) from its transformation.
// @param in_frame               The current frame
// @param in_keyFramesTransform  The transformation mb steps.
//
// @return                       CStatus::OK
//
CStatus LoadQuadLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame, CDoubleArray in_keyFramesTransform)
{
   int nbKeys = (int)in_keyFramesTransform.GetCount();
   AtArray* vertices = AiArrayAllocate(4, (uint8_t)nbKeys, AI_TYPE_VECTOR);
   // The quadligth corners used by Arnold
   double quadshape[12] = {1,-1,0, -1,-1,0, -1,1,0,  1,1,0}; 

   Primitive lightPrimitive = CObjectUtilities().GetPrimitiveAtFrame(in_xsiLight, in_frame);

   double scale_x, scale_y, time;
   AtVector point;
   point.z = 0.0f;

   for (int iKey=0; iKey<nbKeys; iKey++)
   {
      // Getting XSI area light scaling parameters
      time = in_keyFramesTransform[iKey];
      scale_x = (double)ParAcc_GetValue(lightPrimitive, L"LightAreaXformSX", time);
      scale_y = (double)ParAcc_GetValue(lightPrimitive, L"LightAreaXformSY", time);

      for (unsigned int i=0; i<4; i++)
      {
         point.x = (float)(quadshape[     i*3 ] * (scale_x * 0.5));
         point.y = (float)(quadshape[1 + (i*3)] * (scale_y * 0.5));
         CUtilities().SetArrayValue(vertices, point, i, iKey);
      }
   }

   AiNodeSetArray(in_lightNode, "vertices", vertices);

   return CStatus::OK;
}


// Load the parameters for a cylinder light
// Note that for animated area lights (#1125), we use the transformation deform step, since
// a change in the area light size/rottion is equivalent to a change in scale.
//
// @param *in_lightNode            Arnold Cylinder Light Node where to load the parameters.
// @param &in_xsiLight             XSI Light object to get the matrix(s) from its transformation.
// @param in_frame                 The current frame
// @param in_keyFramesTransform    The transformation mb steps.
//
// @return                         CStatus::OK
//
CStatus LoadCylinderLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame, CDoubleArray in_keyFramesTransform)
{
   int nbKeys = (int)in_keyFramesTransform.GetCount();
   AtArray* bottomArray = AiArrayAllocate(1, (uint8_t)nbKeys, AI_TYPE_VECTOR);
   AtArray* topArray    = AiArrayAllocate(1, (uint8_t)nbKeys, AI_TYPE_VECTOR);

   // Getting XSI area light scaling parameters
   Primitive lightPrimitive = CObjectUtilities().GetPrimitiveAtFrame(in_xsiLight, in_frame);

   float scale_x = (float) ParAcc_GetValue(lightPrimitive, L"LightAreaXformSX", in_keyFramesTransform[0]); // radius, not mb-urrable

   double scale_z, rot_x, rot_y, rot_z, time;
   CTransformation lightTransform;

   for (int iKey=0; iKey<nbKeys; iKey++)
   {
      time = in_keyFramesTransform[iKey];

      scale_z = (double)ParAcc_GetValue(lightPrimitive, L"LightAreaXformSZ", time); // Length
      
      rot_x = (double)ParAcc_GetValue(lightPrimitive, L"LightAreaXformRX", time); // euler x in degrees
      rot_y = (double)ParAcc_GetValue(lightPrimitive, L"LightAreaXformRY", time); // euler y in degrees
      rot_z = (double)ParAcc_GetValue(lightPrimitive, L"LightAreaXformRZ", time); // euler z in degrees
   
      CVector3 bottomPosition(0.0, 0.0, -1.0);
      CVector3 topPosition(0.0, 0.0, 1.0);

      lightTransform.SetIdentity();
      lightTransform.SetScaling(CVector3(scale_z, scale_z, scale_z));
      lightTransform.SetRotationFromXYZAngles(CVector3(AI_DTOR * rot_x, AI_DTOR * rot_y, AI_DTOR * rot_z));

      bottomPosition.MulByTransformationInPlace(lightTransform);
      topPosition.MulByTransformationInPlace(lightTransform);

      AtVector bottom    = AtVector((float)bottomPosition.GetX(), (float)bottomPosition.GetY(), (float)bottomPosition.GetZ());
      AtVector topbottom = AtVector((float)topPosition.GetX(), (float)topPosition.GetY(), (float)topPosition.GetZ());

      CUtilities().SetArrayValue(bottomArray, bottom, 0, iKey);
      CUtilities().SetArrayValue(topArray, topbottom, 0, iKey);
   }

   AiNodeSetArray(in_lightNode, "bottom", bottomArray);
   AiNodeSetArray(in_lightNode, "top",    topArray);

   CNodeSetter::SetFloat(in_lightNode, "radius", scale_x);

   return CStatus::OK;
}


// Load the parameters for a disk light
// We don't currently support the local direction
//
// @param *in_lightNode       Arnold Cylinder Light Node where to load the parameters.
// @param &in_xsiLight        XSI Light object to get the matrix(s) from its transformation.
// @param in_frame            Frame to evaluate.
// @return                    CStatus::OK
//
CStatus LoadDiskLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame)
{
   // Getting XSI area light scaling parameters
   Primitive lightPrimitive = CObjectUtilities().GetPrimitiveAtFrame(in_xsiLight, in_frame);
   float scale_x = (float)ParAcc_GetValue(lightPrimitive, L"LightAreaXformSX", in_frame); // radius
   CNodeSetter::SetFloat(in_lightNode, "radius", scale_x);
   return CStatus::OK;
}


// Assign the mesh attribute of an Arnold's mesh_light
//
// @param in_lightNode       The Arnold mesh_light node
// @param in_xsiLight        The Softimage light
// @param in_frame           The frame time
//
// @return                   true if a mesh could be found and assigned, else false 
//
bool LoadMeshLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame)
{
   Primitive lightPrimitive = CObjectUtilities().GetPrimitiveAtFrame(in_xsiLight, in_frame);

   int lightAreaGeom = (int)ParAcc_GetValue(lightPrimitive, L"LightAreaGeom", in_frame);
   if (lightAreaGeom != 5) // Geometry == Object?
      return false;

   Parameter p = lightPrimitive.GetParameter(L"LightAreaObject");
   if (!p.IsValid())
      return false;

   if (p.GetNestedObjects().GetCount() == 0)
      return false;

   CRef ref = p.GetNestedObjects()[0];
   if (!ref.IsValid())
      return false;

   X3DObject obj(ref);
   if (!obj.IsValid())
      return false;

   AtNode *mesh = GetRenderInstance()->NodeMap().GetExportedNode(obj, in_frame);
   if (!mesh)
   {
      CRefArray dummyArray;
      if (PostLoadSingleObject(obj, in_frame, dummyArray, false) == CStatus::OK)
         mesh = GetRenderInstance()->NodeMap().GetExportedNode(obj, in_frame);
   }

   if (!mesh)
      return false;
   if (!AiNodeIs(mesh, ATSTRING::polymesh))
      return false;

   CNodeSetter::SetPointer(in_lightNode, "mesh", mesh);
   return true;
}


// Load the matrix for light_blocker light filter
// It was indeed possible to use a matrix type parameter.
// However, it was not possible to link by expression this matrix to the picked object's one.
// So, I decided to code the Arnold matrix by 3 vectors (scale, rotation, translation) and do the srt->matrix
// conversion in this function.
//
// @param *in_filterNode      Arnold's blocker_filter node where to load the parameter.
// @param &in_filterShader    The filter shader.
// @param in_frame            Frame time.
//
// @return                    CStatus::OK
//
CStatus LoadBlockerFilterMatrix(AtNode* in_filterNode, const Shader &in_filterShader, double in_frame)
{
   AtMatrix m;
   CTransformation  transform;
   Parameter p;
   CParameterRefArray pArray;

   CVector3 s, r, t;
   // get scale
   p = in_filterShader.GetParameter(L"scale");
   pArray = p.GetParameters();
   s.Set((double)((Parameter)(pArray[0])).GetValue(in_frame), (double)((Parameter)(pArray[1])).GetValue(in_frame), (double)((Parameter)(pArray[2])).GetValue(in_frame));

   // get rotation
   p = in_filterShader.GetParameter(L"rotation");
   pArray = p.GetParameters();
   r.Set((double)((Parameter)(pArray[0])).GetValue(in_frame), (double)((Parameter)(pArray[1])).GetValue(in_frame), (double)((Parameter)(pArray[2])).GetValue(in_frame));
   // go to radians
   r.Set(AI_DTOR*r.GetX(), AI_DTOR*r.GetY(), AI_DTOR*r.GetZ());

   // get translation
   p = in_filterShader.GetParameter(L"translation");
   pArray = p.GetParameters();
   t.Set((double)((Parameter)(pArray[0])).GetValue(in_frame), (double)((Parameter)(pArray[1])).GetValue(in_frame), (double)((Parameter)(pArray[2])).GetValue(in_frame));

   // build the transform
   transform.SetScaling(s);
   transform.SetRotationFromXYZAngles(r);
   transform.SetTranslation(t);
   // set the Arnold matrix
   CUtilities().S2A(transform.GetMatrix4(), m);
   // assign it
   CNodeSetter::SetMatrix(in_filterNode, "geometry_matrix", m);
   return CStatus::OK;
}


// Load the offset and rotate for the gobo filter (#1516)
//
// @param *in_filterNode      Arnold's gobo filter node where to load the parameter.
// @param &in_filterShader    The filter shader.
// @param in_frame            Frame time.
//
// @return                    CStatus::OK
//
CStatus LoadGoboFilterOffsetAndRotate(AtNode* in_filterNode, const Shader &in_filterShader, const Light &in_xsiLight, double in_frame)
{
   float x = (float)in_filterShader.GetParameter(L"offset_x").GetValue(in_frame);
   float y = (float)in_filterShader.GetParameter(L"offset_y").GetValue(in_frame);
   // assign it
   CNodeSetter::SetVector2(in_filterNode, "offset", x, y);

   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(in_xsiLight.GetRef(), transfKeys, defKeys, in_frame);

   int nbTransfKeys = (int)transfKeys.GetCount();
   AtArray* rotate = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_FLOAT);

   for (LONG ikey=0; ikey<nbTransfKeys; ikey++)
   {
      double frame = transfKeys[ikey];
      AiArraySetFlt(rotate, ikey, (float)ParAcc_GetValue(in_filterShader, L"rotate", frame));
   }

   AiNodeSetArray(in_filterNode, "rotate", rotate);

   return CStatus::OK;
}

