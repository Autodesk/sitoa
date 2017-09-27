/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

using namespace XSI;

// Update Light for IPR
void UpdateLight(const Light &in_xsiLight, double in_frame);
// Update Light Filter for IPR
void UpdateLightFilters(const Light &in_xsiLight, const Shader &in_lightShader, AtNode *in_lightNode, double in_frame);
// Update Light Primitive (Inclusion, area, etc)
void UpdateLightGroup(const Light &in_xsiLight, double in_frame);

