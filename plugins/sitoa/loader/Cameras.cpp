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
#include "loader/Loader.h"
#include "loader/Properties.h"
#include "renderer/Renderer.h"

#include <xsi_model.h>
#include <xsi_project.h>
#include <xsi_scene.h>


// Load all the Softimage cameras
//
// @param in_frame           The current frame time
//
// @return OK or the first error while cycling the cameras
//
CStatus LoadCameras(double in_frame)
{ 
   CStatus status;
  
   CRefArray camerasArray = Application().GetActiveSceneRoot().FindChildren(L"", siCameraPrimType, CStringArray(), true);

   for (LONG i=0; i<camerasArray.GetCount(); i++)
   {    
      Camera xsiCamera(camerasArray.GetItem(i));
      status = LoadSingleCamera(xsiCamera, in_frame);   
      if (status != CStatus::OK)
         break;
   }

   Camera renderCamera = GetRenderInstance()->GetRenderCamera();
   AtNode *cameraNode = GetRenderInstance()->NodeMap().GetExportedNode(renderCamera, in_frame); 
   if (cameraNode)
      CNodeSetter::SetPointer(AiUniverseGetOptions(), "camera", cameraNode);

   return status;
}


// Load a single Softimage camera
//
// @param in_xsiCamera       The Softimage camera
// @param in_frame           The current frame time
//
// @return OK or the code error in case of failure
//
CStatus LoadSingleCamera(const Camera &in_xsiCamera, double in_frame)
{  
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   CustomProperty cameraOptionsProperty, userOptionsProperty;
   CRefArray properties = in_xsiCamera.GetProperties();

   properties.Find(L"arnold_camera_options", cameraOptionsProperty);

   CString cameraType = GetCameraType(in_xsiCamera, cameraOptionsProperty, in_frame);
   bool isCustom(false);
   Shader lensShader;
   if (cameraType == L"custom_camera")
   {
      if (GetFirstLensShader(in_xsiCamera, lensShader))
      {
         cameraType = GetShaderNameFromProgId(lensShader.GetProgID());
         isCustom = true;
      }
   }
   // the type selected in the property is "custom_camera", but no valid shader was found ?
   if (cameraType == L"custom_camera")
      cameraType = L"persp_camera"; // default back to perspective

   AtNode* cameraNode = AiNode(cameraType.GetAsciiString());
   if (isCustom && (!cameraNode)) // in case the type is custom but the node could not be loaded
   {
      cameraType = L"persp_camera";
      cameraNode = AiNode(cameraType.GetAsciiString());
      isCustom = false;
   }

   GetRenderInstance()->NodeMap().PushExportedNode(in_xsiCamera, in_frame, cameraNode);

   // #917 conforms the camera name to the rest of the nodes
   CString cameraName = CStringUtilities().MakeSItoAName((SIObject)in_xsiCamera, in_frame, L"", false);
   CNodeUtilities().SetName(cameraNode, cameraName);

   // Getting all the camera parameters
   LoadCameraParameters(cameraNode, in_xsiCamera, cameraType, in_frame);
   if (isCustom)
      LoadShaderParameters(cameraNode, lensShader.GetParameters(), in_frame, in_xsiCamera.GetRef(), false);

   LoadCameraOptions(in_xsiCamera, cameraNode, cameraOptionsProperty, in_frame);

   CNodeUtilities::SetMotionStartEnd(cameraNode);
   properties.Find(L"arnold_user_options", userOptionsProperty); /// #680
   LoadUserOptions(cameraNode, userOptionsProperty, in_frame);

   LoadUserDataBlobs(cameraNode, in_xsiCamera, in_frame); // #728

   return CStatus::OK;
}


#if 0
// Collect and return the lens shaders connected to a given pass
//
// @param in_pass   The pass
//
// @return the array of shaders, void if nothing is conncted
//
CRefArray CollectPassLensShaders(const Pass &in_pass)
{
   CRefArray result;
   CRef paramRef;

   int overrideCameraLensShaders = (int)ParAcc_GetValue(in_pass, L"OverrideCameraLensShaders", DBL_MAX);
   if (overrideCameraLensShaders != 3)
      return result; // void, we'll use the camera shaders

   CString passName = in_pass.GetFullName();

   // else overwriting, #1496
   paramRef.Set(passName + L".LensShaderStack.Item");
   Parameter passParam(paramRef);
   
   Shader lensShader = GetConnectedShader(passParam);
   
   // check if this is valid and really a shader of type lens
   if (lensShader.IsValid())
      if (IsLensShader(lensShader))
         result.Add(lensShader);
   // let's check for 10 lens shaders on the pass, at max
   for (LONG i=1; i<10; i++)
   {
      paramRef.Set(passName + L".LensShaderStack.Item[" + (CString)i + "]");
      Parameter passParam(paramRef);
      lensShader = GetConnectedShader(passParam);
      if (lensShader.IsValid())
      {
         if (IsLensShader(lensShader))
            result.Add(lensShader);
      }
      else // done with the stack
         break;
   }

   return result;
}
#endif


// Get the camera type
//
// @param in_xsiCamera         the Softimage camera
// @param in_property          the Arnold Camera Options property
// @param in_frame             the frame time
//
// @return the Arnold camera type
//
CString GetCameraType(const Camera &in_xsiCamera, const Property &in_property, double in_frame)
{
   CString result;

   if (!in_property.IsValid())
      result = L"persp_camera";
   else if (!ParAcc_Valid(in_property, L"camera_type"))
      result = L"persp_camera";
   else
      result = ((CString)ParAcc_GetValue(in_property, L"camera_type", in_frame));

   if (result == L"persp_camera") // maybe this is a Softimage orthographic camera
   {
      Primitive cameraPrimitive = CObjectUtilities().GetPrimitiveAtCurrentFrame(in_xsiCamera);
      if (ParAcc_GetValue(cameraPrimitive, L"proj", DBL_MAX) == 0)
         result = L"ortho_camera";
   }

   return result;
}


// Return the first valid lens shader belonging to the Softimage camera. 
// Called in case the current camera type is set to Custom in the camera options property.
//
// @param in_xsiCamera       The Softimage camera
// @param out_shader         The (first) valid lens shader
//
// @return true if a valid lens shader was found, else false
//
bool GetFirstLensShader(const Camera &in_xsiCamera, Shader &out_shader)
{
   CRefArray lensShaders = in_xsiCamera.GetShaders();
   for (LONG i=0; i<lensShaders.GetCount(); i++)
   {
      out_shader = lensShaders[i];
      if (IsLensShader(out_shader))
         return true;
      else
         GetMessageQueue()->LogMsg(L"[sitoa] Skipping " + out_shader.GetName() + L", not a valid camera shader type", siWarningMsg);
   }

   return false;
}


// Check if a shader is a lens shader. In fact, Softimage allows to connect as lens shaders also
// shaders of type texture, but this is not something we can support
//
// @param in_shader   The shader
//
// @return true if in_shader is an actual lens shader, else false

bool IsLensShader(const Shader &in_shader)
{
   ShaderDef shaderDef = in_shader.GetShaderDef();
   CStringArray families = shaderDef.GetShaderFamilies();
   for (LONG i=0; i<families.GetCount(); i++)
      if (families[i] == L"mrLens")
         return true;

   return false;
}

