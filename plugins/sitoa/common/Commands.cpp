/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "version.h"
#include "loader/Loader.h"
#include "renderer/Renderer.h"

#include <xsi_argument.h>
#include <xsi_command.h>
#include <xsi_plugin.h>
#include <xsi_project.h>
#include <xsi_scene.h>


// Destroy the universe
//
SITOA_CALLBACK SITOA_DestroyScene_Execute(CRef& in_ctxt)
{
   GetRenderInstance()->DestroyScene(true);
   return CStatus::OK;
}


// Export objects to ass
//
SITOA_CALLBACK SITOA_ExportObjects_Init(CRef& in_ctxt)
{
   Context ctx(in_ctxt);
   Command cmd(ctx.GetSource());

   ArgumentArray args = cmd.GetArguments();

   args.Add(L"startFrame",    CValue());
   args.Add(L"endFrame",      CValue());
   args.Add(L"frameStep",     CValue());
   args.Add(L"createStandIn", false );      // Create a standin instead of exporting the scene
   args.AddWithHandler(L"objects", siArgHandlerCollection);
   args.Add(L"recurse",       false);
   args.Add(L"filename",      L"");

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_ExportObjects_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   CValueArray args = ctxt.GetAttribute(L"Arguments");

   double    startFrame    = (double) args[0];
   double    endFrame      = (double) args[1];
   LONG      frameStep     = (LONG)   args[2];
   bool      createStandIn = (bool)   args[3];
   CRefArray objects       = args[4];             // the objects to export
   bool      recurse       = (bool)   args[5];
   CString   filename      = (CString)args[6];

   // Get the render options from the active pass.
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   Property arnoldOptions(pass.GetProperties().GetItem(L"Arnold Render Options"));

   return LoadScene(arnoldOptions, L"Export", startFrame, endFrame, frameStep, createStandIn, true, filename, true, objects, recurse);
}


// Export scene or selection to ass
//
SITOA_CALLBACK SITOA_ExportScene_Init(CRef& in_ctxt)
{
   Context ctx(in_ctxt);
   Command cmd(ctx.GetSource());

   ArgumentArray args = cmd.GetArguments();

   args.Add(L"startFrame",     CValue());
   args.Add(L"endFrame",       CValue());
   args.Add(L"frameStep",      CValue());
   args.Add( L"createStandIn", false );
   args.Add( L"selectionOnly", false );      // selection only
   args.Add( L"filename",      L"" );

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_ExportScene_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   CValueArray args = ctxt.GetAttribute(L"Arguments");

   double  startFrame    = (double) args[0];
   double  endFrame      = (double) args[1];
   LONG    frameStep     = (LONG)   args[2];
   bool    createStandIn = (bool)   args[3];
   bool    selectionOnly = (bool)   args[4];
   CString filename      = (CString)args[5];
   
   // Get render options from active pass.
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   Property arnoldOptions(pass.GetProperties().GetItem(L"Arnold Render Options"));

   return LoadScene(arnoldOptions, L"Export", startFrame, endFrame, frameStep, createStandIn, true, filename, selectionOnly);
}


// Flush the loaded textures from memory
//
SITOA_CALLBACK SITOA_FlushTextures_Execute(CRef& in_ctxt)
{
   GetRenderInstance()->FlushTextures(); 
   return CStatus::OK;
}


// Return a float array with the transformation or deformation mb keys based in the rendering options
//
SITOA_CALLBACK SITOA_GetMotionBlurKeys_Init(CRef& in_ctxt)
{
   Context ctx(in_ctxt);
   Command cmd(ctx.GetSource());

   ArgumentArray args = cmd.GetArguments();

   args.AddObjectArgument(L"obj");
   args.Add(L"deformation", CValue()); // Transformation or deformation blur
   args.Add(L"frame",       CValue()); // Frame

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_GetMotionBlurKeys_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   CValueArray args = ctxt.GetAttribute(L"Arguments");

   CRef objRef = (CRef)args[0];
   bool deformationMBlur = !args[1].IsEmpty() ? (bool)args[1] : false;
   CValue frame = args[2];

   if (frame.IsEmpty())
   {
      CValue currentFrame(CTimeUtilities().GetCurrentFrame());
      frame.Attach(currentFrame);
   }

   CDoubleArray transfKeys, defKeys;
   CFloatArray result;

   CRefArray dummyProperties = X3DObject(objRef).GetLocalProperties();
   CSceneUtilities::GetMotionBlurData(objRef, transfKeys, defKeys, double(frame));

   // We can only return CFloatArray so we must convert the result
   if (deformationMBlur)
   {
      for (LONG i=0; i<defKeys.GetCount(); i++)
         result.Add((float)defKeys[i]);
   }
   else
   {
      for (LONG i=0; i<transfKeys.GetCount(); i++)
         result.Add((float)transfKeys[i]);
   }

   CValue retVal(result);

   ctxt.PutAttribute( L"ReturnValue", retVal);

   return CStatus::OK;
}


// #1343, patch by Xavier Lapointe
// Return the bounds of objects, including the motion blur
//
// Example of how to run it (js) :
// var rtn = SITOA_GetBoundingBox(1, selection);
// var bb = rtn.toArray();
// logMessage("min = " + bb[0] + " " + bb[1] + " " + bb[2]);
// logMessage("max = " + bb[3] + " " + bb[4] + " " + bb[5]);
//
SITOA_CALLBACK SITOA_GetBoundingBox_Init(CRef& in_ctxt)
{
   Context ctx(in_ctxt);
   Command cmd(ctx.GetSource());

   ArgumentArray args = cmd.GetArguments();

   args.Add(L"frame", CValue());
   args.AddWithHandler(L"objects", siArgHandlerCollection);

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_GetBoundingBox_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   CValueArray args = ctxt.GetAttribute(L"Arguments");

   CValue frame = args[0];
   if (frame.IsEmpty())
   {
      CValue currentFrame(CTimeUtilities().GetCurrentFrame());
      frame.Attach(currentFrame);
   }

   CRefArray objects = args[1];

   double xmin, ymin, zmin, xmax, ymax, zmax;
   GetBoundingBoxFromObjects(frame, objects, xmin, ymin, zmin, xmax, ymax, zmax);

   CFloatArray result;
   result.Add((float)xmin);
   result.Add((float)ymin);
   result.Add((float)zmin);
   result.Add((float)xmax);
   result.Add((float)ymax);
   result.Add((float)zmax);

   CValue retVal(result);
   ctxt.PutAttribute( L"ReturnValue", retVal);

   return CStatus::OK;
}


// Return the so/dll shaders list.
//
SITOA_CALLBACK SITOA_GetShaderDef_Init(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   Command oCmd = ctxt.GetSource();
   oCmd.EnableReturnValue(true);
   // this command is called by the js plugin for building the menu, so don't log it
   // into the script editor
   oCmd.SetFlag(siNoLogging, true);

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_GetShaderDef_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   // get the list
   CStringArray progIds = GetRenderInstance()->ShaderDefSet().GetProgIds();

   CString result; // push the items into a single string, each item separated by ';'
   for (LONG i=0; i<progIds.GetCount(); i++)
   {
      result+= progIds[i];
      if (i != progIds.GetCount()-1)
         result+= L";";
   }

   CValue retval(result);
   ctxt.PutAttribute( L"ReturnValue", retval);
   return CStatus::OK;
}


// Print or return the Arnold and SItoA version 
SITOA_CALLBACK SITOA_ShowVersion_Init(CRef& in_ctxt)
{
	Context ctxt(in_ctxt);
	Command oCmd = ctxt.GetSource();
	oCmd.EnableReturnValue(true);

	ArgumentArray oArgs = oCmd.GetArguments();
	oArgs.Add(L"log", true);

	return CStatus::OK;
}


SITOA_CALLBACK SITOA_ShowVersion_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);

	CValueArray args = ctxt.GetAttribute(L"Arguments");
	bool log = (bool)args[0];

   CString sitoaVersion = GetSItoAVersion(log);
   CString aiVersion(AiGetVersion(NULL, NULL, NULL, NULL));

   if (log)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] SItoA " + sitoaVersion + L" loaded.");   
      GetMessageQueue()->LogMsg(L"[sitoa] Arnold " + aiVersion + L" detected.");
   }
   else
   {
      CString outString = sitoaVersion;
      outString+= L";";
      outString+= aiVersion;
      ctxt.PutAttribute(L"ReturnValue", outString);
   }

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_OpenVdbGrids_Init(CRef& in_ctxt)
{
	Context ctxt(in_ctxt);
	Command cmd = ctxt.GetSource();
	cmd.EnableReturnValue(true);

	ArgumentArray args = cmd.GetArguments();
	args.Add( L"filename", L"" );

	return CStatus::OK;
}


SITOA_CALLBACK SITOA_OpenVdbGrids_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);

	CValueArray args = ctxt.GetAttribute(L"Arguments");
   CString filename = (CString)args[0];

   AtArray *grids = AiVolumeFileGetChannels(filename.GetAsciiString());

   CString result = L"";

   uint32_t nb_grids = AiArrayGetNumElements(grids);
   for (uint32_t i = 0; i < nb_grids; i++)
   {
      AtString grid = AiArrayGetStr(grids, i);
      result+= CString(grid.c_str());
      if (i < nb_grids - 1)
         result+= L" ";
   }

   ctxt.PutAttribute(L"ReturnValue", result);

   return CStatus::OK;
}


// Return the MAC address and the kick -licensecheck output
SITOA_CALLBACK SITOA_ShowMac_Init(CRef& in_ctxt)
{
	Context ctxt(in_ctxt);
	Command oCmd = ctxt.GetSource();
	oCmd.EnableReturnValue(true);
   oCmd.SetFlag(siNoLogging, true);
	return CStatus::OK;
}

SITOA_CALLBACK SITOA_ShowMac_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   // this plugin
   Plugin plugin(Application().GetPlugins().GetItem(L"Arnold Render"));
   CString originPath = plugin.GetOriginPath();
   
   // build path to rlmhostid -q ether
   // CString rlmutil = CUtils::IsWindowsOS() ? 
   //                   originPath + L"..\\..\\license\\rlmutil.exe" :
   //                  originPath + L"../../../license/rlmutil";
   CString rlmutil = CUtils::IsWindowsOS() ? 
                     originPath + L"license\\rlmutil.exe" :
                     originPath + L"license/rlmutil";

   if (!CPathUtilities().PathExists(rlmutil.GetAsciiString()))
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Cannot find " + rlmutil, siErrorMsg);   
      ctxt.PutAttribute(L"ReturnValue", L"");
      return CStatus::OK;
   }

   ///////////////////////////////////////////////////////
   CString rlmutilCommand = rlmutil + L" rlmhostid -q ether";
#ifdef _WINDOWS
   FILE* pipe = _popen(rlmutilCommand.GetAsciiString(), "r");
#else
   FILE* pipe = popen(rlmutilCommand.GetAsciiString(), "r");
#endif

   if (!pipe)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Failed opening pipe for " + rlmutilCommand, siErrorMsg);   
      ctxt.PutAttribute(L"ReturnValue", L"");
      return CStatus::OK;
   }

   CString mac;
   char line[1024];
   if (fgets(line, 1024, pipe))
      mac = line;

#ifdef _WINDOWS
   _pclose(pipe);
#else
   pclose(pipe);
#endif

   ///////////////////////////////////////////////////////
   // build path to kick -licensecheck
   CString kick = CUtils::IsWindowsOS() ? CUtils::BuildPath(originPath, L"kick.exe") : CUtils::BuildPath(originPath, L"kick");
   if (!CPathUtilities().PathExists(kick.GetAsciiString()))
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Cannot find " + kick, siErrorMsg);   
      ctxt.PutAttribute(L"ReturnValue", L"");
      return CStatus::OK;
   }

   CString kickCommand = kick + L" -licensecheck";

#ifdef _WINDOWS
   pipe = _popen(kickCommand.GetAsciiString(), "r");
#else
   pipe = popen(kickCommand.GetAsciiString(), "r");
#endif

   if (!pipe)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Failed opening pipe for " + kickCommand, siErrorMsg);   
      ctxt.PutAttribute(L"ReturnValue", L"");
      return CStatus::OK;
   }

   CStringArray licenseCheck;
   while (fgets(line, 1024, pipe))
   {
      line[strlen(line)-1] = '\0';
      
      char* p = line;
      char* p2 = line;
      while (p)
      {
         *p2 = *p;
         if (*p == '\0')
            break;

         p++;
         p2++;

         while (*p == ' ' && *(p+1) == ' ') // shrink spaces to a single one
            p++;
      }

      licenseCheck.Add((CString)line);
   }

#ifdef _WINDOWS
   _pclose(pipe);
#else
   pclose(pipe);
#endif

   ///////////////////////////////////////////////////////
   rlmutilCommand = rlmutil + L" rlmdebug arnold";
#ifdef _WINDOWS
   pipe = _popen(rlmutilCommand.GetAsciiString(), "r");
#else
   pipe = popen(rlmutilCommand.GetAsciiString(), "r");
#endif

   if (!pipe)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Failed opening pipe for " + rlmutilCommand, siErrorMsg);   
      ctxt.PutAttribute(L"ReturnValue", L"");
      return CStatus::OK;
   }

   CStringArray debug;
   while (fgets(line, 1024, pipe))
   {
      line[strlen(line)-1] = '\0';
      debug.Add((CString)line);
   }

#ifdef _WINDOWS
   _pclose(pipe);
#else
   pclose(pipe);
#endif


   CString result = mac;

   result+= L"separator";

   for (LONG i = 0; i < licenseCheck.GetCount(); i++)
   {
      if (i > 0)
         result+= L";";
      result+= licenseCheck[i];
   }

   result+= L"separator";

   for (LONG i = 0; i < debug.GetCount(); i++)
   {
      if (i > 0)
         result+= L";";
      result+= debug[i];
   }

   ctxt.PutAttribute(L"ReturnValue", result);
   return CStatus::OK;
}


SITOA_CALLBACK SITOA_ADPSettings_Init(CRef& in_ctxt)
{
	Context ctxt(in_ctxt);
	Command oCmd = ctxt.GetSource();
	oCmd.EnableReturnValue(false);
   oCmd.SetFlag(siNoLogging, true);
	return CStatus::OK;
}

SITOA_CALLBACK SITOA_ADPSettings_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   AiADPDisplayDialogWindow();
   return CStatus::OK;
}


// Run pitreg
SITOA_CALLBACK SITOA_PitReg_Init(CRef& in_ctxt)
{
	Context ctxt(in_ctxt);
	Command oCmd = ctxt.GetSource();
	oCmd.EnableReturnValue(false);
   oCmd.SetFlag(siNoLogging, true);
	return CStatus::OK;
}

SITOA_CALLBACK SITOA_PitReg_Execute(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   // this plugin
   Plugin plugin(Application().GetPlugins().GetItem(L"Arnold Render"));
   CString originPath = plugin.GetOriginPath();
   
   // build path to pitreg
   CString pitreg = CUtils::IsWindowsOS() ? 
                    originPath + L"license\\pit\\pitreg.exe" :
                    originPath + L"license/pit/pitreg";

   if (!CPathUtilities().PathExists(pitreg.GetAsciiString()))
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Cannot find " + pitreg, siErrorMsg);
      return CStatus::OK;
   }

#ifdef _WINDOWS
   FILE* pipe = _popen(pitreg.GetAsciiString(), "r");
#else
   FILE* pipe = popen(pitreg.GetAsciiString(), "r");
#endif

   if (!pipe)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Failed opening pipe for " + pitreg, siErrorMsg);   
      ctxt.PutAttribute(L"ReturnValue", L"");
      return CStatus::OK;
   }

#ifdef _WINDOWS
   _pclose(pipe);
#else
   pclose(pipe);
#endif

   return CStatus::OK;
}
