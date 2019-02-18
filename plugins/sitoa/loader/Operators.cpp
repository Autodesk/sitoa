/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_passcontainer.h>
#include <xsi_pass.h>

#include "renderer/Renderer.h"

// Load the operators connected to a RenderPass into Arnold
//
// @return CStatus::OK if all went well, else the error code
//
CStatus LoadPassOperator()
{
   CStatus status(CStatus::OK);

   GetMessageQueue()->LogMsg(L"[sitoa] loading some operators...");

   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());

   CRef operatorRef;
   operatorRef.Set(pass.GetFullName() + L".operator");
   CString idName = operatorRef.GetClassIDName();
   GetMessageQueue()->LogMsg(L"[sitoa]     " + idName);

   return status;
}