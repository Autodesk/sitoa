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
#include <xsi_parameter.h>
#include <xsi_property.h>
#include <xsi_hairprimitive.h>

#include <ai_nodes.h>

#include <cstdio>

using namespace XSI;

#define CHUNK_SIZE 300000

// Load all hair primitives into Arnold
CStatus LoadHairs(double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly = false);
// Get the instance group of a hair primitive.
bool GetInstanceGroupName(const HairPrimitive& in_primitive, SIObject &out_group);
// Load a hair primitives into Arnold
CStatus LoadSingleHair(const X3DObject &in_xsiObj, double in_frame);
// Get the map connected to a hair object, driving some instancing distribution, like fuzziness
CString GetConnectedMapName(const X3DObject &in_xsiObj, CString in_connectionName);
// Get the mesh objects under a given model, used for hair instancing
CRefArray GetMeshesAndHairBelowMaster(const X3DObject &in_xsiObj);
// Create objects bended around the strands a hair object
CStatus LoadSingleHairInstance(const X3DObject &in_xsiObj, SIObject in_group, double in_frame);

