/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsShader.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"

#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_passcontainer.h>
#include <xsi_pass.h>

// Update the operators connected to a RenderPass into Arnold
//
// @return CStatus::OK if all went well, else the error code
//
CStatus UpdatePassOperator(const Pass &in_pass, double in_frame)
{
   GetMessageQueue()->LogMsg(L"[sitoa] UpdatePassOperator");
   
   CStatus status(CStatus::OK);

   CRef operatorRef;
   operatorRef.Set(in_pass.GetFullName() + L".operator");
   Parameter operatorParam(operatorRef);

   AtNode* options = AiUniverseGetOptions();

   Shader operatorShader = GetConnectedShader(operatorParam);
   if (operatorShader.IsValid())
   {            
      AtNode* operatorNode = UpdateShader(operatorShader, in_frame);

      if (operatorNode)
         CNodeSetter::SetPointer(options, "operator", operatorNode);
   }
   else
     CNodeSetter::SetPointer(options, "operator", NULL); 

   return status;
}