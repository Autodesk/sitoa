/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "loader/PathTranslator.h"

#include <xsi_vector3f.h>
#include <xsi_x3dobject.h>

#include <ai_nodes.h>

using namespace XSI;
using namespace XSI::MATH;

// Get the bbox from a .asstoc file
bool GetBoundingBoxFromScnToc(const CPathString &in_asstocFilename, CVector3f &out_min, CVector3f &out_max);
// Get the bbox from a Softimage object
void GetBoundingBoxFromObject(const X3DObject &in_xsiObj, const double in_frame, float in_scale, CVector3f &out_min, CVector3f &out_max);
// Load a procedural
CStatus LoadSingleProcedural(const X3DObject &in_xsiObj, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly = false);
// Helge's patch for #1359. Exports shaders, displacement and displacement settings of the procedural object
void ExportAlembicProceduralData(AtNode *in_procNode, const X3DObject &in_xsiObj, CustomProperty &in_arnoldParameters, CRefArray in_proceduralProperties, double in_frame);
// return whether the input string does NOT start with "procedural_material" or "scene_material" (case insensitive)
bool UseProceduralMaterial(const CString &in_materialName);

