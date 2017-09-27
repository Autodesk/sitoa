/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "version.h"
#include "renderer/Renderer.h"

#include <xsi_project.h>
#include <xsi_scene.h>

/////////////////////////////////////
// Interactive events
/////////////////////////////////////

SITOA_CALLBACK SITOA_OnBeginPassChange_OnEvent(CRef& in_ctxt)
{  
   GetRenderInstance()->DestroyScene(false);
   return CStatus::False;
}


SITOA_CALLBACK SITOA_OnCloseScene_OnEvent(CRef& in_ctxt)
{
   GetRenderInstance()->DestroyScene(true);
   // zero the flythrough frame on a new scene, so it will be initialized on the first process
   GetRenderInstance()->SetFlythroughFrame(FRAME_NOT_INITIALIZED_VALUE); 
   return CStatus::False;
}


SITOA_CALLBACK SITOA_OnObjectAdded_OnEvent(CRef& in_ctxt)
{
   return GetRenderInstance()->OnObjectAdded(in_ctxt);
}


SITOA_CALLBACK SITOA_OnObjectRemoved_OnEvent(CRef& in_ctxt)
{
   return GetRenderInstance()->OnObjectRemoved(in_ctxt);
}


SITOA_CALLBACK SITOA_OnValueChange_OnEvent(CRef& in_ctxt)
{
   return GetRenderInstance()->OnValueChange(in_ctxt);
}


/////////////////////////////////////
// Events
/////////////////////////////////////

// Write the currently installed SItoA version into the Arnold options before saving
//
SITOA_CALLBACK SITOA_OnBeginSceneSave_OnEvent(CRef& in_ctxt)
{
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   Property arnoldOptions(pass.GetProperties().GetItem(L"Arnold Render Options"));
   if (!arnoldOptions.IsValid())
      return CStatus::False;
   
   // get the hidden parameter where we write the version
   Parameter p = arnoldOptions.GetParameter(L"sitoa_version");

   if (p.IsValid()) // stamp the version before saving the scene
   {
      CString currentVersion = GetSItoAVersion(false);
      p.PutValue(currentVersion);
   }

   return CStatus::False;
}


// Same as above, but for Save AS
//
SITOA_CALLBACK SITOA_OnBeginSceneSaveAs_OnEvent(CRef& in_ctxt)
{
   return SITOA_OnBeginSceneSave_OnEvent(in_ctxt);
}


// Compare the SItoA version the scene was save in with the currenlty installed SItoA
//
SITOA_CALLBACK SITOA_OnEndSceneOpen_OnEvent(CRef& in_ctxt)
{
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   Property arnoldOptions(pass.GetProperties().GetItem(L"Arnold Render Options"));
   if (!arnoldOptions.IsValid())
      return CStatus::False;
   
   CString sceneVersion(L"2.10 or older");
   CString currentVersion = GetSItoAVersion(false);
   // get the hidden parameter where we write the version
   Parameter p = arnoldOptions.GetParameter(L"sitoa_version");

   if (p.IsValid()) // if not valid, because of scenes < 3.0, stays "2.10 or older"
      sceneVersion = p.GetValue().GetAsText();

   if ((!sceneVersion.IsEmpty()) && (sceneVersion != currentVersion))
      GetMessageQueue()->LogMsg(L"[sitoa] Loaded scene was created with SItoA " + sceneVersion);

   return CStatus::False;
}


// On startup, for the so/dll shaders definition
//
SITOA_CALLBACK SITOA_ShaderDefEvent_OnEvent(CRef& in_ctxt)
{
   CString plugin_origin_path = CPathUtilities().GetShadersPath();
   GetRenderInstance()->ShaderDefSet().Load(plugin_origin_path); // load all shaders definition
   return CStatus::False;
}


// The timer event, triggered each tenth of a second, logging out the massage queue
//
SITOA_CALLBACK SITOA_Timer_OnEvent(CRef& in_ctxt)
{
   // LONG pid = CThreadUtilities::GetThreadId();
   GetMessageQueue()->Log();
   return CStatus::False;
}

