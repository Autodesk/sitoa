/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_camera.h>
#include <xsi_pass.h>
#include <xsi_property.h>
#include <xsi_shader.h>
#include <xsi_status.h>

using namespace XSI;

// Load all the Softimage cameras
CStatus LoadCameras(double in_frame);
// Load a single Softimage camera
CStatus LoadSingleCamera(const Camera &in_xsiCamera, double in_frame);
// Collect and return the lens shaders connected to a given pass 
// CRefArray CollectPassLensShaders(const Pass &in_pass);
// Get the Arnold camera type for this Softimage camera
CString GetCameraType(const Camera &in_xsiCamera, const Property &in_property, double in_frame);
// Return the first valid lens shader belonging to the Softimage camera 
bool GetFirstLensShader(const Camera &in_xsiCamera, Shader &out_shader);
// Check if a shader is a lens shader
bool IsLensShader(const Shader &in_shader);

