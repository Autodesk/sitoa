/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_customproperty.h>
#include <xsi_x3dobject.h>

#include <ai_color.h>
#include <ai_params.h>

using namespace XSI;

// Returns the rays visibility
uint8_t GetVisibility(const CRefArray &in_polyProperties, double in_frame, bool in_checkHideMasterFlag=true);
// Returns the rays visibility of a softimage object
uint8_t GetVisibilityFromObject(const X3DObject in_obj, double in_frame, const bool in_checkHideMasterFlag=true);
// Returns the rays visibility of an xsi object by its id
uint8_t GetVisibilityFromObjectId(const int in_id, double in_frame, const bool in_checkHideMasterFlag=true);
// Evaluates the Arnold Sidedness property and compute the sidedness bitfield. 
bool GetSidedness(const CRefArray &in_polyProperties, double in_frame, uint8_t &out_result);
// Load the Arnold Parameters property for an Arnold node
void LoadArnoldParameters(AtNode* in_node, CParameterRefArray &in_paramsArray, double in_frame, bool in_filterParameters = false);
// Evaluate the Arnold Matte property
void LoadMatte(AtNode *in_node, const Property &in_property, double in_frame);
// Load the user options
bool LoadUserOptions(AtNode* in_node, const Property &in_property, double in_frame);
// Load the camera options property
void LoadCameraOptions(const Camera &in_xsiCamera, AtNode* in_node, const Property &in_property, double in_frame);
// Collect the user data blob properties
CRefArray CollectUserDataBlobProperties(const X3DObject &in_xsiObj, double in_frame);
// Export the user data blob properties
void ExportUserDataBlobProperties(AtNode* in_node, const CRefArray &in_blobProperties, double in_frame);
// Load the user data blobs
void LoadUserDataBlobs(AtNode* in_node, const X3DObject &in_xsiObj, double in_frame);

