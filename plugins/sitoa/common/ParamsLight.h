/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "common/Tools.h"

using namespace XSI;

// Transform a spot light matrix to the one to be set if the light type is photometric_light
CMatrix4 TransformToPhotometricLight(const CMatrix4 &in_lightMatrix);
// Load a light's parameters
CStatus LoadLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, const Shader &in_xsiShader, bool in_isMaster, double in_frame, bool in_ipr);
// Load the parameters for a quad light
CStatus LoadQuadLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame, CDoubleArray in_keyFramesTransform);
// Load the parameters for a cylinder light
CStatus LoadCylinderLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame, CDoubleArray in_keyFramesTransform);
// Load the parameters for a disk light
CStatus LoadDiskLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame);
// Assign the mesh attribute of an Arnold's mesh_light
bool LoadMeshLightParameters(AtNode* in_lightNode, const Light &in_xsiLight, double in_frame);
// Load the matrix for light_blocker light filter
CStatus LoadBlockerFilterMatrix(AtNode* in_filterNode, const Shader &in_filterShader, double in_frame);
// Load the offset for gobo light filter
CStatus LoadGoboFilterOffsetAndRotate(AtNode* in_filterNode, const Shader &in_filterShader, const Light &in_xsiLight, double in_frame);

