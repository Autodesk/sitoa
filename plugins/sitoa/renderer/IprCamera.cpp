/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCamera.h"
#include "common/ParamsShader.h"
#include "loader/Cameras.h"
#include "loader/Properties.h"
#include "renderer/IprCamera.h"
#include "renderer/Renderer.h"

#include <ai_universe.h>

#include <xsi_primitive.h>
#include <xsi_shader.h>

void UpdateCamera(double in_frame)
{
   double frame(in_frame);
   // "frame" is use to look up the existing camera node (if any). If we are in flythrough mode,
   // the node was created at time flythrough_frame, and never destroyed since then.
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      frame = GetRenderInstance()->GetFlythroughFrame();

   const Camera xsiCamera = GetRenderInstance()->GetRenderCamera();
   AtNode* cameraNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiCamera, frame);
   // also in flythrough mode, we must use in_frame to evaluate the camera at the current frame,
   // although the node that will be updated is the one looked up above at flythrough_frame time

   bool createCamera(true);

   if (cameraNode)
   {
      CString currentCameraType = CNodeUtilities().GetEntryName(cameraNode);

      CustomProperty cameraOptionsProperty, userOptionsProperty;
      CRefArray properties = xsiCamera.GetProperties();
      properties.Find(L"arnold_camera_options", cameraOptionsProperty);
      CString cameraType = GetCameraType(xsiCamera, cameraOptionsProperty, in_frame);
      // is the current camera one of the Arnold's ones? If not, it is a custom one
      bool isCurrentCameraAnArnoldCamera = currentCameraType == L"persp_camera" || currentCameraType == L"spherical_camera" || 
                                           currentCameraType == L"cyl_camera"   || currentCameraType == L"fisheye_camera" || 
                                           currentCameraType == L"vr_camera"    || currentCameraType == L"ortho_camera";

      // if the current camera is a custom one, and the camera type selected in the property is "custom",
      // then we do not want to destroy and re-create back the same camera.
      if ((!isCurrentCameraAnArnoldCamera) && (cameraType == "custom_camera"))
      {
         Shader lensShader;
         if (GetFirstLensShader(xsiCamera, lensShader)) // update the custom camera out of the lens shader
            LoadShaderParameters(cameraNode, lensShader.GetParameters(), in_frame, xsiCamera.GetRef(), false);
         createCamera = false;
      }
      else
         createCamera = cameraType != currentCameraType;

      if (createCamera) // the user changed the camera type of the camera options property
      {
         AiNodeDestroy(cameraNode);
         GetRenderInstance()->NodeMap().EraseExportedNode(cameraNode);
      }
      else // standard update
      {
         LoadCameraParameters(cameraNode, xsiCamera, cameraType, in_frame);
         CNodeUtilities::SetMotionStartEnd(cameraNode);
         properties.Find(L"arnold_camera_options", cameraOptionsProperty);
         LoadCameraOptions(xsiCamera, cameraNode, cameraOptionsProperty, in_frame);
         properties.Find(L"arnold_user_options", userOptionsProperty);
         LoadUserOptions(cameraNode, userOptionsProperty, in_frame);
      }
   }
   // Here, in 2 cases:
   // 1. Camera did not exist and was created during an IPR session
   // 2. The user changed the camera type of the camera options property
   // Use frame, not in_frame, so it stay consistent with flythrough
   if (createCamera)
   {
      LoadSingleCamera(xsiCamera, frame);         
      cameraNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiCamera, frame);
   }  

   CNodeSetter::SetPointer(AiUniverseGetOptions(NULL), "camera", cameraNode);
}

