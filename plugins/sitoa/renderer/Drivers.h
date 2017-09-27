/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_string.h>

#include <ai_nodes.h>

using namespace XSI;

// Return the Arnold driver name for an image format
CString GetDriverName(const CString &in_format);
// Return the Arnold layer name from in_datatype
CString GetLayerName(const CString &in_datatype);
// Return the bit depth as a CString
CString GetDriverBitDepth(unsigned int in_bitDepth);
// Return the dataType of a given siRenderChannelType
CString GetDriverLayerChannelType(LONG in_renderChannelType);
// Return the dataType of a given LayerName
CString GetDriverLayerDataTypeByName(const CString &in_layerName);
// Exports the exr metadata
void ExportExrMetadata(AtNode *in_node);

