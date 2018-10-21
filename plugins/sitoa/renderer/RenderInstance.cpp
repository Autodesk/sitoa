/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCamera.h"
#include "common/Tools.h"
#include "loader/Cameras.h"
#include "loader/Loader.h"
#include "loader/Options.h"
#include "loader/PathTranslator.h"
#include "renderer/IprCamera.h"
#include "renderer/IprCommon.h"
#include "renderer/IprCreateDestroy.h"
#include "renderer/IprLight.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"
#include "renderer/RenderInstance.h"
#include "renderer/RenderMessages.h"
#include "renderer/RenderTree.h"

#include <xsi_project.h>
#include <xsi_scene.h>

#include <ai.h>


void CNodeMap::Clear()
{
   m_map.clear();
}


// Push a node into the exported objects map
//
// @param in_item          Softimage item whose name is used as key
// @param in_frame         the frame time
// @param in_node          the node to associate with in_item
//
void CNodeMap::PushExportedNode(ProjectItem in_item, double in_frame, AtNode *in_node)
{
   CString name = in_item.GetFullName();
   m_map.insert(AtNodeLookupPair(AtNodeLookupKey(name, in_frame), in_node));
}


// Get a node from the exported objects map
//
// @param in_objectName    Softimage item name to be found in the map
// @param in_frame         the frame time
//
// @return                 the node associated with in_objectName, or NULL if not found
//
AtNode* CNodeMap::GetExportedNode(CString &in_objectName, double in_frame)
{
   AtNodeLookupIt it = m_map.find(AtNodeLookupKey(in_objectName, in_frame));
   if (it != m_map.end())
      return it->second;
   return NULL;
}


// Get a node from the exported objects map
//
// @param in_item          Softimage item whose name is to be found in the map
// @param in_frame         the frame time
//
// @return                 the node associated with in_objectName, or NULL if not found
//
AtNode* CNodeMap::GetExportedNode(ProjectItem in_item, double in_frame)
{
   CString name = in_item.GetFullName();
   return GetExportedNode(name, in_frame);
}


// Erase a member from the exported nodes map
//
// @param in_objectName    Softimage item name to be found in the map
// @param in_frame         the frame time
//
void CNodeMap::EraseExportedNode(CString &in_objectName, double in_frame)
{
   AtNodeLookupIt it = m_map.find(AtNodeLookupKey(in_objectName, in_frame));
   if (it != m_map.end())
      m_map.erase(it);
}


// Erase a member pointing to a given node from the exported nodes map
//
// @param in_node         the node whose mamber must be erased from the map
//
void CNodeMap::EraseExportedNode(AtNode *in_node)
{
   for (AtNodeLookupIt it=m_map.begin(); it!=m_map.end(); it++)
   {
      if (it->second == in_node)
      {
         m_map.erase(it);
         return;
      }
   }
}


// Update all the shapes in the scene, when in flythrough mode
//
void CNodeMap::FlythroughUpdate()
{
   // loop the whole map and update the kine
   for (AtNodeLookupIt it=m_map.begin(); it!=m_map.end(); it++)
   {
      CString softObjectName = it->first.m_objectName;
      CRef ref;
      ref.Set(softObjectName);
      if (!ref.IsValid())
         continue;

      X3DObject object(ref);
      if (!object.IsValid())
         continue;

      UpdateShapeMatrix(object, GetRenderInstance()->GetFrame());
   }
}


// debug
void CNodeMap::LogExportedNodes()
{
   GetMessageQueue()->LogMsg(L"----- CNodeMap::LogExportedNodes -----");
   for (AtNodeLookupIt it=m_map.begin(); it!=m_map.end(); it++)
   {
      CString nodeName = CNodeUtilities().GetName(it->second);
      GetMessageQueue()->LogMsg(it->first.m_objectName + L" " + nodeName);
   }
   GetMessageQueue()->LogMsg(L"---------------");
}


CRenderInstance::CRenderInstance()
: m_interruptRender(false), 
  m_flythrough_frame(FRAME_NOT_INITIALIZED_VALUE),
  m_renderStatus(eRenderStatus_Uninitialized)
{
   AiCritSecInit(&m_interruptRenderBarrier);
   AiCritSecInit(&m_destroySceneBarrier);
   AiCritSecInit(&m_renderStatusBarrier);
}


CRenderInstance::~CRenderInstance()
{
   AiCritSecClose(&m_interruptRenderBarrier);
   AiCritSecClose(&m_destroySceneBarrier);
   AiCritSecClose(&m_renderStatusBarrier);
}


CStatus CRenderInstance::Export()
{
   CStatus status = CStatus::OK;

   CString fileName = m_renderContext.GetAttribute(L"ArchiveFileName");
   if (CUtils::EnsureFolderExists(fileName, true))
      status = LoadScene(m_renderOptionsProperty, L"Export", m_frame, m_frame, 1, true, false, fileName, true);

   return status;
}


unsigned int CRenderInstance::UpdateRenderRegion(unsigned int in_width, unsigned int in_height)
{   
   unsigned int displayArea = 0;

   AtNode *optionsNode = AiUniverseGetOptions();
   
   // Check if there is a crop area defined. If the offset is 0,0 and the crop 
   // width/height is the same as the image width/height, then no cropping should take
   // place. The crop window is always fully inside of the rendered image.
   unsigned int cropOffsetX = (LONG)m_renderContext.GetAttribute(L"CropLeft");
   unsigned int cropOffsetY = (LONG)m_renderContext.GetAttribute(L"CropBottom");
   unsigned int cropWidth   = (LONG)m_renderContext.GetAttribute(L"CropWidth"); 
   unsigned int cropHeight  = (LONG)m_renderContext.GetAttribute(L"CropHeight");

   // Assigning Render Region to Arnold
   CNodeSetter::SetInt(optionsNode, "xres", in_width);
   CNodeSetter::SetInt(optionsNode, "yres", in_height);
   CNodeSetter::SetInt(optionsNode, "region_min_x", cropOffsetX);
   CNodeSetter::SetInt(optionsNode, "region_min_y", (in_height - cropOffsetY - cropHeight));
   CNodeSetter::SetInt(optionsNode, "region_max_x", (cropOffsetX + cropWidth -1));
   CNodeSetter::SetInt(optionsNode, "region_max_y", (in_height - cropOffsetY - 1));

   // Calculating Display Area for progress status
   displayArea = ((cropOffsetX + cropWidth -1) - cropOffsetX) * ((in_height - cropOffsetY - 1) - (in_height - cropOffsetY - cropHeight)); 

   return displayArea;
}


int CRenderInstance::RenderProgressiveScene()
{
   int render_result = AI_INTERRUPT;
   
   m_renderContext.ProgressUpdate(L"Rendering", L"Rendering", 0);
   AiMsgDebug("[sitoa] Sending to Render");   

   int  aa_max = GetRenderOptions()->m_AA_samples;
   bool dither = GetRenderOptions()->m_dither;

   int verbosity = AiMsgGetConsoleFlags(); // current log level

   set <int> aa_steps;

   if ((aa_max > -3) && GetRenderOptions()->m_progressive_minus3)
      aa_steps.insert(-3);
   if ((aa_max > -2) && GetRenderOptions()->m_progressive_minus2)
      aa_steps.insert(-2);
   if ((aa_max > -1) && GetRenderOptions()->m_progressive_minus1)
      aa_steps.insert(-1);
   if (!GetRenderOptions()->m_enable_progressive_render)
   {
      if ((aa_max > 1) && GetRenderOptions()->m_progressive_plus1)
         aa_steps.insert(1);
   }

   aa_steps.insert(aa_max); // the main value for aa, so aa_steps is never empty, and aaMax will always be the final step used
   
   // We need to change some values of the aspect ratio and camera when we are in an IPR render
   AtNode* options = AiUniverseGetOptions();
   // override the aspect ratio, for the viewport is always 1.0
   CNodeSetter::SetFloat(options, "pixel_aspect_ratio", 1.0);   
   // disable adaptive sampling during progressive rendering
   CNodeSetter::SetBoolean(options, "enable_adaptive_sampling", false);
   // Disable random dithering during progressive rendering, for speed
   m_displayDriver.SetDisplayDithering(false);
   // loop the aa steps
   for (set<int>::iterator aa_it = aa_steps.begin(); aa_it != aa_steps.end(); aa_it++)
   {
      // Enable dithering for the final pass of the progressive rendering
      if (*aa_it == aa_max)
      {
         // restore adaptive sampling again on final aa pass
         CNodeSetter::SetBoolean(options, "enable_adaptive_sampling", GetRenderOptions()->m_enable_adaptive_sampling);
         m_displayDriver.SetDisplayDithering(dither);
         AiMsgSetConsoleFlags(verbosity);
      }
      else if (verbosity > VERBOSITY::warnings)
         AiMsgSetConsoleFlags(VERBOSITY::warnings);
      
      CNodeSetter::SetInt(options, "AA_samples", *aa_it);    
      // Data for Progress Bar (resetting values for progressive)
      m_displayDriver.ResetAreaRendered();

      render_result = AI_INTERRUPT;
      // Check if the render has not been aborted just before trying to render!
      if (!InterruptRenderSignal())
         render_result = DoRender();

      if (render_result != AI_SUCCESS)
      {             
         if (render_result != AI_INTERRUPT)
            GetMessageQueue()->LogMsg(L"[sitoa] Render Aborted (" + GetRenderCodeDesc(render_result) + L")", siErrorMsg);    

         break; // get out from the progressive rendering loop
      }
   }

   AiMsgSetConsoleFlags(verbosity); // restore log level
   GetRenderInstance()->CloseLogFile();

   return render_result;
}


CRef CRenderInstance::GetUpdateType(const CRef &in_ref, eUpdateType &out_updateType)
{   
   out_updateType = eUpdateType_Undefined;
   
   SIObject xsiObj = SIObject(in_ref);
   
   /*
   GetMessageQueue()->LogMsg( L"GetUpdateType" );
   GetMessageQueue()->LogMsg( L"---------------------" );
   GetMessageQueue()->LogMsg( L"cRef.GetClassIDName()     = " + in_ref.GetClassIDName() + L"(" + CValue((LONG)in_ref.GetClassID()).GetAsText() + L")");
   GetMessageQueue()->LogMsg( L"cRef.Parent.GeClasIDNe()  = " + SIObject(in_ref).GetParent().GetClassIDName() );   
   GetMessageQueue()->LogMsg( L"-" );
   GetMessageQueue()->LogMsg( L"SIObject.GetFullName()    = " + xsiObj.GetFullName() );
   GetMessageQueue()->LogMsg( L"SIObject.GetUniqueName()  = " + xsiObj.GetUniqueName() );
   GetMessageQueue()->LogMsg( L"SIObject.GetType()        = " + xsiObj.GetType() );
   GetMessageQueue()->LogMsg( L"-" );
   GetMessageQueue()->LogMsg( L"SIObject.GetParent().Name = " + SIObject( xsiObj.GetParent() ).GetName() );
   GetMessageQueue()->LogMsg( L"SIObject.GetParent().Type = " + SIObject( xsiObj.GetParent() ).GetType() );
   GetMessageQueue()->LogMsg( L"---------------------" );
   GetMessageQueue()->LogMsg( L"---------------------" );
   */

   siClassID classId = in_ref.GetClassID();
   switch (classId)
   {
      case siStaticKinematicStateID:
      case siKinematicStateID:
      {   
         // Ignoring kine.local change events.
         if (strstr(in_ref.GetAsText().GetAsciiString(),".global"))
         {
            // For Kinematics changes the cRef will come like "Camera.kine.local"
            // so we need to ask for the parent of the parent to know the Object 
            // owner of the change.
            xsiObj = X3DObject(SIObject(SIObject(in_ref).GetParent()).GetParent());   

            // Special Case if we are modifying areaShape of a Quad Light 
            // (we will reassign xsiObj with the Light so on Update we will get again its vertices)
            if (xsiObj.GetName() == L"areaShape")
                xsiObj = X3DObject(X3DObject(SIObject(SIObject(in_ref).GetParent()).GetParent()).GetParent());   

            if (xsiObj.IsA(siLightID))
               out_updateType = eUpdateType_LightKinematics;
            else if (xsiObj.IsA(siCameraID))
               out_updateType = eUpdateType_Camera;
            else if (xsiObj.IsA(siX3DObjectID))
               out_updateType = eUpdateType_ShapeKinematics;
            else if (xsiObj.IsA(siSIObjectID) && !(xsiObj.GetType() == siSpotRootPrimType) && 
                     strstr(xsiObj.GetName().GetAsciiString(),"_Interest") == NULL)
            {
               // We will ignore if Object name has "_Interest" or if the object moved is the Light Root
               // (camera movements are sending that event of "Camera_Interest", Spot are sending "Spot_Interest", etc)
               out_updateType = eUpdateType_IncompatibleIPR;                            
            }
         }

         break;
      }      

      // ParameterID and ShaderParameterID are elements sent by OnValueChange Event() (manually selected)
      case siShaderParameterID:
      case siParameterID:
      {
         Parameter param(xsiObj.GetRef());
         SIObject paramOwner(param.GetParent());
         CString paramName = param.GetScriptName();

         // parameters incompatible with IPR
         CStringArray incompatible;
         incompatible.Add(L"enable_motion_blur");
         incompatible.Add(L"enable_motion_deform");
         incompatible.Add(L"motion_shutter_onframe");
         incompatible.Add(L"motion_step_transform");
         incompatible.Add(L"motion_step_deform");
         incompatible.Add(L"motion_shutter_length");
         incompatible.Add(L"motion_shutter_custom_start");
         incompatible.Add(L"motion_shutter_custom_end");
         incompatible.Add(L"motion_transform");
         incompatible.Add(L"motion_deform");

         // #1293
         incompatible.Add(L"exact_ice_mb");
         
         incompatible.Add(L"max_subdivisions");
         incompatible.Add(L"adaptive_error");

         incompatible.Add(L"skip_license_check");
         incompatible.Add(L"abort_on_license_fail");
         incompatible.Add(L"export_pref");
         incompatible.Add(L"subdiv_smooth_derivs");
         // #1240
         incompatible.Add(L"procedurals_path");
         incompatible.Add(L"textures_path");
         incompatible.Add(L"save_texture_paths");
         incompatible.Add(L"save_procedural_paths");
         // #1665
         incompatible.Add(L"plugins_path");
         // #1286
         incompatible.Add(L"ignore_hair");
         incompatible.Add(L"ignore_pointclouds");
         incompatible.Add(L"ignore_procedurals");
         // #680
         incompatible.Add(L"ignore_user_options");
         // #742
         incompatible.Add(L"ignore_matte");

         // Looping over all incompatible parameters
         for (LONG i=0; i<incompatible.GetCount(); i++)
         {
            if (incompatible[i] == paramName)
            {
               out_updateType = eUpdateType_IncompatibleIPR;
               return in_ref;
            }
         }

         // Special cases for Arnold Render Option parameters
         if (paramOwner.GetType() == L"Arnold_Render_Options")
         {
            if ( paramName == L"texture_max_open_files" ||
                 paramName == L"texture_automip" ||
                 paramName == L"texture_autotile" ||     
                 paramName == L"enable_autotile" ||
                 paramName == L"texture_accept_untiled")
            {
               out_updateType = eUpdateType_RenderOptionsTexture;
            }

            xsiObj = paramOwner;
         }
         // Special cases for Arnold Parameters
         else if (paramOwner.GetType() == L"arnold_parameters")
         {
            // new arnold parameters displacement params that must force a scene destroy
            if ( paramName == L"disp_height" || 
                 paramName == L"disp_zero_value" ||
                 paramName == L"disp_padding" || 
                 paramName == L"subdiv_iterations" || 
                 paramName == L"adaptive_subdivision" ||
                 paramName == L"subdiv_adaptive_error" ||
                 paramName == L"subdiv_adaptive_metric" || 
                 paramName == L"subdiv_adaptive_space" ||
                 paramName == L"use_pointvelocity" )
               out_updateType = eUpdateType_IncompatibleIPR;
            else
               out_updateType = eUpdateType_ArnoldParameters;
            
            xsiObj = paramOwner;
         }
         // Modifying ImageClip parameter
         else if (paramOwner.IsA(siImageClipID))
         {
            out_updateType = eUpdateType_ImageClip;    
            xsiObj = paramOwner;
         }
         // Changing the inclusive/exclusive-ness of a light
         else if (paramName == L"SelectiveInclusive")
         {
            xsiObj = paramOwner.GetParent();
            out_updateType = eUpdateType_Group; 
            break;
         }
         else if (paramOwner.GetType() == L"arnold_user_options")
         {
            // although the FIRST change to arnold_user_options could be treated
            // as a fast update (since a lot of options could be trivial to set and 
            // the would be no need to destroy the scene), there are 2 problems:
            // 1. We can't be sure 100% that the change is ipr compatible
            // 2. We have no way to roll back the previous user option if not void.
            //    For instance the user could have set an option and then delete the string.
            //    In this case, we have no way to "undo" the previous option setting.
            // So, let just destroy the scene
            out_updateType = eUpdateType_IncompatibleIPR;
            break;
         }

         break;
      }

      case siPropertyID: 
      case siCustomPropertyID:      
      {
         SIObject propertyOwner(xsiObj.GetParent());
         CString propertyType(xsiObj.GetType());

         if (propertyType == L"Arnold_Render_Options")
         {
            out_updateType = eUpdateType_RenderOptions;
            xsiObj = in_ref;
         }
         else if (propertyOwner.IsA(siLightID))
         {
             out_updateType = eUpdateType_Light; 
             xsiObj = propertyOwner;
         }
         else if (propertyType == L"arnold_visibility" || propertyType == L"visibility")
         {
            xsiObj = propertyOwner;            
            X3DObject obj(xsiObj);
            vector <AtNode*> *tempV;
   
            // if the viz owner was not exported yet, it means that it was previously hidden
            if (obj.IsValid() && (!NodeMap().GetExportedNode(obj, m_frame)) && (!GroupMap().GetGroupNodes(&tempV, obj, m_frame)))
            {
               if (obj.GetType() == L"polymsh" || obj.GetType() == L"hair")
                  out_updateType = eUpdateType_ObjectUnhidden;
               else
                  out_updateType = eUpdateType_IncompatibleIPR;
            }
            else
               out_updateType = eUpdateType_ArnoldVisibility;
         }
         else if (propertyType == L"arnold_sidedness")
         {
            xsiObj = propertyOwner;
            out_updateType = eUpdateType_ArnoldSidedness;
         }
         else if (propertyType == L"arnold_matte")
            out_updateType = eUpdateType_ArnoldMatte;
         else if (propertyType == L"geomapprox" || propertyType == L"motionblur")
            out_updateType = eUpdateType_IncompatibleIPR;
         else if (propertyType == L"arnold_procedural")
            out_updateType = eUpdateType_IncompatibleIPR;
         else if (propertyType == L"arnold_user_options")
            out_updateType = eUpdateType_IncompatibleIPR;
         else if (propertyType == L"arnold_volume")
            out_updateType = eUpdateType_IncompatibleIPR;
           
         break;
      }

      case siMaterialID:
      {
         out_updateType = eUpdateType_Material;
         xsiObj = in_ref;
         break;
      }

      case siClusterID:
      case siClusterPropertyID:
      case siPrimitiveID:      
      {
         // More incompatible cases with IPR
         // If we have detected a primitive change of a polymesh is because
         // we have deformated it (moving its points)
         if (xsiObj.GetType() == siPolyMeshType) 
         {
            // we have modified a polymesh primitive (for example, the length of a grid)
            out_updateType = eUpdateType_IncompatibleIPR;
         }
         else if (xsiObj.GetParent().GetClassID() == siPrimitiveID)
         {
            // this happens when moving a point or edge or polygon
            out_updateType = eUpdateType_IncompatibleIPR;
         }
         else if (xsiObj.GetParent().GetClassID() == siHairPrimitiveID)
         {
            out_updateType = eUpdateType_IncompatibleIPR;
         }
         else if (xsiObj.GetType() == siLightPrimType)
         {
            xsiObj = SIObject(xsiObj.GetParent());
            out_updateType = eUpdateType_Light; 
         }
         else if (xsiObj.GetType() == siUVProjDefType)
         {
            ClusterProperty prop(xsiObj.GetParent());
            xsiObj = prop.GetParent3DObject();
            out_updateType = eUpdateType_WrappingSettings;
         }
         else if (xsiObj.GetType() == siCameraPrimType)
         {
            out_updateType = eUpdateType_Camera;
         }

         break;
      }

      case siPassID:
      {
         break;
      }

      case siPartitionID:
      {
         out_updateType = eUpdateType_IncompatibleIPR;
         break;
      }

      // This one happens when adding/removing objects to/from a light group or a group with a material
      case siX3DObjectID:
      {
         out_updateType = eUpdateType_Group;
         break;
      }

      case siHairPrimitiveID: // #902
      {
         out_updateType = eUpdateType_IncompatibleIPR;
         break;
      }

      default:
         break;
   }

   return xsiObj.GetRef();
}


CStatus CRenderInstance::UpdateScene(const CRef &in_ref, eUpdateType in_updateType)
{
   CStatus status;

   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Manual)
   if (in_updateType == eUpdateType_IncompatibleIPR)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Incompatible IPR event detected (by " + in_ref.GetAsText() + L"). Not destroying the scene because in manual rebuild mode");
      return CStatus::OK;
   }

   switch (in_updateType)
   {
      case eUpdateType_Light:               
         UpdateLight(Light(in_ref), m_frame);           
         break;
      case eUpdateType_Material:            
         UpdateMaterial(Material(in_ref), m_frame);     
         break;
      case eUpdateType_Shader:              
         UpdateShader(Shader(in_ref), m_frame);         
         break;
      case eUpdateType_Camera:              
         break;
      case eUpdateType_ImageClip:           
         UpdateImageClip(ImageClip2(in_ref), m_frame);  
         break;
      case eUpdateType_WrappingSettings:   
         UpdateWrappingSettings(in_ref, m_frame);                      
         break;
      case eUpdateType_IncompatibleIPR:    
         DestroyScene(false);
         status = LoadScene(m_renderOptionsProperty, L"Region", m_frame, m_frame, 1, false, false, L"", false);                       
         break;
      case eUpdateType_ArnoldVisibility:   
         UpdateVisibility(in_ref, m_frame);                      
         break;
      case eUpdateType_ArnoldSidedness:    
         UpdateSidedness(in_ref, m_frame);                       
         break;
      case eUpdateType_ArnoldMatte:    
         UpdateMatte(in_ref, m_frame);                       
         break;
      case eUpdateType_Group:
         // An object could have been added/removed into a group with material. Let's update its material
         UpdateObjectMaterial(X3DObject(in_ref), m_frame);
         // And/or could have been added/removed into a light association group
         UpdateLightGroup(Light(in_ref), m_frame);      
         break;
      case eUpdateType_ArnoldParameters:   
         UpdateParameters(CustomProperty(in_ref), m_frame);      
         break;
      case eUpdateType_RenderOptions:      
         break; // no action, already update from UpdateRenderRegion()
      case eUpdateType_PassShaderStack:    
         break; // no action, already update from ProcessCall back everytime()
      case eUpdateType_LightKinematics: 
      {
         X3DObject xsiObj(in_ref);
         UpdateShapeMatrix(xsiObj, m_frame);
         break;
      }
      case eUpdateType_RenderOptionsTexture:
      {
         AiUniverseCacheFlush(AI_CACHE_TEXTURE);
         LoadOptionsParameters(AiUniverseGetOptions(), Property(in_ref), m_frame);
         break;
      }
      case eUpdateType_ShapeKinematics: 
      {
         X3DObject xsiObj(in_ref);
         UpdateShapeMatrix(xsiObj, m_frame);
         break;
      }

      case eUpdateType_ObjectUnhidden:
      {
         X3DObject xsiObj(in_ref);
         CRefArray objArray;
         if (xsiObj.GetType() == L"polymsh")
         {
            objArray.Add(xsiObj);
            CIprCreateDestroy().CreateObjects(objArray, m_frame);
         }
         else if (xsiObj.GetType() == L"hair")
         {
            objArray.Add(xsiObj);
            CIprCreateDestroy().CreateHairs(objArray, m_frame);
         }
         break;
      }

      case eUpdateType_Undefined:
         break;
   }

   return status;
}


// Destroy the Arnold scene and reset the render instance class
//
void CRenderInstance::DestroyScene(bool in_flushTextures)
{   
   AiCritSecEnter(&m_destroySceneBarrier);
   if (AiUniverseIsActive())
   {
      AiMsgDebug("[sitoa] Destroying Scene");

      SetInterruptRenderSignal(true);

      while (RenderStatus() == eRenderStatus_Started)
      {
         if (AiRendering())
            AiRenderAbort();

         CTimeUtilities().SleepMilliseconds(100);
      }

      if (in_flushTextures)
         FlushTextures();

      AiEnd();

      SetInterruptRenderSignal(false);
      SetRenderStatus(eRenderStatus_Uninitialized);
   }

   // clear the lookup maps
   m_nodeMap.Clear();
   m_groupMap.Clear();
   m_lightMap.Clear();
   m_shaderMap.Clear();
   m_missingShaderMap.Clear();

   // clear all the search paths
   GetTexturesSearchPath().Clear();
   GetProceduralsSearchPath().Clear();
   GetPluginsSearchPath().Clear();

   // reset the unique id generator
   m_uniqueIdGenerator.Reset();
   // reset the flythrough frame
   m_flythrough_frame = FRAME_NOT_INITIALIZED_VALUE;

   AiCritSecLeave(&m_destroySceneBarrier);
}


void CRenderInstance::InterruptRender()
{   
   AiCritSecEnter(&m_destroySceneBarrier);
   if (AiUniverseIsActive())
   {
      AiMsgDebug("[sitoa] Interrupting Render");

      SetInterruptRenderSignal(true);
      AiRenderInterrupt();
      SetInterruptRenderSignal(false);
   }

   CloseLogFile();

   AiCritSecLeave(&m_destroySceneBarrier);
}


void CRenderInstance::FlushTextures()
{
   GetMessageQueue()->LogMsg(L"[sitoa] Flushing Textures from memory.");
   AiUniverseCacheFlush(AI_CACHE_TEXTURE);
}


////////////////////////////////
// CALLBACK EVENTS
////////////////////////////////

CStatus CRenderInstance::OnValueChange(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   CRef cRef((CRef)ctxt.GetAttribute(L"Object"));
   
   // Be careful, we could be in a Region renderer but the m_renderType still be a shaderball or a Preview
   if (!AiUniverseIsActive())
      return CStatus::False;

   SIObject xsiObj(cRef);
   SIObject xsiOwner(xsiObj.GetParent());
   CString  xsiOwnerType = xsiOwner.GetType();

   eUpdateType updateType = eUpdateType_Undefined;

   Shader xsiShader(xsiOwner);

   // first check if something changed in the displacement branch. If so, always go update
   bool displacementChange = false;
   if (xsiShader.IsValid())
   {
      Shader displacementShader;
      // if something is attached to the displacement branch, check if xsiShader is part of that branch
      if (CRenderTree().GetDisplacementShader(xsiShader, displacementShader))
         CRenderTree().FindBackward(displacementShader, xsiShader, displacementChange);
   }

   if (displacementChange)
      UpdateScene(cRef, eUpdateType_IncompatibleIPR);
   else
   {
      // SelectiveInclusive case: We receive this change with an event of light primitive. Light Shader changes also
      // enters with that event but we don't want to re-update always object lightgroups.
      // We are going to treat this special case as parameter.

      // r3d We should be able to move some of these checks to the dirty list block :?!
      if (
            xsiOwnerType == L"Arnold_Render_Options" || 
            xsiOwnerType == L"arnold_parameters" || 
            xsiOwnerType == L"arnold_user_options" || 
            xsiOwnerType == L"ImageClip" ||
            strstr(cRef.GetAsText().GetAsciiString(), "SelectiveInclusive") || 
            (
               xsiObj.GetName() == L"Global Transform" && 
               (  
                  xsiOwner.GetParent().GetClassID() == siModelID || 
                  xsiOwner.GetParent().GetClassID() == siNullID
               )
            )
         )
      {
         CRef objRef = GetUpdateType(cRef, updateType);      
         UpdateScene(objRef, updateType);
      }
   }

   // Returns CStatus::False if you don't want to abort the event.
   return CStatus::False;
}


// Check the node type of the deleted objects. In most cases, we'll destroy the scene.
// Unique exception in case of a light, in which case we set its intensity to 0.
// Remember to review this section when AiDestroy will become reliable for shape nodes.
//
CStatus CRenderInstance::OnObjectRemoved(CRef& in_ctxt)
{
   if (!AiUniverseIsActive() || m_renderType != L"Region")
      return CStatus::False;

   Context ctxt(in_ctxt);
   // array the names of the deleted objects. Note that the objects are gone,
   // so it's not possible access their CRef anymore
   CValueArray removedObjects = ctxt.GetAttribute(L"ObjectNames");

   LONG nbObjects = removedObjects.GetCount();

   // first, let's check if any of the removed items is ipr uncompatible
   for (LONG i=0; i<nbObjects; i++)
   {
      CString objectName = removedObjects[i].GetAsText();
      CString nodeName = CStringUtilities().MakeSItoAName(removedObjects[i], m_frame, L"", false);
      AtNode* node = AiNodeLookUpByName(nodeName.GetAsciiString());
      
      if (!node)
         continue;

      int nodeType = AiNodeEntryGetType(AiNodeGetNodeEntry(node));
      if (nodeType == AI_NODE_CAMERA)
      {
         if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Manual)
            GetMessageQueue()->LogMsg(L"[sitoa] Incompatible IPR event detected (removing " + objectName + L"). Not destroying the scene because in manual rebuild mode");
         else
            DestroyScene(false);

         return CStatus::False;
      }
   }

   // ok, we're ipr compatible (probably)

   // let's now check if a light or object was removed
   CValueArray removedLights, removedShapes;
   CStringArray unresolvedObjectsNames;
   for (LONG i=0; i<nbObjects; i++)
   {
      CString objectName = removedObjects[i].GetAsText();

      // GetMessageQueue()->LogMsg(L"Removing " + objectName);
      CString nodeName = CStringUtilities().MakeSItoAName(removedObjects[i], m_frame, L"", false);
      AtNode* node = AiNodeLookUpByName(nodeName.GetAsciiString());
      
      if (node)
      {         
         // here we collect the base shapes (NOT the instanced objects, for instance) 
         int nodeType = AiNodeEntryGetType(AiNodeGetNodeEntry(node));
         if (nodeType == AI_NODE_LIGHT)
            removedLights.Add(removedObjects[i]);  // light removed
         else if (nodeType == AI_NODE_SHAPE)
            removedShapes.Add(removedObjects[i]); // shape removed
      }
      else
         // node not found, so it's a Softimage model instance, or some group like a pointcloud
         unresolvedObjectsNames.Add(removedObjects[i].GetAsText());
   }

   // destroy the lights we found
   CIprCreateDestroy().DestroyLights(removedLights, m_frame);
   // destroy the objects we found
   CIprCreateDestroy().DestroyObjects(removedShapes, m_frame);

   // here we cycle the light nodes, searching for those whose name begins with the instance model name
   if (unresolvedObjectsNames.GetCount() > 0)
   {
      // collect the lights under instanced models
      vector < AtNode* > lightNodes, shapeNodes;
      AtNodeIterator *iter = AiUniverseGetNodeIterator(AI_NODE_LIGHT);
      while (!AiNodeIteratorFinished(iter))
      {
         AtNode *node = AiNodeIteratorGetNext(iter);
         if (!node)
            break;

         CString nodeName = CNodeUtilities().GetName(node);

         // loop the deleted objects that we did not resolve. If the light node name
         // begins with the model name followed by a " ", then we add it to the nodes to be deleted
         for (LONG i=0; i<unresolvedObjectsNames.GetCount(); i++)
            if (nodeName.FindString(unresolvedObjectsNames[i] + L" ") == 0)
               lightNodes.push_back(node);
      }
      AiNodeIteratorDestroy(iter);

      // destroy all the found light instances
      CIprCreateDestroy().DestroyInstancedLights(lightNodes, m_frame);
      // destroy the groups
      CIprCreateDestroy().DestroyGroupObjects(unresolvedObjectsNames, m_frame);
   }

   // Returns CStatus::False not to abort the event.
   return CStatus::False;
}


CStatus CRenderInstance::OnObjectAdded(CRef& in_ctxt)
{
   if (!AiUniverseIsActive())
      return CStatus::False;
   if (m_renderType != L"Region")
      return CStatus::False;

   Context ctxt(in_ctxt);
   CRefArray objectsAdded = ctxt.GetAttribute(L"Objects");

   for (LONG i=0; i<objectsAdded.GetCount(); i++)
   {
      // GetMessageQueue()->LogMsg(L"RenderInstance::OnObjectAdded " + objectsAdded[i].GetAsText() + L" of class " + objectsAdded[i].GetClassIDName());

      siClassID classId = objectsAdded[i].GetClassID();
      
      // OnObjectAdded is called also when creating a material. Let's skip it
      if (classId == siMaterialID)
         continue;

      // we only accept objects and lights atm, no models etc
      if (classId == siX3DObjectID || classId == siLightID)
      {
         X3DObject object(objectsAdded[i]);
         if (object.IsValid())
         {
            // GetMessageQueue()->LogMsg(L"RenderInstance::OnObjectAdded Adding " + objectsAdded[i].GetAsText());
            m_objectsAdded.Add(objectsAdded[i]);
         }
      }
      else // uncompatible object type
      {
         m_objectsAdded.Clear();

         if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Manual)
            GetMessageQueue()->LogMsg(L"[sitoa] Incompatible IPR event detected (adding " + objectsAdded[i].GetAsText() + L"). Not destroying the scene because in manual rebuild mode");
         else
            DestroyScene(false);

         break;
      }
   }

   return CStatus::False;
}


CRef CRenderInstance::GetRendererRef()
{
   return m_renderContext.GetSource();
}


Camera CRenderInstance::GetRenderCamera()
{
   Primitive camera(m_renderContext.GetAttribute(L"Camera")); 
   
   if (camera.IsValid())
      return (Camera)camera.GetParent3DObject();
   else
   {
      // Get The camera from the current pass
      CRef cameraRef;
      Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
      cameraRef.Set(ParAcc_GetValue(pass, L"Camera", DBL_MAX).GetAsText());   
      return Camera(cameraRef);
   }
}


DisplayDriver* CRenderInstance::GetDisplayDriver()
{
   return &m_displayDriver;
}


bool CRenderInstance::InterruptRenderSignal()
{
   AiCritSecEnter(&m_interruptRenderBarrier);
   bool interrupt = m_interruptRender;
   AiCritSecLeave(&m_interruptRenderBarrier);
   return interrupt;
}


void CRenderInstance::SetInterruptRenderSignal(bool in_value)
{
   AiCritSecEnter(&m_interruptRenderBarrier);
   m_interruptRender = in_value;
   AiCritSecLeave(&m_interruptRenderBarrier);
}


eRenderStatus CRenderInstance::RenderStatus()
{
   AiCritSecEnter(&m_renderStatusBarrier);
   eRenderStatus renderStatus = m_renderStatus;
   AiCritSecLeave(&m_renderStatusBarrier);
   return renderStatus;
}


void CRenderInstance::SetRenderStatus(const eRenderStatus in_status)
{
   AiCritSecEnter(&m_renderStatusBarrier);
   m_renderStatus = in_status;
   AiCritSecLeave(&m_renderStatusBarrier);
}


int CRenderInstance::DoRender(const AtRenderMode in_mode)
{
   SetRenderStatus(eRenderStatus_Started);
   int result = AiRender(in_mode);
   SetRenderStatus(eRenderStatus_Finished);
   return result;
}


CStatus CRenderInstance::TriggerBeginRenderEvent()
{
   CStatus status;

   siRenderingType renderType = (m_renderContext.GetSequenceLength( ) > 1 ) ? siRenderSequence : siRenderFramePreview;

   // Triggering OnBeginSequence Event
   if (m_renderContext.GetSequenceIndex() == 0)
      status = m_renderContext.TriggerEvent(siOnBeginSequence, renderType, m_renderContext.GetTime(), m_outputImgNames, siRenderFieldNone);

   if (status != CStatus::OK)
      return status;

   // Triggering OnBeginFrame Event of Type Sequence
   status = m_renderContext.TriggerEvent(siOnBeginFrame, renderType, m_renderContext.GetTime(), m_outputImgNames, siRenderFieldNone);

   return status;
}


CStatus CRenderInstance::TriggerEndRenderEvent(bool in_skipped)
{
   CStatus status;

   siRenderingType renderType = (m_renderContext.GetSequenceLength() > 1 ) ? siRenderSequence : siRenderFramePreview;

   // Triggering OnEndFrame Event of Type Sequence
   if (!in_skipped)
   {
      status = m_renderContext.TriggerEvent(siOnEndFrame, renderType, m_renderContext.GetTime(), m_outputImgNames, siRenderFieldNone);

      if (status != CStatus::OK)
         return status;
   }

   // Triggering OnEndSequence Event
   if (m_renderContext.GetSequenceIndex() + 1 == m_renderContext.GetSequenceLength())
      status = m_renderContext.TriggerEvent(siOnEndSequence, renderType, m_renderContext.GetTime(), m_outputImgNames, siRenderFieldNone);

   return status;
}


// Create the directories for all the output filenames of all the buffers
//
// @return true if all the directories were created correctly, false if something went wrong
// 
bool CRenderInstance::OutputDirectoryExists()
{
   CRefArray frameBuffers = m_pass.GetFramebuffers();
   LONG nbBuffers = frameBuffers.GetCount();

   for (LONG i=0; i<nbBuffers; i++)
   {
      Framebuffer softFramebuffer(frameBuffers[i]); // the softimage buffer

      if (!ParAcc_GetValue(softFramebuffer, L"Enabled", DBL_MAX))
         continue;

      CFrameBuffer fb(softFramebuffer, CTimeUtilities().GetCurrentFrame(), false);

      if (!CUtils::EnsureFolderExists(fb.m_fileName, true)) // create the missing directories
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Image output path is not valid: " + fb.m_fileName, siErrorMsg);
         return false;
      }
      // adding the output filename to m_outputImgNames, that is used exclusively for the events triggerers,
      // that can probably be nuked, implemented because of a 2010 bug (see #495)
      m_outputImgNames.Add(fb.m_fileName);
   }

   return true;
}


CStatus CRenderInstance::Process()
{
   CStatus status;

   if (GetRenderType() == L"Shaderball")
      return status;

   m_displayDriver.ResetAreaRendered();

   // size of the image to render in pixels. origin is bottom-left
   m_renderWidth = (int) m_renderContext.GetAttribute(L"ImageWidth");
   m_renderHeight = (int) m_renderContext.GetAttribute(L"ImageHeight");

   m_pass.ResetObject();
   m_pass  = Application().GetActiveProject().GetActiveScene().GetActivePass();

   m_outputImgNames.Clear();

   // Notify the renderer manager that a new frame is about to begin. This is necessary so
   // that any recipient tile sink can re-adjust its own size to accommodate.
   status = m_renderContext.NewFrame(m_renderWidth, m_renderHeight);

   if (GetRenderType() == L"Region")
      status = ProcessRegion();
   else if (GetRenderType() == L"Pass")
      status = ProcessPass();

   return status;
}


CStatus CRenderInstance::ProcessPass()
{
   CStatus status;

   if (!OutputDirectoryExists())
      return CStatus::Fail;

   // The event doesn't trigger in previous versions. 
   if (TriggerBeginRenderEvent() != CStatus::OK)
      return CStatus::Fail;

   bool enableDisplayDriver = CSceneUtilities::DisplayRenderedImage();

   bool fileOutput = m_renderContext.GetAttribute(L"FileOutput");
   // Checking if the Frame is already rendered and file output is enabled
   if (m_renderContext.GetAttribute(L"SkipExistingFiles") && fileOutput)
   {
      // Getting Output Image Name
      CString filename;

      Framebuffer frameBuffer(m_pass.GetFramebuffers().GetItem(L"Main"));
      filename.PutAsciiString(CPathTranslator::TranslatePath(frameBuffer.GetResolvedPath(m_renderContext.GetTime()).GetAsciiString(), false));

      // Opening file to check if it exists (brute force, stat() is not working properly)
      std::ifstream file(filename.GetAsciiString());
      if (file.is_open())
      {
         file.close();            
         GetMessageQueue()->LogMsg(L"[sitoa] Skipping Frame " + CValue(m_frame).GetAsText());

         // The event doesn't trigger in previous versions. 
         TriggerEndRenderEvent(true);
         return CStatus::OK;
      }
      // We create a temporary output file to let other render nodes know
      // that we are going to render this frame.
      // Arnold will overwrite this temp file with the final image data
      else
      {
         FILE* pFile=NULL;

#ifdef _MSC_VER
         fopen_s(&pFile, filename.GetAsciiString(), "w");
#else
         pFile = fopen(filename.GetAsciiString(), "wx");
#endif

         if (pFile)
         {
            fputs("temporary file", pFile);
            fflush(pFile);
            fclose(pFile);
         }
      }
   }
   
   // Check if the render has not been aborted just before trying to load the scene (long process)
   if (InterruptRenderSignal())
      return CStatus::Abort;
 
   m_renderContext.ProgressUpdate(L"Loading Scene", L"Loading Scene", 0);

   bool skipLoadingScene(false);
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
   {
      if (GetRenderInstance()->GetFlythroughFrame() == FRAME_NOT_INITIALIZED_VALUE)
         GetRenderInstance()->SetFlythroughFrame(m_frame);
      else
         skipLoadingScene = true;
   }

   if (!skipLoadingScene)
   {
      if (LoadScene(m_renderOptionsProperty, L"Pass", m_frame, m_frame, 1, false, false, L"", false) != CStatus::OK)
      {
         // The event doesn't trigger in previous versions. 
         TriggerEndRenderEvent();
         return CStatus::Fail;
      }
   }
   else // flythrough mode
   {
      LoadOptions(m_renderOptionsProperty, m_frame, true);
      UpdateCamera(m_frame);
      // update all the lights
      GetRenderInstance()->LightMap().FlythroughUpdate();
      // update the kine of all the nodes (including lights)
      GetRenderInstance()->NodeMap().FlythroughUpdate();
      // update all the shaders
      GetRenderInstance()->ShaderMap().FlythroughUpdate();
      if (enableDisplayDriver)
      {
         // destroy and rebuild the display driver node
         AtNode *driver = AiNodeLookUpByName("xsi_driver");
         if (driver)
         {
            AiNodeDestroy(driver);
            driver = AiNode("display_driver");
            if (driver)
               CNodeUtilities().SetName(driver, "xsi_driver");
         }
      }
   }

   if (enableDisplayDriver)
      m_displayDriver.UpdateDisplayDriver(m_renderContext, m_renderWidth*m_renderHeight, 
                                          GetRenderOptions()->m_filter_color_AOVs, GetRenderOptions()->m_filter_numeric_AOVs);
 
   // Check if the render has not been aborted just before render
   if (InterruptRenderSignal())
      return CStatus::Abort;

   int renderResult = AI_INTERRUPT;
   renderResult = DoRender();

   if (renderResult == AI_SUCCESS)  
      m_renderContext.ProgressUpdate(L"Image Rendered", L"Image Rendered", 100);
   else
   {
      CString errorMessage = L"[sitoa] Render Aborted (" + GetRenderCodeDesc(renderResult) + L")";
      GetMessageQueue()->LogMsg(errorMessage, siErrorMsg);
      if (!Application().IsInteractive()) // #1797 for Royal Render
      {
         printf("%s\n", errorMessage.GetAsciiString());
         fflush(stdout);
      }

      status = CStatus::Abort;
      
      if (renderResult != AI_INTERRUPT)
         status = CStatus::Fail;
      
      // Remove unfinished rendered files
      if (fileOutput)
      {
         CStringArray driverNames = GetDriverNames();
         for (LONG i=0; i<driverNames.GetCount(); i++)
         {
            bool keepFile(false);
            AtNode* driverNode = AiNodeLookUpByName(driverNames[i].GetAsciiString());
            if (!driverNode)
               continue;

            CString driverName = CNodeUtilities().GetEntryName(driverNode);
            if (driverName == L"driver_exr" || driverName == L"driver_tiff")
               keepFile = AiNodeGetBool(driverNode, "tiled") && AiNodeGetBool(driverNode, "append");

            if (!keepFile) // delete the file, unless if the driver is not in tiled and append mode
               std::remove(AiNodeGetStr(driverNode, "filename"));
         }
      }
   }

   if (GetRenderOptions()->m_ipr_rebuild_mode != eIprRebuildMode_Flythrough)
      DestroyScene(false);
  
   GetRenderInstance()->CloseLogFile();

   TriggerEndRenderEvent();

   return status;
}


CStatus CRenderInstance::ProcessRegion()
{
   CStatus status;

   // Check if the render has not been aborted just before trying to load the scene (long process)
   if (InterruptRenderSignal())
      return CStatus::Abort;

   // The event doesn't trigger in previous versions. 
   if (TriggerBeginRenderEvent() != CStatus::OK)
      return CStatus::Fail;
 
   bool emptyDirtyList(false);

   if (!AiUniverseIsActive())
   {
      CRefArray visibleObjects = m_renderContext.GetAttribute(L"ObjectList");
      bool selOnly = visibleObjects.GetCount() > 0;
      if (selOnly) // we're in isolate selection mode, and there are visible objects.
      { 
         // Let's add all the lights, if they are not in the objects list yet
         // Users prefer to have all the lights on while in isolate selection mode, as in mental ray
         CRefArray lightsArray = m_renderContext.GetAttribute(L"Lights");
         for (LONG i=0; i<lightsArray.GetCount(); i++)
         {
            CRef ref = lightsArray.GetItem(i);
            if (!ArrayContainsCRef(visibleObjects, ref))
               visibleObjects.Add(ref);
         }
      }

      m_renderContext.ProgressUpdate(L"Loading Scene", L"Loading Scene", 0);
      status = LoadScene(m_renderOptionsProperty, L"Region", m_frame, m_frame, 1, false, false, L"", selOnly, visibleObjects);

      if (status != CStatus::OK)
         return status;
   }   
   else
   {
      m_renderContext.ProgressUpdate(L"Updating Scene", L"Updating Scene", 0);
      LockSceneData lock;
      if (lock.m_status != CStatus::OK)
         return CStatus::Abort;

      // If OnObjectAdded was triggered, we find the added refs in m_objectsAdded
      // If so, we'll just create the new objects, and skip the dirty list
      CRefArray objectsAdded = m_objectsAdded.Get();
      if (objectsAdded.GetCount() > 0)
      {
         CRefArray lightArray, meshArray, hairArray, instanceModelArray;
         for (LONG i=0; i<objectsAdded.GetCount(); i++)
         {
            if (objectsAdded[i].GetClassID() == siLightID)
               lightArray.Add(objectsAdded[i]);
            else if (objectsAdded[i].GetClassID() == siX3DObjectID)
            {
               X3DObject object(objectsAdded[i]);
               CString objectType = object.GetType();
               if (objectType == L"polymsh")
                  meshArray.Add(objectsAdded[i]);
               else if (objectType == L"hair")
                  hairArray.Add(objectsAdded[i]);
            }

            if (lightArray.GetCount() > 0)
               CIprCreateDestroy().CreateLights(lightArray, m_frame);
            if (meshArray.GetCount() > 0)
               CIprCreateDestroy().CreateObjects(meshArray, m_frame);
            if (hairArray.GetCount() > 0)
               CIprCreateDestroy().CreateHairs(hairArray, m_frame);
         }

         // clear the list, so for further ipr changes (not adding object) we work as usual
         m_objectsAdded.Clear();
         // clear the dirty list, else the new objects will still be there on the next 
         // IPR iteration, if the current one is interrupted, see #1638
         CValue dirtyRefsValue = m_renderContext.GetAttribute(L"DirtyList");
         CRefArray dirtyRefs = dirtyRefsValue;
         for (LONG i=0; i<dirtyRefs.GetCount(); i++)
            m_renderContext.SetObjectClean(dirtyRefs[i]);
      }
      else
      {
         CValue dirtyRefsValue = m_renderContext.GetAttribute(L"DirtyList");
         emptyDirtyList = dirtyRefsValue.IsEmpty();
         if (emptyDirtyList)
         {
            // this happens for instance when you change the Render Region Options -> Use Current Pass Options.
            // GetMessageQueue()->LogMsg(L"-----------> EMPTY DIRTY LIST !!!");
            m_renderContext.SetObjectClean(CRef()); // as suggested by the SetObjectClean doc
            // status = UpdateScene(CRef(), eUpdateType_IncompatibleIPR); // #1128
            // commented out, because the dirty list is also void in the following case:
            // 1. region running
            // 2. change frame
            // 3. orbit before the ipr completes.
            // So, no go
         }
         else
         {
            CRefArray dirtyRefs = dirtyRefsValue;

            // Check if the render has not been aborted just before trying to update the scene (long process)
            if (InterruptRenderSignal())
               return CStatus::Abort;

            bool sceneDestroyed = false;

            // First, let's push the dirty refs into a set, so to avoid duplication
            // For example, when creating a light during ipr, the light is passed twice into the dirty ref list (sigh!)
            set <CRef> refSet;
            for (LONG i=0; i<dirtyRefs.GetCount(); i++)
               refSet.insert(dirtyRefs[i]);
            
            for (set <CRef>::iterator refIt=refSet.begin(); refIt!=refSet.end(); refIt++)
            {
               // GetMessageQueue()->LogMsg(L"  Checking update type for " + refIt->GetAsText() + L" = " + refIt->GetClassIDName());   

               eUpdateType updateType = eUpdateType_Undefined;
               CRef ref = GetUpdateType(*refIt, updateType);

               if (ref.IsValid())
               {
                  // Update the scene only if the scene has not been destroyed and the previous updates were OK
                  if (status == CStatus::OK && !sceneDestroyed)
                  if (updateType != eUpdateType_Undefined)
                  {
                     status = UpdateScene(ref, updateType);
                     if (updateType == eUpdateType_IncompatibleIPR)
                        sceneDestroyed = true;
                     // don't break for sceneDestroy, as we need to SetObjectClean
                  }

                  m_renderContext.SetObjectClean(ref);
               }
               else
                  m_renderContext.SetObjectClean(CRef()); // as suggested by the SetObjectClean doc
            }
         }
      }

      // An UpdateScene could have destroyed a scene with a subsequent LoadScene executed. 
      // We must check that this LoadScene worked as expected
      if (status != CStatus::OK)
         return status;
   }

   { // do not remove the {} as we need the local scope for the thread lock (see trac#1044)
      LockSceneData lock;
      if (lock.m_status != CStatus::OK)
         return CStatus::Abort;

      // We have to update Render Options with the given by the process callback
      // Perhaps we are rendering from another viewport with other render settings
      LoadOptionsParameters(AiUniverseGetOptions(), m_renderOptionsProperty, m_frame);

      // Fix to resolve an XSI Bug that doesn't send accumulative changes of shader stacks updates
      // when autorefresh is off and we do a manual refresh.
      // https://trac.solidangle.com/sitoa/ticket/275
      // We are going to update always what we have connected to current pass shader stack.
      UpdatePassShaderStack(m_pass, m_frame);

      CRefArray visibleObjects = m_renderContext.GetAttribute(L"ObjectList");

      UpdateIsolateSelection(visibleObjects, m_frame);

      // first time we render the region in flythrough mode?
      if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      {
         if (GetFlythroughFrame() == FRAME_NOT_INITIALIZED_VALUE)
            SetFlythroughFrame(m_frame);
      }

      UpdateCamera(m_frame);

      // In flythrough mode, update only when the dirty list is void, ie on a frame change.
      // If not void, the update is already managed by the dirty list loop above
      if (emptyDirtyList)
      if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      {
         // update all the lights
         GetRenderInstance()->LightMap().FlythroughUpdate();
         // update the kine of all the nodes (including lights)
         GetRenderInstance()->NodeMap().FlythroughUpdate();
         // update all the shaders
         GetRenderInstance()->ShaderMap().FlythroughUpdate();
      }

      // Updating RenderRegion and DisplayArea
      unsigned int displayArea = UpdateRenderRegion(m_renderWidth, m_renderHeight);

      // for these new render options (1.12), let's check their existance. Else, filterColorAov defaults to false,
      // and all the previously saved scenes render aliased
      m_displayDriver.UpdateDisplayDriver(m_renderContext, displayArea, 
                                          GetRenderOptions()->m_filter_color_AOVs, GetRenderOptions()->m_filter_numeric_AOVs);

      SetLogSettings(L"Region", m_frame);
   }
  
   int renderStatus = RenderProgressiveScene();

   if (renderStatus != AI_SUCCESS)
   {
      status = CStatus::Abort;
      if (renderStatus != AI_INTERRUPT)
         status = CStatus::Fail;
   }

   TriggerEndRenderEvent();

   return status;
}


// called by ArnoldRender_Process only
CStatus CRenderInstance::InitializeRender(CRef &in_ctxt)
{
   CRenderMessages::Initialize();   
   
   m_renderContext.ResetObject();
   m_renderContext = RendererContext(in_ctxt);

   m_renderType    = m_renderContext.GetAttribute(L"RenderType");
   m_frame = (double)m_renderContext.GetTime();
   m_renderOptionsProperty.ResetObject();
   m_renderOptionsProperty = m_renderContext.GetRendererProperty(m_frame);
   m_renderOptions.Read(m_renderOptionsProperty); // read all the rendering options

   // Check if the render has not been aborted just before notifying the new frame to the render manager
   if (InterruptRenderSignal())
      return CStatus::Abort;

   return CStatus::OK;
}


double CRenderInstance::GetFrame() const
{
   return m_frame;
}


void CRenderInstance::SetFrame(const double in_frame)
{
   m_frame = in_frame;
}


double CRenderInstance::GetFlythroughFrame() const
{
   return m_flythrough_frame;
}


void CRenderInstance::SetFlythroughFrame(const double in_frame)
{
   m_flythrough_frame = in_frame;
}


const CString& CRenderInstance::GetRenderType() const
{
   return m_renderType;
}


void CRenderInstance::SetRenderType(const CString& in_renderType)
{
   m_renderType = in_renderType;
}


// node map accessor
CNodeMap& CRenderInstance::NodeMap()
{
   return m_nodeMap;
}


// group map accessor
CGroupMap& CRenderInstance::GroupMap()
{
   return m_groupMap;
}


// light map accessor
CLightMap& CRenderInstance::LightMap()
{
   return m_lightMap;
}


// shader map accessor
CShaderMap& CRenderInstance::ShaderMap()
{
   return m_shaderMap;
}


// missing shaders map accessor
CMissingShaderMap& CRenderInstance::MissingShaderMap()
{
   return m_missingShaderMap;
}


// handle to the class for the auto shader definition
CShaderDefSet& CRenderInstance::ShaderDefSet()
{
   return m_shaderDefSet;
}

// access the textures search path
CSearchPath& CRenderInstance::GetTexturesSearchPath()
{
   return m_texturesSearchPath;
}


// access the procedurals search path
CSearchPath& CRenderInstance::GetProceduralsSearchPath()
{
   return m_proceduralsSearchPath;
}


// access the shaders search path
CSearchPath& CRenderInstance::GetPluginsSearchPath()
{
   return m_pluginsSearchPath;
}


// Get a unique id, for assigning different names to duplicated shaders
int CRenderInstance::GetUniqueId()
{
   return m_uniqueIdGenerator.Get();
}


// Open the log file
//
void CRenderInstance::OpenLogFile(const CString &in_path)
{
   CloseLogFile(); // in case we forgot to close it before
   m_logFile.open(in_path.GetAsciiString(), ios::out);
}


// Close the log file, if it's open
//
void CRenderInstance::CloseLogFile()
{
   if (m_logFile.is_open())
   {
      m_logFile.flush();
      m_logFile.close();
   }
}


void CObjectsAdded::Add(CRef in_ref)
{
   m_objects.Add(in_ref);
}


CRefArray CObjectsAdded::Get()
{
   return m_objects;
}


void CObjectsAdded::Clear()
{
   m_objects.Clear();
}




