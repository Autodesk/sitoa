/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/Drivers.h"
#include "renderer/Renderer.h"


// Return the Arnold driver name for an image format
//
// @param in_format     the image format
//
// @return the driver name
//
CString GetDriverName(const CString &in_format)
{
   if (in_format.IsEqualNoCase(L"tif"))
      return L"driver_tiff";   
   if (in_format.IsEqualNoCase(L"jpg"))
      return L"driver_jpeg";
   if (in_format.IsEqualNoCase(L"png"))
      return L"driver_png";
   if (in_format.IsEqualNoCase(L"exr"))
   {
      if (GetRenderOptions()->m_deep_exr_enable)
         return L"driver_deepexr";
      return L"driver_exr";
   }
   return L"";
}


CString GetLayerName(const CString &in_datatype)
{
   // "Main" is the only Softimage channel we recycle for the RGBA (beauty) AOV
   if (in_datatype.IsEqualNoCase(L"Main"))
      return L"RGBA";

   // is this one of the AOVs created by CreateRenderChannels in ArnoldScenePreferences.js ?
   if (in_datatype.FindString(L"Arnold_") == 0) // trim the "Arnold_" prefix, so to get back the Arnold factory name
      return CStringUtilities().ReplaceString(L"Arnold_", L"", in_datatype);

   // Support for wildcards (used when you want all lightgroups)
   // You can't have asterix in a Softimage Render Channel, it will be reformated to an underscore,
   // so lets look for double underscores and convert the last underscore back to a wildcard.
   if (CStringUtilities().EndsWith(in_datatype, L"__"))
      return (in_datatype.GetSubString(0, in_datatype.Length()-1) + L"*");

   return in_datatype;
}


CString GetDriverLayerChannelType(LONG in_renderChannelType)
{
   switch(in_renderChannelType)
   {
      case siRenderChannelColorType:
         return L"RGBA";
      case siRenderChannelGrayscaleType:
      case siRenderChannelDepthType:
         return L"FLOAT";
      case siRenderChannelNormalVectorType:
      case siRenderChannelVectorType:
         return L"VECTOR";
      case siRenderChannelLabelType:
         return L"INT";
      default:
         return L"";
   }
}


CString GetDriverLayerDataTypeByName(const CString &in_layerName)
{
   if (in_layerName.IsEqualNoCase(L"RGBA"))
      return L"RGBA";
   if (in_layerName.IsEqualNoCase(L"RGB"))
      return L"RGB";
   if (in_layerName.IsEqualNoCase(L"A"))
      return L"FLOAT";
   if (in_layerName.IsEqualNoCase(L"Z"))
      return L"FLOAT";
   if (in_layerName.IsEqualNoCase(L"N"))
      return L"VECTOR";
   if (in_layerName.IsEqualNoCase(L"P"))
      return L"VECTOR";
   if (in_layerName.IsEqualNoCase(L"ID"))
      return L"INT";
   if (in_layerName.IsEqualNoCase(L"opacity"))
      return L"RGB";
   if (in_layerName.IsEqualNoCase(L"cputime"))
      return L"FLOAT";
   if (in_layerName.IsEqualNoCase(L"raycount"))
      return L"FLOAT"; 

   return L"";
}


CString GetDriverBitDepth(unsigned int in_bitDepth)
{
   switch (in_bitDepth)
   {
      case siImageBitDepthInteger8:
         return L"int8";
      case siImageBitDepthInteger16:
         return L"int16";
      case siImageBitDepthInteger32:
         return L"int32";
      case siImageBitDepthFloat16:
         return L"float16";
      case siImageBitDepthFloat32:
         return L"float32";
      default:
         return L"";
   }
}


// Exports the exr metadata
//
// @param in_node     the input driver_exr node
//
void ExportExrMetadata(AtNode *in_node)
{
   CStringArray paramTypeTable, exrMetadata;

   paramTypeTable.Add(L"INT");
   paramTypeTable.Add(L"FLOAT");
   paramTypeTable.Add(L"VECTOR2");
   paramTypeTable.Add(L"STRING");
   paramTypeTable.Add(L"MATRIX");

   for (int i=0; i<NB_EXR_METADATA; i++)
   {
      CString paramName  = GetRenderOptions()->m_exr_metadata_name[i];
      int     paramType  = GetRenderOptions()->m_exr_metadata_type[i];
      CString paramValue = GetRenderOptions()->m_exr_metadata_value[i];

      if (paramName.IsEmpty() || paramValue.IsEmpty()) // no valid name or value
         continue;

      exrMetadata.Add(paramTypeTable[paramType] + L" " + paramName + L" " + paramValue);
   }

   if (exrMetadata.GetCount() < 1)
      return;

   AtArray *exrMetadataArray = AiArrayAllocate(exrMetadata.GetCount(), 1, AI_TYPE_STRING);
   for (LONG i=0; i<exrMetadata.GetCount(); i++)
      AiArraySetStr(exrMetadataArray, i, exrMetadata[i].GetAsciiString());
   AiNodeSetArray(in_node, "custom_attributes", exrMetadataArray);
}

