/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_status.h>
#include <xsi_ref.h>
#include <xsi_property.h>

using namespace XSI;

CStatus LoadScene(const Property &in_arnoldOptions, const CString& in_renderType, double in_frameIni, double in_frameEnd, LONG in_frameStep, 
                  bool in_createStandIn, bool in_useProgressBar, CString in_filename = L"", bool in_selectionOnly = false, CRefArray in_objects = CRefArray(), bool in_recurse = true);

void AbortFrameLoadScene();
// postload a single object.
CStatus PostLoadSingleObject(const CRef in_ref, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly);
//void BypassClosurePassthroughForAss();

