/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCamera.h"
#include "common/ParamsCommon.h"
#include "common/ParamsShader.h"
#include "common/Tools.h"
#include "common/UserDataGrid.h"
#include "loader/Properties.h"
#include "renderer/Renderer.h"

#include <xsi_fcurve.h>
#include <xsi_userdatablob.h>

#include <ai_ray.h>

// Given an array of properties, return all those of a given type (for example "arnold_visibility")
//
// @param in_array     the input array
// @param in_type      the type used to filter ewParamName, (float)Parameter(in input array
// @param out_array    the returned array
//
// @return true if at least one match was found, else false
//
bool FilterRefArrayByType(const CRefArray &in_array, const CString &in_type, CRefArray &out_array)
{
   LONG count = in_array.GetCount();
   for (LONG i=0; i<count; i++)
   {
      SIObject obj(in_array[i]);
      if (!obj.IsValid())
         continue;
      if (obj.GetType() == in_type)
         out_array.Add(in_array[i]);
   }

   return out_array.GetCount() > 0;
}


// Return the strongest class id of the owners of a property.
// For example, a property on a partition has both the partition AND the objects of the
// partition as owners. In this case, return the partition class id
//
// @param in_prop     the input property
//
// @return the strongest class id
//
siClassID GetBestClassId(const Property &in_prop)
{
   siClassID result(siX3DObjectID);

   CRefArray owners = in_prop.GetOwners();
   LONG ownersCount = owners.GetCount();
   for (LONG i=0; i<ownersCount; i++)
   {
      CRef ref = owners[i];
      siClassID classId = ref.GetClassID();

      if (classId == siPartitionID)
         return siPartitionID; // done

      if (classId == siGroupID)
         result = siGroupID;
   }

   return result;
}


// Given an array of properties of the same type, for example visibility, return the one
// that wins over the others. For example, an object can have a visibility property, and
// belong to a partition with another visibility property. In this case, the latter is returned
//
// @param in_properties      the array of properties of the same type owned by an object
//
// @return the property with highest power
//
Property GetOverridingProperty(const CRefArray &in_properties)
{
   Property result;
   LONG propertiesCount = in_properties.GetCount();
   if (propertiesCount == 0) // should not happen
      return result; // the caller must check the validity

   result = in_properties[0];
   if (propertiesCount == 1)
      return result;

   siClassID classId;
   for (LONG i=0; i<propertiesCount; i++)
   {
      Property prop(in_properties[i]);
      if (!prop.IsValid())
         continue;

      // the property can have several owners. Let's get the stronger one (object -> group -> partition)
      classId = GetBestClassId(prop);
      // if the strongest owner is a partition, then this is the property to be used
      if (classId == siPartitionID)
         return prop;
      // if it's a group, then it's a candidate
      if (classId == siGroupID)
         result = prop;
   }

   return result;
}


// Return the rays visibility
//
// Evaluates the Arnold Visibility property (or the Softimage one) and returns a bitfield that specifies
// the visibility for each ray type.
//
// @param in_polyProperties         the properties of the shape (polymesh or hair)
// @param in_frame                  the evaluation time
// @param in_checkHideMasterFlag    true if to check for the Instance Master Hidden flag, default true
//
// @return   the visibility bitfield
//
uint8_t GetVisibility(const CRefArray &in_polyProperties, double in_frame, bool in_checkHideMasterFlag)
{
   uint8_t visibility = AI_RAY_ALL;  // Default: all rays

   Property arnold_visibility;

   CRefArray visibility_properties_array;
   if (FilterRefArrayByType(in_polyProperties, L"arnold_visibility", visibility_properties_array))
      arnold_visibility = GetOverridingProperty(visibility_properties_array);

   Property soft_visibility = in_polyProperties.GetItem(L"Visibility"); // The Softimage visibility

   if (arnold_visibility.IsValid())
   {
      bool camera                = (bool)ParAcc_GetValue(arnold_visibility, L"camera", in_frame);
      bool cast_shadow           = (bool)ParAcc_GetValue(arnold_visibility, L"cast_shadow", in_frame);
      bool diffuse_reflection    = (bool)ParAcc_GetValue(arnold_visibility, L"diffuse_reflection", in_frame);
      bool specular_reflection   = (bool)ParAcc_GetValue(arnold_visibility, L"specular_reflection", in_frame);
      bool diffuse_transmission  = (bool)ParAcc_GetValue(arnold_visibility, L"diffuse_transmission", in_frame);
      bool specular_transmission = (bool)ParAcc_GetValue(arnold_visibility, L"specular_transmission", in_frame);
      bool volume                = (bool)ParAcc_GetValue(arnold_visibility, L"volume", in_frame);

		if (!camera)                visibility = visibility ^ AI_RAY_CAMERA;
		if (!cast_shadow)           visibility = visibility ^ AI_RAY_SHADOW;
	   if (!diffuse_reflection)    visibility = visibility ^ AI_RAY_DIFFUSE_REFLECT;
	   if (!specular_reflection)   visibility = visibility ^ AI_RAY_SPECULAR_REFLECT;
	   if (!diffuse_transmission)  visibility = visibility ^ AI_RAY_DIFFUSE_TRANSMIT;
	   if (!specular_transmission) visibility = visibility ^ AI_RAY_SPECULAR_TRANSMIT;
	   if (!volume)                visibility = visibility ^ AI_RAY_VOLUME;
   }

   // Checking render visibility. If is false we will set visibility to 0 
   // (for IPR, in exporting/loading process the object won't be loaded into arnold
   // so if the object was invisible, we can't do it visible)
   if (soft_visibility.IsValid())
   {
      if (!(bool)ParAcc_GetValue(soft_visibility, L"rendvis", in_frame))
         visibility = 0;
      // let's change the visibility of this if the instance master is hidden!
      // the only case when in_checkHideMasterFlag is false is from Instances.cpp
      if (in_checkHideMasterFlag)
         if (ParAcc_GetValue(soft_visibility, L"hidemaster", in_frame))
            visibility = 0;
   }

   return visibility;
}


// Returns the rays visibility of an xsi object
//
// @param in_obj                  the object
// @param inframe                 the evaluation time
// @param in_checkHideMasterFlag  true if to look check for the Instance Master Hidden flag, default true
// @return  visibility bitfield
//
uint8_t GetVisibilityFromObject(const X3DObject in_obj, double in_frame, const bool in_checkHideMasterFlag)
{
   if (!in_obj.IsValid())
      return 0; // whatever
   CRefArray properties = in_obj.GetProperties();
   return GetVisibility(properties, in_frame, in_checkHideMasterFlag);
}


// Returns the rays visibility of an xsi object by its id
//
// @param in_id                      the object's id
// @param in_frame                   the evaluation time
// @param in_checkHideMasterFlag     true if to look check for the Instance Master Hidden flag, default true
// @return  the visibility bitfield
//
uint8_t GetVisibilityFromObjectId(const int in_id, double in_frame, const bool in_checkHideMasterFlag)
{
   ProjectItem item = Application().GetObjectFromID(in_id);
   X3DObject xsiObj = (X3DObject)item;
   return GetVisibilityFromObject(xsiObj, in_frame, in_checkHideMasterFlag);
}


// Evaluates the Arnold Sidedness property and compute the sidedness bitfield. 
//
// @param in_polyProperties    The properties array
// @param in_frame             The current frame time
// @param out_result           The result bitfiels
//
// @return  true if the sidedness property exists, alse false
//
bool GetSidedness(const CRefArray &in_polyProperties, double in_frame, uint8_t &out_result)
{ 
   out_result = AI_RAY_ALL; // Double sideness for all rays
   
   Property sidedness;
   in_polyProperties.Find(L"arnold_sidedness", sidedness);

   if (!sidedness.IsValid())
      return false;

   bool camera                = (bool)ParAcc_GetValue(sidedness, L"camera", in_frame);
   bool cast_shadow           = (bool)ParAcc_GetValue(sidedness, L"cast_shadow", in_frame);
   bool diffuse_reflection    = (bool)ParAcc_GetValue(sidedness, L"diffuse_reflection", in_frame);
   bool specular_reflection   = (bool)ParAcc_GetValue(sidedness, L"specular_reflection", in_frame);
   bool diffuse_transmission  = (bool)ParAcc_GetValue(sidedness, L"diffuse_transmission", in_frame);
   bool specular_transmission = (bool)ParAcc_GetValue(sidedness, L"specular_transmission", in_frame);
   bool volume                = (bool)ParAcc_GetValue(sidedness, L"volume", in_frame);

	if (!camera)                out_result = out_result ^ AI_RAY_CAMERA;
	if (!cast_shadow)           out_result = out_result ^ AI_RAY_SHADOW;
	if (!diffuse_reflection)    out_result = out_result ^ AI_RAY_DIFFUSE_REFLECT;
	if (!specular_reflection)   out_result = out_result ^ AI_RAY_SPECULAR_REFLECT;
	if (!diffuse_transmission)  out_result = out_result ^ AI_RAY_DIFFUSE_TRANSMIT;
	if (!specular_transmission) out_result = out_result ^ AI_RAY_SPECULAR_TRANSMIT;
	if (!volume)                out_result = out_result ^ AI_RAY_VOLUME;

   return true;
}


// Load the Arnold Parameters property for an Arnold node
//
// @param in_node                   The Arnold node
// @param in_paramsArray            Array of parameters
// @param in_frame                  Frame time
// @param in_filterCurveParameters  If to filter parameters based on the node type (a little slower due to string comparisons)
//
void LoadArnoldParameters(AtNode* in_node, CParameterRefArray &in_paramsArray, double in_frame, bool in_filterParameters)
{
   LONG nbParameters = in_paramsArray.GetCount();

   // in_filterParameters by now is always false except in the case of ice strands
   bool isPoints(false), isPointsDisk(false), isMesh(false);
   bool isCurve = AiNodeIs(in_node, ATSTRING::curves); // is it a curves node ?

   if (in_filterParameters)
   {
      // note that AiNodeIs sees through ginstances: if in_node is a ginstance,
      // AiNodeIs checks for the type of the ginstanced node, not the ginstance itself
      isMesh = AiNodeIs(in_node, ATSTRING::polymesh); // is it a polymesh node ?      
      isPoints = AiNodeIs(in_node, ATSTRING::points); // is it a points node ?
      if (isPoints)
      {
         const char* mode = AiNodeGetStr(in_node, "mode");
         isPointsDisk = strcmp(mode, "disk") == 0; // is it a points node in disk node ?
      }
   }

   for (LONG i=0; i<nbParameters; i++)
   {
      Parameter param(in_paramsArray[i]);
      CString paramName(param.GetScriptName());
      const char* charParamName = paramName.GetAsciiString();

      // skip subdiv_ params of the new (2.2) parameters property, since it's already
      // fully managed by LoadSinglePolymesh.
      if ( !strcmp(charParamName, "subdiv_pixel_error") || 
           !strcmp(charParamName, "subdiv_adaptive_error") || 
           !strcmp(charParamName, "subdiv_iterations")  || 
           !strcmp(charParamName, "subdiv_adaptive_metric") ||
           !strcmp(charParamName, "subdiv_adaptive_space"))
         continue;

      // for ice objects, the custom property cannot be "shaped" at apply time, as it happens for
      // other types of objects. For instance, on a mesh, the hair options are not loaded with the other
      // arnold parameters. So, for ice, the arnold parameter panel exposes all the parameters
      // and so we must filter here, so not to give "min_pixel_width: (..) to ice objects
      // other than strands (which are exported as curves)
      if (in_filterParameters)
      {
         // sss does not apply on curves, points, etc (just polymesh), so skip these params
         if ( !strcmp(charParamName, "export_pref") || 
              !strcmp(charParamName, "sss_setname"))
         {
            if (!isMesh)
               continue;
         }

         // min_pixel_width is allowed only for curves and disk points
         if (!strcmp(charParamName, "min_pixel_width"))
            if ((!isCurve) && (!isPointsDisk))
               continue;

         // don't export the curve mode parameter if this is not a curve
         if (!strcmp(charParamName, "mode"))
            if (!isCurve)
               continue;
      }
      // let's fix the case of curves mode set to "oriented", but on a regular hair object, not a ICE strand one
      // in_filterParameters is always false for objects loaded from modules other than ICE
      else 
      {
         if (isCurve)
            if (!strcmp(charParamName, "mode"))
               if (param.GetValue(in_frame).GetAsText() == L"oriented")
                  continue;
      }

      if (!strcmp(charParamName, "sss_setname")) // #1553, adding sss_setname to the polymeshes
      {
         CString setName = param.GetValue(); 
         if (setName.IsEmpty()) // #1764 Avoid exporting the sss_setname if it is empty
            continue;
         if (!AiNodeLookUpUserParameter(in_node, "sss_setname"))
            AiNodeDeclare(in_node, "sss_setname", "constant STRING");
         if (AiNodeLookUpUserParameter(in_node, "sss_setname"))
            CNodeSetter::SetString(in_node, "sss_setname", setName.GetAsciiString());

         continue;
      }

      if (!strcmp(charParamName, "toon_id"))
      {
         CString toonId = param.GetValue(); 
         if (toonId.IsEmpty()) // Avoid exporting the toon_id if it is empty
            continue;
         if (!AiNodeLookUpUserParameter(in_node, "toon_id"))
            AiNodeDeclare(in_node, "toon_id", "constant STRING");
         if (AiNodeLookUpUserParameter(in_node, "toon_id"))
            CNodeSetter::SetString(in_node, "toon_id", toonId.GetAsciiString());

         continue;
      }

      if (!strcmp(charParamName, "trace_sets")) // #783: Expose the trace sets string for shapes
      {
         CString traceSets = param.GetValue(); 
         if (traceSets.IsEmpty())
            continue;

         CStringArray traceSetsArray = traceSets.Split(L" ");
         LONG nbStrings = traceSetsArray.GetCount();

         AtArray *a = AiArrayAllocate(nbStrings, 1, AI_TYPE_STRING);
         for (LONG sIndex=0; sIndex<nbStrings; sIndex++)
            AiArraySetStr(a, sIndex, traceSetsArray[sIndex].GetAsciiString());

         AiNodeSetArray(in_node, "trace_sets", a);
      }

      // Skip Autobump Visibility. We handle it later.
      if (!strcmp(charParamName, "autobump_camera") ||
          !strcmp(charParamName, "autobump_diffuse_reflection") ||
          !strcmp(charParamName, "autobump_specular_reflection") ||
          !strcmp(charParamName, "autobump_diffuse_transmission") ||
          !strcmp(charParamName, "autobump_specular_transmission") ||
          !strcmp(charParamName, "autobump_volume_scatter"))
         continue;

      // As XSI Custom Parameter, colors are defined as individual parameters 
      // we need to treat it as special & very ugly case. 
      if (strstr(charParamName, "_R") == NULL)
      {
         CRef tempRef;
         LoadParameterValue(in_node, L"", paramName, param, in_frame, -1, tempRef);
      }
      else
      {
         // Optimize... & look for a better method to set the param through setParam()
         // we cant create at the moment a CompoundParameter so setParam() will not work
         char* newParamName = (char*)alloca(strlen(charParamName));
         strncpy(newParamName, charParamName, strlen(charParamName) - 2);
         newParamName[strlen(charParamName) - 2] = 0;

         CNodeSetter::SetRGB(in_node, newParamName, (float)Parameter(in_paramsArray[i]  ).GetValue(in_frame),
                                                    (float)Parameter(in_paramsArray[i+1]).GetValue(in_frame),
                                                    (float)Parameter(in_paramsArray[i+2]).GetValue(in_frame));
         i+=2;
      }
   }

   // set the autobump visibility introduced in arnold 5.3
   // need to do some logic from LoadParameterValue manually here because that function can't be used for this.
   const char* aiParamName = "autobump_visibility";
   int aiParamType = GetArnoldParameterType(in_node, aiParamName, true);
   if (aiParamType != AI_TYPE_NONE)
      AiNodeUnlink(in_node, aiParamName);
   if (aiParamType == AI_TYPE_BYTE)
      CNodeSetter::SetByte(in_node, aiParamName, GetAutobumpVisibility(in_paramsArray, in_frame));
}


// Return the rays visibility of autobump
//
// Evaluates the Autobump Visibility in the Arnold Parameter property and returns a bitfield that specifies
// the visibility for each ray type.
//
// @param in_paramsArray            Array of parameters
// @param in_frame                  the evaluation time
//
// @return   the autobump visibility bitfield
//
uint8_t GetAutobumpVisibility(CParameterRefArray &in_paramsArray, double in_frame)
{
   uint8_t autobump_visibility = AI_RAY_CAMERA;  // default is camera only

   CRef ab_camera = in_paramsArray.GetItem(L"autobump_camera");
   if (ab_camera.IsValid())
   {
      autobump_visibility = AI_RAY_UNDEFINED;

      bool camera                = (bool)in_paramsArray.GetValue(L"autobump_camera", in_frame);
      bool diffuse_reflection    = (bool)in_paramsArray.GetValue(L"autobump_diffuse_reflection", in_frame);
      bool specular_reflection   = (bool)in_paramsArray.GetValue(L"autobump_specular_reflection", in_frame);
      bool diffuse_transmission  = (bool)in_paramsArray.GetValue(L"autobump_diffuse_transmission", in_frame);
      bool specular_transmission = (bool)in_paramsArray.GetValue(L"autobump_specular_transmission", in_frame);
      bool volume                = (bool)in_paramsArray.GetValue(L"autobump_volume", in_frame);

      if (camera)                autobump_visibility += AI_RAY_CAMERA;
      if (diffuse_reflection)    autobump_visibility += AI_RAY_DIFFUSE_REFLECT;
      if (specular_reflection)   autobump_visibility += AI_RAY_SPECULAR_REFLECT;
      if (diffuse_transmission)  autobump_visibility += AI_RAY_DIFFUSE_TRANSMIT;
      if (specular_transmission) autobump_visibility += AI_RAY_SPECULAR_TRANSMIT;
      if (volume)                autobump_visibility += AI_RAY_VOLUME;
   }

   return autobump_visibility;
}

// Evaluate the Arnold Matte property
//
// @param in_node       The node
// @param in_property   The matte property
// @param in_frame      The current frame time
//
void LoadMatte(AtNode *in_node, const Property &in_property, double in_frame)
{
   if (!in_property.IsValid())
      return;

   bool matte = (bool)ParAcc_GetValue(in_property, L"on", in_frame);
   CNodeSetter::SetBoolean(in_node, "matte", matte);
}


// Load the user options
//
// @param in_node                  the Arnold node
// @param in_property              the Arnold User Options property
// @param in_frame                 the frame time
//
// @return                         CStatus::OK
//
bool LoadUserOptions(AtNode* in_node, const Property &in_property, double in_frame)
{
   if (!in_property.IsValid())
      return false;

   // are the rendering options muting the options overall ?
   if (GetRenderOptions()->m_ignore_user_options)
      return false;

   if ((bool)ParAcc_GetValue(in_property, L"mute", in_frame))
      return false;

   CString userOptions =((CString)ParAcc_GetValue(in_property, L"user_options", in_frame));

   bool resolveTokens = (bool)ParAcc_GetValue(in_property, L"resolve_tokens", in_frame);
   if (!userOptions.IsEmpty())
   {
      if (resolveTokens)
         userOptions = CUtils::ResolveTokenString(userOptions, CTime(in_frame), false);   

      AiNodeSetAttributes(in_node, userOptions.GetAsciiString());
   }

   // export the user data grid (#1682)
   GridData userDataGrid = (GridData)in_property.GetParameterValue(L"userDataGrid");
   if (userDataGrid.IsValid())
      ExportUserDataGrid(in_node, userDataGrid, resolveTokens, in_frame);

   return true;
}


// Load the camera options property
//
// @param in_xsiCamera             the Softimage camera
// @param in_node                  the Arnold camera node
// @param in_property              the Arnold Camera Options property
// @param in_frame                 the frame time
//
void LoadCameraOptions(const Camera &in_xsiCamera, AtNode* in_node, const Property &in_property, double in_frame)
{
   bool motion_blur_on = GetRenderOptions()->m_enable_motion_blur || GetRenderOptions()->m_enable_motion_deform;
   float shutterStart(0.0f), shutterEnd(0.0f); // Arnold defaults

   if (!in_property.IsValid()) // if no property is available, default to the rendering options shutter times
   {
      if (motion_blur_on)
      {
         CSceneUtilities::GetMotionStartEnd(shutterStart, shutterEnd);
         CNodeSetter::SetFloat(in_node, "shutter_start", shutterStart); 
         CNodeSetter::SetFloat(in_node, "shutter_end",   shutterEnd);
      }
      return;
   }

   CDoubleArray transfKeys, defKeys;
   CSceneUtilities::GetMotionBlurData(in_xsiCamera.GetRef(), transfKeys, defKeys, in_frame);
   LONG nbTransfKeys = transfKeys.GetCount();

   CString cameraType = ((CString)ParAcc_GetValue(in_property, L"camera_type", in_frame));

   CNodeSetter::SetFloat(in_node, "exposure", (float)ParAcc_GetValue(in_property, L"exposure", in_frame));

   if (cameraType == L"fisheye_camera")
      CNodeSetter::SetBoolean(in_node, "autocrop", (bool)ParAcc_GetValue(in_property, L"fisheye_autocrop", in_frame));
   else if (cameraType == L"cyl_camera")
   {
      AtArray* horizontal_fov = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_FLOAT);
      AtArray* vertical_fov = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_FLOAT);
      for (LONG ikey=0; ikey<nbTransfKeys; ikey++)
      {
         double frame = transfKeys[ikey];
         float h = (float)ParAcc_GetValue(in_property, L"cyl_horizontal_fov", frame);
         float v = (float)ParAcc_GetValue(in_property, L"cyl_vertical_fov", frame);
         AiArraySetFlt(horizontal_fov, ikey, h);
         AiArraySetFlt(vertical_fov, ikey, v);
      }
      AiNodeSetArray(in_node, "horizontal_fov", horizontal_fov);
      AiNodeSetArray(in_node, "vertical_fov",   vertical_fov);
      CNodeSetter::SetBoolean(in_node, "projective", (bool)ParAcc_GetValue(in_property, L"cyl_projective", in_frame));      
   }
   else if (cameraType == L"vr_camera")
   {
      CString s;
      s = ((CString)ParAcc_GetValue(in_property, L"vr_mode", in_frame));
      CNodeSetter::SetString(in_node, "mode", s.GetAsciiString());
      s = ((CString)ParAcc_GetValue(in_property, L"vr_projection", in_frame));
      CNodeSetter::SetString(in_node, "projection", s.GetAsciiString());
      CNodeSetter::SetFloat(in_node, "eye_separation", (float)ParAcc_GetValue(in_property, L"vr_eye_separation", in_frame));
      CNodeSetter::SetFloat(in_node, "eye_to_neck", (float)ParAcc_GetValue(in_property, L"vr_eye_to_neck", in_frame));
      s = ((CString)ParAcc_GetValue(in_property, L"vr_top_merge_mode", in_frame));
      CNodeSetter::SetString(in_node, "top_merge_mode", s.GetAsciiString());
      CNodeSetter::SetFloat(in_node, "top_merge_angle", (float)ParAcc_GetValue(in_property, L"vr_top_merge_angle", in_frame));
      s = ((CString)ParAcc_GetValue(in_property, L"vr_bottom_merge_mode", in_frame));
      CNodeSetter::SetString(in_node, "bottom_merge_mode", s.GetAsciiString());
      CNodeSetter::SetFloat(in_node, "bottom_merge_angle", (float)ParAcc_GetValue(in_property, L"vr_bottom_merge_angle", in_frame));
   }

   CString shutterType = ((CString)ParAcc_GetValue(in_property, L"shutter_type", in_frame));
   CNodeSetter::SetString(in_node, "shutter_type", shutterType.GetAsciiString());
   if (shutterType == L"curve")
   {
      FCurve fCurve(in_property.GetParameter(L"shutter_curve").GetValue());
      // sample the fcurve with 100 samples (if not linear). If linear (as adviced)
      // only the key values are returned
      AtArray *shutterCurve = GetFCurveRawArray(fCurve, 100); 
      AiNodeSetArray(in_node, "shutter_curve", shutterCurve);
   }

   CString rollingShutter = ((CString)ParAcc_GetValue(in_property, L"rolling_shutter", in_frame));
   CNodeSetter::SetString(in_node, "rolling_shutter", rollingShutter.GetAsciiString());
   if (rollingShutter != L"off")
   {
      float rollingShutterDuration = (float)ParAcc_GetValue(in_property, L"rolling_shutter_duration", in_frame);
      CNodeSetter::SetFloat(in_node, "rolling_shutter_duration", rollingShutterDuration);
   }

   if (motion_blur_on)
   {
      CSceneUtilities::GetMotionStartEnd(shutterStart, shutterEnd); // default to the rendering options shutter times

      if (ParAcc_Valid(in_property, L"override_camera_shutter"))
      {
         if ((bool)ParAcc_GetValue(in_property, L"override_camera_shutter", in_frame))
         {
            shutterStart = (float)ParAcc_GetValue(in_property, L"shutter_start", in_frame);
            shutterEnd   = (float)ParAcc_GetValue(in_property, L"shutter_end", in_frame);
         }
      }
   }

   CNodeSetter::SetFloat(in_node, "shutter_start", shutterStart); 
   CNodeSetter::SetFloat(in_node, "shutter_end",   shutterEnd);

   // DOF and aperture
   const AtNodeEntry *entry = AiNodeGetNodeEntry(in_node);
   bool hasDOF = AiNodeEntryLookUpParameter(entry, "aperture_size") != NULL;

   AtArray *focus_distance(NULL), *aperture_size(NULL);
   bool enableDepthOfField = (bool)ParAcc_GetValue(in_property, L"enable_depth_of_field", in_frame);

   if (hasDOF && enableDepthOfField)
   {
      aperture_size = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_FLOAT);
      focus_distance = AiArrayAllocate(1, (uint8_t)nbTransfKeys, AI_TYPE_FLOAT);     
   }

   bool useInterestDistance = (bool)ParAcc_GetValue(in_property, L"use_interest_distance", in_frame);
   for (LONG ikey=0; ikey<nbTransfKeys; ikey++)
   {
      double frame = transfKeys[ikey];

      if (focus_distance)
      {
         float dist = useInterestDistance ? (float)ParAcc_GetValue(in_xsiCamera, L"interestdist", frame) : 
                                            (float)ParAcc_GetValue(in_property, L"focus_distance", frame);
         AiArraySetFlt(focus_distance, ikey, dist);
      }
      if (aperture_size)
      {
         float apertureSize = (float)ParAcc_GetValue(in_property, L"aperture_size", frame);
         AiArraySetFlt(aperture_size, ikey, apertureSize);
      }
   }

   if (hasDOF)
   {
      if (aperture_size)
      {
         int apertureBlades(0);
         if ((bool)ParAcc_GetValue(in_property, L"use_polygonal_aperture",   in_frame))
            apertureBlades = (int)ParAcc_GetValue(in_property, L"aperture_blades", in_frame);
         float apertureBladeCurvature = (float)ParAcc_GetValue(in_property, L"aperture_blade_curvature", in_frame);
         float apertureRotation       = (float)ParAcc_GetValue(in_property, L"aperture_rotation",        in_frame);
         float apertureAspectRatio    = (float)ParAcc_GetValue(in_property, L"aperture_aspect_ratio",    in_frame);

         CNodeSetter::SetInt(in_node,   "aperture_blades",          apertureBlades); 
         CNodeSetter::SetFloat(in_node, "aperture_blade_curvature", apertureBladeCurvature); 
         CNodeSetter::SetFloat(in_node, "aperture_rotation",        apertureRotation); 
         CNodeSetter::SetFloat(in_node, "aperture_aspect_ratio",    apertureAspectRatio);       

         AiNodeSetArray(in_node, "aperture_size", aperture_size);
      }
   }

   if (focus_distance)
      AiNodeSetArray(in_node, "focus_distance", focus_distance);

   if (cameraType == L"persp_camera" && ParAcc_Valid(in_property, L"radial_distortion"))
   {
      float radial_distortion = (float)ParAcc_GetValue(in_property, L"radial_distortion", in_frame);
      CNodeSetter::SetFloat(in_node, "radial_distortion", radial_distortion); 
   }

   // filtermap (all cameras)
   CRef clipRef;
   bool resetFiltermap(false);

   if ((bool)ParAcc_GetValue(in_property, L"enable_filtermap", in_frame))
   {
      clipRef.Set((CString)ParAcc_GetValue(in_property, L"filtermap", in_frame));
      ImageClip2 clip(clipRef);
      if (clip.IsValid())
      {
         AtNode* clipNode = LoadImageClip(clip, in_frame);
         if (clipNode)
            CNodeSetter::SetPointer(in_node, "filtermap", clipNode);
      }
      else
         resetFiltermap = true;
   }
   else
      resetFiltermap = true;

   if (resetFiltermap)
   {
      void* ptr = AiNodeGetPtr(in_node, "filtermap");
      if (ptr)
         CNodeSetter::SetPointer(in_node, "filtermap", NULL);
   }

   // uvremap (perspective camera only)
   if (cameraType == L"persp_camera")
   {
      bool resetRemap(false);

      if ((bool)ParAcc_GetValue(in_property, L"enable_uv_remap", in_frame))
      {
         clipRef.Set((CString)ParAcc_GetValue(in_property, L"uv_remap", in_frame));
         ImageClip2 clip(clipRef);
         if (clip.IsValid())
         {
            AtNode* clipNode = LoadImageClip(clip, in_frame);
            if (clipNode)
               AiNodeLink(clipNode, "uv_remap", in_node);
         }
         else
            resetRemap = true;
      }
      else
         resetRemap = true;
      
      if (resetRemap)
      {
         AtNode* linkedNode = AiNodeGetLink(in_node, "uv_remap");
         if (linkedNode)
         {
            AiNodeUnlink(in_node, "uv_remap");
            CNodeSetter::SetRGBA(in_node, "uv_remap", 0.0f, 0.0f, 0.0f, 0.0f);
         }
      }
   }
}


// Collect the user data blobs.
//
// @param in_xsiObj                The Softimage object
// @param in_frame                 The frame time
//
// @return                         The array of user data blob properties
//
CRefArray CollectUserDataBlobProperties(const X3DObject &in_xsiObj, double in_frame)
{
   CRefArray result;
   CRefArray properties = in_xsiObj.GetProperties();
   // don't use properties.Find as we do for the other properties. Instead,
   // cycle all the blob properties, so we are ok supporting several blobs on the same object
   for (LONG i=0; i<properties.GetCount(); i++)
   {
      Property prop = properties[i];

      if (prop.GetType() != L"UserDataBlob")
         continue;
      // enpty blob ?
      UserDataBlob udb(prop);
      if (udb.IsEmpty())
         continue;
      // Render Data flag unchecked ?
      if (!(bool)ParAcc_GetValue(udb, L"RenderData", in_frame))
         continue;
      // ok, add it to the returned array
      result.Add(prop);
   }

   return result;
}


// Export the user data blobs
//
// @param in_node                  The Arnold node
// @param CRefArray                The blob properties
// @param in_frame                 The frame time
//
void ExportUserDataBlobProperties(AtNode* in_node, const CRefArray &in_blobProperties, double in_frame)
{
   for (LONG i=0; i<in_blobProperties.GetCount(); i++)
   {
      UserDataBlob udb(in_blobProperties[i]);

      CString name = udb.GetName();
      unsigned int id = (unsigned int)(int)ParAcc_GetValue(udb, L"UserDataID", in_frame);
      // userDataID
      CString userDataIDAttribute = name + L"_ID";
      AiNodeDeclare(in_node,  userDataIDAttribute.GetAsciiString(), "constant UINT");
      CNodeSetter::SetUInt(in_node, userDataIDAttribute.GetAsciiString(), id);

      const unsigned char *buffer;
      unsigned int size;
      udb.GetValue(buffer, size);

      AtArray *blobArray = AiArrayAllocate(size, 1, AI_TYPE_BYTE);

      const unsigned char *c = buffer; 
      for (unsigned int j=0; j<size; j++, c++)
         AiArraySetByte(blobArray, j, *c);

      AiNodeDeclare(in_node, name.GetAsciiString(), "constant ARRAY BYTE");
      AiNodeSetArray(in_node, name.GetAsciiString(), blobArray);
   }
}


// Load the user data blobs. Single objects are ok calling this, but clones of the same object,
// in particular ICE instances, should collect and export, so to avoid searching the blob properties 
// many time on the same master object (#1563)
//
// @param in_node                  The Arnold node
// @param in_xsiObj                The Softimage object
// @param in_frame                 The frame time
//
void LoadUserDataBlobs(AtNode* in_node, const X3DObject &in_xsiObj, double in_frame)
{
   CRefArray blobProperties = CollectUserDataBlobProperties(in_xsiObj, in_frame);
   ExportUserDataBlobProperties(in_node, blobProperties, in_frame);
}


