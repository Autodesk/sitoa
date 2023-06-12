/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/Tools.h"
#include "loader/Cameras.h"
#include "loader/Hairs.h"
#include "loader/Imagers.h"
#include "loader/Instances.h"
#include "loader/Loader.h"
#include "loader/Options.h"
#include "loader/Polymeshes.h"
#include "loader/Shaders.h"
#include "loader/Procedurals.h"
#include "loader/Operators.h"
#include "renderer/RenderMessages.h"
#include "renderer/Renderer.h"

#include "version.h"

#include <xsi_passcontainer.h>
#include <xsi_progressbar.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_selection.h>
#include <xsi_uitoolkit.h>

#include <ctime>


CStatus LoadScene(const Property &in_arnoldOptions, const CString& in_renderType, double in_frameIni, double in_frameEnd, LONG in_frameStep, 
                  bool in_createStandIn, bool in_useProgressBar, CString in_filename, bool in_selectionOnly, CRefArray in_objects, bool in_recurse)
{
   CStatus status;
   Application app;
   
   GetRenderOptions()->Read(in_arnoldOptions); // read all the rendering options
   GetRenderInstance()->SetRenderType(in_renderType);

   bool toRender = true;

   if (in_renderType == L"Export")
   {
      GetRenderInstance()->SetInterruptRenderSignal(false);
      toRender = false;
   }

   if (in_createStandIn)
   {
      // if we create a standin and the frame count == 1
      if (in_filename.IsEmpty())
      {
         if (in_frameIni == in_frameEnd)
         {
            GetMessageQueue()->LogMsg(L"[sitoa] Trying to create standin without filename.",siErrorMsg);
            return CStatus::Fail;
         }
      }
   }

   CRefArray selectedObjs;
   if (in_renderType == L"Region" && in_selectionOnly) // isolate selection case
   {
      for (LONG i=0; i<in_objects.GetCount(); i++)
      {
         CRef ref(in_objects.GetItem(i));
         bool branchSel = ProjectItem(ref).GetSelected(siBranch);
         AddCRefToArray(selectedObjs, ref, branchSel);
      }
   }
   // if in_objects, it means that we were called by SITOA_ExportObjects, and, in this case, 
   // in_selectionOnly is set to true
   else if (in_objects.GetCount() > 0) 
   {
      for (LONG i=0; i<in_objects.GetCount(); i++)
      {
         CRef ref(in_objects.GetItem(i));
         AddCRefToArray(selectedObjs, ref, in_recurse);
      }
   }
   else if (in_selectionOnly) // in_objects is void, we're using the Soft selection
   {
      // First get all objects selected in branch
      Selection selection = app.GetSelection();
      LONG nbSelection = selection.GetCount();
      for (LONG i=0; i<nbSelection; i++)
      {
         CRef ref(selection.GetItem(i));
         bool branchSel = ProjectItem(ref).GetSelected(siBranch);
         AddCRefToArray(selectedObjs, ref, branchSel);
      }
   }

   // Clocks for Time statistics
   clock_t loadStart(0), loadEnd(0), dumpStart(0), dumpEnd(0);
   // Progress bar
   ProgressBar progressBar;

   // Compute the node mask
   // With a temporary Checking until it is completelly instaurated 
   // in all production scenes (for backward compatibility). Initializating to true
   int output_options         = AI_NODE_OPTIONS + AI_NODE_COLOR_MANAGER;
   int output_drivers_filters = AI_NODE_DRIVER + AI_NODE_FILTER;
   int output_geometry        = AI_NODE_SHAPE;
   int output_cameras         = AI_NODE_CAMERA;
   int output_lights          = AI_NODE_LIGHT;
   int output_shaders         = AI_NODE_SHADER;
   int output_operators       = AI_NODE_OPERATOR;

   CPathString outputAssDir, assOutputName;
   bool useTranslation;

   output_options  = toRender || GetRenderOptions()->m_output_options ? AI_NODE_OPTIONS + AI_NODE_COLOR_MANAGER: 0;
   output_drivers_filters = toRender || GetRenderOptions()->m_output_drivers_filters ? AI_NODE_DRIVER + AI_NODE_FILTER : 0;
   output_geometry   = toRender || GetRenderOptions()->m_output_geometry  ? AI_NODE_SHAPE : 0;
   output_cameras    = toRender || GetRenderOptions()->m_output_cameras ? AI_NODE_CAMERA : 0;
   output_lights     = toRender || GetRenderOptions()->m_output_lights  ? AI_NODE_LIGHT  : 0;
   output_shaders    = toRender || GetRenderOptions()->m_output_shaders ? AI_NODE_SHADER : 0;
   output_operators  = toRender || GetRenderOptions()->m_output_operators ? AI_NODE_OPERATOR : 0;

   SceneRenderProperty sceneRenderProp(app.GetActiveProject().GetActiveScene().GetPassContainer().GetProperties().GetItem(L"Scene Render Options"));

   useTranslation = GetRenderOptions()->m_use_path_translations;

   // Initializing translations paths tables (only to export to ass)
   if (!toRender && useTranslation)
      InitializePathTranslator();

   // The PlayControl property set is stored with scene data under the project
   Property playctrl = app.GetActiveProject().GetProperties().GetItem(L"Play Control");

   // Checking output ass path
   if (!toRender)
   {
      if (in_filename.IsEmpty())
         outputAssDir = CPathUtilities().GetOutputAssPath();
      else
      {
         // We could use filename.ReverseFindString(CUtils::Slash()) but it's
         // only available for XSISDK >= 7500 and it causes all sorts of
         // issues on Linux (cf. ticket #956). Let's do it manually.
         LONG index = in_filename.Length() - 1;
         while (index >= 0 && CString(in_filename[index]) != CString(CUtils::Slash()[0]))
             --index;
         
         if (index == -1)
            outputAssDir = L".";
         else
            outputAssDir = in_filename.GetSubString(0, index);
      }

      if (!CUtils::EnsureFolderExists(outputAssDir, false))
      {
         GetMessageQueue()->LogMsg(CString(L"[sitoa] ASS output path is not valid: ") + outputAssDir, siErrorMsg);
         return CStatus::Fail;
      }

      // Progress bar
      if (in_useProgressBar)
      {
         UIToolkit kit = app.GetUIToolkit();
         progressBar = kit.GetProgressBar();
         progressBar.PutMaximum((((LONG)in_frameEnd - (LONG)in_frameIni + 2)));
         progressBar.PutValue(1);
         progressBar.PutStep(1);
         progressBar.PutVisible(true);
      }
   }

   bool enableDisplayDriver = in_renderType == L"Region" || CSceneUtilities::DisplayRenderedImage();

   for (double iframe = in_frameIni; iframe <= in_frameEnd; iframe += in_frameStep)
   {
      if (GetRenderInstance()->InterruptRenderSignal())
         return CStatus::Abort;

      if (toRender) 
      {
         if (GetRenderOptions()->m_ipr_rebuild_mode != eIprRebuildMode_Flythrough)
            GetRenderInstance()->DestroyScene(false);
      }
      else // don't allow flythrogh mode when exporting to .ass, always destroy
         GetRenderInstance()->DestroyScene(false);

      // Setting time to statistics
      loadStart = clock();

      AiBegin(GetSessionMode());
      // Setting Log Level
      SetLogSettings(in_renderType, iframe);

      AiMsgDebug("[sitoa] Loading Arnold Plugins");
      // Load the plugins before creating nodes of the types declared in them 
      // The paths are cleared by DestroyScene, so let's reload them
      GetRenderInstance()->GetPluginsSearchPath().Put(CPathUtilities().GetShadersPath(), true);
      GetRenderInstance()->GetPluginsSearchPath().LoadPlugins();
      // note that the other search paths are loaded by LoadOptions->LoadOptionsParameters

      // Let's log the search paths as a debugging courtesy
      GetRenderInstance()->GetPluginsSearchPath().LogDebug(L"Plugins Search Path");
      // Let's also log the other search paths. Not to confuse things, let's us a temporary CSearchPath,
      // and not the real ones
      CSearchPath dummySearhPath;
      dummySearhPath.Put(CPathUtilities().GetProceduralsPath(), true);
      dummySearhPath.LogDebug(L"Procedurals Search Path");
      dummySearhPath.Clear();
      dummySearhPath.Put(CPathUtilities().GetTexturesPath(), true);
      dummySearhPath.LogDebug(L"Textures Search Path");

      CString appString = L"SItoA " + GetSItoAVersion();
      appString+= L" Softimage ";
      appString+= app.GetVersion().GetAsciiString();
      AiSetAppString(appString.GetAsciiString());

      if (!toRender)
      {
         if (in_useProgressBar)
         {
            progressBar.PutCaption( L"Exporting ASS Frame " + CValue((LONG)iframe).GetAsText());
            progressBar.PutValue((LONG)(iframe - in_frameIni));
         }
       
         // Positioning TimeControl to frame (in order to get working sequences evaluation ticket:498)
         playctrl.PutParameterValue(L"Current", iframe);
         
         // Updating RenderInstance current frame (needed for TimeShifting System ticket:995)
         GetRenderInstance()->SetFrame(iframe);
      }
      else if (enableDisplayDriver)
         GetRenderInstance()->GetDisplayDriver()->CreateDisplayDriver();

      AiMsgDebug("[sitoa] Start Loading Scene");

      ////////////////////////////////////
      // Loading Options

      // if we're exporting an object, load the options so to honor them (for instance ascii or binary?)
      if (in_createStandIn)
      {
         status = LoadOptions(in_arnoldOptions, iframe);
         // then zero output_options, so that the options block doesn't show up in the .ass (#1644)
         output_options = 0;
         // and for the same reason deny filters and drivers
         output_drivers_filters = 0;
      }
      else if ((output_options & AI_NODE_OPTIONS) == AI_NODE_OPTIONS)
      {
         AiMsgDebug("[sitoa] Loading Options");
         status = LoadOptions(in_arnoldOptions, iframe);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Operators ////////////
      if (!in_createStandIn)
      {
         AiMsgDebug("[sitoa] Loading Operators");
         status = LoadPassOperator(iframe);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Cameras //////////// 
      if (!in_createStandIn && output_cameras == AI_NODE_CAMERA)
      {
         AiMsgDebug("[sitoa] Loading Cameras");
         status = LoadCameras(iframe);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Imagers ////////////
      if (!in_createStandIn)
      {
         AiMsgDebug("[sitoa] Loading Imagers");
         
         status = LoadImagers(iframe);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Pass shaders //////////// 
      if (!in_createStandIn && output_shaders == AI_NODE_SHADER)
      {
         AiMsgDebug("[sitoa] Loading ShaderStack");
         status = LoadPassShaders(iframe, in_selectionOnly);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Lights //////////// 
      if (output_lights == AI_NODE_LIGHT)
      {
         AiMsgDebug("[sitoa] Loading Lights");
         status = LoadLights(iframe, selectedObjs, in_selectionOnly);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Polymeshes //////////// 
      if (output_geometry == AI_NODE_SHAPE || output_shaders == AI_NODE_SHADER)
      {
         AiMsgDebug("[sitoa] Loading Polymeshes");
         status = LoadPolymeshes(iframe, selectedObjs, in_selectionOnly);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// Hair //////////// 
      if (output_geometry == AI_NODE_SHAPE || output_shaders == AI_NODE_SHADER)
      {
         AiMsgDebug("[sitoa] Loading Hairs");
         status = LoadHairs(iframe, selectedObjs, in_selectionOnly);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      //////////// ICE //////////// 
      if (output_geometry == AI_NODE_SHAPE || output_shaders == AI_NODE_SHADER)
      {
         AiMsgDebug("[sitoa] Loading ICE");
         status = LoadPointClouds(iframe, selectedObjs, in_selectionOnly);
         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }
      
      //////////// Instances //////////// 
      if (output_geometry == AI_NODE_SHAPE || output_shaders == AI_NODE_SHADER)
      {
         AiMsgDebug("[sitoa] Loading Instances");
         status = LoadInstances(iframe, selectedObjs, in_selectionOnly);

         if (progressBar.IsCancelPressed() || status == CStatus::Abort)
         {
            AbortFrameLoadScene();
            break;
         }
      }

      status = PostLoadOptions(in_arnoldOptions, iframe);

      // Write the plugin_searchpath in the options node
      if (GetRenderInstance()->GetPluginsSearchPath().GetCount() > 0)
      {
         // rebuild the full search path, (re)joining together the paths. The non existing are excluded
         CPathString translatedPluginsSearchPath = GetRenderInstance()->GetPluginsSearchPath().Translate();
         CNodeSetter::SetString(AiUniverseGetOptions(NULL), "plugin_searchpath", translatedPluginsSearchPath.GetAsciiString());
      }

      loadEnd = clock(); // time for statistics

      if (!toRender)
      {
         dumpStart = clock();

         // Getting ass output file name (includes .gz if compressed is true)
         if (!in_filename.IsEmpty())
         {
            assOutputName = in_filename;
            if (GetRenderOptions()->m_compress_output_ass) // compressed ?
               assOutputName+= ".gz";
         }
         else
         {
            CTime frametime(iframe);
            assOutputName = outputAssDir + CUtils::Slash() + CPathUtilities().GetOutputExportFileName(true, true, frametime);
         }

         // #1467
         assOutputName.ResolveTokensInPlace(iframe);

         AiMsgDebug("[sitoa] Writing ASS file");

         AiASSWrite(NULL,
                    assOutputName.GetAsciiString(), 
                    output_cameras + output_drivers_filters + output_lights + output_options + output_geometry + output_shaders + output_operators, 
                    GetRenderOptions()->m_open_procs,
                    GetRenderOptions()->m_binary_ass
                   );

         AiEnd();

         // Adding the bbox info in the scntoc file
         if (in_createStandIn)
         {
            double xmin, ymin, zmin, xmax, ymax, zmax;
            CRefArray validObjects;

            if (in_selectionOnly) // filter the selection (mesh, hair, pointclouds only)
               validObjects = FilterShapesFromArray(selectedObjs);
            else // get all the objects under the root
               validObjects = GetAllShapesBelowTheRoot();

            GetBoundingBoxFromObjects(validObjects, iframe, xmin, ymin, zmin, xmax, ymax, zmax);

            char bounds[200];
            // use 9 digits decimals for #1396
            sprintf(bounds, "%.9g %.9g %.9g %.9g %.9g %.9g", (float)xmin, (float)ymin, (float)zmin, (float)xmax, (float)ymax, (float)zmax);
            CString bboxString = L"bounds " + (CString)bounds;

            CPathString standintoc(assOutputName);
            if (standintoc.IsAss()) // ass or ass.gz extension, replace it with .asstoc
               standintoc = standintoc.GetAssToc();
            else // simply add the .asstoc extension to the current path. However, this should never be the case
               standintoc+= L".asstoc";

            FILE* bboxfile = fopen(standintoc.GetAsciiString(), "w");
            if (bboxfile)
            {
               fwrite(bboxString.GetAsciiString(), 1, bboxString.Length(), bboxfile);
               fclose(bboxfile);
            }
         }

         dumpEnd = clock();

         double loadDelay = (loadEnd - loadStart) / (double)CLOCKS_PER_SEC;
         double dumpDelay = (dumpEnd - dumpStart) / (double)CLOCKS_PER_SEC;

         GetMessageQueue()->LogMsg(L"[sitoa] Frame " + CValue(iframe).GetAsText()    + L" exported" +
                        L" (to Arnold: "  + CValue(loadDelay).GetAsText() + L" sec.)" +
                        L" (to .ass: "    + CValue(dumpDelay).GetAsText() + L" sec.)");
                                
      }
      else
      {
         double loadDelay = (loadEnd - loadStart) / (double)CLOCKS_PER_SEC;
         GetMessageQueue()->LogMsg( L"[sitoa] Frame " + CValue(iframe).GetAsText() + L" exported to Arnold in " + CValue(loadDelay).GetAsText() + L" sec.");
      }

      AiMsgDebug("[sitoa] End Loading Scene");

      // if exporting to .ass, do a further scene destroy. Since the (original) scene destroy is called
      // at the beginning of the frames loop, we finish exporting and the scene is still alive.
      // We want to be sure that after a scene/frame export the scene will be rebuilt, else
      // the flythrough mode, if triggered after an export, gets confused
      if (!toRender)
         GetRenderInstance()->DestroyScene(false);
   }

    // Destroying Translations Paths tables
   if (!toRender && useTranslation)
      CPathTranslator::Destroy();

   return status;
}


void AbortFrameLoadScene()
{
   GetMessageQueue()->LogMsg(L"[sitoa] Export process aborted");
   AiEnd();
}


CStatus PostLoadSingleObject(const CRef in_ref, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{
   X3DObject xsiObj(in_ref);
   if (!xsiObj.IsValid())
   {
      // if this is a primitive
      Primitive prim(in_ref);
      if (prim.IsValid())
         xsiObj = prim.GetParent3DObject();
   }

   // early abort
   if (xsiObj.IsValid())
   {
      CString objType = xsiObj.GetType();

      if (objType.IsEqualNoCase(L"polymsh"))
      {
         return LoadSinglePolymesh(xsiObj, in_frame, in_selectedObjs, in_selectionOnly);
      }
      else if (objType.IsEqualNoCase(L"hair"))
      {
         // check if this hair is selected
         if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, xsiObj.GetRef()))
            return CStatus::Unexpected;
         return LoadSingleHair(xsiObj, in_frame);
      }
      // #1199
      else if (objType.IsEqualNoCase(L"pointcloud"))
      {

         if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, xsiObj.GetRef()))
            return CStatus::Unexpected;
         vector <AtNode*> postLoadedNodesToHide;
         CStatus status = LoadSinglePointCloud(xsiObj, in_frame, in_selectedObjs, in_selectionOnly, postLoadedNodesToHide);
         if (status == CStatus::OK)
         {
            // hide the master nodes generated by time-shifted ICE instances
            for (vector <AtNode*>::iterator it=postLoadedNodesToHide.begin(); it!=postLoadedNodesToHide.end(); it++)
               CNodeSetter::SetByte(*it, "visibility", 0, true);
            postLoadedNodesToHide.clear();
         }
         return status;
      }
      // #1339
      else if (objType.IsEqualNoCase(L"light"))
      {
         Light light(in_ref);
         return LoadSingleLight(light, in_frame, true);
      }
   }

   return CStatus::Unexpected;
}
