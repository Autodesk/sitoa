/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_x3dobject.h>
#include <xsi_customproperty.h>

#include <ai_nodes.h>

using namespace XSI;

// Update the matrix of the nodes originated by a Softimage object or model
void UpdateShapeMatrix(X3DObject &in_xsiObj, double in_frame);
// Update the matrix of a node originated by a Softimage object or model
void UpdateNodeMatrix(AtNode *in_node, const X3DObject &in_xsiObj, double in_frame);
// Update the objects that depend on an Arnold Parameters property
void UpdateParameters(const CustomProperty &in_cp, double in_frame);
// Update an object that depends on an Arnold Parameters property
void UpdateObjectParameters(const X3DObject &in_xsiObj, const CustomProperty &in_cp, double in_frame);
// Update the visibility of an object, group or partition
void UpdateVisibility(const CRef &in_ref, double in_frame);
// Update an object or model visibility
void UpdateObjectVisibility(const X3DObject &in_xsiObj, ULONG in_objId, bool in_checkHideMasterFlag, double in_frame, bool in_visibilityOnGroup, uint8_t in_groupVisibility);
// Update the sidedness of an object, group or partition
void UpdateSidedness(const CRef &in_ref, double in_frame);
// Update an object or model sidedness
void UpdateObjectSidedness(const X3DObject &in_xsiObj, ULONG in_objId, uint8_t in_sidedness, double in_frame, bool in_sidednessOnGroup, uint8_t in_groupSidedness);
// Update the matte of an object, group or partition
void UpdateMatte(const CustomProperty &in_cp, double in_frame);
// Update the matte of an object
void UpdateObjectMatte(const X3DObject &in_xsiObj, const CustomProperty &in_cp, double in_frame);
// Update the visible objects when in an isolate selection region session
void UpdateIsolateSelection(const CRefArray &in_visibleObjects, double in_frame);

