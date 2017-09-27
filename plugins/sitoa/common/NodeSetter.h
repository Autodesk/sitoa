/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <ai_nodes.h>

class CNodeSetter
{
private:
   // Get the default value of an enum parameter
   static const char* GetEnumDefault(AtNode *in_node, const char* in_name);
   // Get the default value of a parameter. Also retuned is the parameter type and if it is an array
   static bool GetDefault(AtNode *in_node, const char* in_name, const AtParamValue **out_paramDefault, int *out_type);
   // Set the value of a single parameter or user parameter. 
   static bool SetCommon(AtNode *in_node, const char* in_name, const int in_type, void *in_value, bool in_forceSet);
public:
   static bool SetBoolean(AtNode *in_node, const char* in_name, bool  in_value,         bool in_forceSet=false);
   static bool SetByte   (AtNode *in_node, const char* in_name, uint8_t in_value,       bool in_forceSet=false);
   static bool SetFloat  (AtNode *in_node, const char* in_name, float in_value,         bool in_forceSet=false);
   static bool SetInt    (AtNode *in_node, const char* in_name, int   in_value,         bool in_forceSet=false);
   static bool SetMatrix (AtNode *in_node, const char* in_name, AtMatrix in_value,      bool in_forceSet=false);
   static bool SetPointer(AtNode *in_node, const char* in_name, AtNode* in_value,       bool in_forceSet=false);
   static bool SetString (AtNode *in_node, const char* in_name, const char* in_value,   bool in_forceSet=false);
   static bool SetUInt   (AtNode *in_node, const char* in_name, unsigned int in_value,  bool in_forceSet=false);
   static bool SetRGB    (AtNode *in_node, const char* in_name, float in_valueR, float in_valueG, float in_valueB, bool in_forceSet=false);
   static bool SetRGBA   (AtNode *in_node, const char* in_name, float in_valueR, float in_valueG, float in_valueB, float in_valueA, bool in_forceSet=false);
   static bool SetVector (AtNode *in_node, const char* in_name, float in_valueX, float in_valueY, float in_valueZ, bool in_forceSet=false);
   static bool SetVector2(AtNode *in_node, const char* in_name, float in_valueX, float in_valueY, bool in_forceSet=false);
};


