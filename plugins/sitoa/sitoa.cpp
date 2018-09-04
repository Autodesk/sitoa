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

#include <xsi_application.h>
#include <xsi_argument.h>
#include <xsi_context.h>
#include <xsi_command.h>
#include <xsi_pluginregistrar.h>

using namespace XSI;

SITOA_CALLBACK XSILoadPlugin(PluginRegistrar& in_reg)
{
   // the arnold path needs to be in PATH for optix_denoiser.dll to be found
   // we don't know if Linux is affected so lets just implement it for Windows right now
   // https://github.com/Autodesk/sitoa/issues/8
   #ifdef _WINDOWS
      // get pluginPath and remove trailing slash
      CString pluginPath = in_reg.GetOriginPath();
      pluginPath.TrimRight(L"\\");

      // get PATH env
      char *pValue;
      size_t envBufSize;
      int err = _dupenv_s(&pValue, &envBufSize, "PATH");
      if (err)
         GetMessageQueue()->LogMsg(L"[sitoa] Failed to retrieve PATH environment.", siErrorMsg);
      else
      {
         const CString currentPath = pValue;
         free(pValue);

         // check so that pluginPath isn't already in PATH
         if (currentPath.FindString(pluginPath) == UINT_MAX)
         {
            // add pluginPath to begining of PATH
            string envPath = pluginPath.GetAsciiString();
            envPath += ";";
            envPath += currentPath.GetAsciiString();

            // set the new path
            err = _putenv_s("PATH", envPath.c_str());
            if (err)
                GetMessageQueue()->LogMsg(L"[sitoa] Failed to add Arnold path to PATH environment.", siErrorMsg);
         }
      }
   #endif

   // Plugins names are referenced from other cpps so
   // DO NOT change their names. (ex: GetPlugin("Arnold Render"))
   in_reg.PutAuthor(L"SolidAngle");
   in_reg.PutName(L"Arnold Render");
   in_reg.PutEmail(L"plugin-dev@solidangle.com");
   in_reg.PutURL(L"http://www.solidangle.com");
   // Don't set the help line here, else it overwrites the ones set for individual properties (rendering options and graphic sequencer)
   // in_reg.PutHelp(...
   in_reg.PutVersion(GetMajorVersion(), GetMinorVersion());
   ///////////////// Commands /////////////////
   // Destroy the universe
   in_reg.RegisterCommand (L"SITOA_DestroyScene",      L"SITOA_DestroyScene");
   // Export objects to ass
   in_reg.RegisterCommand (L"SITOA_ExportObjects",     L"SITOA_ExportObjects");
   // Export scene or selection to ass
   in_reg.RegisterCommand (L"SITOA_ExportScene",       L"SITOA_ExportScene");
   // Flush the loaded textures from memory
   in_reg.RegisterCommand (L"SITOA_FlushTextures",     L"SITOA_FlushTextures");
   // Return a float array with the transformation or deformation mb keys based in the rendering options
   in_reg.RegisterCommand (L"SITOA_GetMotionBlurKeys", L"SITOA_GetMotionBlurKeys");
   // Return the bounds of objects, including the motion blur
   in_reg.RegisterCommand (L"SITOA_GetBoundingBox",    L"SITOA_GetBoundingBox");
   // Return the so/dll shaders list
   in_reg.RegisterCommand (L"SITOA_GetShaderDef",      L"SITOA_GetShaderDef");
   // Print or return the Arnold and SItoA version 
   in_reg.RegisterCommand (L"SITOA_ShowVersion",       L"SITOA_ShowVersion");
   // Reurns the grids in a vdb file
   in_reg.RegisterCommand (L"SITOA_OpenVdbGrids",      L"SITOA_OpenVdbGrids");

   // Log the MAC address
   in_reg.RegisterCommand (L"SITOA_ShowMac", L"SITOA_ShowMac");
   // pitreg runner
   in_reg.RegisterCommand (L"SITOA_PitReg", L"SITOA_PitReg");

   ///////////////// Rendering options, preferences, engine /////////////////
   in_reg.RegisterProperty(L"Arnold Render Options");   // Render options
   in_reg.RegisterProperty(L"ArnoldRenderPreferences"); // Render preferences
   in_reg.RegisterRenderer(L"Arnold Render");           // The renderer

   ///////////////// The graphic sequencer for procedurals and its property /////////////////
   in_reg.RegisterDisplayPass(L"SITOA_Viewer");
   in_reg.RegisterProperty(L"SITOA_ViewerProperty");

   CString plugin_origin_path = in_reg.GetOriginPath();
   ///////////////// Events /////////////////
   if (Application().IsInteractive())
   {
      in_reg.RegisterEvent(L"SITOA_OnBeginPassChange", siOnBeginPassChange);
      in_reg.RegisterEvent(L"SITOA_OnCloseScene",      siOnCloseScene);
      in_reg.RegisterEvent(L"SITOA_OnObjectAdded",     siOnObjectAdded);
      in_reg.RegisterEvent(L"SITOA_OnObjectRemoved",   siOnObjectRemoved);
      in_reg.RegisterEvent(L"SITOA_OnValueChange",     siOnValueChange);
      in_reg.RegisterEvent(L"SITOA_ShaderDefEvent",    siOnStartup);
   }
   else // the shader def event does not work in batch mode
      GetRenderInstance()->ShaderDefSet().Load(plugin_origin_path);

   // Events to manage scene versioning (#1013)
   in_reg.RegisterEvent(L"SITOA_OnBeginSceneSave",   siOnBeginSceneSave);
   in_reg.RegisterEvent(L"SITOA_OnBeginSceneSaveAs", siOnBeginSceneSaveAs);
   in_reg.RegisterEvent(L"SITOA_OnEndSceneOpen",     siOnEndSceneOpen);
   // The event logging the messages. Triggered each tenth of a second, starting 1 second from now
   in_reg.RegisterTimerEvent(L"SITOA_Timer", 100, 1000);

   // Print the versions
   GetMessageQueue()->LogMsg(L"[sitoa] SItoA " + GetSItoAVersion() + L" loaded.");   
   CString aiVersion(AiGetVersion(NULL, NULL, NULL, NULL));
   GetMessageQueue()->LogMsg(L"[sitoa] Arnold " + aiVersion + L" detected.");

   return CStatus::OK;
}


SITOA_CALLBACK XSIUnloadPlugin(PluginRegistrar& in_reg)
{
   // lets remove pluginPath from PATH
   // https://github.com/Autodesk/sitoa/issues/8
   #ifdef _WINDOWS
      // get pluginPath and remove trailing slash
      CString pluginPath = in_reg.GetOriginPath();
      pluginPath.TrimRight(L"\\");

      // get PATH env
      char *pValue;
      size_t envBufSize;
      int err = _dupenv_s(&pValue, &envBufSize, "PATH");
      if (err)
         Application().LogMessage(L"[sitoa] Failed to retrieve PATH environment.", siErrorMsg);
      else
      {
         const CString currentPath = pValue;
         free(pValue);

         // check so that pluginPath is in beginning of PATH
         if (currentPath.FindString(pluginPath) == 0)
         {
            // remove pluginPath; from currentPath
            string envPath = currentPath.GetSubString(pluginPath.Length()+1).GetAsciiString();

            // set the new PATH
            err = _putenv_s("PATH", envPath.c_str());
            if (err)
                Application().LogMessage(L"[sitoa] Failed to remove Arnold path from PATH environment.", siErrorMsg);
         }
      }
   #endif

   Application().LogMessage(L"[sitoa] SItoA " + GetSItoAVersion() + L" has been unloaded.");
   return CStatus::OK;
}

