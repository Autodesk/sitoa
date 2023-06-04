/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/NodeSetter.h"
#include "common/Tools.h"
#include "renderer/RenderMessages.h"
#include "renderer/Renderer.h"

#include <xsi_framebuffer.h>
#include <xsi_kinematics.h>
#include <xsi_model.h>
#include <xsi_passcontainer.h>
#include <xsi_plugin.h>
#include <xsi_preferences.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_utils.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sstream>

// We use a different function on windows than linux for the sleep function
#ifdef _WINDOWS
#include <io.h>
#include <windows.h>
#endif
#ifdef _LINUX
#include <unistd.h>
// #include <pthread.h>
#endif


// Convert a CMatrix4 to a AtMatrix
//
// @param in_matrix4   The input matrix
// @param out_matrix   The output Arnold matrix
//
void CUtilities::S2A(const CMatrix4& in_matrix4, AtMatrix& out_matrix)
{
   double f00, f01, f02, f03, f10, f11, f12, f13, f20, f21, f22, f23, f30, f31, f32, f33;
   in_matrix4.Get(f00, f01, f02, f03, f10, f11, f12, f13, f20, f21, f22, f23, f30, f31, f32, f33);

   out_matrix[0][0] = (float)f00;  
   out_matrix[0][1] = (float)f01;
   out_matrix[0][2] = (float)f02;
   out_matrix[0][3] = (float)f03;
   out_matrix[1][0] = (float)f10;
   out_matrix[1][1] = (float)f11;  
   out_matrix[1][2] = (float)f12;
   out_matrix[1][3] = (float)f13;
   out_matrix[2][0] = (float)f20;
   out_matrix[2][1] = (float)f21;  
   out_matrix[2][2] = (float)f22;
   out_matrix[2][3] = (float)f23;
   out_matrix[3][0] = (float)f30;
   out_matrix[3][1] = (float)f31;  
   out_matrix[3][2] = (float)f32;
   out_matrix[3][3] = (float)f33;
}


// Convert a CMatrix4f to a AtMatrix
//
// @param in_matrix4   The input matrix
// @param out_matrix   The output Arnold matrix
//
void CUtilities::S2A(const CMatrix4f& in_matrix4, AtMatrix& out_matrix)
{
   in_matrix4.Get(out_matrix[0][0], out_matrix[0][1], out_matrix[0][2], out_matrix[0][3], 
                  out_matrix[1][0], out_matrix[1][1], out_matrix[1][2], out_matrix[1][3],
                  out_matrix[2][0], out_matrix[2][1], out_matrix[2][2], out_matrix[2][3],
                  out_matrix[3][0], out_matrix[3][1], out_matrix[3][2], out_matrix[3][3]);
}


// Convert a CMatrix3 to a AtMatrix
//
// @param in_matrix3   The input matrix
// @param out_matrix   The output Arnold matrix
//
void CUtilities::S2A(const CMatrix3& in_matrix3, AtMatrix& out_matrix)
{
   double f00, f01, f02, f10, f11, f12, f20, f21, f22;
   in_matrix3.Get(f00, f01, f02, f10, f11, f12, f20, f21, f22);

   out_matrix = AiM4Identity();
   out_matrix[0][0] = (float)f00;  
   out_matrix[0][1] = (float)f01;
   out_matrix[0][2] = (float)f02;
   out_matrix[1][0] = (float)f10;
   out_matrix[1][1] = (float)f11;  
   out_matrix[1][2] = (float)f12;
   out_matrix[2][0] = (float)f20;
   out_matrix[2][1] = (float)f21;
   out_matrix[2][2] = (float)f22;
}


// Convert a CRotationf to a AtMatrix
//
// @param in_rotf      The input CRotationf
// @param out_matrix   The output Arnold matrix
//
void CUtilities::S2A(const CRotationf &in_rotf, AtMatrix &out_matrix)
{
   CRotation rot = CIceUtilities().RotationfToRotation(in_rotf);
   CMatrix3 m3 = rot.GetMatrix();
   S2A(m3, out_matrix);
}


// Destroy an array of nodes. Be VERY careful when calling
//
// @param in_array      The input array
//
// @return true if all the nodes were deleted successfully, else false
//
bool CUtilities::DestroyNodesArray(AtArray *in_array)
{
   if (AiArrayGetType(in_array) != AI_TYPE_NODE) // not a AtNode* array
      return false;
   for (unsigned int i=0; i<AiArrayGetNumElements(in_array); i++)
   {
      AtNode *node = (AtNode*)AiArrayGetPtr(in_array, i);
      if (!AiNodeDestroy(node))
         return false;
      // This method is called only when rebuilding a light's array of filters.
      // So, also erase the node from the exported shaders map for #1647
      GetRenderInstance()->ShaderMap().EraseExportedNode(node);
   }
   return true;
}



////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////


// return the entry name of a node 
//
// @param in_node      The node
//
// @return the entry name
//
CString CNodeUtilities::GetEntryName(AtNode *in_node)
{
   CString result(AiNodeEntryGetName(AiNodeGetNodeEntry(in_node)));
   return result;
}


// return the entry type (shape, light, etc. of a node 
//
// @param in_node      The node
//
// @return the entry name
//
CString CNodeUtilities::GetEntryType(AtNode *in_node)
{
   CString result(AiNodeEntryGetTypeName(AiNodeGetNodeEntry(in_node)));
   return result;
}


// Find all the nodes whose name has " "+in_name, or whose name begins by in_name
// The first case is the ginstances one, where the master object is at the tail of the ginstance name
// The second case is to catch the time shifted instances of an object. Say 
// sphere.SItoA.10000 exists, and it's time-instanced by a pointcloud. What we do at time, say 9, is to create
// a sphere.SItoA.9000 with visibility 0, and then create a ginstance for the point(s) whose instance time is 9.
// If we delete sphere.SItoA.10000, we'll pass "sphere.SItoA." as the argument ot his function, so we'll be able
// to catch all the objects that were generated because of "sphere
//
// @param in_name      The name to be searched for while iterating
//
// @return the vector of the found nodes
//
vector <AtNode*> CNodeUtilities::GetInstancesOf(CString &in_name)
{
   vector <AtNode*> result;
   CString masterName = L" " + in_name;

   AtNodeIterator *iter = AiUniverseGetNodeIterator(NULL, AI_NODE_SHAPE);
   while (!AiNodeIteratorFinished(iter))
   {
      AtNode *node = AiNodeIteratorGetNext(iter);
      if (!node)
         break;

      CString nodeName = CNodeUtilities().GetName(node);

      if ( (nodeName.FindString(masterName) != UINT_MAX) ||
           (nodeName.FindString(in_name) == 0) )
      {
         // GetMessageQueue()->LogMsg(L"Found " + nodeName);
         result.push_back(node);
      }
   }

   AiNodeIteratorDestroy(iter);

   return result;
}


// Assign motion_start, motion_end
void CNodeUtilities::SetMotionStartEnd(AtNode *in_node)
{
   if (!in_node)
      return;

   const AtNodeEntry* node_entry = AiNodeGetNodeEntry(in_node);
   if (AiNodeEntryLookUpParameter(node_entry, "motion_start"))
   {
      float motion_start, motion_end;
      CSceneUtilities::GetMotionStartEnd(motion_start, motion_end);
      CNodeSetter::SetFloat(in_node, "motion_start", motion_start);
      CNodeSetter::SetFloat(in_node, "motion_end",   motion_end);
   }
}


// Utilities for setting constant user data

// Declare and set a constant INT user data
//
// @param in_node               The Arnold node
// @param in_name               The data name
// @param in_value              The data value
//
void CNodeUtilities::DeclareConstantUserParameter(AtNode *in_node, CString &in_name, int in_value)
{
   if (!AiNodeLookUpUserParameter(in_node, in_name.GetAsciiString()))
      AiNodeDeclare(in_node, in_name.GetAsciiString(), "constant INT");
   CNodeSetter::SetInt(in_node, in_name.GetAsciiString(), in_value);
}


// Declare and set a constant FLOAT user data
//
// @param in_node               The Arnold node
// @param in_name               The data name
// @param in_value              The data value
//
void CNodeUtilities::DeclareConstantUserParameter(AtNode *in_node, CString &in_name, float in_value)
{
   if (!AiNodeLookUpUserParameter(in_node, in_name.GetAsciiString()))
      AiNodeDeclare(in_node, in_name.GetAsciiString(), "constant FLOAT");
   CNodeSetter::SetFloat(in_node, in_name.GetAsciiString(), in_value);
}


// Declare and set a constant BOOL user data
//
// @param in_node               The Arnold node
// @param in_name               The data name
// @param in_value              The data value
//
void CNodeUtilities::DeclareConstantUserParameter(AtNode *in_node, CString &in_name, bool in_value)
{
   if (!AiNodeLookUpUserParameter(in_node, in_name.GetAsciiString()))
      AiNodeDeclare(in_node, in_name.GetAsciiString(), "constant BOOL");
   CNodeSetter::SetBoolean(in_node, in_name.GetAsciiString(), in_value);
}


// Declare and set a constant STRING user data
//
// @param in_node               The Arnold node
// @param in_name               The data name
// @param in_value              The data value
//
void CNodeUtilities::DeclareConstantUserParameter(AtNode *in_node, CString &in_name, CString &in_value)
{
   if (!AiNodeLookUpUserParameter(in_node, in_name.GetAsciiString()))
      AiNodeDeclare(in_node, in_name.GetAsciiString(), "constant STRING");
   CNodeSetter::SetString(in_node, in_name.GetAsciiString(), in_value.GetAsciiString());
}


// Declare and set a constant RGBA user data
//
// @param in_node               The Arnold node
// @param in_name               The data name
// @param in_value              The data value
//
void CNodeUtilities::DeclareConstantUserParameter(AtNode *in_node, CString &in_name, AtRGBA &in_value)
{
   if (!AiNodeLookUpUserParameter(in_node, in_name.GetAsciiString()))
      AiNodeDeclare(in_node, in_name.GetAsciiString(), "constant RGBA");
   CNodeSetter::SetRGBA(in_node, in_name.GetAsciiString(), in_value.r, in_value.g, in_value.b, in_value.a);
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////

// Converts a string to lower case
//
// @param in_s  The input string
//
// @return the lower cased string 
//
CString CStringUtilities::ToLower(CString in_s)
{
   CString out_s;
   for (ULONG i=0; i<in_s.Length(); i++)
      out_s+= (char)tolower(in_s[i]);
   return out_s;
}


// Converts a string to lower case in place
//
// @param io_s  The string to convert
//
char* CStringUtilities::ToLower(char* io_s)
{
   char* p = io_s;
   while (*p)
   {
      *p = (char)tolower(*p);
      p++;
   }
   return io_s;
}


// Replace all the occurrences of a substring with another in a string
// Using the Split method as we did is not very safe, because for instance
// CString s(L"1234);
// CStringArray split = s.Split(L"12")
// gives split.GetCount == 1. Also
// CString s(L"3412);
// CStringArray split = s.Split(L"12")
// gives 1. So, here are a few exceptions to manage that I missed (#1381)
// With a plain GetSubsString in loop it's easier
//
// @param in_searchString    The substring to search
// @param in_replaceString   The substring replacing in_searchString
// @param in_targetString    The string to modify
//
// @return the input string (in_targetString) after the replacements
//
CString CStringUtilities::ReplaceString(CString in_searchString, CString in_replaceString, CString in_targetString)
{
   CString res(in_targetString);
   ULONG start;

   while ((start = res.FindString(in_searchString)) != UINT_MAX)
      res = res.GetSubString(0, start) + in_replaceString + res.GetSubString(start + in_searchString.Length());

   return res;
}


// Replacement for strdup because free'ing a strdup() string crashes in VS2005
char* CStringUtilities::Strdup(const char *in_str)
{
   // This causes a crash: return strdup(str);
   char *ptr;
   unsigned int len;

   if (in_str == NULL)
      return NULL;

   len = (unsigned int)strlen(in_str) + 1;
   ptr = (char*)AiMalloc(len);

   if (ptr == NULL)
      return NULL;

   memcpy(ptr, in_str, len);
   return ptr;
}


// Build the name for an Arnold node
//
// @param in_obj           The Softimage object/light/property
// @param in_frame         The frame time
// @param in_suffix        A string to be added after the ".SItoA." label
// @param in_addUniqueId   If true, add a unique integer to the end of the name
//
// @return the node name
//
CString CStringUtilities::MakeSItoAName(const SIObject &in_obj, double in_frame, CString in_suffix, bool in_addUniqueId)
{
   CString s = in_obj.GetFullName() + L".SItoA.";
   CString result = MakeSItoAName(s, in_frame, in_suffix, in_addUniqueId);
   return result;
}


// Build the name for an Arnold node (overload for CValue input types)
//
// @param in_obj           The cvalue
// @param in_frame         The frame time
// @param in_suffix        A string to be added after the ".SItoA." label
// @param in_addUniqueId   If true, add a unique integer to the end of the name
//
// @return the node name
//
CString CStringUtilities::MakeSItoAName(CValue in_value, double in_frame, CString in_suffix, bool in_addUniqueId)
{
   CString s = in_value.GetAsText() + L".SItoA.";
   CString result = MakeSItoAName(s, in_frame, in_suffix, in_addUniqueId);
   return result;
}


// Build the name for an Arnold node (overload for a CString input type)
//
// @param in_string        The input string
// @param in_frame         The frame time
// @param in_suffix        A string to be added after the ".SItoA." label
// @param in_addUniqueId   If true, add a unique integer to the end of the name
//
// @return the node name
//
CString CStringUtilities::MakeSItoAName(CString in_string, double in_frame, CString in_suffix, bool in_addUniqueId)
{
   CString result = in_string;

   if (in_suffix != L"")
      result = result + in_suffix + L".";

   result+= CString(CTimeUtilities().FrameTimes1000(in_frame));

   if (in_addUniqueId)
      result = result + L"." + (CString)GetRenderInstance()->GetUniqueId();

   return result;
}


// @param in_name        The input node name
//
// @return the Softimage object's name
//
CString CStringUtilities::GetSoftimageNameFromSItoAName(CString &in_name)
{
   if (in_name == L"")
      return L"";

   CStringArray sitoaSplits = in_name.Split(L".SItoA");
   if (sitoaSplits.GetCount() < 2) // no ".SItoA" in name, not a native SItoA exported shape
      return L"";

   CString beforeSItoA = sitoaSplits[0];
   // ginstances' names contain " ", separating the masters names
   CStringArray spaceSplit = beforeSItoA.Split(L" ");

   CString result = spaceSplit.GetCount() > 1 ? spaceSplit[0] : beforeSItoA;

   return result;
}


// Return the name of the master node of a ginstance or a cloned light
//
// @param in_name        The input node name
//
// @return the master node's name
//
CString CStringUtilities::GetMasterBaseNodeName(CString &in_name)
{
   CStringArray splits = in_name.Split(L" ");
   LONG count = splits.GetCount();
   if (count < 2)
      return in_name;
   return splits[count-1];
}


// Return true or false if string starts with substring
//
// @param in_string        The input string
// @param in_subString     The start string
//
// @return true or false
//
bool CStringUtilities::StartsWith(CString in_string, CString in_subString)
{
   return (in_string.FindString(in_subString) == 0);
}


// Return true or false if string ends with substring
//
// @param in_string        The input string
// @param in_subString     The end string
//
// @return true or false
//
bool CStringUtilities::EndsWith(CString in_string, CString in_subString)
{
   return (in_string.ReverseFindString(in_subString) == (in_string.Length() - in_subString.Length()));
}


// Converts a parameter name to prettier Title Case formated string
//
// @param in_string        The input string
//
// @return the string in Title Case format
//
CString CStringUtilities::PrettifyParameterName(CString in_string)
{
   CString label;
   // replace "_" with " ". "_" is very common in the Arnold nodes
   // Ex: "emission_color" -> "emission color: 
   CString t_label = CStringUtilities().ReplaceString(L"_", L" ", in_string);
   // capitalize the first char of the name, and each token after a space, as we do for the SItoA shaders
   // Ex: "emission color" -> "Emission Color: 
   for (ULONG i=0; i<t_label.Length(); i++)
   {
      if (i==0)
         label+= (char)toupper(t_label[i]);
      else if (t_label[i-1] == ' ')
         label+= (char)toupper(t_label[i]);
      else
         label+= t_label[i];
   }

   return label;
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////

// Get the output image resolution and aspect ratio.
//
// @param out_width         the returned image width
// @param out_height        the returned image height
// @param out_aspectRatio   the returned image aspect ratio
//
void CSceneUtilities::GetSceneResolution(int &out_width, int &out_height, float &out_aspectRatio)
{
   Scene activeScene = Application().GetActiveProject().GetActiveScene();
   float cameraAspectRatio = (float)ParAcc_GetValue(GetRenderInstance()->GetRenderCamera(), L"aspect", DBL_MAX);

   ProjectItem optionsItem = activeScene.GetActivePass(); // get the current pass options
   // keep using them only if the "override scene render options" check is enabled
   if (! (bool)ParAcc_GetValue(optionsItem, L"ImageFormatOverride", DBL_MAX)) // else use the scene options 
      optionsItem = activeScene.GetPassContainer().GetProperties().GetItem(L"Scene Render Options");

   out_width              = (int)  ParAcc_GetValue(optionsItem, L"ImageWidth",       DBL_MAX);
   out_height             = (int)  ParAcc_GetValue(optionsItem, L"ImageHeight",      DBL_MAX);
   float imageAspectRatio = (float)ParAcc_GetValue(optionsItem, L"ImageAspectRatio", DBL_MAX);
   float imagePixelRatio  = (float)ParAcc_GetValue(optionsItem, L"ImagePixelRatio",  DBL_MAX);

   out_aspectRatio = imageAspectRatio / imagePixelRatio;
   // Divide now by the camera aspect ratio
   out_aspectRatio/= cameraAspectRatio; 
}


// Return the global shutter data
//
// @param in_frame        the current frame time
// @param out_position    the returned shutter position
// @param out_start       the returned shutter start time
// @param out_end         the returned shutter end time
// @param out_duration    the returned shutter duration
//
void CSceneUtilities::GetShutter(double in_frame, int &out_position, double &out_start, double &out_end, double &out_duration)
{
   out_position = GetRenderOptions()->m_motion_shutter_onframe;
   out_duration = GetRenderOptions()->m_motion_shutter_length;
   // let's start computing the start, end, duration depending on the position

   switch (out_position)
   {
      case eMbPos_Start:
         out_start = in_frame;
         out_end   = out_start + out_duration;
         break;
      case eMbPos_Center:
         out_start = in_frame - out_duration * 0.5;
         out_end   = out_start + out_duration;
         break;
      case eMbPos_End:
         out_start = in_frame - out_duration;
         out_end   = in_frame;
         break;
      case eMbPos_Custom:
         out_start = in_frame + GetRenderOptions()->m_motion_shutter_custom_start;
         out_end   = in_frame + GetRenderOptions()->m_motion_shutter_custom_end;
         out_duration = out_end - out_start; // override duration
         break;
      default:
         return;
   }
}


void CSceneUtilities::GetMotionStartEnd(float &out_motion_start, float &out_motion_end)
{
   int   position = GetRenderOptions()->m_motion_shutter_onframe;
   float duration = GetRenderOptions()->m_motion_shutter_length;

   switch (position)
   {
      case eMbPos_Start:
         out_motion_start = 0;
         out_motion_end = out_motion_start + duration;
         break;
      case eMbPos_Center:
         out_motion_start = - duration * 0.5f;
         out_motion_end = out_motion_start + duration;
         break;
      case eMbPos_End:
         out_motion_start = - duration;
         out_motion_end = out_motion_start + duration;
         break;
      case eMbPos_Custom:
         out_motion_start = GetRenderOptions()->m_motion_shutter_custom_start;
         out_motion_end   = GetRenderOptions()->m_motion_shutter_custom_end;
         break;
      default: // just to make gcc happy
         out_motion_start = 0.0f;
         out_motion_end = 1.0f;
         return;
   }
}

// Compute the motion key times, given the input frame and the number of keys
//
// @param out_keys        the returned array of times
// @param in_nbKeys       the number of keys
// @param in_frame        the current frame time
//
void CSceneUtilities::GetKeyTimes(CDoubleArray &out_keys, int in_nbKeys, double in_frame)
{
   double key, startTime, endTime, duration;
   int    position;
   
   GetShutter(in_frame, position, startTime, endTime, duration);

   double timeStep = duration / (in_nbKeys - 1);

   switch (position)
   {
      case eMbPos_Start:
         for (LONG i=0; i<in_nbKeys-1; i++)
         {
            key = startTime + timeStep * i;
            out_keys.Add(key);
         }
         out_keys.Add(endTime);
         break;
      case eMbPos_Center:
         for (LONG i=0; i<in_nbKeys-1; i++)
         {
            // if the number of keys is an odd number, then the central key is in_frame
            if ( (in_nbKeys%2 == 1) && (i == (in_nbKeys-1) / 2) )
               key = in_frame;
            else
               key = startTime + timeStep * i;
            out_keys.Add(key);
         }
         out_keys.Add(endTime);
         break;
      case eMbPos_End:
         out_keys.Add(startTime);
         for (LONG i=1; i<in_nbKeys; i++)
         {
            key = endTime - timeStep * (in_nbKeys - 1 - i);
            out_keys.Add(key);
         }
         break;
      case eMbPos_Custom:
         for (LONG i=0; i<in_nbKeys-1; i++)
         {
            key = startTime + timeStep * i;
            out_keys.Add(key);
         }
         out_keys.Add(endTime);
         break;
   }

   /*
   GetMessageQueue()->LogMsg(L"-----------");
   for (LONG i=0; i<in_nbKeys; i++)
      GetMessageQueue()->LogMsg(((CValue)i).GetAsText() + L" = " + ((CValue)out_keys[i]).GetAsText());
   */
}


// Compute the transformation and deformation motion key times
//
// @param in_ref          the object for which the times must be computed
// @param out_transfKeys  the returned array of times for transformation blur
// @param out_defKeys     the returned array of times for transformation blur
// @param in_frame        the current frame time
// @param in_force        if true, get the deformation keys regardless if the object is not deformable
//
void CSceneUtilities::GetMotionBlurData(const CRef &in_ref, CDoubleArray &out_transfKeys, CDoubleArray &out_defKeys, double in_frame, bool in_force)
{
   X3DObject obj(in_ref);

   bool transfOn = GetRenderOptions()->m_enable_motion_blur;
   bool defOn    = GetRenderOptions()->m_enable_motion_deform;

   CustomProperty arnoldParameters = obj.GetProperties().GetItem(L"arnold_parameters");
   // does the property exist?
   bool useArnoldParameterProperty = arnoldParameters.IsValid();

   bool apTransfOn(true), apDefOn(true);
   if (useArnoldParameterProperty)
   {
      if (transfOn)
         apTransfOn = (bool)ParAcc_GetValue(arnoldParameters, L"motion_transform", in_frame);
      if (defOn)
         apDefOn    = (bool)ParAcc_GetValue(arnoldParameters, L"motion_deform", in_frame);
   }

   transfOn = transfOn && apTransfOn;
   defOn    = defOn    && apDefOn;

   if (!transfOn)
      out_transfKeys.Add(in_frame);
   if (!defOn)
      out_defKeys.Add(in_frame);

   if (!(transfOn || defOn)) // both off, return
      return;

   LONG stepTransform = transfOn ? GetRenderOptions()->m_motion_step_transform : 0;
   LONG stepDeform    = defOn ?    GetRenderOptions()->m_motion_step_deform    : 0;

   if (useArnoldParameterProperty && (bool)ParAcc_GetValue(arnoldParameters, L"override_motion_step", in_frame))
   {  // get the steps from arnold_parameters
      if (transfOn)
         stepTransform = (LONG)ParAcc_GetValue(arnoldParameters, L"motion_step_transform", in_frame);
      if (defOn)
         stepDeform    = (LONG)ParAcc_GetValue(arnoldParameters, L"motion_step_deform",    in_frame);
   }

   // In Arnold, the number of keys is an unsigned char, not an int.
   stepTransform = AiClamp(stepTransform, (LONG)0, (LONG)255);
   stepDeform    = AiClamp(stepDeform,    (LONG)0, (LONG)255);

   if (transfOn)
      GetKeyTimes(out_transfKeys, stepTransform, in_frame);

   if (defOn)
   if (in_ref.GetClassID() == siX3DObjectID)
   if ( in_force || obj.GetType().IsEqualNoCase(L"hair")  || IsDeformable(obj, in_frame) )
      GetKeyTimes(out_defKeys, stepDeform, in_frame);

   // if we missed some case (#1657), be sure to have at least on key, set at the current frame time
   if (out_transfKeys.GetCount() == 0)
      out_transfKeys.Add(in_frame);
   if (out_defKeys.GetCount() == 0)
      out_defKeys.Add(in_frame);
}


// If one of the input mb time is equal to in_frame, move it to the first position in the keys array
// See #1579 for details. In case of a polymesh, since the geometry at in_frame has just been evaluated 
// by Create, we can save 1 geo evaluation if we make in_frame the first evaluated geo in the mb keys loop. 
// In fact, Softimage will NOT re-pull the geo, but re-use the one pulled in Create.
// Also, we return in out_keysPosition the new order of the keys, so that we know where to insert the data
// (if any) into the Arnold array.
//
// Example
//
// Input:
// in_frame = 10.0
// in_key[0] = 9.0
// in_key[1] = 9.5
// in_key[2] = 10.0
//
// Result:
// outKeys[0] = 10.0
// outKeys[1] = 9.5
// outKeys[2] = 9.0
//
// out_keysPosition[0] = 2
// out_keysPosition[1] = 1
// out_keysPosition[2] = 0
//
// So, we have switched key[0] with key[2], because key[2]==in_frame, and recorder this change into out_keysPosition
//
// @param in_keys            The input keys array
// @param out_keysPosition   The returned indeces order
// @param in_frame           The current frame time
//
// @return                   The re-ordered array of keys
//
CDoubleArray CSceneUtilities::OptimizeMbKeysOrder(const CDoubleArray &in_keys, CLongArray &out_keysPosition, double in_frame)
{
   LONG firstKey = -1;
   for (LONG key=0; key<in_keys.GetCount(); key++)
   {
      if (in_frame == in_keys[key])
      {
         firstKey = key;
         break;
      }
   }
   // copy the keys and the positions
   CDoubleArray outKeys(in_keys);
   for (LONG key=0; key<in_keys.GetCount(); key++)
      out_keysPosition.Add(key);

   // switch #0 with #firstKey
   if (firstKey > 0)
   {
      outKeys[0] = in_keys[firstKey];
      outKeys[firstKey] = in_keys[0];
      out_keysPosition[0] = firstKey;
      out_keysPosition[firstKey] = 0;
   }

   return outKeys;
}


// Returns true if the rendering image is to be displayed by the display driver
// It's false in case of batch rendering, or if render_pass_show_rendered_images is 
// turned off in File->Preferences
//
bool CSceneUtilities::DisplayRenderedImage()
{
   bool displayImage = Application().IsInteractive();
   if (displayImage)
   {
       Property renderPrefs;
       Application().GetPreferences().GetCategories().Find(L"Rendering", renderPrefs);
       if (renderPrefs.IsValid())
          displayImage = renderPrefs.GetParameterValue(L"render_pass_show_rendered_images");
   }

   return displayImage;
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////

// Get the object id of the input item. Let's use this utility whenever possible
// in place of local GetObjectIDs, so to have a centralized method to identify an object.
ULONG CObjectUtilities::GetId(const ProjectItem &in_pi)
{
   return in_pi.GetObjectID();
}


// Return the active primitive of an object at a given frame time
//
// @param in_obj           The object
// @param in_frame         The frame time
//
// @return the primitive
//
Primitive CObjectUtilities::GetPrimitiveAtFrame(const X3DObject &in_obj, double in_frame)
{
   return in_obj.GetActivePrimitive(in_frame);
}


// Return the active primitive of an object at the current time
//
// @param in_obj           The object
//
// @return the primitive
//
Primitive CObjectUtilities::GetPrimitiveAtCurrentFrame(const X3DObject &in_obj)
{
   return in_obj.GetActivePrimitive();
}


// Return the geometry of an object at a given frame time
//
// @param in_obj           The object
// @param in_frame         The frame time
//
// @return the geometry
//
Geometry CObjectUtilities::GetGeometryAtFrame(const X3DObject &in_obj, double in_frame)
{
   return in_obj.GetActivePrimitive(in_frame).GetGeometry(in_frame);
}


// Check if an object is parent (up to the sceen root) of another object
//
// @param in_child            The child object
// @param in_parent           The parent object
//
// @return true if in_parent is parent of in_child, else false
//
bool CObjectUtilities::HasParent(const X3DObject &in_child, const X3DObject &in_parent)
{
   X3DObject object = in_child;
   CRef rootRef = Application().GetActiveSceneRoot().GetRef();

   int i(0);
   while (i<100) // emergency exit against potential infinite loop
   {
      CRef parentRef = object.GetParent3DObject().GetRef();

      if (parentRef == object.GetRef())
         return false; // myself, not sure if this can ever happen, but is documented so by GetParent,
                       // not by GetParent3DObject
      if (parentRef == rootRef)
         return false; // reached the scene root, no more parents to climb up
      if (parentRef == in_parent.GetRef())
         return true; // found

      object = parentRef;
      i++;
   }

   return false;
}


// Return the geometry of an object at a given frame time for a given contruction mode
//
// @param in_obj           The object
// @param in_mode          The requested contruction mode
// @param in_frame         The frame time
//
// @return the geometry
//
Geometry CObjectUtilities::GetGeometryAtFrame(const X3DObject &in_obj, siConstructionMode in_mode, double in_frame)
{
   return in_obj.GetActivePrimitive(in_frame).GetGeometry(in_frame, in_mode);
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////


// Get the shaders search path from the rendering options
// If void, return the SItoA bin directory.
CPathString CPathUtilities::GetShadersPath()
{
   CPathString path(GetRenderOptions()->m_plugins_path);
   if (path.IsVoid())
   {
      Plugin plugin(Application().GetPlugins().GetItem(L"Arnold Render"));
      path = plugin.GetOriginPath();
   }

   return path;
}


// Get the procedurals search path from the rendering options
// If void, return the SItoA bin directory.
CPathString CPathUtilities::GetProceduralsPath()
{
   CPathString path(GetRenderOptions()->m_procedurals_path);
   if (path.IsVoid())
   {
      Plugin plugin(Application().GetPlugins().GetItem(L"Arnold Render"));
      path = plugin.GetOriginPath();
   }

   return path;
}


// Get the textures search path from the rendering options
CPathString CPathUtilities::GetTexturesPath()
{
   CPathString path(GetRenderOptions()->m_textures_path);
   return path;
}


CString CPathUtilities::GetOutputAssPath()
{
   CPathString dir(GetRenderOptions()->m_output_file_tagdir_ass);
   // Resolve the tokens
   CPathString path = dir.ResolveTokens(CTimeUtilities().GetCurrentFrame(), L"[Pass]");

   if (path.IsEmpty())
      path = dir;

   // Resolve the environment variables
   path.ResolvePath();
   return path;
}


CString CPathUtilities::GetOutputLogPath()
{
   CPathString dir = GetRenderOptions()->m_output_file_tagdir_log;
   // Resolve the tokens
   CPathString path = dir.ResolveTokens(CTimeUtilities().GetCurrentFrame());
   // Resolve the environment variables
   path.ResolvePath();
   return path;
}


CString CPathUtilities::GetOutputExportFileName(bool in_extension, bool in_resolvedFrame, double in_frame)
{
   // Get Active Pass
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
   // Getting the resolved output path filename.
   // We have to remove output extension. (its the output image filename)
   CStringArray fileParts;

   if (in_resolvedFrame)
      fileParts = Framebuffer(pass.GetFramebuffers().GetItem(L"Main")).GetResolvedPath(in_frame).Split(CUtils::Slash());
   else
      fileParts = Framebuffer(pass.GetFramebuffers().GetItem(L"Main")).GetResolvedPath().Split(CUtils::Slash());

   CStringArray fileNameParts = fileParts[fileParts.GetCount()-1].Split(L".");
   LONG nparts = fileNameParts.GetCount();

   CString outputFileName = L"";
   for (LONG i=0; i<nparts-1; i++)
      outputFileName+=fileNameParts[i] + (i < nparts-2 ? L"." : L"");

   if (in_extension)
   {
      outputFileName+= L".ass";
      // compressed ?
      if (GetRenderOptions()->m_compress_output_ass)
         outputFileName += L".gz";
   }

   return outputFileName;
}


bool CPathUtilities::PathExists(const char *in_path)
{
#ifdef _WINDOWS
   int intStat = _access(in_path, 0);
#else
   struct stat st;
   int intStat = stat(in_path, &st);
#endif

   return intStat == 0;
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////


// Get the current frame
// 
// @return the frame 
//
double CTimeUtilities::GetCurrentFrame()
{
   CRefArray proplist = Application().GetActiveProject().GetProperties();
   Property playctrl(proplist.GetItem(L"Play Control"));
   double frame = ParAcc_GetValue(playctrl, L"Current", DBL_MAX);
   return frame;
}


// Get the frame rate
// 
// @return the frame rate (fps)
//
double CTimeUtilities::GetFps()
{
   CRefArray proplist = Application().GetActiveProject().GetProperties();
   Property playctrl(proplist.GetItem(L"Play Control"));
   double fps = ParAcc_GetValue(playctrl, L"Rate", DBL_MAX);
   return fps;
}


// Sleep
void CTimeUtilities::SleepMilliseconds(unsigned int in_ms)
{
   // 1 milliseconds = 1000 microsecond.
   // Windows Sleep uses miliseconds, linux usleep uses microsecond
   // and since the default usage of the code was under windows
   // so the argument is   coming in millisecond.
#ifdef _WINDOWS
   Sleep(in_ms);
#endif
#ifdef _LINUX
   usleep(in_ms*1000);
#endif
}


CString CTimeUtilities::FormatTime(unsigned int in_millis, int in_msDigits, bool in_padZeros, bool in_padSpaces)
{
   char hms_string[16], ms_string[16], hstr[16], mstr[16], sstr[16];
   static char return_string[32]; 
   uint32_t h, m, s;

   h = (uint32_t)((in_millis/(1000*3600)))%99;      // hours
   m = (uint32_t)((in_millis/(1000*60  )))%60;      // minutes
   s = (uint32_t)((in_millis/(1000     )))%60;      // seconds

   if (in_padZeros)
      sprintf(hms_string, "%02u:%02u:%02u", h, m, s);
   else
   {
      // format carefully so that 2h 1m 44s ---> 2:01:44, 0h 0m 0s --->0:00

      // setup hours string
      if (h)
         sprintf(hstr, "%u:", h);
      else
         // zero-length string
         hstr[0] = 0; //sprintf(hstr, "");

      // setup minutes string
      if (m && h)
         sprintf(mstr, "%02u:", m);
      else
      {
         if (m)
            sprintf(mstr, "%u:", m);
         else
            sprintf(mstr, "0:");
      }

      // setup seconds string
      sprintf(sstr, "%02u", s);

      sprintf(hms_string, "%s%s%s", hstr, mstr, sstr);
   }

   switch (in_msDigits)
   {
      case 0: strcpy(ms_string, "");                                    break;
      case 1: sprintf(ms_string, ".%01d", (int)((in_millis%1000)/100)); break;
      case 2: sprintf(ms_string, ".%02d", (int)((in_millis%1000)/10));  break;
      case 3: sprintf(ms_string, ".%03d", (int)(in_millis%1000));       break;
   }

   if (in_padSpaces)
   {
      memset(return_string, ' ', 8-strlen(hms_string));
      return_string[8-strlen(hms_string)] = '\0';
      strcat(return_string, hms_string);
      strcat(return_string, ms_string);
   }
   else
   {
      strcpy(return_string, hms_string);
      strcat(return_string, ms_string);
   }

   CString returnStr;
   returnStr.PutAsciiString(return_string);
   return returnStr;
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////

#if 0
// not used, may be useful ine day
LONG CThreadUtilities::GetThreadId()
{
   LONG id(0);
#ifdef _WINDOWS
   id = (LONG)GetCurrentThreadId();
#else
   id = (LONG)pthread_self();
#endif
   return id;
}
#endif
////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////

const CString GetShaderNameFromProgId(const CString &in_shaderProgID)
{
   // We are doing split with XSI CString methos.
   CStringArray strParts = in_shaderProgID.Split(L".");

   char *prog = new char[strParts[1].Length() + 1];
   strcpy(prog, strParts[1].GetAsciiString());

   // Replace invalid chars for node entry names (Arnold ticket#1285)
   // We will implement here the invalid characters & corrections
   unsigned int length = (unsigned int)strlen(prog);
   for (unsigned int i=0; i<length; i++)
   {
      if (prog[i]=='-')
         prog[i] = '_';
   }

   CString shaderType;
   shaderType.PutAsciiString(prog);

   // Deleting temporal char* created.
   delete[] prog;

   return shaderType;
}


bool IsCameraGroup(const Pass &in_pass)
{
   // Checks if the value in the 'camera' parameter for the given pass is a camera or a group.
   CRef objectRef;
   objectRef.Set(ParAcc_GetValue(in_pass, L"Camera", DBL_MAX).GetAsText());
   
   return objectRef.GetClassID() == siGroupID;
}


bool IsDeformable(const X3DObject &in_xsiObj, double in_frame)
{
   // We can loose a lot of time checking on whole animation or keyframes
   // if it has really deform animation so if we detect some deformation operators or
   // something like that we will return that the object is deformated.

   // If it has envelopes, return true
   if (in_xsiObj.GetEnvelopes().GetCount() > 0)
      return true;

   Primitive primitive = CObjectUtilities().GetPrimitiveAtFrame(in_xsiObj, in_frame);

   // Custom Operators like KP_PointCache belongs to siOperatorFamily
   // There are other operators that could deform the mesh
   // that are not DeformOperators so we NEED to filter with siOperatorFamily
   CStringArray families;
   families.Add(siOperatorFamily);

   CRefArray listFiltered;
   CRefArray nestedObjects(primitive.GetNestedObjects());
   nestedObjects.Filter(L"", families, L"", listFiltered);

   // If we have deform operators we will return true
   return listFiltered.GetCount() > 0;
}


CString GetRenderCodeDesc(int in_errorCode)
{
   CString desc;
   switch (in_errorCode)
   {
      case AI_SUCCESS:
         desc = L"no error";
         break;
      case AI_ABORT:
         desc = L"render aborted";
         break;
      case AI_ERROR_NO_CAMERA:
         desc = L"camera not defined";
         break;
      case AI_ERROR_BAD_CAMERA:
         desc = L"bad camera data";
         break;
      case AI_ERROR_VALIDATION:
         desc = L"unable to validate the license"; 
         break;
      case AI_ERROR_RENDER_REGION:
         desc = L"invalid render region";
         break;
      case AI_INTERRUPT:
         desc = L"render interrupted by user";
         break;
      case AI_ERROR_NO_OUTPUTS:
         desc = L"no rendering outputs";
         break;
      case AI_ERROR_UNAVAILABLE_DEVICE:
         desc = L"Cannot create GPU context.";
         break;
      case AI_ERROR:
         desc = L"generic error";
         break;
   }

   return desc;
}


const CString GetFramePadded(LONG in_frame)
{
   Application app;
   SceneRenderProperty sceneRenderProp(app.GetActiveProject().GetActiveScene().GetPassContainer().GetProperties().GetItem(L"Scene Render Options"));
   LONG pad = (LONG)ParAcc_GetValue(sceneRenderProp, L"FramePadding", DBL_MAX);

   char format[8];
   sprintf(format,"%s0%dd", "%", pad);

   // size of the string is at least the padding, but can be longer, if the frame number is longer
   LONG nbChars = CString(in_frame).Length();
   if (nbChars < pad)
      nbChars = pad;

   char* padding = new char[nbChars+1];// +1 to add the \0
   sprintf(padding, (const char*)format, in_frame);

   CString framePadded(padding);

   delete[] padding;
   return framePadded;
}


// Initialize the path translation class, reading the SITOA_LINKTAB_LOCATION env variable and its content
//
// @return true if the linktab file was found and its syntax correct, else false
//
bool InitializePathTranslator()
{
   const char *envP = getenv("SITOA_LINKTAB_LOCATION");
   if (CPathTranslator::Initialize(envP, CUtils::IsWindowsOS() ? TRANSLATOR_WIN_TO_LINUX : TRANSLATOR_LINUX_TO_WIN))
      return true; // we've found the file

   CString location(envP);
   if (location.IsEmpty())
      location = L"empty"; // so we get a meaningful error between ()

   GetMessageQueue()->LogMsg(L"[sitoa] Cannot read the file pointed by SITOA_LINKTAB_LOCATION (" + location + L")", siErrorMsg);
   CString whichWay = CUtils::IsWindowsOS() ? L"Windows to Linux" : L"Linux to Windows";
   GetMessageQueue()->LogMsg(L"[sitoa] Disabling " + whichWay + L" path translation", siErrorMsg);
   return false;
}


CStringArray GetDriverNames()
{
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());

   CRefArray frameBuffers = pass.GetFramebuffers();
   LONG nbuffers = frameBuffers.GetCount();

   CStringArray driverNames;
   for (LONG i=0; i<nbuffers; ++i)
   {
      Framebuffer frameBuffer = Framebuffer(frameBuffers[i]);
      driverNames.Add(frameBuffer.GetFullName());
   }

   return driverNames;
}


void SetLogSettings(const CString& in_renderType, double in_frame)
{
   CString outputLogDir(CPathUtilities().GetOutputLogPath());
   unsigned int logLevel     = GetRenderOptions()->m_log_level;
   unsigned int maxWarnings  = GetRenderOptions()->m_max_log_warning_msgs;
   bool enableConsole        = GetRenderOptions()->m_enable_log_console;
   bool enableFile           = GetRenderOptions()->m_enable_log_file;

   int verbosity(0);
   switch (logLevel)
   {
      case eSItoALogLevel_Errors:   
         verbosity = VERBOSITY::errors;
         break;
      case eSItoALogLevel_Warnings:      
         verbosity = VERBOSITY::warnings;
         break;
      case eSItoALogLevel_Info:      
         verbosity = VERBOSITY::info;
         break;
      case eSItoALogLevel_Debug:
         verbosity = VERBOSITY::all;      
         break;
   }

   // RenderMessage class log level and flags
   CRenderMessages::SetLogLevel(logLevel, enableConsole, enableFile);

   if ((!enableFile) && (!enableConsole)) // all logs off
      AiMsgSetConsoleFlags(NULL, AI_LOG_NONE);
   else
   {
      AiMsgSetConsoleFlags(NULL, verbosity);
      AiMsgSetMaxWarnings(maxWarnings);
      AiMsgSetCallback(CRenderMessages::LogCallback); // all the messages go through the cb
   }

   if (enableFile)
   {
      if (CUtils::EnsureFolderExists(outputLogDir, false))
      {
         CString filename;

         if (in_renderType == L"Export")
            filename = CPathUtilities().GetOutputExportFileName(false, true, in_frame) + L".Loader.log";
         else if (in_renderType == L"Region")
            filename = CPathUtilities().GetOutputExportFileName(false, false, in_frame) + L".RenderRegion.log";
         else if (in_renderType == L"Pass")
            filename = CPathUtilities().GetOutputExportFileName(false, true, in_frame) + L".RenderPass.log";

         CString fullPath = outputLogDir + CUtils::Slash() + filename;
         // open the log file
         GetRenderInstance()->OpenLogFile(fullPath);
      }
      else
         GetMessageQueue()->LogMsg(L"[sitoa] Logging path is not valid", siWarningMsg);
   }

   // stats and profile
   bool enableStats        = GetRenderOptions()->m_enable_stats;
   bool enableProfile      = GetRenderOptions()->m_enable_profile;

   if (enableStats)
   {
      CPathString statsFile = GetRenderOptions()->m_stats_file;
      statsFile.ResolveTokensInPlace(CTimeUtilities().GetCurrentFrame(), L"[Pass]");
      statsFile.ResolvePathInPlace();
      if (CUtils::EnsureFolderExists(statsFile, true))
      {
         AiStatsSetFileName(statsFile.GetAsciiString());
         AiStatsSetMode(AI_STATS_MODE_APPEND);
      }
      else
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Logging Stats path is not valid", siWarningMsg);
         AiStatsSetFileName("");
      }
   }
   else
      AiStatsSetFileName("");

   if (enableProfile)
   {
      CPathString profileFile = GetRenderOptions()->m_profile_file;
      profileFile.ResolveTokensInPlace(CTimeUtilities().GetCurrentFrame(), L"[Pass]");
      profileFile.ResolvePathInPlace();
      if (CUtils::EnsureFolderExists(profileFile, true))
      {
         AiProfileSetFileName(profileFile.GetAsciiString());
      }
      else
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Logging Profile path is not valid", siWarningMsg);
         AiProfileSetFileName("");
      }
   }
   else
      AiProfileSetFileName("");
}


// Filter the polymesh, hair and pointcloud objects in in_array
// 
// @param  in_array  The input array
//
// @return the filtered array 
//
CRefArray FilterShapesFromArray(CRefArray in_array)
{
   CRefArray result;

   for (LONG i=0; i<in_array.GetCount(); i++)
   {
      X3DObject obj(in_array[i]);
      if (!obj.IsValid())
         continue;
      CString objType = obj.GetType();
      if (objType == siPolyMeshType || objType == L"pointcloud" || objType == L"hair")
         result.Add(in_array.GetItem(i));
   }

   return result;
}


// Get the polymesh, hair and pointcloud objects under a model
// 
// @param  in_model  The input model
//
// @return the filtered array 
//
CRefArray GetAllShapesBelowModel(const Model &in_model)
{
   // let's use one array per family, for debugging purposes
   CRefArray meshes, hair, pointClouds, result;

   meshes = in_model.FindChildren(L"", siPolyMeshType, CStringArray(), true);
   // GetMessageQueue()->LogMsg(L"meshes count = " + (CString)meshes.GetCount());
   result+= meshes;

   CStringArray families;
   families.Add(siGeometryFamily);
   families.Add(siPointCloudFamily);
   CRefArray hairAndPcArray = in_model.FindChildren(L"", L"", families, true);

   for (LONG i = 0; i < hairAndPcArray.GetCount(); ++i)
   {
      X3DObject obj(hairAndPcArray[i]);
      if (obj.GetType() == L"hair")
         hair.Add(hairAndPcArray[i]);
      else if (obj.GetType() == L"pointcloud")
         pointClouds.Add(hairAndPcArray[i]);
   }

   // GetMessageQueue()->LogMsg(L"hair count = " + (CString)hair.GetCount());
   result+= hair;
   // GetMessageQueue()->LogMsg(L"pointClouds count = " + (CString)pointClouds.GetCount());
   result+= pointClouds;
   // GetMessageQueue()->LogMsg(L"validObjects count = " + (CString)result.GetCount());
   return result;
}


// Get the polymesh, hair and pointcloud objects under the root
CRefArray GetAllShapesBelowTheRoot()
{
   return GetAllShapesBelowModel(Application().GetActiveSceneRoot());
}



// Get the bbox, taking into account the motion blur setting (#1246).
// I am leaving below another (commented) version of this function, which to me was more precise than this one (see the comment in the 3rd for)
CStatus GetBoundingBoxFromObjects(const CRefArray in_objects, double in_frame,
                                  double &out_min_x, double &out_min_y, double &out_min_z, 
                                  double &out_max_x, double &out_max_y, double &out_max_z)
{
   double obj_min_x, obj_min_y, obj_min_z, obj_max_x, obj_max_y, obj_max_z;
   double c_x, c_y, c_z, ext_x, ext_y, ext_z;

   bool firstTime(true);
   for (LONG i=0; i<in_objects.GetCount(); i++)
   {
      X3DObject object(in_objects.GetItem(i));

      // Get Motion Blur Data
      CRefArray properties = object.GetProperties();

      Property xsiVizProperty = properties.GetItem(L"Visibility");
      if (!(bool)ParAcc_GetValue(xsiVizProperty, L"rendvis", in_frame))
         continue;

      CDoubleArray keyFramesTransform, keyFramesDeform;
      CSceneUtilities::GetMotionBlurData(in_objects.GetItem(i), keyFramesTransform, keyFramesDeform, in_frame);
      LONG nbTransformKeys = keyFramesTransform.GetCount();
      LONG nbDeformKeys = keyFramesDeform.GetCount();

      // Get the transforms at the transf times
      for (LONG trKey=0; trKey<nbTransformKeys; trKey++)
      {
         CTransformation objGlobalTransform = object.GetKinematics().GetGlobal().GetTransform(keyFramesTransform[trKey]);

         // Get the geo at the def times
         for (LONG defKey=0; defKey<nbDeformKeys; defKey++)
         {
            Geometry geometry = CObjectUtilities().GetGeometryAtFrame(object, keyFramesDeform[defKey]);
            if (!geometry.IsValid())
               continue;

            // Get the box of the deformed geo, transformed at the transform key time.
            // To me this is not really correct, but apparently matches the way Arnold computes
            // the bbox from the ass file, when it complaints about a mismatch, for instance
            // WARNING : [arnold] [proc] cube: incorrect user bounds! given: (-4, -4, -4) to (4, 4, 4), generated: ...
            // If there are both transf and def keys, in general the resulting box is greater than the actual one.
            // For instance, if we have 2 keys for both transf and def (t0,t1) we get the box of geo(t0) at transform times t0 and t1,
            // instead of just at t0.
            if (geometry.GetBoundingBox(c_x, c_y, c_z, ext_x, ext_y, ext_z, objGlobalTransform) != CStatus::OK)
               continue;

            if (firstTime)
            {
               out_min_x = c_x - ext_x * 0.5;
               out_min_y = c_y - ext_y * 0.5;
               out_min_z = c_z - ext_z * 0.5;

               out_max_x = c_x + ext_x * 0.5;
               out_max_y = c_y + ext_y * 0.5;
               out_max_z = c_z + ext_z * 0.5;
               firstTime = false;
            } 
            else
            {
               obj_min_x = c_x - ext_x * 0.5;
               out_min_x = obj_min_x < out_min_x ? obj_min_x : out_min_x;
               obj_min_y = c_y - ext_y * 0.5;
               out_min_y = obj_min_y < out_min_y ? obj_min_y : out_min_y;
               obj_min_z = c_z - ext_z * 0.5;
               out_min_z = obj_min_z < out_min_z ? obj_min_z : out_min_z;

               obj_max_x = c_x + ext_x * 0.5;
               out_max_x = obj_max_x > out_max_x ? obj_max_x : out_max_x;
               obj_max_y = c_y + ext_y * 0.5;
               out_max_y = obj_max_y > out_max_y ? obj_max_y : out_max_y;
               obj_max_z = c_z + ext_z * 0.5;
               out_max_z = obj_max_z > out_max_z ? obj_max_z : out_max_z;
            }
         }
      }
   }

   return CStatus::OK;
}


// Checks whether a CRefArray contains a given CRef
//
bool ArrayContainsCRef(const CRefArray& in_array, const CRef& in_ref)
{
   for (LONG i=0; i<in_array.GetCount(); i++)
      if (in_array[i] == in_ref)
         return true;

   return false;
}


// Add a CRef to the output array, optionally recursing over the children
//
void AddCRefToArray(CRefArray& out_array, const CRef& in_item, bool in_recursive)
{
   out_array.Add(in_item);
   
   if (in_recursive)
   {
      X3DObject obj(in_item);
      CRefArray children(obj.GetChildren());
      for (LONG i=0; i<children.GetCount(); i++)
         AddCRefToArray(out_array, children[i], true);
   }
}


// Checks whether running in interactive or batch mode and returns the correct Arnold enum.
//
const AtSessionMode GetSessionMode()
{
   if(Application().IsInteractive())
      return AI_SESSION_INTERACTIVE;
   else
      return AI_SESSION_BATCH;
}
