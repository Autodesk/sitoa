/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_parameter.h>
#include <xsi_shader.h>
#include <xsi_status.h>

#include <ai_nodes.h>

using namespace XSI;

// Load the source of a parameter
CRef GetParameterSource(const Parameter& in_param);
// Load the parameters of a certain shader
CStatus LoadShaderParameters(AtNode* in_node, CRefArray &in_paramsArray, double in_frame, const CRef &in_ref, bool in_recursively);
// Load a shader parameter
CStatus LoadShaderParameter(AtNode* in_node, const CString &in_entryName, Parameter &in_param, double in_frame, const CRef &in_ref, bool in_recursively, const CString& in_arrayParamName=CString(), int in_arrayElement=-1);
// Load the n-th element of the array parameters of the array switcher shaders.
CStatus LoadArraySwitcherParameter(AtNode *in_node, const Parameter &in_param, double in_frame, int in_arrayElement, CRef in_ref);
// Get the shader from a given source
Shader GetShaderFromSource(const CRef &in_refCnxSrc);
// Get the shader from a parameter
Shader GetConnectedShader(const Parameter& in_param);

