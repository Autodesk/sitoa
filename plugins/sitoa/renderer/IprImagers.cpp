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
#include "renderer/IprImagers.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"

#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_shader.h>
#include <xsi_status.h>
#include <xsi_passcontainer.h>
#include <xsi_pass.h>
#include <xsi_shaderarrayparameter.h>


// Update the imagers connected as LensShaders to a RenderPass and Cameras into Arnold and chains them correctly
//
// @param in_frame      the frame time
//
// @return CStatus::OK if all went well, else the error code
//
CStatus UpdateImagers(double in_frame)
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
      cameraImagerShader = UpdateCameraImagers(cameraNode, renderCamera, in_frame);
   }

   if (overrideCameraLensShaders > 0)
   {
      // Load pass lens shaders
      passImagerShader = UpdatePassImagers(pass, in_frame);
   }

   AtNode* rootImagerNode(NULL);

   if (cameraImagerShader.IsValid() && passImagerShader.IsValid())
      rootImagerNode = ConcatenateImagers(cameraImagerShader, passImagerShader, in_frame);
   else if (cameraImagerShader.IsValid())
      rootImagerNode = GetRenderInstance()->ShaderMap().Get(cameraImagerShader, in_frame);
   else if (passImagerShader.IsValid())
      rootImagerNode = GetRenderInstance()->ShaderMap().Get(passImagerShader, in_frame);

   SetImagerNode(rootImagerNode);

   return status;
}


// Update the first branch of imagers connected to a Camera
//
// @param in_cameraNode the exported Camera node in Arnold
// @param in_xsiCamera  the Softimage Camera
// @param in_frame      the frame time
//
// @return the top imager shader
//
Shader UpdateCameraImagers(AtNode* in_cameraNode, const Camera &in_xsiCamera, double in_frame)
{
   Shader topImagerShader;

   if (in_cameraNode)
   {
      CRefArray lensShaders = in_xsiCamera.GetShaders();
      //CRefArray cameraImagers;
      for (LONG i=0; i<lensShaders.GetCount(); i++)
      {
         Shader lensShader = lensShaders[i];
         topImagerShader = UpdateImager(lensShader, in_frame);
         if (topImagerShader.IsValid())
            return topImagerShader;
      }
   }
   return Shader();
}


// Update the first branch of imagers connected to a Pass
//
// @param in_pass       the pass
// @param in_frame      the frame time
//
// @return the top imager shader
//
Shader UpdatePassImagers(const Pass &in_pass, double in_frame)
{
   Shader topImagerShader;

   CRef lensStackRef;
   lensStackRef.Set(in_pass.GetFullName() + L".LensShaderStack");
   ShaderArrayParameter arrayParam = ShaderArrayParameter(lensStackRef);

   for (LONG i=0; i<arrayParam.GetCount(); i++)
   {
      Parameter param = Parameter(arrayParam[i]);

      Shader lensShader = GetConnectedShader(param);
      topImagerShader = UpdateImager(lensShader, in_frame);
      if (topImagerShader.IsValid())
         return topImagerShader;
   }
   return Shader();
}


// Update the real imagers and export them to Arnold
//
// @param in_imagerDummyShader   the dommy imager shader that imagers are connected to
// @param in_frame               the frame time
//
// @return the top imager shader
//
Shader UpdateImager(const Shader &in_imagerDummyShader, double in_frame)
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
            AtNode* imagerNode = UpdateShader(imagerShader, in_frame);
            if (imagerNode)
               return imagerShader;
         }
      }
   }
   GetMessageQueue()->LogMsg(L"Returning empty shader");
   return Shader();
}
