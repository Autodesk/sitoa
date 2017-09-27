/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCamera.h"
#include "common/ParamsCommon.h"
#include "common/ParamsShader.h"
#include "common/Tools.h"
#include "loader/Cameras.h"
#include "loader/Shaders.h"
#include "renderer/Renderer.h"

#include <xsi_kinematics.h>

// Load all the camera parameters that depends of the camera type.
//
// @param in_cameraNode    The Arnold camera node
// @param in_xsiCamera     The Softimage camera
// @param in_cameraType    The camera type
// @param in_frame         The frame time.
//
// @return                 CStatus::OK
//
CStatus LoadCameraParameters(AtNode* in_cameraNode, const Camera &in_xsiCamera, const CString &in_cameraType, double in_frame)
{ 
   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(in_xsiCamera.GetRef(), transfKeys, defKeys, in_frame);
   LONG nbTransfKeys = transfKeys.GetCount();

   const AtNodeEntry *entry = AiNodeGetNodeEntry(in_cameraNode);
   AtArray *fov(NULL);
   bool hasFOV(false);

   // #896: the fov attributes of cyl_camera are called horizontal_fov and vertical_fov
   // so skip getting the Softimage camera fov. We'll set them later
   if (!AiNodeIs(in_cameraNode, ATSTRING::cyl_camera))
      hasFOV = AiNodeEntryLookUpParameter(entry, "fov") != NULL;
   
   if (hasFOV)
      fov = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_FLOAT);     

   AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);
   
   for (LONG ikey=0; ikey<nbTransfKeys; ikey++)
   {
      double frame = transfKeys[ikey];

      if (hasFOV)
         AiArraySetFlt(fov, ikey, GetCameraHorizontalFov(in_xsiCamera, frame));

      // Getting Camera Transformation
      CTransformation cameraTransform = in_xsiCamera.GetKinematics().GetGlobal().GetTransform(frame);
      // Get the transform matrix
      AtMatrix matrix;
      CUtilities().S2A(cameraTransform, matrix);
      
      // The original viewport ortho camera is at 10000 units away
      // from the origin. This causes shading artifacts because of
      // the renderer's limited floating point accuracy when computing
      // ray-object intersections. Here we attempt to fix the problem
      // by relocating the ortho cameras just outside the scene's
      // bounding box (plus a small slack of 1.0).

      // But in Softimage 2011.5 with the viewcube we can convert one of these
      // cameras into a user type view, and we have to avoid this modification.
      // Our only choice is to check the rotation values of the camera

      if (in_cameraType == L"ortho_camera")
      {
         CVector3 rotationAngles = in_xsiCamera.GetKinematics().GetGlobal().GetTransform().GetRotationXYZAngles();
         if (in_xsiCamera.GetName().IsEqualNoCase(L"RightCamera"))
         {
            if (rotationAngles.EpsilonEquals(CVector3(0.0, 90.0, 0.0), AI_EPSILON))
            {
               AtBBox bbox = AiUniverseGetSceneBounds();
               matrix[3][0] = bbox.max.x + 1.0f;
            }
         }
         else if (in_xsiCamera.GetName().IsEqualNoCase(L"TopCamera"))
         {
            if (rotationAngles.EpsilonEquals(CVector3(-90.0, 0.0, 0.0), AI_EPSILON))
            {
               AtBBox bbox = AiUniverseGetSceneBounds();
               matrix[3][1] = bbox.max.y + 1.0f;
            }
         }
         else if (in_xsiCamera.GetName().IsEqualNoCase(L"FrontCamera"))
         {
            if (rotationAngles.EpsilonEquals(CVector3(0.0, 0.0, 0.0), AI_EPSILON))
            {
               AtBBox bbox = AiUniverseGetSceneBounds();
               matrix[3][2] = bbox.max.z + 1.0f;
            }
         }
      }
      AiArraySetMtx(matrices, ikey, matrix);
   }

   if (hasFOV)
      AiNodeSetArray(in_cameraNode, "fov", fov);

   // Setting the camera matrix - this is a default arnold camera parameter
   AiNodeSetArray(in_cameraNode, "matrix", matrices);

   // Set the screen_window values which are default arnold camera properties
   // The subpixelzoom mode should only affects a render region mode
   if (GetRenderInstance()->GetRenderType() == L"Region" && (bool)ParAcc_GetValue(in_xsiCamera, L"subpixelzoom", in_frame))
   {
      float subfrustumleft   = ParAcc_GetValue(in_xsiCamera, L"subfrustumleft",   in_frame);
      float subfrustumright  = ParAcc_GetValue(in_xsiCamera, L"subfrustumright",  in_frame);
      float subfrustumtop    = ParAcc_GetValue(in_xsiCamera, L"subfrustumtop",    in_frame);
      float subfrustumbottom = ParAcc_GetValue(in_xsiCamera, L"subfrustumbottom", in_frame);

      float screenWindowMinX = (subfrustumleft - 0.5f) * 2.0f;
      float screenWindowMinY = (subfrustumbottom - 0.5f) * 2.0f;
      float screenWindowMaxX = (subfrustumright - 0.5f) * 2.0f;
      float screenWindowMaxY = (subfrustumtop -0.5f) * 2.0f;

      CNodeSetter::SetVector2(in_cameraNode, "screen_window_min", screenWindowMinX, screenWindowMinY);
      CNodeSetter::SetVector2(in_cameraNode, "screen_window_max", screenWindowMaxX, screenWindowMaxY);   
   }
   else if (ParAcc_GetValue(in_xsiCamera, L"proj", in_frame) == 0)
   {
      // Orthographic camera
      float width  = (float)ParAcc_GetValue(in_xsiCamera, L"planewidth", in_frame);
      float height = (float)ParAcc_GetValue(in_xsiCamera, L"orthoheight", in_frame);
      float aspect = (float)ParAcc_GetValue(in_xsiCamera, L"aspect", in_frame);

      CNodeSetter::SetVector2(in_cameraNode, "screen_window_min", -width/2, -height/2*aspect);
      CNodeSetter::SetVector2(in_cameraNode, "screen_window_max", width/2, height/2*aspect);
   }
   else // Optical Center Shift (only in perspective cameras)
   {
      float factorX(0.0f), factorY(0.0f);
      
      if ((bool)ParAcc_GetValue(in_xsiCamera, L"projplane", in_frame))
      {
         float offsetX = (float)ParAcc_GetValue(in_xsiCamera, L"projplaneoffx", in_frame);
         float offsetY = (float)ParAcc_GetValue(in_xsiCamera, L"projplaneoffy", in_frame);
         
         if (offsetX!=0.0f || offsetY!=0.0f)
         {
            float apertureX = (float)ParAcc_GetValue(in_xsiCamera, L"projplanewidth", in_frame);
            float apertureY = (float)ParAcc_GetValue(in_xsiCamera, L"projplaneheight", in_frame);

            factorX = (offsetX / apertureX) * 2;
            factorY = (offsetY / apertureY) * 2;
         }
      }

      // Perspective camera - so let's just set defaults
      CNodeSetter::SetVector2(in_cameraNode, "screen_window_min", (-1.0f + factorX), (-1.0f + factorY));
      CNodeSetter::SetVector2(in_cameraNode, "screen_window_max", (1.0f + factorX), (1.0f + factorY));
   }

   // Set the clipping - this is a default arnold camera property
   float near_clip = ParAcc_GetValue(in_xsiCamera, L"near", in_frame);
   float far_clip  = ParAcc_GetValue(in_xsiCamera, L"far", in_frame);
   CNodeSetter::SetFloat(in_cameraNode, "near_clip", near_clip);
   CNodeSetter::SetFloat(in_cameraNode, "far_clip",  far_clip);

   // If shaders have a plane_distance parameter switch it on now that the clipping range is set
   if (AiNodeEntryLookUpParameter(entry, "plane_distance"))
      CNodeSetter::SetBoolean(in_cameraNode, "plane_distance", true);

   return CStatus::OK;
}


// Get the horizontal fov of a camera
//
// @param in_xsiCamera     The Softimage camera
// @param in_frame         The frame time.
//
// @return                 the horizontal fov value
//
float GetCameraHorizontalFov(const Camera& in_xsiCamera, double in_frame)
{
   /////////////////////////////////////////////////////////////////
   // Re-Updating FOV for Vertical FOV cases 
   // (not working for now with mblur on render region)
   // We will get Aspect Ratio from Region Camera instead Scene or Pass setting
   float fovValue = (float)ParAcc_GetValue(in_xsiCamera, L"fov", in_frame);

   if (ParAcc_GetValue(in_xsiCamera, L"fovtype", in_frame) == eCameraFov_Vertical)
   {
      // Horizontal = 2 * arctan (aspect * tan(Vertical / 2) )    
      // Need conversions from degrees to radians to calculate and from radians to degrees to assign
      double aspect_ratio = ParAcc_GetValue(in_xsiCamera, L"aspect", in_frame);
      fovValue = (float)(AI_RTOD * 2.0f * atan(aspect_ratio * tan(fovValue * AI_DTOR * 0.5f)));
   }

   return fovValue;
}
