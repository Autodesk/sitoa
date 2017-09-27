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

#include <xsi_camera.h> 
#include <xsi_constraint.h>
#include <xsi_shader.h>
#include <xsi_status.h>

using namespace XSI;

enum eCameraFov
{
   eCameraFov_Vertical,
   eCameraFov_Horizontal
};

// Load all the camera parameters that depends of the camera type.
CStatus LoadCameraParameters(AtNode* in_cameraNode, const Camera &in_xsiCamera, const CString &in_cameraType, double in_frame);
// Get the horizontal fov of a camera
float GetCameraHorizontalFov(const Camera& in_xsiCamera, double in_frame);

