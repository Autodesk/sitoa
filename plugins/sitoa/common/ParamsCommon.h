/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "loader/PathTranslator.h"

#include <xsi_customproperty.h>
#include <xsi_parameter.h>
#include <xsi_status.h>

#include <ai_nodes.h>

// Loads correctly the param value to Arnold (depends of Parameter Type)
CStatus LoadParameterValue(AtNode *in_node, const CString &in_entryName, const CString &in_paramName, const Parameter &in_param, double in_frame, int in_arrayElement, CRef in_ref);
// Get Arnold Param Type
int GetArnoldParameterType(AtNode *in_node, const char* in_paramName, bool in_checkInsideArrayParameter=false);
// Assigns to outParameter the parameter to evaluate through an expression. Returns the number of indexs ocuppied by 
// Expression parameters for compoundParameters
unsigned int GetEvaluatedExprParameter(const Parameter &in_parameter, Parameter &out_parameter);
// Converts an fcurve to a float array
AtArray* GetFCurveArray(const FCurve &in_fc, int in_nbKeys=100);
// Sample a fcurve into a float array. The array is made of pairs of time,value.
AtArray* GetFCurveRawArray(const FCurve &in_fc, const int in_nbKeys);
// Load the user options into an Arnold node
bool LoadUserOptions(AtNode* in_node, const Property &in_property, double in_frame);

