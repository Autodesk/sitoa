/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_application.h>
#include <xsi_command.h>
#include <xsi_factory.h>
#include <xsi_menu.h>
#include <xsi_shaderdef.h>
#include <xsi_shaderstructparamdef.h>
#include <xsi_status.h>
#include <xsi_string.h>
#include <xsi_utils.h>

#include <ai.h>
#include <ai_nodes.h>

using namespace std;
using namespace XSI;


class CShaderDefParameter
{
private:

   int           m_type;       // output type
   int           m_arrayType;  // array type if m_type is AI_TYPE_ARRAY
   AtParamValue  m_default;    // default value, left undefined for arrays
   AtEnum        m_enum;       // enum descriptor for the AI_TYPE_ENUM type

   bool     m_has_label;
   CString  m_label;
   bool     m_has_min, m_has_max;
   float    m_min, m_max;
   bool     m_has_softmin, m_has_softmax;
   float    m_softmin, m_softmax;
   bool     m_has_linkable;
   bool     m_linkable;
   bool     m_has_inspectable;
   bool     m_inspectable;
   bool     m_has_viewport_guid;
   CString  m_viewport_guid;
   bool     m_has_node_type;
   CString  m_node_type;

public:

   CString       m_name;       // name of the parameter

   CShaderDefParameter() : 
      m_has_label(false), m_has_min(false), m_has_max(false), 
      m_has_softmin(false), m_has_softmax(false), m_has_linkable(false), m_has_viewport_guid(false),
      m_has_node_type(false)
   {}

   CShaderDefParameter(const CShaderDefParameter &in_arg) :
      m_type(in_arg.m_type), 
      m_arrayType(in_arg.m_arrayType),
      m_has_label(in_arg.m_has_label), m_label(in_arg.m_label), 
      m_has_min(in_arg.m_has_min), m_has_max(in_arg.m_has_max),  
      m_min(in_arg.m_min), m_max(in_arg.m_max),
      m_has_softmin(in_arg.m_has_softmin), m_has_softmax(in_arg.m_has_softmax), 
      m_softmin(in_arg.m_softmin), m_softmax(in_arg.m_softmax),
      m_has_linkable(in_arg.m_has_linkable), m_linkable(in_arg.m_linkable),
      m_has_inspectable(in_arg.m_has_inspectable), m_inspectable(in_arg.m_inspectable),
      m_has_viewport_guid(in_arg.m_has_viewport_guid), m_viewport_guid(in_arg.m_viewport_guid),
      m_has_node_type(in_arg.m_has_node_type), m_node_type(in_arg.m_node_type),
      m_name(in_arg.m_name)
   {
      m_default = in_arg.m_default;
      m_enum    = in_arg.m_enum;
   }

   CShaderDefParameter(const AtParamEntry* in_paramEntry, const AtNodeEntry* in_node_entry);

   ~CShaderDefParameter()
   {}

   // Define an input shader parameter
   void Define(ShaderParamDefContainer &in_paramDef, const CString &in_shader_name);
   // Add the parameter to the input layout
   void Layout(PPGLayout &in_layout);
   // void Log();
};


class CShaderDefShader
{
private:
   ShaderDef m_sd;
   bool m_sd_created;

   AtNodeEntry* m_node_entry;
   CString m_filename;                         // so/dll full path
   int     m_type;                             // output type
   vector <CShaderDefParameter> m_parameters;  // parameters

   bool     m_has_desc;
   CString  m_desc;
   bool     m_has_category;
   CString  m_category;
   bool     m_has_order;
   CString  m_order;
   bool     m_has_deprecated;
   bool     m_deprecated;

   void CheckOrderMetadata(const CStringArray &in_order_array);

public:
   CString m_name;                            // name of the shader
   CString m_so_name;                          // the so/dll file name
   bool    m_is_camera_node;                   // is this a custom camera node?
   bool    m_is_passthrough_closure;           // is this the SItoA shader called "closure" ?
   bool    m_has_skip;
   bool    m_skip;

   CShaderDefShader() : 
      m_sd_created(false), m_node_entry(NULL), m_has_desc(false), m_has_category(false), 
      m_has_order(false), m_has_deprecated(false), m_is_camera_node(false), m_has_skip(false)
   {
   }

   CShaderDefShader(const CShaderDefShader &in_arg) : 
      m_sd(in_arg.m_sd), 
      m_sd_created(in_arg.m_sd_created), 
      m_node_entry(in_arg.m_node_entry),
      m_filename(in_arg.m_filename), 
      m_type(in_arg.m_type), 
      m_parameters(in_arg.m_parameters),
      m_has_desc(in_arg.m_has_desc), m_desc(in_arg.m_desc),
      m_has_category(in_arg.m_has_category), m_category(in_arg.m_category),
      m_has_order(in_arg.m_has_order), m_order(in_arg.m_order),
      m_has_deprecated(in_arg.m_has_deprecated), m_deprecated(in_arg.m_deprecated),
      m_name(in_arg.m_name),
      m_so_name(in_arg.m_so_name), 
      m_is_camera_node(in_arg.m_is_camera_node), 
      m_is_passthrough_closure(in_arg.m_is_passthrough_closure),
      m_has_skip(in_arg.m_has_skip), m_skip(in_arg.m_skip)
   {
   }

   CShaderDefShader(AtNodeEntry* in_node_entry, const bool in_clone_vector_map = false);

   ~CShaderDefShader()
   {
      m_parameters.clear();
   }

   // Define this shader
   CString Define(const bool in_clone_vector_map = false);
   // Build the layout
   void Layout();
   // void Log();
};


class CShaderDefSet
{
private:
   // We keep the shader sorted by so/dll and then by alphabetical order
   set < CString > m_prog_ids;

public:

   CShaderDefSet()
   {}

   CShaderDefSet(const CShaderDefSet &in_arg) : 
      m_prog_ids(in_arg.m_prog_ids)
   {}

   ~CShaderDefSet()
   {
      m_prog_ids.clear();
   }

   // Loads all the shaders and build their shader definition
   void Load(const CString &in_plugin_origin_path);
   // Return the array of the progIds of all the defined shaders
   CStringArray GetProgIds();
   // Remove all the shader definitions and clear the progId set
   void Clear();
};


