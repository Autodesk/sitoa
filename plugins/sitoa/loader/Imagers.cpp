/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsShader.h"
#include "loader/Imagers.h"
#include "loader/Shaders.h"
#include "renderer/Renderer.h"

#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_shader.h>
#include <xsi_status.h>
#include <xsi_passcontainer.h>
#include <xsi_pass.h>
#include <xsi_shaderarrayparameter.h>


// Load the imagers connected as LensShaders to a RenderPass and Cameras into Arnold and chains them correctly
//
// @param in_frame      the frame time
//
// @return CStatus::OK if all went well, else the error code
//
CStatus LoadImagers(double in_frame)
{
   CStatus status(CStatus::OK);

   // First check the pass to see how to handle lens shaders
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   int overrideCameraLensShaders = (int)ParAcc_GetValue(pass, L"OverrideCameraLensShaders", DBL_MAX);

   Shader cameraImagerShader;
   Shader passImagerShader;

   if (overrideCameraLensShaders != 3)
   {
      // Load camera lens shaders
      Camera renderCamera = GetRenderInstance()->GetRenderCamera();
      AtNode* cameraNode = GetRenderInstance()->NodeMap().GetExportedNode(renderCamera, in_frame);
      cameraImagerShader = LoadCameraImagers(cameraNode, renderCamera, in_frame);
   }

   if (overrideCameraLensShaders > 0)
   {
      // Load pass lens shaders
      passImagerShader = LoadPassImagers(pass, in_frame);
   }

   AtNode* rootImagerNode(NULL);
   if (cameraImagerShader.IsValid() && passImagerShader.IsValid())
      rootImagerNode = ConcatenateImagers(cameraImagerShader, passImagerShader, in_frame);
   else if (cameraImagerShader.IsValid())
      rootImagerNode = GetRenderInstance()->ShaderMap().Get(cameraImagerShader, in_frame);
   else if (passImagerShader.IsValid())
      rootImagerNode = GetRenderInstance()->ShaderMap().Get(passImagerShader, in_frame);

   if (rootImagerNode)
      SetImagerNode(rootImagerNode);

   return status;
}


// Load the first branch of imagers connected to a Camera
//
// @param in_cameraNode   the exported Camera node in Arnold
// @param in_xsiCamera    the Softimage Camera
// @param in_frame        the frame time
//
// @return the top imager shader
//
Shader LoadCameraImagers(AtNode* in_cameraNode, const Camera &in_xsiCamera, double in_frame)
{
   Shader topImagerShader;

   if (in_cameraNode)
   {
      CRefArray lensShaders = in_xsiCamera.GetShaders();
      //CRefArray cameraImagers;
      for (LONG i=0; i<lensShaders.GetCount(); i++)
      {
         Shader lensShader = lensShaders[i];
         topImagerShader = LoadImager(lensShader, in_frame);
         if (topImagerShader.IsValid())
            return topImagerShader;
      }
   }
   return Shader();
}


// Load the first branch of imagers connected to a Pass
//
// @param in_pass       the pass
// @param in_frame      the frame time
//
// @return the top imager shader
//
Shader LoadPassImagers(const Pass &in_pass, double in_frame)
{
   Shader topImagerShader;

   CRef lensStackRef;
   lensStackRef.Set(in_pass.GetFullName() + L".LensShaderStack");
   ShaderArrayParameter arrayParam = ShaderArrayParameter(lensStackRef);

   for (LONG i=0; i<arrayParam.GetCount(); i++)
   {
      Parameter param = Parameter(arrayParam[i]);

      Shader lensShader = GetConnectedShader(param);
      topImagerShader = LoadImager(lensShader, in_frame);
      if (topImagerShader.IsValid())
         return topImagerShader;
   }
   return Shader();
}


// Load the real imagers and export them to Arnold
//
// @param in_imagerDummyShader   the dommy imager shader that imagers are connected to
// @param in_frame               the frame time
//
// @return the top imager shader
//
Shader LoadImager(const Shader &in_imagerDummyShader, double in_frame)
{
   if (in_imagerDummyShader.IsValid())
   {
      // find the first 'imager' shader, dummy shader node for supporting arnold imagers
      if (GetShaderNameFromProgId(in_imagerDummyShader.GetProgID()) == L"imager")
      {
         Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
         // get what's connected to that dummy shader's operator parameter
         Parameter imagerParam = in_imagerDummyShader.GetParameter(L"imager");
         Shader imagerShader = GetShaderFromSource(imagerParam.GetSource());
         if (imagerShader.IsValid())
         {
            AtNode* imagerNode = LoadShader(imagerShader, in_frame, pass.GetRef(), RECURSE_FALSE);
            if (imagerNode)
               return imagerShader;
         }
      }
   }
   return Shader();
}


// Get the first imager in the branch
//
// @param in_xsiShader   the top imager shader
//
// @return the bottom imager shader
//
Shader GetFirstImagerShaderInBranch(const Shader &in_xsiShader)
{
   Shader connectedShader;

   if (CStringUtilities().StartsWith(GetShaderNameFromProgId(in_xsiShader.GetProgID()), L"imager_"))
   {
      CRefArray paramsArray = in_xsiShader.GetParameters();
      // loop the parameters
      for (LONG i=0; i<paramsArray.GetCount(); i++)
      {
         Parameter param(paramsArray[i]);

         if (!param.IsValid())
            continue;

         CString paramScriptName = param.GetScriptName();

         // skip all the params with name other than "input"
         if (!paramScriptName.IsEqualNoCase(L"input"))
            continue;

         CRef source = GetParameterSource(param);
         siClassID sourceID = source.GetClassID();
         if (sourceID == siTextureID)
         {
            Shader shader(source);
            connectedShader = GetFirstImagerShaderInBranch(shader);
         }
         else
            connectedShader = in_xsiShader;

         break; // early exit after "input" is found
      }
   }

   return connectedShader;
}


// Concatenate imagers from Pass and Camera according to Pass settings
//
// @param in_cameraImagerShader   the top camera imager shader
// @param in_passImagerShader     the top pass imager shader
//
// @return the top imager node
//
AtNode* ConcatenateImagers(const Shader &in_cameraImagerShader, const Shader &in_passImagerShader, double in_frame)
{
   AtNode* root_imager(NULL);
   // First check the pass to see how to handle lens shaders
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   int overrideCameraLensShaders = (int)ParAcc_GetValue(pass, L"OverrideCameraLensShaders", DBL_MAX);

   if (overrideCameraLensShaders == 0) // Use Only Camera Lens Shaders
      root_imager = GetRenderInstance()->ShaderMap().Get(in_cameraImagerShader, in_frame);

   else if (overrideCameraLensShaders == 1) // Add After Camera Lens Shaders
   {
      root_imager = GetRenderInstance()->ShaderMap().Get(in_passImagerShader, in_frame);
      Shader firstPassShader = GetFirstImagerShaderInBranch(in_passImagerShader);
      AtNode* firstPassNode = GetRenderInstance()->ShaderMap().Get(firstPassShader, in_frame);
      AtNode* cameraImager = GetRenderInstance()->ShaderMap().Get(in_cameraImagerShader, in_frame);
      CNodeSetter::SetPointer(firstPassNode, "input", cameraImager);  // attach camera imager to first pass imager
   }

   else if (overrideCameraLensShaders == 2) // Add Before Camera Lens Shaders
   {
      root_imager = GetRenderInstance()->ShaderMap().Get(in_cameraImagerShader, in_frame);
      Shader firstCameraShader = GetFirstImagerShaderInBranch(in_cameraImagerShader);
      AtNode* firstCameraNode = GetRenderInstance()->ShaderMap().Get(firstCameraShader, in_frame);
      AtNode* passImager = GetRenderInstance()->ShaderMap().Get(in_passImagerShader, in_frame);
      CNodeSetter::SetPointer(firstCameraNode, "input", passImager);  // attach pass imager to first camera imager
   }

   else if (overrideCameraLensShaders == 3) // Overwrite Camera Lens Shaders
      root_imager = GetRenderInstance()->ShaderMap().Get(in_passImagerShader, in_frame);

   return root_imager;
}


// Sets the root imager to all output drivers
//
// @param in_rootImagerNode   the top imager
//
// @return true if the value was well set or let in place, else false
//
bool SetImagerNode(AtNode* in_rootImagerNode)
{
   bool status(false);
   // iterate the (possible) output drivers and add the imagers to the ones that exist
   CStringArray driverNames = GetDriverNames();
   driverNames.Add(L"xsi_driver");
   for (LONG i=0; i<driverNames.GetCount(); i++)
   {
      AtNode* driverNode = AiNodeLookUpByName(NULL, driverNames[i].GetAsciiString());
      if (driverNode)
      {
         status = CNodeSetter::SetPointer(driverNode, "input", in_rootImagerNode);
      }
   }
   return status;
}
