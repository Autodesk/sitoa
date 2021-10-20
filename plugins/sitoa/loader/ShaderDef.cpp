/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <xsi_plugin.h>

#include "common/Tools.h"
#include "loader/Loader.h"
#include "loader/PathTranslator.h"
#include "loader/ShaderDef.h"
#include "renderer/Renderer.h"

#define DLL_SHADERS_URL L"https://support.solidangle.com/display/A5SItoAUG/Dll-so+shaders"
#define SITOA_SHADERS_URL L"https://support.solidangle.com/display/A5SItoAUG/Shaders"

// see https://groups.google.com/forum/#!searchin/xsi_list/shader$20ui$20mapping/xsi_list/-P4lsUfHRmY/jpYabBR2c_kJ
#define VIEWPORT_MAPPING_GUID  L"{8C80DEF3-1064-11d3-B0B7-00A0C982A112}"


bool MetaDataGetCStr(const AtNodeEntry* in_node_entry, const char *in_param, AtString in_name, CString &out_value)
{
   AtString s;
   bool result = AiMetaDataGetStr(in_node_entry, in_param, in_name, &s);
   if (result)
      out_value.PutAsciiString(s.c_str());
   return result;
}

bool MetaDataGetFltOrInt(const AtNodeEntry* in_node_entry, const char *in_param, AtString in_name, float &out_value)
{
   bool result = AiMetaDataGetFlt(in_node_entry, in_param, in_name, &out_value);
   if (!result)
   {
      int int_md;
      result = AiMetaDataGetInt(in_node_entry, in_param, in_name, &int_md);
      if (result)
         out_value = (float)int_md;
   }
   return result;
}

// Return the Softimage shader-def parameter type for the input Arnold data type
//
// @param in_type   The Arnold data type
//
// @return          The Softimage data type
//
siShaderParameterDataType GetParamSdType(int in_type)
{
   switch (in_type)
   {
      case AI_TYPE_BYTE:
         return siShaderDataTypeInteger;
      case AI_TYPE_INT:
         return siShaderDataTypeInteger;
      case AI_TYPE_UINT:
         return siShaderDataTypeInteger;
      case AI_TYPE_BOOLEAN:
         return siShaderDataTypeBoolean;
      case AI_TYPE_FLOAT:
         return siShaderDataTypeScalar;
      case AI_TYPE_RGB:
         return siShaderDataTypeColor3;
      case AI_TYPE_RGBA:
         return siShaderDataTypeColor4;
      case AI_TYPE_VECTOR:
         return siShaderDataTypeVector3;
      case AI_TYPE_VECTOR2:
         return siShaderDataTypeVector2;
      case AI_TYPE_STRING:
         return siShaderDataTypeString;
      case AI_TYPE_POINTER:
         return siShaderDataTypeUnknown; // Unknown
      case AI_TYPE_NODE:
         return siShaderDataTypeReference;
      case AI_TYPE_ARRAY:
         return siShaderDataTypeUnknown; // Unknown
      case AI_TYPE_MATRIX:
         return siShaderDataTypeMatrix44;
      case AI_TYPE_ENUM:
         return siShaderDataTypeString;
      case AI_TYPE_CLOSURE:
         return siShaderDataTypeCustom;
      default:
         break;
   }
   return siShaderDataTypeUnknown;
}

// Return the Softimage reference filter type for the input metadata node_type
//
// @param in_type   The node_type
//
// @return          The Softimage data type
//
siShaderReferenceFilterType GetShaderReferenceFilterType(CString in_type)
{
   if (in_type == L"object")
      return siObjectReferenceFilter;
   else if (in_type == L"camera")
      return siCameraReferenceFilter;
   else if (in_type == L"light")
      return siLightReferenceFilter;
   else if (in_type == L"material")
      return siMaterialReferenceFilter;
   else if (in_type == L"shader")
      return siShaderReferenceFilter;
   else if (in_type == L"geometric")
      return siGeometryReferenceFilter;
   else if (in_type == L"userdata")
      return siUserDataBlobReferenceFilter;
   else
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Unknown ReferenceFilterType: \"" + in_type + L"\". Check your metadata file.", siWarningMsg);
      return siUnknownReferenceFilter;
   }
}


// Constructor for the CShaderDefParameter class, from the parameter entry
//
// @param in_paramEntry   The Arnold parameter entry
//
CShaderDefParameter::CShaderDefParameter(const AtParamEntry* in_paramEntry, const AtNodeEntry* in_node_entry)
{
   m_name = AiParamGetName(in_paramEntry);
   m_type = AiParamGetType(in_paramEntry);

   if (m_type == AI_TYPE_ARRAY)
   {
      const AtParamValue* defaultValue =  AiParamGetDefault(in_paramEntry);
      m_arrayType = AiArrayGetType(defaultValue->ARRAY());
   }
   else
      m_arrayType = AI_TYPE_UNDEFINED;

   m_default = *AiParamGetDefault(in_paramEntry);
   m_enum    = AiParamGetEnum(in_paramEntry);

   const char* c = m_name.GetAsciiString();
   m_has_label         = MetaDataGetCStr    (in_node_entry, c, ATSTRING::soft_label, m_label);
   m_has_min           = MetaDataGetFltOrInt(in_node_entry, c, ATSTRING::min, m_min);
   m_has_max           = MetaDataGetFltOrInt(in_node_entry, c, ATSTRING::max, m_max);
   m_has_softmin       = MetaDataGetFltOrInt(in_node_entry, c, ATSTRING::softmin, m_softmin);
   m_has_softmax       = MetaDataGetFltOrInt(in_node_entry, c, ATSTRING::softmax, m_softmax);
   m_has_linkable      = AiMetaDataGetBool  (in_node_entry, c, ATSTRING::linkable, &m_linkable);
   m_has_inspectable   = AiMetaDataGetBool  (in_node_entry, c, ATSTRING::soft_inspectable, &m_inspectable);
   m_has_viewport_guid = MetaDataGetCStr    (in_node_entry, c, ATSTRING::soft_viewport_guid, m_viewport_guid);
   m_has_node_type     = MetaDataGetCStr    (in_node_entry, c, ATSTRING::soft_node_type, m_node_type);
}


// Define an input shader parameter
//
// @param in_paramDef   the input param container
//
void CShaderDefParameter::Define(ShaderParamDefContainer &in_paramDef, const CString &in_shader_name)
{
   ShaderParamDefOptions defOptions = ShaderParamDefOptions(Application().GetFactory().CreateShaderParamDefOptions());

   bool texturable = ! (m_type == AI_TYPE_STRING || m_type == AI_TYPE_ENUM);
   bool animatable = ! (m_type == AI_TYPE_STRING || m_type == AI_TYPE_NODE || m_type == AI_TYPE_MATRIX || 
                        m_type == AI_TYPE_ENUM || m_type == AI_TYPE_CLOSURE);
   bool inspectable = m_has_inspectable ? m_inspectable : true;

   if (m_has_linkable)
      texturable = m_linkable;

   defOptions.SetInspectable(inspectable);
   defOptions.SetTexturable(texturable);
   defOptions.SetAnimatable(animatable);

   if (m_has_viewport_guid) // map a parameter to a gl viewport color
      defOptions.SetAttribute(VIEWPORT_MAPPING_GUID, m_viewport_guid);

   // here we set the default for the simple parameters. Struct parameters are set later.
   switch(m_type)
   {
      case AI_TYPE_BYTE:
         defOptions.SetDefaultValue((int)m_default.BYTE());
         break;
      case AI_TYPE_INT:
         defOptions.SetDefaultValue(m_default.INT());
         break;
      case AI_TYPE_UINT:
         defOptions.SetDefaultValue((int)m_default.UINT());
         break;
      case AI_TYPE_BOOLEAN:
         defOptions.SetDefaultValue(m_default.BOOL());
         break;
      case AI_TYPE_FLOAT:
         defOptions.SetDefaultValue(m_default.FLT());
         break;
      case AI_TYPE_STRING:
         defOptions.SetDefaultValue(m_default.STR().c_str());
         break;
      case AI_TYPE_POINTER:
         break;
      case AI_TYPE_NODE:
         break;
      case AI_TYPE_ARRAY:
         break;
      case AI_TYPE_ENUM:
         defOptions.SetDefaultValue((CString)AiEnumGetString(m_enum, m_default.INT()));
         break;
      default:
         break;
   }

   // special case for imagers where we want to set a custom dafult value for layer_selection
   if (CStringUtilities().StartsWith(in_shader_name, L"imager_") && m_name == L"layer_selection")
      defOptions.SetDefaultValue(L"RGBA or RGBA_denoise");

   if (m_has_min && m_has_max)
      defOptions.SetHardLimit((CValue)m_min, (CValue)m_max);
   else if (m_has_min)
      defOptions.SetHardLimit((CValue)m_min, (CValue)1000000);
   else if (m_has_max)
      defOptions.SetHardLimit((CValue)-1000000, (CValue)m_max);

   // copy min and max to softmin and softmax if one of them don't exist.
   if (m_has_softmin && !m_has_softmax && m_has_max)
   {
      m_has_softmax = true;
      m_softmax = m_max;
   }
   if (m_has_softmax && !m_has_softmin && m_has_min)
   {
      m_has_softmin = true;
      m_softmin = m_min;
   }

   if (m_has_softmin && m_has_softmax)
      defOptions.SetSoftLimit((CValue)m_softmin, (CValue)m_softmax);
   // metadata check, fire a message if we have only one of the soft limit
   else if (m_has_softmin && !m_has_softmax)
      GetMessageQueue()->LogMsg(L"[sitoa] " + in_shader_name + L"." + m_name + " has softmin metadata, but no softmax.", siWarningMsg);
   else if (!m_has_softmin && m_has_softmax)
      GetMessageQueue()->LogMsg(L"[sitoa] " + in_shader_name + L"." + m_name + " has softmax metadata, but no softmin.", siWarningMsg);


   bool paramIsArray = m_type == AI_TYPE_ARRAY;
   int paramType = paramIsArray ? m_arrayType : m_type;
   CString customNodeType = L"";

   // check for node type overrides
   // strings can also be overriden to nodes
   if ((paramType == AI_TYPE_STRING || paramType == AI_TYPE_NODE) && m_has_node_type)
   {
      // override node type be AI_TYPE_NODE
      paramType = AI_TYPE_NODE;
      CStringArray nodeTypes = CStringUtilities().ToLower(m_node_type).Split(L" ");
      CString nodeType = nodeTypes[0];

      // force node type even if string (toon shader uses this)
      paramType = AI_TYPE_NODE;
      // set the reference filter type
      defOptions.SetAttribute(siReferenceFilterAttribute, GetShaderReferenceFilterType(nodeType));

      if (nodeTypes.GetCount() > 1)
      {
         if (nodeTypes[1] == L"array")
         {
            // if array is specified in the soft.node_type metadata, a parameter array will be created even though it's not an array in arnold
            // toon shader uses this for lights, but it's data is converted to semicolon-delimited string on rendering/export
            paramIsArray = true;
         }
         else
            GetMessageQueue()->LogMsg(L"[sitoa] " + in_shader_name + L"." + m_name + " has unknown node type override: " + m_node_type, siWarningMsg);
      }
   }
   else if (paramType == AI_TYPE_CLOSURE)
      customNodeType = L"closure";

   ShaderParamDef pDef;
   if (paramIsArray)
   {
      // shaderarrays doesn't use the label but uses SetLongName instead
      CString label;
      if (m_has_label)
         label = m_label;
      else
         label = CStringUtilities().PrettifyParameterName(m_name);
      defOptions.SetLongName(label);

      if (customNodeType != "")
         pDef = in_paramDef.AddArrayParamDef(m_name, customNodeType, defOptions);
      else
         pDef = in_paramDef.AddArrayParamDef(m_name, GetParamSdType(paramType), defOptions);
   }
   else
   {
      if (customNodeType != "")
         pDef = in_paramDef.AddParamDef(m_name, customNodeType, defOptions);
      else
         pDef = in_paramDef.AddParamDef(m_name, GetParamSdType(paramType), defOptions);
   }

   // setting the default for the struct parameters
   if (pDef.IsStructure())
   {
      ShaderStructParamDef structParam(pDef);
      ShaderParamDefContainer container = structParam.GetSubParamDefs();

      switch(m_type)
      {
         case AI_TYPE_RGB:
            container.GetParamDefByName(L"red").SetDefaultValue(m_default.RGB().r);
            container.GetParamDefByName(L"green").SetDefaultValue(m_default.RGB().g);
            container.GetParamDefByName(L"blue").SetDefaultValue(m_default.RGB().b);
            break;
         case AI_TYPE_RGBA:
            container.GetParamDefByName(L"red").SetDefaultValue(m_default.RGBA().r);
            container.GetParamDefByName(L"green").SetDefaultValue(m_default.RGBA().g);
            container.GetParamDefByName(L"blue").SetDefaultValue(m_default.RGBA().b);
            container.GetParamDefByName(L"alpha").SetDefaultValue(m_default.RGBA().a);
            break;
         case AI_TYPE_VECTOR:
            container.GetParamDefByName(L"x").SetDefaultValue(m_default.VEC().x);
            container.GetParamDefByName(L"y").SetDefaultValue(m_default.VEC().y);
            container.GetParamDefByName(L"z").SetDefaultValue(m_default.VEC().z);
            break;
         case AI_TYPE_VECTOR2:
            container.GetParamDefByName(L"x").SetDefaultValue(m_default.VEC2().x);
            container.GetParamDefByName(L"y").SetDefaultValue(m_default.VEC2().y);
            break;
         case AI_TYPE_MATRIX:
         {
            AtMatrix* m = m_default.pMTX();
            container.GetParamDefByName(L"_00").SetDefaultValue((*m)[0][0]);
            container.GetParamDefByName(L"_01").SetDefaultValue((*m)[0][1]);
            container.GetParamDefByName(L"_02").SetDefaultValue((*m)[0][2]);
            container.GetParamDefByName(L"_03").SetDefaultValue((*m)[0][3]);

            container.GetParamDefByName(L"_10").SetDefaultValue((*m)[1][0]);
            container.GetParamDefByName(L"_11").SetDefaultValue((*m)[1][1]);
            container.GetParamDefByName(L"_12").SetDefaultValue((*m)[1][2]);
            container.GetParamDefByName(L"_13").SetDefaultValue((*m)[1][3]);

            container.GetParamDefByName(L"_20").SetDefaultValue((*m)[2][0]);
            container.GetParamDefByName(L"_21").SetDefaultValue((*m)[2][1]);
            container.GetParamDefByName(L"_22").SetDefaultValue((*m)[2][2]);
            container.GetParamDefByName(L"_23").SetDefaultValue((*m)[2][3]);

            container.GetParamDefByName(L"_30").SetDefaultValue((*m)[3][0]);
            container.GetParamDefByName(L"_31").SetDefaultValue((*m)[3][1]);
            container.GetParamDefByName(L"_32").SetDefaultValue((*m)[3][2]);
            container.GetParamDefByName(L"_33").SetDefaultValue((*m)[3][3]);
            break;
         }
         default:
            break;
      }

   }
}


// Add the parameter to the input layout
//
// @param in_layout      The shader's layout
//
void CShaderDefParameter::Layout(PPGLayout &in_layout)
{
   if (m_has_inspectable && !m_inspectable)
      return;

   // if the param is of vector/matrix type, park it into a void group, else it gets scattered
   // at the very end of the ppg (weird)
   bool add_group = m_type == AI_TYPE_VECTOR  || m_arrayType == AI_TYPE_VECTOR  ||
                    m_type == AI_TYPE_VECTOR2 || m_arrayType == AI_TYPE_VECTOR2 ||
                    m_type == AI_TYPE_MATRIX  || m_arrayType == AI_TYPE_MATRIX;

   if (add_group)
      in_layout.AddGroup(L""); 

   CString label;
   PPGItem item;

   if (m_has_label)
      label = m_label;
   else
      label = CStringUtilities().PrettifyParameterName(m_name);

   // if a string parameter is called "filename", it's reasonable to provide a file browser widget
   if (m_type == AI_TYPE_STRING && (m_name == ATSTRING::filename || m_name == ATSTRING::lut_filename))
   {
      item = in_layout.AddItem(m_name, label, siControlFilePath);
      item.PutAttribute(siUIOpenFile, true);
   }
   else if (m_type == AI_TYPE_ENUM) // provide the dropdown
   {
      CValueArray dropdown;
      for (int i=0; i<1000; i++)
      {
         const char* enum_string = AiEnumGetString(m_enum, i);
         if (!enum_string)
            break;
         dropdown.Add(enum_string); dropdown.Add(enum_string);
      }
      item = in_layout.AddEnumControl(m_name, dropdown, label, siControlCombo);
      item.PutAttribute(siUILabelMinPixels, 110);
      item.PutAttribute(siUILabelPercentage, 35);
   }
   else
   {
      item = in_layout.AddItem(m_name, label);
      item.PutAttribute(siUILabelMinPixels, 110);
      item.PutAttribute(siUILabelPercentage, 35);
   }

   if (add_group)
      in_layout.EndGroup(); 
}


// Constructor for the CShaderDefShader class, from the node entry
//
// @param in_node_entry   The Arnold shader node entry
//
CShaderDefShader::CShaderDefShader(AtNodeEntry* in_node_entry, const bool in_clone_vector_map)
{
   m_node_entry = in_node_entry;

   if (in_clone_vector_map)
      m_name = AtString("vector_displacement");
   else
      m_name = AtString(AiNodeEntryGetName(m_node_entry));

   m_is_passthrough_closure = m_name == ATSTRING::closure;
   m_filename = AiNodeEntryGetFilename(m_node_entry);

   // extract the so/dll filename
   m_so_name = L"";
   if (!m_filename.IsEmpty())
   {
      // AiNodeEntryGetFilename mixes up / and \, so let's find the right-most slash, in whatever os
      CString wSlash = L"\\";
      CString lSlash = L"/";

      ULONG lastSlashPos;
      ULONG wLastSlashPos = m_filename.ReverseFindString(wSlash);
      ULONG lLastSlashPos = m_filename.ReverseFindString(lSlash);

      if (wLastSlashPos == ULONG_MAX)
         lastSlashPos = lLastSlashPos;
      else if (lLastSlashPos == ULONG_MAX)
         lastSlashPos = wLastSlashPos;
      else // both found, get the max
         lastSlashPos = wLastSlashPos > lLastSlashPos ? wLastSlashPos : lLastSlashPos;

      if (lastSlashPos != ULONG_MAX)
      {
         lastSlashPos++;
         m_so_name = m_filename.GetSubString(lastSlashPos);
      }
   }
   else // since we're storing the names by so/dll + " " + name, we can't let this void
      m_so_name = L"core";

   int entry_type = AiNodeEntryGetType(m_node_entry);
   m_is_camera_node = entry_type == AI_NODE_CAMERA;
   m_is_operator_node = entry_type == AI_NODE_OPERATOR;
   m_is_imager_node = CStringUtilities().StartsWith(m_name, L"imager_");

   if (in_clone_vector_map)
      m_type = AI_TYPE_FLOAT;
   else
      m_type = AiNodeEntryGetOutputType(m_node_entry);

   // camera nodes may have null output, let's turn them into RGB for allowing the connection to the camera
   if (m_is_camera_node && (m_type == AI_TYPE_NONE))
      m_type = AI_TYPE_RGB;

   m_has_desc     = MetaDataGetCStr(m_node_entry, NULL, ATSTRING::desc, m_desc);
   m_has_category = MetaDataGetCStr(m_node_entry, NULL, ATSTRING::soft_category, m_category);
   m_has_order    = MetaDataGetCStr(m_node_entry, NULL, ATSTRING::soft_order, m_order);
   m_has_skip     = AiMetaDataGetBool(m_node_entry, NULL, ATSTRING::soft_skip, &m_skip);

   m_has_deprecated = AiMetaDataGetBool(m_node_entry, NULL, ATSTRING::deprecated, &m_deprecated);

   // loop the parameters
   AtParamIterator* p_iter = AiNodeEntryGetParamIterator(m_node_entry);
   while (!AiParamIteratorFinished(p_iter))
   {
      const AtParamEntry *p_entry = AiParamIteratorGetNext(p_iter);
      if (AiParamGetName(p_entry) == ATSTRING::name)
         continue;

      if (m_is_camera_node) // skip all the camera-inherited parameters
      {
         AtString p_entry_name = AiParamGetName(p_entry);
         if (p_entry_name == ATSTRING::position    ||
             p_entry_name == ATSTRING::position  || 
             p_entry_name == ATSTRING::look_at   || 
             p_entry_name == ATSTRING::up        || 
             p_entry_name == ATSTRING::matrix    || 
             p_entry_name == ATSTRING::near_clip || 
             p_entry_name == ATSTRING::far_clip  || 
             p_entry_name == ATSTRING::shutter_start   || 
             p_entry_name == ATSTRING::shutter_end     || 
             p_entry_name == ATSTRING::shutter_type    || 
             p_entry_name == ATSTRING::shutter_curve   || 
             p_entry_name == ATSTRING::rolling_shutter || 
             p_entry_name == ATSTRING::rolling_shutter_duration || 
             p_entry_name == ATSTRING::filtermap         || 
             p_entry_name == ATSTRING::handedness        || 
             p_entry_name == ATSTRING::time_samples      || 
             p_entry_name == ATSTRING::screen_window_min || 
             p_entry_name == ATSTRING::screen_window_max || 
             p_entry_name == ATSTRING::exposure)
            continue;
      }

      CShaderDefParameter param(p_entry, m_node_entry);
      m_parameters.push_back(param);
   }

   AiParamIteratorDestroy(p_iter);
}


// Define this shader
//
// @return    the shader's progId or void if the shader is already defined
// 
CString CShaderDefShader::Define(const bool in_clone_vector_map)
{
   Factory factory = Application().GetFactory();

   CString shader_prog_id = L"Arnold." + m_name;
   shader_prog_id+= L".1.0";

   m_sd = Application().GetShaderDef(shader_prog_id);
   m_sd_created = !m_sd.IsValid();

   if (m_sd_created)
      m_sd = factory.CreateShaderDef(L"Arnold", m_name , 1, 0);

   if (m_is_passthrough_closure)
   {
      m_sd.AddShaderFamily(siShaderFamilySurfaceMat, true);
      // Allow the closer node to connect to the environment shader stack
      // github issue #36
      m_sd.AddShaderFamily(siShaderFamilyEnvironment, true);
      // This is the only way a closure can be connected to output shader stack
      // Support for 'Global AOV Shaders'... github issue #13
      m_sd.AddShaderFamily(siShaderFamilyOutput, true);
      m_sd.AddShaderFamily(siShaderFamilyVolume, true);
   }
   else
      m_sd.AddShaderFamily(m_is_camera_node ? siShaderFamilyLens : siShaderFamilyTexture, true);
   
   CString category = L"Arnold/Shaders";
   if (in_clone_vector_map) // let vector_displacement (clone of vector_map) be in a Displacement category
      category = category + L"/Displacement";
   else
   {
      if (m_has_deprecated && m_deprecated)
         category = category + L"/Deprecated";
      else if (m_has_category)
         category = category + L"/" + m_category;
   }

   if (m_is_operator_node)
   {
      category = L"Arnold/Operators";
      if (m_has_category)
         category = category + L"/" + m_category;
   }

   if (m_is_imager_node)
   {
      category = L"Arnold/Imagers";
      if (m_has_category)
         category = category + L"/" + m_category;
   }

   m_sd.PutCategory(category);

   if (m_has_deprecated && m_deprecated)
      m_sd.PutDisplayName(m_name + L" (deprecated)");

   m_sd.AddRendererDef(L"Arnold Render");

   if (m_name == L"set_parameter") // defined in js, just categorize and bail out
      return L"";

   vector <CShaderDefParameter>::iterator it;
   for (it=m_parameters.begin(); it!=m_parameters.end(); it++)
   {
      ShaderParamDefContainer inParamDef = m_sd.GetInputParamDefs();
      it->Define(inParamDef, m_name);
   }

   ShaderParamDefContainer outParamDef = m_sd.GetOutputParamDefs();
   ShaderParamDefOptions outOpts = ShaderParamDefOptions(factory.CreateShaderParamDefOptions());

   if (m_is_passthrough_closure) // hack the closure output for the closure connector to color
      outParamDef.AddParamDef("out", siShaderDataTypeColor4, outOpts);
   else if (m_is_operator_node)
      outParamDef.AddParamDef("out", siShaderDataTypeReference, outOpts);
   else if (m_is_imager_node)
      outParamDef.AddParamDef("out", siShaderDataTypeReference, outOpts);
   else
   {
      if (m_type == AI_TYPE_CLOSURE)
         outParamDef.AddParamDef("out", L"closure", outOpts);
      else
         outParamDef.AddParamDef("out", GetParamSdType(m_type), outOpts);
   }

   Layout();

   // return the progId only is the shader definition was built from scratch here
   return m_sd_created ? m_sd.GetProgID() : L"";
}

void CShaderDefShader::CheckOrderMetadata(const CStringArray &in_order_array)
{
   LONG order_array_count = in_order_array.GetCount();
   vector <CShaderDefParameter>::iterator it;

   CStringArray order_parameters;
   for (LONG i=0; i<order_array_count; i++)
   {
      CString p_s = in_order_array[i];
      if (p_s == L"BeginGroup")
      {
         i++; // skip the group name
         continue;
      }
      if (p_s == L"EndGroup")
         continue;

      order_parameters.Add(p_s);
   }

   LONG order_parameters_count = order_parameters.GetCount();
   if (order_parameters_count != (LONG)m_parameters.size())
   {
      GetMessageQueue()->LogMsg(L"[sitoa] parameters / order metadata mismatch for " + m_name);         
      return;
   }

   // check if the order metadata strings are part of the parameters names
   for (LONG i = 0; i < order_parameters_count; i++)
   {
      CString p_s = order_parameters[i];
      bool p_s_found = false;
      for (it = m_parameters.begin(); it != m_parameters.end(); it++)
      {
         if (it->m_name == p_s)
         {
            p_s_found = true;
            break;
         }
      }

      if (!p_s_found)
         GetMessageQueue()->LogMsg(L"[sitoa]" + p_s + L" order metadata not found in " + m_name + L"parameters"); 
   }

   // check if the parameters are part of the order metadata
   for (it = m_parameters.begin(); it != m_parameters.end(); it++)
   {
      bool p_s_found = false;
      for (LONG i = 0; i < order_parameters_count; i++)
      {
         CString p_s = order_parameters[i];
         if (it->m_name == p_s)
         {
            p_s_found = true;
            break;
         }
      }

      if (!p_s_found)
         GetMessageQueue()->LogMsg(L"[sitoa]" + it->m_name + L" not found in " + m_name + L"order metadata");         
   }

}


void CShaderDefShader::Layout()
{
   // build the layout
   PPGLayout layout = m_sd.GetPPGLayout();
   layout.Clear();

   vector <CShaderDefParameter>::iterator it;

   if (m_has_order)
   {
      CStringArray order_array = m_order.Split(L" ");
      
      // uncomment to check that soft.order is well compiled
      // CheckOrderMetadata(order_array);
      
      LONG order_array_count = order_array.GetCount();

      bool begin_group = false;
      bool add_tab = false;
      CString group_name;
      CString tab_name;
      for (LONG i = 0; i < order_array_count; i++)
      {
         CString p_s = order_array[i];
         if (add_tab)
         {
            tab_name = CStringUtilities().ReplaceString(L"_", L" ", p_s);
            layout.AddTab(tab_name);
            add_tab = false;
            continue;
         }
         if (begin_group)
         {
            group_name = CStringUtilities().ReplaceString(L"_", L" ", p_s);
            layout.AddGroup(group_name);
            begin_group = false;
            continue;
         }
         if (p_s == L"AddTab")
         {
            add_tab = true;
            continue;
         }
         if (p_s == L"BeginGroup")
         {
            begin_group = true;
            continue;
         }
         if (p_s == L"EndGroup")
         {
            layout.EndGroup();
            continue;
         }

         bool p_s_found = false;
         for (it = m_parameters.begin(); it != m_parameters.end(); it++)
         {
            if (it->m_name == p_s)
            {
               it->Layout(layout);
               p_s_found = true;
               break;
            }
         }
      }
   }
   else
      for (it = m_parameters.begin(); it != m_parameters.end(); it++)
         it->Layout(layout);
   
   if (m_sd_created)
   {
      // add some info lines
      layout.AddTab("Info");
      if (!m_filename.IsEmpty())
      {
         layout.AddStaticText(L"This shader is defined in " + m_so_name);
         layout.AddStaticText(L"Full path: " + m_filename);
         layout.AddStaticText(L"This UI was auto-defined by SItoA");
      }
      else
      {
         layout.AddStaticText(L"This shader is defined in the Arnold core");
         layout.AddStaticText(L"The UI was auto defined by SItoA");
      }

      layout.PutAttribute(siUIHelpFile, DLL_SHADERS_URL);
   }
   // unfortunately the following does not work (in the case the definition already exists)
   else // if there is no specific desc metadata, set the help to the shader page
      layout.PutAttribute(siUIHelpFile, m_has_desc ? m_desc : SITOA_SHADERS_URL);      
}


// Loads all the shaders defined in the so/dll shader search path and build the shader definition for them
void CShaderDefSet::Load(const CString &in_plugin_origin_path)
{
   // register a new "closure" parameter type
   CStringArray type_filter, family_filter;
   type_filter.Add(L"closure"); // only closure shaders can connect to closure ports
   Application().RegisterShaderCustomParameterType(L"closure", L"closure", L"closure", 128, 0, 255, type_filter, family_filter);

   CString progId;

   GetRenderInstance()->DestroyScene(false);

   AiBegin(GetSessionMode());
   // load the plugins (installation, + the ones in the shader search path)
   GetRenderInstance()->GetPluginsSearchPath().Put(in_plugin_origin_path, true);
   GetRenderInstance()->GetPluginsSearchPath().LoadPlugins();
   // load the shader metadata file
   CString metadata_path = CUtils::BuildPath(in_plugin_origin_path, L"arnold_shaders.mtd");
   bool metadata_exists = AiMetaDataLoadFile(metadata_path.GetAsciiString());
   if (!metadata_exists)
      GetMessageQueue()->LogMsg(L"[sitoa] Missing shader metadata file " + metadata_path, siWarningMsg);
   // load the operator metadata file
   metadata_path = CUtils::BuildPath(in_plugin_origin_path, L"arnold_operators.mtd");
   metadata_exists = AiMetaDataLoadFile(metadata_path.GetAsciiString());
   if (!metadata_exists)
      GetMessageQueue()->LogMsg(L"[sitoa] Missing operator metadata file " + metadata_path, siWarningMsg);
   // load the imager metadata file
   metadata_path = CUtils::BuildPath(in_plugin_origin_path, L"arnold_imagers.mtd");
   metadata_exists = AiMetaDataLoadFile(metadata_path.GetAsciiString());
   if (!metadata_exists)
      GetMessageQueue()->LogMsg(L"[sitoa] Missing imager metadata file " + metadata_path, siWarningMsg);

   // iterate the nodes
   AtNodeEntryIterator* node_entry_it = AiUniverseGetNodeEntryIterator(AI_NODE_SHADER | AI_NODE_CAMERA | AI_NODE_OPERATOR);
   while (!AiNodeEntryIteratorFinished(node_entry_it))
   {
      AtNodeEntry* node_entry = AiNodeEntryIteratorGetNext(node_entry_it);
      AtString node_entry_name(AiNodeEntryGetName(node_entry));
      CString node_name(node_entry_name.c_str());

      progId = L"ArnoldLightShaders." + node_name + L".1.0"; // is this a light filter, whose UI is alread defined in ArnoldLightShaderDef.js ?
      ShaderDef sd = Application().GetShaderDef(progId);
      if (sd.IsValid())
      {
         // set the category. From .js, setting subcategories doesn't seem to work
         sd.PutCategory(L"Arnold/Light Filters");   
         continue;
      }

      CShaderDefShader shader_def(node_entry); // collect everything
      if (shader_def.m_has_skip && shader_def.m_skip)
         continue;

      // skip the shaders shipping in sitoa_shaders, that implement the factory Softimage shaders 
      if (shader_def.m_so_name == L"sitoa_shaders.dll" || shader_def.m_so_name == L"sitoa_shaders.so")
         if (!shader_def.m_is_passthrough_closure) // only exception is the closure connector
            continue;
      // skip the core camera nodes, already exposed by the camera options property
      if (shader_def.m_so_name == L"core" && shader_def.m_is_camera_node)
          continue;

      // xsibatch needs to completely skip the shaders defined in ArnoldShaderDef.js
      // there's no need to categorize them when in batch anyway
      // https://github.com/Autodesk/sitoa/issues/77
      if (!Application().IsInteractive())
      {
         if (node_name == L"set_parameter")
            continue;
      }

      progId = shader_def.Define(); // build parameters and the UI

      if (!progId.IsEmpty()) // enter in the list only the shaders whose definition was actually created
         m_prog_ids.insert(shader_def.m_so_name + L" " + progId);

      // duplicate vector_map to vector_displacement (with float output)
      if (node_entry_name == ATSTRING::vector_map) 
      {
         CShaderDefShader shader_def(node_entry, true);
         progId = shader_def.Define(true);
      }
   }

   AiNodeEntryIteratorDestroy(node_entry_it);

   // imagers are of type AI_NODE_DRIVER so we need to check the name of the driver to see if its an imager
   node_entry_it = AiUniverseGetNodeEntryIterator(AI_NODE_DRIVER);
   while (!AiNodeEntryIteratorFinished(node_entry_it))
   {
      AtNodeEntry* node_entry = AiNodeEntryIteratorGetNext(node_entry_it);
      AtString node_entry_name(AiNodeEntryGetName(node_entry));
      CString node_name(node_entry_name.c_str());

      if (!CStringUtilities().StartsWith(node_name, L"imager_"))
         continue;

      CShaderDefShader shader_def(node_entry); // collect everything
      progId = shader_def.Define(); // build parameters and the UI

      if (!progId.IsEmpty()) // enter in the list only the shaders whose definition was actually created
         m_prog_ids.insert(shader_def.m_so_name + L" " + progId);
   }
   AiNodeEntryIteratorDestroy(node_entry_it);

   node_entry_it = AiUniverseGetNodeEntryIterator(AI_NODE_LIGHT);
   while (!AiNodeEntryIteratorFinished(node_entry_it))
   {
      AtNodeEntry* node_entry = AiNodeEntryIteratorGetNext(node_entry_it);
      CString nodeName(AiNodeEntryGetName(node_entry));

      progId = L"ArnoldLightShaders.arnold_" + nodeName + L".1.0"; // is this a light , whose UI is alread defined in ArnoldLightShaderDef.js ?
      ShaderDef sd = Application().GetShaderDef(progId);
      if (sd.IsValid()) // set the category. From .js, setting subcategories doesn't seem to work
      {
         sd.PutCategory(L"Arnold/Lights");
         sd.PutDisplayName(nodeName);
      }
   }

   AiNodeEntryIteratorDestroy(node_entry_it);
   AiEnd();
}


// Return the array of the progIds of all the defined shaders
//
// @return    the array, one id per item, plus a "separator" string separating different so/dll files
//
CStringArray CShaderDefSet::GetProgIds()
{
   set < CString >::iterator it;
   CString name, so, previousSo;
   CStringArray result;

   // we iterate the set, so the shader names are sorted by so/dll and then by name
   for (it=m_prog_ids.begin(); it!=m_prog_ids.end(); it++)
   {
      // remove the so/dll for the returned array
      // for example, "mtoa_shaders.dll something" -> "something"
      CStringArray splitS = it->Split(L" ");
      so = splitS[0];
      name = splitS[1];

      if (it!=m_prog_ids.begin())
      {
         // if this shader belongs to a different so/dll than the previous one, 
         // add a "separator" string instead of the shader name 
         if (so != previousSo)
            result.Add(L"separator");
      }

      name = name.GetSubString(7); // remove the initial "Arnold."
      name = name.GetSubString(0, name.Length() - 4); // remove ".1.0"

      result.Add(name); // adding the shader name
      previousSo = so;
   }

   return result;
}


// Remove all the shader definitions and clear the progId set
//
void CShaderDefSet::Clear()
{
   Factory factory = Application().GetFactory();

   set < CString >::iterator it;

   // shader names are sorted by so/dll and then by name
   for (it=m_prog_ids.begin(); it!=m_prog_ids.end(); it++)
   {
      CStringArray splitS = it->Split(L" ");
      CString progId = splitS[1];

      CRef sd = Application().GetShaderDef(progId);
      if (sd.IsValid())
      {
         factory.RemoveShaderDef(sd);
         // GetMessageQueue()->LogMessage(L"Removing shaderdef = " + progId);
      }
   }

   m_prog_ids.clear();
}


