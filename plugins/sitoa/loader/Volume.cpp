/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "loader/ICE.h"
#include "loader/Properties.h"
#include "loader/Procedurals.h"
#include "loader/Volume.h"
#include "renderer/Renderer.h"

#include <xsi_geometryaccessor.h>

#include <iostream>
#include <fstream>
using namespace std;

// Return the step_size value for an object.
//
// @param in_xsiObj   The Softimage object
// @param in_frame    The frame time
//
// @return the step_size
float GetStepSize(const X3DObject in_xsiObj, double in_frame)
{
   Property arnold_parameters;
   CRefArray properties = in_xsiObj.GetProperties();
   if (properties.Find(L"arnold_parameters", arnold_parameters) != CStatus::OK)
      return 0.0f;

   float step_size = ParAcc_GetValue(arnold_parameters, L"step_size", in_frame);
   return step_size <= 0.0f ? 0.0f : step_size;
}

// Load a volume plugin
//
// @param in_xsiObj              the Softimage object owner of the volume property
// @param in_frame               the frame time
// @param in_selectedObjs        the selected objs to render (if in_selectionOnly==true)
// @param in_selectionOnly       true is only in_selectedObjs must be rendered
//
CStatus LoadSingleVolume(const X3DObject &in_xsiObj, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{  
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, in_xsiObj.GetRef()))
      return CStatus::OK;

   CRefArray volumeProperties = in_xsiObj.GetProperties();

   Property volume_property = Property(volumeProperties.GetItem(L"arnold_volume"));

   if (!ParAcc_GetValue(Property(volumeProperties.GetItem(L"Visibility")), L"rendvis", in_frame))
      return CStatus::OK;

   AtNode* volume = AiNode("volume");

   if (!volume)
      return CStatus::OK; // not really ok...

   GetRenderInstance()->NodeMap().PushExportedNode(in_xsiObj, in_frame, volume);
   CString name = CStringUtilities().MakeSItoAName(in_xsiObj, in_frame, L"", false);
   CNodeUtilities().SetName(volume, name);

   CPathString filename = ParAcc_GetValue(volume_property, L"filename", in_frame).GetAsText();
   filename.ResolveTokensInPlace(in_frame); // resolve the tokens
   CNodeSetter::SetString(volume, "filename", filename.GetAsciiString());

   float volume_padding = (float)ParAcc_GetValue(volume_property, L"volume_padding", in_frame);
   CNodeSetter::SetFloat(volume, "volume_padding", volume_padding);

   CString grids = ParAcc_GetValue(volume_property, L"grids", in_frame).GetAsText();
   CStringArray grids_string_array = grids.Split(L" ");
   LONG nb_grids = grids_string_array.GetCount();
   if (nb_grids > 0)
   {
      AtArray* grids_array = AiArrayAllocate(nb_grids, 1, AI_TYPE_STRING);
      for (LONG i = 0; i < nb_grids; i++)
         AiArraySetStr(grids_array, i, grids_string_array[i].GetAsciiString());

      AiNodeSetArray(volume, "grids", grids_array);
   }

   CString velocity_grids = ParAcc_GetValue(volume_property, L"velocity_grids", in_frame).GetAsText();
   grids_string_array = velocity_grids.Split(L" ");
   nb_grids = grids_string_array.GetCount();
   if (nb_grids > 0)
   {
      AtArray* grids_array = AiArrayAllocate(nb_grids, 1, AI_TYPE_STRING);
      for (LONG i = 0; i < nb_grids; i++)
         AiArraySetStr(grids_array, i, grids_string_array[i].GetAsciiString());

      AiNodeSetArray(volume, "velocity_grids", grids_array);
   }

   float velocity_scale = (float)ParAcc_GetValue(volume_property, L"velocity_scale", in_frame);
   CNodeSetter::SetFloat(volume, "velocity_scale", velocity_scale);

   float velocity_outlier_threshold = (float)ParAcc_GetValue(volume_property, L"velocity_outlier_threshold", in_frame);
   CNodeSetter::SetFloat(volume, "velocity_outlier_threshold", velocity_outlier_threshold);

   float velocity_fps = (float)ParAcc_GetValue(volume_property, L"velocity_fps", in_frame);
   CNodeSetter::SetFloat(volume, "velocity_fps", velocity_fps);

   bool compress = (bool)ParAcc_GetValue(volume_property, L"compress", in_frame);
   CNodeSetter::SetBoolean(volume, "compress", compress);

   // do not show in the property
   /*
   float motion_start = (float)ParAcc_GetValue(volume_property, L"motion_start", in_frame);
   float motion_end   = (float)ParAcc_GetValue(volume_property, L"motion_end", in_frame);
   CNodeSetter::SetFloat(volume, "motion_start", motion_start);
   CNodeSetter::SetFloat(volume, "motion_end", motion_end);
   */

   float step_scale = (float)ParAcc_GetValue(volume_property, L"step_scale", in_frame);
   CNodeSetter::SetFloat(volume, "step_scale", step_scale);

   float step_size = (float)ParAcc_GetValue(volume_property, L"step_size", in_frame);
   CNodeSetter::SetFloat(volume, "step_size", step_size);

   // Motion Blur
   CDoubleArray keyFramesTransform, keyFramesDeform;
   CSceneUtilities::GetMotionBlurData(in_xsiObj.GetRef(), keyFramesTransform, keyFramesDeform, in_frame);

   // Get matrix transform
   int nbTransfKeys = keyFramesTransform.GetCount();
   AtArray* matrices = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_MATRIX);

   for (int ikey=0; ikey<nbTransfKeys; ikey++)
   {
      // Get the transform matrix
      AtMatrix matrix;
      CUtilities().S2A(in_xsiObj.GetKinematics().GetGlobal().GetTransform(keyFramesTransform[ikey]).GetMatrix4(), matrix);
      AiArraySetMtx(matrices, ikey, matrix);
   }

   AiNodeSetArray(volume, "matrix", matrices);

   // LIGHT GROUP
   AtArray* light_group = GetRenderInstance()->LightMap().GetLightGroup(in_xsiObj);
   if (light_group)
   {
      CNodeSetter::SetBoolean(volume, "use_light_group", true);
      if (AiArrayGetNumElements(light_group) > 0)
         AiNodeSetArray(volume, "light_group", light_group);
   }

   CNodeSetter::SetByte(volume, "visibility", GetVisibility(volumeProperties, in_frame), true);

   uint8_t sidedness;
   if (GetSidedness(volumeProperties, in_frame, sidedness))
      CNodeSetter::SetByte(volume, "sidedness", sidedness, true);

   CNodeUtilities::SetMotionStartEnd(volume);

   // Arnold-specific parameters
   CustomProperty paramsProperty, userOptionsProperty;
   volumeProperties.Find(L"arnold_parameters", paramsProperty);
   volumeProperties.Find(L"arnold_user_options", userOptionsProperty); /// #680

   if (paramsProperty.IsValid())
      LoadArnoldParameters(volume, paramsProperty.GetParameters(), in_frame);
   LoadUserOptions(volume, userOptionsProperty, in_frame); /// #680
   LoadUserDataBlobs(volume, in_xsiObj, in_frame); // #728

   if (!GetRenderOptions()->m_ignore_matte)
   {
      Property matteProperty;
      volumeProperties.Find(L"arnold_matte", matteProperty);
      LoadMatte(volume, matteProperty, in_frame);      
   }

   Material material(in_xsiObj.GetMaterial());
   // load the volume shader
   AtNode* shaderNode = LoadMaterial(material, LOAD_MATERIAL_SURFACE, in_frame, in_xsiObj.GetRef());
   if (shaderNode)
      AiNodeSetArray(volume, "shader", AiArray(1, 1, AI_TYPE_NODE, shaderNode));

   return CStatus::OK;
}



 
