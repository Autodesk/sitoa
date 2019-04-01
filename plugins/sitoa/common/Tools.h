/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_camera.h>
#include <xsi_color4f.h>
#include <xsi_doublearray.h>
#include <xsi_geometry.h>
#include <xsi_imageclip2.h>
#include <xsi_matrix4.h>
#include <xsi_pass.h>
#include <xsi_scenerenderproperty.h>
#include <xsi_shader.h>
#include <xsi_string.h>   // Do not remove. Needed by xsi_doublearray.h
#include "xsi_vector3f.h"

#include "common/NodeSetter.h"
#include "loader/PathTranslator.h"

#include <ai.h>

using namespace XSI;
using namespace XSI::MATH;

// enum for the motion blur position
enum eMbPos
{
   eMbPos_Start = 0,
   eMbPos_Center,
   eMbPos_End,
   eMbPos_Custom
};

namespace ATSTRING
{
   // node names
   const AtString box("box");
   const AtString curves("curves");
   const AtString cyl_camera("cyl_camera");
   const AtString cylinder_light("cylinder_light");
   const AtString disk_light("disk_light");
   const AtString ginstance("ginstance");
   const AtString image("image");
   const AtString mesh_light("mesh_light");
   const AtString photometric_light("photometric_light");
   const AtString physical_sky("physical_sky");
   const AtString points("points");
   const AtString polymesh("polymesh");
   const AtString quad_light("quad_light");
   const AtString skydome_light("skydome_light");
   const AtString sphere("sphere");
   const AtString vector_map("vector_map");
   // node params
   const AtString filename("filename");
   const AtString name("name");
   // camera param names
   const AtString position("position");
   const AtString look_at("look_at");
   const AtString up("up");
   const AtString matrix("matrix");
   const AtString near_clip("near_clip");
   const AtString far_clip("far_clip");
   const AtString shutter_start("shutter_start");
   const AtString shutter_end("shutter_end");
   const AtString shutter_type("shutter_type");
   const AtString shutter_curve("shutter_curve");
   const AtString rolling_shutter("rolling_shutter");
   const AtString rolling_shutter_duration("rolling_shutter_duration");
   const AtString filtermap("filtermap");
   const AtString handedness("handedness");
   const AtString time_samples("time_samples");
   const AtString screen_window_min("screen_window_min");
   const AtString screen_window_max("screen_window_max");
   const AtString exposure("exposure");
   // common metadata
   const AtString desc("desc");
   const AtString min("min");
   const AtString max("max");
   const AtString softmin("softmin");
   const AtString softmax("softmax");
   const AtString linkable("linkable");
   const AtString deprecated("deprecated");
   // sitoa-specific metadata
   const AtString soft_category("soft.category");
   const AtString soft_order("soft.order");
   const AtString soft_label("soft.label");
   const AtString soft_skip("soft.skip");
   const AtString soft_inspectable("soft.inspectable");
   const AtString soft_viewport_guid("soft.viewport_guid");
   const AtString soft_node_type("soft.node_type");
   // sitoa custom ports/types
   const AtString closure("closure");
};

namespace VERBOSITY
{
   const int errors = AI_LOG_ERRORS | AI_LOG_TIMESTAMP | AI_LOG_MEMORY | AI_LOG_BACKTRACE;
   const int warnings = AI_LOG_ERRORS | AI_LOG_TIMESTAMP | AI_LOG_MEMORY | AI_LOG_BACKTRACE | AI_LOG_WARNINGS;
   const int info = AI_LOG_ERRORS | AI_LOG_TIMESTAMP | AI_LOG_MEMORY | AI_LOG_BACKTRACE | AI_LOG_WARNINGS | AI_LOG_INFO | AI_LOG_STATS | AI_LOG_PROGRESS;
   const int all = AI_LOG_ALL;
};


// macros for accessing parameters
#define ParAcc_GetParameter(in_obj, in_name) in_obj.GetParameter(in_name)
#define ParAcc_GetValue(in_obj, in_name, in_frame) in_obj.GetParameterValue(in_name, in_frame)
#define ParAcc_Valid(in_obj, in_name) in_obj.GetParameter(in_name).IsValid()


class CUtilities
{
public:
   // Clamp in_f between in_min and in_max
   inline float Clamp(float in_f, float in_min, float in_max)

   {
      float result = in_f < in_min ? in_min : in_f;
      result = result > in_max ? in_max : result;
      return result;
   }

   ///////////////////////////////////
   // Softimage to Arnold transformers
   ///////////////////////////////////
   // Convert a CVector3f to a AtVector
   inline void S2A(const CVector3f &in_v, AtVector &out_v)
   {
      out_v.x = in_v.GetX();
      out_v.y = in_v.GetY();
      out_v.z = in_v.GetZ();
   }

   // Convert a CVector3 to a AtVector
   inline void S2A(const CVector3 &in_v, AtVector &out_v)
   {
      out_v.x = (float)in_v.GetX();
      out_v.y = (float)in_v.GetY();
      out_v.z = (float)in_v.GetZ();
   }

   // Convert 3 float to a AtVector
   inline void S2A(const float &in_x, const float &in_y, const float &in_z, AtVector &out_v)
   {
      out_v.x = in_x;
      out_v.y = in_y;
      out_v.z = in_z;
   }

   // Convert 3 double to a AtVector
   inline void S2A(const double &in_x, const double &in_y, const double &in_z, AtVector &out_v)
   {
      S2A((float)in_x, (float)in_y, (float)in_z, out_v);
   }

   // Convert a CColor4f to a AtRGBA
   inline void S2A(CColor4f in_c, AtRGBA &out_c)
   {
      out_c.r = in_c.GetR();
      out_c.g = in_c.GetG();
      out_c.b = in_c.GetB();
      out_c.a = in_c.GetA();
   }

   // Convert a CMatrix4 to a AtMatrix
   void S2A(const CMatrix4 &in_matrix, AtMatrix &out_matrix);
   // Convert a CMatrix4f to a AtMatrix
   void S2A(const CMatrix4f &in_matrix, AtMatrix &out_matrix);
   // Convert a CMatrix3 to a AtMatrix
   void S2A(const CMatrix3 &in_matrix3, AtMatrix &out_matrix);
   // Convert a CRotationf to a AtMatrix
   void S2A(const CRotationf &in_rotf, AtMatrix &out_matrix);

   // Convert a CTransformation to a AtMatrix
   inline void S2A(const CTransformation &in_transform, AtMatrix &out_matrix)
   {
      S2A(in_transform.GetMatrix4(), out_matrix);
   }

// array out of bound check
#define CHECK_ARRAY()                                                     \
{                                                                         \
      uint32_t _nelements = AiArrayGetNumElements(in_a);                  \
      uint8_t _nkeys = AiArrayGetNumKeys(in_a);                           \
      if (in_index >= (int)_nelements || in_key >= (int)_nkeys)           \
         return false;                                                    \
}                                                                         \

// absolute index for the following array set/get-ers
#define ARRAY_INDEX (in_key * AiArrayGetNumElements(in_a) + in_index)

   // Set a float value into a AtArray
   // @param in_a      The input array
   // @param in_value  The value to set
   // @param in_index  The array index
   // @param in_key    The mb key
   // @return false if in_index/in_key exceed the array size, else true 
   inline bool SetArrayValue(AtArray *in_a, float in_value, const int in_index, const int in_key)
   {
      CHECK_ARRAY()
      AiArraySetFlt(in_a, ARRAY_INDEX, in_value);
      return true;
   }

   // Set a AtVector value into a AtArray
   // @param in_a      The input array
   // @param in_value  The value to set
   // @param in_index  The array index
   // @param in_key    The mb key
   // @return false if in_index/in_key exceed the array size, else true 
   inline bool SetArrayValue(AtArray *in_a, AtVector in_value, const int in_index, const int in_key)
   {
      CHECK_ARRAY()
      AiArraySetVec(in_a, ARRAY_INDEX, in_value);
      return true;
   }

   // Set a matrix value into a AtArray
   // @param in_a      The input array
   // @param in_value  The value to set
   // @param in_key    The mb key
   // @return false if in_key exceed the array size, else true 
   inline bool SetArrayValue(AtArray *in_a, AtMatrix in_value, const int in_key)
   {
      if (in_key >= AiArrayGetNumKeys(in_a))
         return false;
      AiArraySetMtx(in_a, in_key, in_value);
      return true;
   }

   // Get a float value from a AtArray
   // @param in_a      The input array
   // @param out_value The returned value
   // @param in_index  The array index
   // @param in_key    The mb key
   // @return false if in_index/in_key exceed the array size, else true 
   inline bool GetArrayValue(AtArray *in_a, float *out_value, const int in_index, const int in_key)
   {
      CHECK_ARRAY()
      *out_value = AiArrayGetFlt(in_a, ARRAY_INDEX);
      return true;
   }

   // Get a AtVector value from a AtArray
   // @param in_a      The input array
   // @param out_value The returned value
   // @param in_index  The array index
   // @param in_key    The mb key
   // @return false if in_index/in_key exceed the array size, else true 
   inline bool GetArrayValue(AtArray *in_a, AtVector *out_value, const int in_index, const int in_key)
   {
      CHECK_ARRAY()
      *out_value = AiArrayGetVec(in_a, ARRAY_INDEX);
      return true;
   }

   // Get a AtMatrix value from a AtArray
   // @param in_a      The input array
   // @param out_value The returned value
   // @param in_key    The mb key
   // @return false if in_index/in_key exceed the array size, else true 
   inline bool GetArrayValue(AtArray *in_a, AtMatrix *out_value, const int in_key)
   {
      if (in_key >= AiArrayGetNumKeys(in_a))
         return false;
      *out_value = AiArrayGetMtx(in_a, in_key);
      return true;
   }

   // Destroy an array of nodes. Be VERY careful when calling
   bool DestroyNodesArray(AtArray *in_array);

};


class CNodeUtilities
{
public:
   // I want to centralize here all the node naming get and set,
   // so not to have any "name" string around in the code
   inline CString GetName(AtNode *in_node)
   {
      CString name(AiNodeGetName(in_node));
      return name;
   }
   inline void SetName(AtNode *in_node, CString in_name)
   {
      CNodeSetter::SetString(in_node, "name", in_name.GetAsciiString());
   }
   inline void SetName(AtNode *in_node, const char *in_name)
   {
      CNodeSetter::SetString(in_node, "name", in_name);
   }
   inline void SetName(AtNode *in_node, string in_name)
   {
      CNodeSetter::SetString(in_node, "name", in_name.c_str());
   }

   // return the entry name of a node 
   CString GetEntryName(AtNode *in_node);
   // return the entry type (shape, light, etc. of a node 
   CString GetEntryType(AtNode *in_node);
   // Find all the nodes whose name has " "+in_name, or whose name begins by in_name
   vector <AtNode*> GetInstancesOf(CString &in_name);
   // Assign motion_start, motion_end
   static void SetMotionStartEnd(AtNode *in_node);
   // Utilities for setting constant user data
   //
   // Declare and set a constant INT user data
   void DeclareConstantUserParameter(AtNode *in_node, CString &in_name, int in_value);
   // Declare and set a constant FLOAT user data
   void DeclareConstantUserParameter(AtNode *in_node, CString &in_name, float in_value);
   // Declare and set a constant BOOL user data
   void DeclareConstantUserParameter(AtNode *in_node, CString &in_name, bool in_value);
   // Declare and set a constant STRING user data
   void DeclareConstantUserParameter(AtNode *in_node, CString &in_name, CString &in_value);
   // Declare and set a constant RGBA user data
   void DeclareConstantUserParameter(AtNode *in_node, CString &in_name, AtRGBA &in_value);
};


class CStringUtilities
{
public:
   // Converts a string to lower case
   CString ToLower(CString in_s);
   // Converts a string to lower case in place
   char* ToLower(char* io_s);
   // Replace all the occurrences of a substring with another in a string
   CString ReplaceString(CString in_searchString, CString in_replaceString, CString in_targetString);
   // Replacement for strdup because free'ing a strdup() string crashes in VS2005
   char* Strdup(const char *in_str);
   // Build the name for an Arnold node
   CString MakeSItoAName(const SIObject &in_obj, double in_frame, CString in_suffix, bool in_addUniqueId);
   // Build the name for an Arnold node
   CString MakeSItoAName(CValue in_value, double in_frame, CString in_suffix, bool in_addUniqueId);
   // Get the name of the Softimage object that originated an Arnold node
   CString GetSoftimageNameFromSItoAName(CString &in_nane);
   // Return the name of the master node of a ginstance or a cloned light
   CString GetMasterBaseNodeName(CString &in_name);
   // Checks if string starts with substring
   bool StartsWith(CString in_string, CString in_subString);
   // Checks if string ends with substring
   bool EndsWith(CString in_string, CString in_subString);
   // Converts a parameter name to prettier Title Case formated string
   CString PrettifyParameterName(CString in_string);

private:
   // Build the name for an Arnold node (overload for a CString input type)
   CString MakeSItoAName(CString in_string, double in_frame, CString in_suffix, bool in_addUniqueId);
};


class CSceneUtilities
{
public:
   // Get the output image resolution and aspect ratio.
   static void GetSceneResolution(int &out_width, int &out_height, float &out_aspectRatio);
   // Return the global shutter data
   static void GetShutter(double in_frame, int &out_position, double &out_start, double &out_end, double &out_duration);
   // Return the motion start/end
   static void GetMotionStartEnd(float &out_motion_start, float &out_motion_end);
   // Compute the transformation and deformation motion key times
   static void GetMotionBlurData(const CRef &in_ref, CDoubleArray &out_transfKeys, CDoubleArray &out_defKeys, double in_frame, bool in_force=false);
   // If one of the mb time is equal to in_frame, move it to the first position in the keys array
   static CDoubleArray OptimizeMbKeysOrder(const CDoubleArray &in_keys, CLongArray &out_keysPosition, double in_frame);
   // Returns true if the rendering image is to be displayed by the display driver
   static bool DisplayRenderedImage();
private:
   // Compute the motion key times, given the input frame and the number of keys
   static void GetKeyTimes(CDoubleArray &out_keys, int in_nbKeys, double in_frame);
};


class CObjectUtilities
{
public:
   // Get the object id of the input item
   ULONG GetId(const ProjectItem &in_pi);
   // Return the active primitive of an object at a given frame time
   Primitive GetPrimitiveAtFrame(const X3DObject &in_obj, double in_frame);
   // Return the active primitive of an object at the current time
   Primitive GetPrimitiveAtCurrentFrame(const X3DObject &in_obj);
   // Return the geometry of an object at a given frame time
   Geometry  GetGeometryAtFrame(const X3DObject &in_obj, double in_frame);
   // Return the geometry of an object at a given frame time for a given contruction mode
   Geometry  GetGeometryAtFrame(const X3DObject &in_obj, siConstructionMode in_mode, double in_frame);
   // Check if an object is parent (up to the sceen root) of another object
   bool HasParent(const X3DObject &in_object, const X3DObject &in_parent);
};


class CPathUtilities
{
public:
   // Get the shaders search path from the rendering option
   CPathString GetShadersPath();
   // Get the procedurals search path from the rendering options
   CPathString GetProceduralsPath();
   // Get the textures search path from the rendering options
   CPathString GetTexturesPath();
   // We will get the Output Ass path
   CString GetOutputAssPath();
   // We will get the Output Log path
   CString GetOutputLogPath();
   // We will get the Output Exported (ass) File Name
   CString GetOutputExportFileName(bool in_extension, bool in_resolvedFrame, double in_frame);
   // Return true if a directory or file exists
   bool PathExists(const char *in_path);
};


class CTimeUtilities
{
public:
   // Get the current frame
   double GetCurrentFrame();
   // Get the frame rate
   double GetFps();
   // Sleeps specified ms
   void SleepMilliseconds(unsigned int in_ms);
   // Returns a Formated Time
   CString FormatTime(unsigned int in_millis, int in_msDigits, bool in_padZeros, bool in_padSpaces);
   // Frame thousands to integer
   inline LONG FrameTimes1000(double in_frame)
   {
      return (LONG)floor(in_frame * 1000.0f + 0.5f);
   }
};


// As progID is like Softimage.bump2d.1, we need to get the type of the shader ("bump2d")
const CString GetShaderNameFromProgId(const CString &in_shaderProgID);
// Returns if the render camera from the given pass is a group (stereo rendering)
bool IsCameraGroup(const Pass &in_pass);
// Will return if the polymesh is Deformable by Deform Operators or Envelopes
bool IsDeformable(const X3DObject &in_xsiObj, double in_frame);
// Returns a description about the error code that returns AiRender()
CString GetRenderCodeDesc(int in_errorCode);
// Will return the frame padded with Scene Padding as CString
const CString GetFramePadded(LONG in_frame);
// Initialized Path Translator tables
bool InitializePathTranslator();
// Return a pointer array to Arnold Driver Nodes
CStringArray GetDriverNames();
// Establish Log Options to Arnold from Render Settings
void SetLogSettings(const CString& in_renderType, double in_frame);
// Filter the polymesh, hair and pointcloud objects in in_array
CRefArray FilterShapesFromArray(CRefArray in_array);
// Get the polymesh, hair and pointcloud objects under a model
CRefArray GetAllShapesBelowModel(const Model &in_model);
// Get the polymesh, hair and pointcloud objects under the scene root
CRefArray GetAllShapesBelowTheRoot();
// Get the bbox, taking into account the motion blur setting (#1246).
CStatus GetBoundingBoxFromObjects(const CRefArray in_objects, double in_frame, double &out_min_x, double &out_min_y, double &out_min_z, double &out_max_x, double &out_max_y, double &out_max_z);
// Checks whether a CRefArray contains a given CRef
bool ArrayContainsCRef(const CRefArray& in_array, const CRef& in_ref);
// Add a CRef to the output array, optionally recursing over the children
void AddCRefToArray(CRefArray& out_array, const CRef& in_item, bool in_recursive);
// Checks whether running in interactive or batch mode and returns the correct Arnold enum.
const AtSessionMode GetSessionMode();
