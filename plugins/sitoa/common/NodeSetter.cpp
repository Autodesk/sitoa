/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////
// Wrapper for the AiNodeSet* set, except AiNodeSetArray.
////////////////////////////////////////////////////////////////////////////////////////

#include "common/NodeSetter.h"
#include "renderer/Renderer.h"
#include <string.h>

// Get the default value of an enum parameter
//
// @param in_node     the node the parameter belongs to
// @param in_name     the parameter name
//
// @return the enum defauls as a char*
//
const char* CNodeSetter::GetEnumDefault(AtNode *in_node, const char* in_name)
{
   const AtNodeEntry* nodeEntry = AiNodeGetNodeEntry(in_node);
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(nodeEntry, in_name);
   const AtParamValue* paramDefault = AiParamGetDefault(paramEntry);
   AtEnum paramEnum = AiParamGetEnum(paramEntry);
   return AiEnumGetString(paramEnum, paramDefault->INT());
}


// Get the default value of a parameter. Also retuned is the parameter type and if it is an array
//
// @param in_node           the node the parameter belongs to
// @param in_name           the parameter name
// @param out_paramDefault  the returned param default union
// @param out_type          the returned parameter type
//
// @return true if the node and the parameter are valid, false in the other cases and also false if the paremeter is an array
//
bool CNodeSetter::GetDefault(AtNode *in_node, const char* in_name, const AtParamValue **out_paramDefault, int *out_type)
{
   if (!in_node)
      return false; // null node

   const AtNodeEntry* nodeEntry = AiNodeGetNodeEntry(in_node);
   if (!nodeEntry)
      return false; // should never happen

   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(nodeEntry, in_name);
   if (!paramEntry)
      return false; // the parameter does not exist

   *out_type = AiParamGetType(paramEntry);

   if (*out_type == AI_TYPE_ARRAY) // return false if this is an array
      return false;

   *out_paramDefault = AiParamGetDefault(paramEntry);

   if (*out_type == AI_TYPE_NODE)
      *out_type = AI_TYPE_POINTER; // there is no a AiNodeSetNode, only a AiNodeSetPtr

   return true;
}


// Set the value of a single parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_type            the parameter type
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetCommon(AtNode *in_node, const char* in_name, const int in_type, void *in_value, bool in_forceSet)
{
   if (!in_node)
      return false;
   // protect against null names
   if (!in_name || !strcmp(in_name, ""))
      return false;

   bool forceSet = in_forceSet;
 
   const AtParamValue* paramDefault(NULL);
   int type(AI_TYPE_NONE);

   if (!forceSet)
   {
      if (!GetDefault(in_node, in_name, &paramDefault, &type))
      {
         if (type == AI_TYPE_ARRAY) // I don't allow the setters call to set single-valued arrays, always use AiNodeSetArray
         {
            return false;
         }
         // maybe this is a user-defined parameter ?
         if (AiNodeLookUpUserParameter(in_node, in_name))
            forceSet = true;
         else
         {
            CString entry_name = CNodeUtilities().GetEntryName(in_node);
            GetMessageQueue()->LogMsg(L"[sitoa] Skipping invalid parameter " + entry_name + L"." + in_name, XSI::siWarningMsg); 
            return false;
         }
      }
      else // default well collected
      {
          if (!(type == AI_TYPE_ENUM && in_type == AI_TYPE_STRING)) // this is the only mismatch allowed
          if (type != in_type)
          {
            return false;
          }
      }
   }

   if (!forceSet)
   {
      // check if the value is equal to the default. If so, bail out
      switch (type)
      {
         case AI_TYPE_BOOLEAN:
            forceSet = paramDefault->BOOL() != *(bool*)in_value;
            break;
         case AI_TYPE_BYTE:
            forceSet = paramDefault->BYTE() != *(uint8_t*)in_value;
            break;
         case AI_TYPE_FLOAT:
            forceSet = paramDefault->FLT() != *(float*)in_value;
            break;
         case AI_TYPE_INT:
            forceSet = paramDefault->INT() != *(int*)in_value;
            break;
         case AI_TYPE_MATRIX:
            forceSet = memcmp(paramDefault->pMTX(), in_value, sizeof(AtMatrix)) != 0;
            break;
         case AI_TYPE_NODE:
         case AI_TYPE_POINTER:
            forceSet = paramDefault->PTR() != (AtNode*)in_value;
            break;
         case AI_TYPE_ENUM:
            forceSet = strcmp(GetEnumDefault(in_node, in_name), (const char*)in_value) != 0;
            break;
         case AI_TYPE_STRING:
            forceSet = strcmp(paramDefault->STR(), (const char*)in_value) != 0;
            break;
         case AI_TYPE_UINT:
            forceSet = paramDefault->UINT() != *(unsigned int*)in_value;
            break;
         // struct
         case AI_TYPE_RGB:
            forceSet = paramDefault->RGB() != *(AtRGB*)in_value;
            break;
         case AI_TYPE_RGBA:
            forceSet = paramDefault->RGBA() != *(AtRGBA*)in_value;
            break;
         case AI_TYPE_VECTOR:
            forceSet = paramDefault->VEC() != *(AtVector*)in_value;
            break;
         case AI_TYPE_VECTOR2:
            forceSet = paramDefault->VEC2() != *(AtVector2*)in_value;
            break;
         default:
            break;
      }
   }

   if (!forceSet)
   {
      // Check if the value differs from the current value. If so, force the setting on.
      // During an IPR session, the user could restore a parameter to its default, in which
      // case, without this extra check, we would skip setting the value
      switch (type)
      {
         case AI_TYPE_BOOLEAN:
            forceSet = AiNodeGetBool(in_node, in_name) != *(bool*)in_value;
            break;
         case AI_TYPE_BYTE:
            forceSet = AiNodeGetByte(in_node, in_name) != *(uint8_t*)in_value;
            break;
         case AI_TYPE_FLOAT:
            forceSet = AiNodeGetFlt(in_node, in_name) != *(float*)in_value;
            break;
         case AI_TYPE_INT:
            forceSet = AiNodeGetInt(in_node, in_name) != *(int*)in_value;
            break;
         case AI_TYPE_MATRIX:
         {
            AtMatrix m = AiNodeGetMatrix(in_node, in_name);
            forceSet = memcmp((void*)&m, in_value, sizeof(AtMatrix)) != 0;
            break;
         }
         case AI_TYPE_NODE:
         case AI_TYPE_POINTER:
            forceSet = AiNodeGetPtr(in_node, in_name) != in_value;
            break;
         case AI_TYPE_ENUM:
         case AI_TYPE_STRING:
            forceSet = strcmp(AiNodeGetStr(in_node, in_name), (const char*)in_value) != 0;
            break;
         case AI_TYPE_UINT:
            forceSet = AiNodeGetUInt(in_node, in_name) != *(unsigned int*)in_value;
            break;
         // struct
         case AI_TYPE_RGB:
            forceSet = AiNodeGetRGB(in_node, in_name) != *(AtRGB*)in_value;
            break;
         case AI_TYPE_RGBA:
            forceSet = AiNodeGetRGBA(in_node, in_name) != *(AtRGBA*)in_value;
            break;
         case AI_TYPE_VECTOR:
            forceSet = AiNodeGetVec(in_node, in_name) != *(AtVector*)in_value;
            break;
         case AI_TYPE_VECTOR2:
            forceSet = AiNodeGetVec2(in_node, in_name) != *(AtVector2*)in_value;
            break;
         default:
            break;
      }
   }

   if (!forceSet)
      return true;

   // go set
   switch (in_type)
   {
      case AI_TYPE_BOOLEAN:
         AiNodeSetBool(in_node, in_name, *(bool*)in_value);
         return true;
      case AI_TYPE_BYTE:
         AiNodeSetByte(in_node, in_name, *(uint8_t*)in_value);
         return true;
      case AI_TYPE_FLOAT:
         AiNodeSetFlt(in_node, in_name, *(float*)in_value);
         return true;
      case AI_TYPE_INT:
         AiNodeSetInt(in_node, in_name, *(int*)in_value);
         return true;
      case AI_TYPE_MATRIX:
         AiNodeSetMatrix(in_node, in_name, *(AtMatrix*)in_value);
         return true;
      case AI_TYPE_NODE:
      case AI_TYPE_POINTER:
         AiNodeSetPtr(in_node, in_name, (AtNode*)in_value);
         return true;
      case AI_TYPE_ENUM:
      case AI_TYPE_STRING:
         AiNodeSetStr(in_node, in_name, (const char*)in_value);
         return true;
      case AI_TYPE_UINT:
         AiNodeSetUInt(in_node, in_name, *(unsigned int*)in_value);
         return true;
      // struct
      case AI_TYPE_RGB:
         AiNodeSetRGB(in_node, in_name, ((AtRGB*)in_value)->r, ((AtRGB*)in_value)->g, ((AtRGB*)in_value)->b);
         return true;
      case AI_TYPE_RGBA:
         AiNodeSetRGBA(in_node, in_name, ((AtRGBA*)in_value)->r, ((AtRGBA*)in_value)->g, ((AtRGBA*)in_value)->b, ((AtRGBA*)in_value)->a);
         return true;
      case AI_TYPE_VECTOR:
         AiNodeSetVec(in_node, in_name, ((AtVector*)in_value)->x, ((AtVector*)in_value)->y, ((AtVector*)in_value)->z);
         return true;
      case AI_TYPE_VECTOR2:
         AiNodeSetVec2(in_node, in_name, ((AtVector2*)in_value)->x, ((AtVector2*)in_value)->y);
         return true;
      default:
         return false;
   }
}


// Set the value of a single boolean parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetBoolean(AtNode *in_node, const char* in_name, bool in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_BOOLEAN, (void*)&in_value, in_forceSet);
}


// Set the value of a single byte parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetByte(AtNode *in_node, const char* in_name, uint8_t in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_BYTE, (void*)&in_value, in_forceSet);
}


// Set the value of a single float parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetFloat(AtNode *in_node, const char* in_name, float in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_FLOAT, (void*)&in_value, in_forceSet);
}


// Set the value of a single int parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetInt(AtNode *in_node, const char* in_name, int in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_INT, (void*)&in_value, in_forceSet);
}


// Set the value of a single matrix parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetMatrix(AtNode *in_node, const char* in_name, AtMatrix in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_MATRIX, (void*)&in_value, in_forceSet);
}


// Set the value of a single pointer or node parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetPointer(AtNode *in_node, const char* in_name, AtNode *in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_POINTER, (void*)in_value, in_forceSet);
}


// Set the value of a single string or enum parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetString(AtNode *in_node, const char* in_name, const char* in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_STRING, (void*)in_value, in_forceSet);
}


// Set the value of a single unsigned int parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetUInt(AtNode *in_node, const char* in_name, unsigned int in_value, bool in_forceSet)
{
   return SetCommon(in_node, in_name, AI_TYPE_UINT, (void*)&in_value, in_forceSet);
}


// Set the value of a single RGB parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetRGB(AtNode *in_node, const char* in_name, float in_valueR, float in_valueG, float in_valueB, bool in_forceSet)
{
   AtRGB rgb = AtRGB(in_valueR, in_valueG, in_valueB);
   return SetCommon(in_node, in_name, AI_TYPE_RGB, (void*)&rgb, in_forceSet);
}


// Set the value of a single RGBA parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetRGBA(AtNode *in_node, const char* in_name, float in_valueR, float in_valueG, float in_valueB, float in_valueA, bool in_forceSet)
{
   AtRGBA rgba = AtRGBA(in_valueR, in_valueG, in_valueB, in_valueA);
   return SetCommon(in_node, in_name, AI_TYPE_RGBA, (void*)&rgba, in_forceSet);
}


// Set the value of a single vector parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetVector(AtNode *in_node, const char* in_name, float in_valueX, float in_valueY, float in_valueZ, bool in_forceSet)
{
   AtVector v = AtVector(in_valueX, in_valueY, in_valueZ);
   return SetCommon(in_node, in_name, AI_TYPE_VECTOR, (void*)&v, in_forceSet);
}


// Set the value of a single Vector2 parameter or user parameter. 
// Skip the setting if the input value is equal to the default value of the parameter.
// This behavior can be overriddded by the in_forceSet flag.
//
// @param in_node            the node the parameter belongs to
// @param in_name            the parameter name
// @param in_value           the value to set as a void*
// @param in_forceSet        if true, write the value regardless of the parameter default value. Also allows writing into single-valued arrays
//
// @return true if the value was well set or let in place, else false
//
bool CNodeSetter::SetVector2(AtNode *in_node, const char* in_name, float in_valueX, float in_valueY, bool in_forceSet)
{
   AtVector2 p = AtVector2(in_valueX, in_valueY);
   return SetCommon(in_node, in_name, AI_TYPE_VECTOR2, (void*)&p, in_forceSet);
}
