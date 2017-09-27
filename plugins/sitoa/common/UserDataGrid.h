/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_string.h>
#include <xsi_griddata.h>

#include <ai_nodes.h>

using namespace XSI;

// Parse an int value in the input string
void ParseAttributeString(CString in_s, int *out_value);
// Parse a bool value in the input string
void ParseAttributeString(CString in_s, bool *out_value);
// Parse up to 4 float value in the input string
void ParseAttributeString(CString in_s, float *out_value_0, float *out_value_1=NULL, float *out_value_2=NULL, float *out_value_3=NULL);
// Parse a matrix value in the input string
void ParseAttributeString(CString in_s, AtMatrix &out_value);
// Set a node attribute data, getting the data from an input string
void SetUserDataOnNode(AtNode *in_node, CString in_name, int in_paramType, CPathString in_s);
// Set a node array attribute data, getting the data from an input string
void SetArrayUserDataOnNode(AtNode *in_node, CString in_name, int in_paramType, CPathString in_s);
// Exports the data grid as user data
void ExportUserDataGrid(AtNode *in_node, GridData &in_grid, bool in_resolveTokens, double in_frame);

