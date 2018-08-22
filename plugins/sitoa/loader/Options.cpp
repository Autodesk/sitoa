/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "loader/Options.h"
#include "loader/Properties.h"
#include "renderer/Drivers.h"
#include "renderer/Renderer.h"

#include <xsi_project.h>
#include <xsi_scene.h>


// Construct by a Softimage framebuffer
//
// @param in_fb           the input Softimage framebuffer
// @param in_frame        the evaluation frame time
//
CFrameBuffer::CFrameBuffer(Framebuffer in_fb, double in_frame, bool in_checkLightsAov)
{
   m_fb = in_fb;

   CTime frameTime(in_frame);
   // get the current filename
   CString savedFileName   = ParAcc_GetValue(in_fb, L"Filename", DBL_MAX).GetAsText();
   bool resolveCameraToken = savedFileName.FindString(L"[Camera]") != UINT_MAX;

   if (resolveCameraToken) 
   {
      // there is a [Camera] token. Let resolve it by hand, by the current camera, so to support
      // stereo cameras (#810, #1313, #1364)
      CString fn = CStringUtilities().ReplaceString("[Camera]", GetRenderInstance()->GetRenderCamera().GetName(), savedFileName);
      // store this resolved filename into the fb, so to be able to use GetResolvedPath
      in_fb.PutParameterValue(L"Filename", fn, in_frame);
   }

   CString outputImageName = in_fb.GetResolvedPath(frameTime);

   if (resolveCameraToken) // restore the filename in the pass ppg
      in_fb.PutParameterValue(L"Filename", savedFileName, in_frame);

   CString format = ParAcc_GetValue(in_fb, L"Format", in_frame).GetAsText();

   m_fileName   = CPathTranslator::TranslatePath(outputImageName.GetAsciiString(), false);
   m_name       = in_fb.GetName().GetAsciiString();
   m_fullName   = in_fb.GetFullName().GetAsciiString();
   m_driverName = GetDriverName(format);
   m_layerName  = GetLayerName(ParAcc_GetValue(in_fb, L"RenderChannel", in_frame).GetAsText());
   // Getting DataType from FrameBuffer instead RenderChannel 
   // (we can save in RGBA or RGB so we are going to get that info from FrameBuffer config)
   m_layerDataType  = ParAcc_GetValue(in_fb, L"DataType", in_frame).GetAsText();
   m_driverBitDepth = GetDriverBitDepth((LONG)ParAcc_GetValue(in_fb, L"BitDepth", in_frame));
   m_format         = ParAcc_GetValue(in_fb, L"Format", in_frame).GetAsText();

   if (in_checkLightsAov)
   {
      if (m_layerName == L"RGBA.*")
      {
         if (m_driverName != L"driver_exr")
            GetMessageQueue()->LogMsg(L"[sitoa] Invalid format (" + format + L") specified for the Arnold_Lights render channel. Please switch to EXR.", siWarningMsg);
         else if (m_layerDataType != L"RGBA")
            GetMessageQueue()->LogMsg(L"[sitoa] Invalid data type (" + m_layerDataType + L") specified for the Arnold_Lights render channel. Please switch to RGBA.", siWarningMsg);
      }
      if (m_layerName == L"volume.*")
      {
         if (m_driverName != L"driver_exr")
            GetMessageQueue()->LogMsg(L"[sitoa] Invalid format (" + format + L") specified for the Arnold_Volume_Lights render channel. Please switch to EXR.", siWarningMsg);
         else if (m_layerDataType != L"RGBA")
            GetMessageQueue()->LogMsg(L"[sitoa] Invalid data type (" + m_layerDataType + L") specified for the Arnold_Volume_Lights render channel. Please switch to RGBA.", siWarningMsg);
      }
   }
}


// Check if the format is ok. If not, for the main channel, default to tiff
//
// @param in_mainFormat           the main channel format
//
// @return if the buffer is valid or not.
//
bool CFrameBuffer::IsValid(CString in_mainFormat)
{
   // No valid driver found for specified format, skipping framebuffer
   if (m_driverName.IsEqualNoCase(L""))
   {
      if (m_name == L"Main")
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Not valid format (" + in_mainFormat + L") specified for the Main framebuffer. Switching to TIFF.", siWarningMsg );
         m_driverName = L"driver_tiff";
         m_layerDataType = L"RGBA";
         m_driverBitDepth = L"int8";
         return true;
      }
      else
      {
         GetMessageQueue()->LogMsg(L"[sitoa] Not valid format (" + m_format + L") specified for framebuffer " + m_fullName, siErrorMsg);
         return false;
      }
   }

   return true;
}


// Check if the framebuffer is an exr
//
// @return if the buffer is an exr or not.
//
bool CFrameBuffer::IsExr()
{
   return m_driverName.IsEqualNoCase(L"driver_exr") || m_driverName.IsEqualNoCase(L"driver_deepexr");
}


// return true if this is a half float precision framebuffer
bool CFrameBuffer::IsHalfFloat()
{
   return m_driverBitDepth.IsEqualNoCase(L"float16");
}

// Compare the bitdepth with another framebuffer one, and prompt a warning if they differ
//
// @param in_fb     the framebuffer to compare
//
void CFrameBuffer::CheckBitDepth(CFrameBuffer in_fb)
{
   if (m_driverBitDepth != in_fb.m_driverBitDepth)
      GetMessageQueue()->LogMsg(L"[sitoa] Bit Depth for " + m_name + L" (" + m_driverBitDepth + L") differs from " + in_fb.m_name + L" (" + in_fb.m_driverBitDepth + L"). Defaulting to " + in_fb.m_driverBitDepth);
}


// Log all the string data
void CFrameBuffer::Log()
{
   GetMessageQueue()->LogMsg(L"-------------------");
   GetMessageQueue()->LogMsg(L"Name             = " + m_name);
   GetMessageQueue()->LogMsg(L"Full Name        = " + m_fullName);
   GetMessageQueue()->LogMsg(L"File Name        = " + m_fileName);
   GetMessageQueue()->LogMsg(L"Driver Name      = " + m_driverName);
   GetMessageQueue()->LogMsg(L"Layer Name       = " + m_layerName);
   GetMessageQueue()->LogMsg(L"Layer Data Type  = " + m_layerDataType);
   GetMessageQueue()->LogMsg(L"Driver Bit Depth = " + m_driverBitDepth);
   GetMessageQueue()->LogMsg(L"Format           = " + m_format);
}


// Export the frame and fps into the options node
//
// @param in_optionsNode     the Arnold options node
// @param in_frame           the frame time
//
void LoadPlayControlData(AtNode* in_optionsNode, double in_frame)
{
   CNodeSetter::SetFloat(in_optionsNode, "frame", (float)in_frame);

   double fps = CTimeUtilities().GetFps();
   CNodeSetter::SetFloat(in_optionsNode, "fps", (float)fps);
}


// Load the output filters
//
// @return true if the filter nodes were well created, else false
//
bool LoadFilters()
{
   CString filterType = GetRenderOptions()->m_output_filter;
   AtNode* filterNode = AiNode(CString(filterType + L"_filter").GetAsciiString());
   if (!filterNode)
      return false;

   CNodeUtilities().SetName(filterNode, "sitoa_output_filter");

   // width, if the filter node has it
   if (AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(filterNode), "width"))
      CNodeSetter::SetFloat(filterNode, "width", GetRenderOptions()->m_output_filter_width);

   // also add a closest (aliased) filter for aovs (#1028)
   AtNode* closestFilterNode = AiNode("closest_filter");
   if (!closestFilterNode)
      return false;

   CNodeUtilities().SetName(closestFilterNode, "sitoa_closest_filter");
   return true;
}


// class used to store the layers associated with a driver
//
class CDeepExrLayersDrivers
{
public:
   CString      m_driverName; // driver name
   CStringArray m_layerName;  // layer names
   CStringArray m_bitDepth;   // bit depths of the layer

   CDeepExrLayersDrivers()
   {}

   CDeepExrLayersDrivers(const CDeepExrLayersDrivers &in_arg) :
      m_driverName(in_arg.m_driverName), m_layerName(in_arg.m_layerName), m_bitDepth(in_arg.m_bitDepth)
   {}
   // construct by driver name layer name and bitdepth
   CDeepExrLayersDrivers(const CString &in_driverName, const CString &in_layerName, const CString &in_bitDepth)
   {
      m_driverName = in_driverName;
      m_layerName.Add(in_layerName);
      m_bitDepth.Add(in_bitDepth);
   }
   // add a layer and bitdept
   void AddLayerAndBitDepth(const CString &in_layerName, const CString &in_bitDepth)
   {
      m_layerName.Add(in_layerName);
      m_bitDepth.Add(in_bitDepth);
   }

   ~CDeepExrLayersDrivers()
   {
      m_layerName.Clear();
      m_bitDepth.Clear();
   }
};


// Export the layer arrays for the all deep exr drivers
//
// @param in_deepExrLayersDrivers     the array of drivers name with their layers name
//
void SetDeepExrLayers(vector <CDeepExrLayersDrivers> &in_deepExrLayersDrivers)
{
   vector <CDeepExrLayersDrivers>::iterator it;
   for (it=in_deepExrLayersDrivers.begin(); it!=in_deepExrLayersDrivers.end(); it++)
   {
      AtNode *driver = AiNodeLookUpByName(it->m_driverName.GetAsciiString());
      if (!driver)
         continue;
      // layers for this driver
      LONG nbLayers = it->m_layerName.GetCount();
      AtArray *toleranceArray = AiArrayAllocate(nbLayers, 1, AI_TYPE_FLOAT);
      AtArray *filteringArray = AiArrayAllocate(nbLayers, 1, AI_TYPE_BOOLEAN);
      AtArray *precisionArray = AiArrayAllocate(nbLayers, 1, AI_TYPE_BOOLEAN);

      for (LONG i=0; i<nbLayers; i++)
      {
         CString layerName = it->m_layerName[i];
         bool halfPrecision = it->m_bitDepth[i].IsEqualNoCase(L"float16");

         float tolerance(0.01f);
         bool  enableFiltering(true);
         // loop the layer parameters, and find the one whose name is equal to layerName
         for (LONG j=0; j<NB_MAX_LAYERS; j++)
         {
            // if found, get the same line (j-th) tolerance value and filtering flag
            if (GetRenderOptions()->m_deep_layer_name[j] == layerName)
            {
               tolerance       = GetRenderOptions()->m_deep_layer_tolerance[j];
               enableFiltering = GetRenderOptions()->m_deep_layer_enable_filtering[j];
               break;
            }
         }

         AiArraySetFlt (toleranceArray, i, tolerance);
         AiArraySetBool(filteringArray, i, enableFiltering);
         AiArraySetBool(precisionArray, i, halfPrecision);
      }

      AiNodeSetArray(driver, "layer_tolerance",        toleranceArray);
      AiNodeSetArray(driver, "layer_enable_filtering", filteringArray);
      AiNodeSetArray(driver, "layer_half_precision",   precisionArray);
   }
}


// Load the drivers
//
// @param in_optionsNode     the Arnold options node
// @param in_pass            the current pass
// @param in_frame           the frame time
// @param in_flythrough      true if in flythrough mode
//
// @return true is there is at least an active framebuffer, else false
//
bool LoadDrivers(AtNode *in_optionsNode, Pass &in_pass, double in_frame, bool in_flythrough)
{
   Framebuffer frameBuffer(in_pass.GetFramebuffers().GetItem(L"Main"));
   CString mainFormat(ParAcc_GetValue(frameBuffer, L"Format", in_frame));

   CRefArray frameBuffers = in_pass.GetFramebuffers();
   LONG nbBuffers = frameBuffers.GetCount();

   vector <CFrameBuffer> fbVector;
   
   // number of buffers to render
   unsigned int activeBuffers = 0;
   for (LONG i = 0; i<nbBuffers; i++)
   {
      frameBuffer = Framebuffer(frameBuffers[i]);
      activeBuffers+= (LONG)ParAcc_GetValue(frameBuffer, L"Enabled", in_frame);
   }

   // allocate the output string
   AtArray* outputs = AiArrayAllocate(activeBuffers, 1, AI_TYPE_STRING);
   CString colorFilter   = GetRenderOptions()->m_filter_color_AOVs   ? L"sitoa_output_filter" : L"sitoa_closest_filter";
   CString numericFilter = GetRenderOptions()->m_filter_numeric_AOVs ? L"sitoa_output_filter" : L"sitoa_closest_filter";

   // vector of the drivers, each with theirs layers
   vector <CDeepExrLayersDrivers> deepExrLayersDrivers;

   unsigned int activeBuffer = 0;
   for (LONG i = 0; i<nbBuffers; i++)
   {
      frameBuffer = Framebuffer(frameBuffers[i]);
      //check if this framebuffer is enabled
      if (ParAcc_GetValue(frameBuffer,L"Enabled", in_frame) == false) // skip it
         continue;

      CFrameBuffer thisFb(frameBuffer, in_frame, true);
      CFrameBuffer masterFb(thisFb); // make a copy
      if (!thisFb.IsValid(mainFormat))
         continue;

      bool fbFound = false;
      if (thisFb.IsExr())
      {
         for (int i=0; i<(int)fbVector.size(); i++)
         {
            if (fbVector[i].IsExr() && (thisFb.m_fileName == fbVector[i].m_fileName))
            {
               masterFb = fbVector[i]; // the masterFb, used just to set the output name in case of layered exr 
               fbFound = true;
               break;
            }
         }
      }

      if (fbFound) // check the bit depth and warn, in case
         thisFb.CheckBitDepth(masterFb);

      // if the same exr filename was found for thisFb, don't push a new driver
      if (!fbFound)
      {
         fbVector.push_back(thisFb);

         AtNode* driverNode = in_flythrough ? AiNodeLookUpByName(thisFb.m_fullName.GetAsciiString()) : 
                                              AiNode(thisFb.m_driverName.GetAsciiString());;

         if (driverNode)
         {
            CNodeUtilities().SetName(driverNode, thisFb.m_fullName);
            CNodeSetter::SetString(driverNode, "color_space", GetRenderOptions()->m_output_driver_color_space.GetAsciiString());

            if (thisFb.m_driverName == L"driver_tiff")
            {
               CNodeSetter::SetString (driverNode, "compression",     GetRenderOptions()->m_output_tiff_compression.GetAsciiString());
               CNodeSetter::SetString (driverNode, "format",          thisFb.m_driverBitDepth.GetAsciiString());
               CNodeSetter::SetBoolean(driverNode, "tiled",           GetRenderOptions()->m_output_tiff_tiled);            
               CNodeSetter::SetBoolean(driverNode, "unpremult_alpha", GetRenderOptions()->m_unpremult_alpha);
               CNodeSetter::SetBoolean(driverNode, "output_padded",   (bool)ParAcc_GetValue(in_pass, L"CropWindowEnabled", DBL_MAX));
               // dither, only for 8bit
               if (thisFb.m_driverBitDepth.IsEqualNoCase(L"int8"))
                  CNodeSetter::SetBoolean(driverNode, "dither", GetRenderOptions()->m_dither);

               if (GetRenderOptions()->m_output_tiff_tiled)
                  CNodeSetter::SetBoolean(driverNode, "append", GetRenderOptions()->m_output_tiff_append);
            }
            else if (thisFb.m_driverName == "driver_png")
            {
               CNodeSetter::SetString(driverNode, "format", thisFb.m_driverBitDepth.GetAsciiString());
               // dither, only for 8bit
               if (thisFb.m_driverBitDepth.IsEqualNoCase(L"int8"))
                  CNodeSetter::SetBoolean(driverNode, "dither", GetRenderOptions()->m_dither);
            }
            else if (thisFb.m_driverName.IsEqualNoCase(L"driver_jpeg") || thisFb.m_driverName.IsEqualNoCase(L"driver_png"))
            {
               CNodeSetter::SetBoolean(driverNode, "dither",        GetRenderOptions()->m_dither);
               CNodeSetter::SetBoolean(driverNode, "output_padded", (bool)ParAcc_GetValue(in_pass, L"CropWindowEnabled", DBL_MAX));
            }
            else if (thisFb.m_driverName == L"driver_exr")
            {
               bool tiled = GetRenderOptions()->m_output_exr_tiled;
               CNodeSetter::SetBoolean(driverNode, "half_precision",      thisFb.IsHalfFloat());
               CNodeSetter::SetBoolean(driverNode, "tiled",               tiled);            
               CNodeSetter::SetString (driverNode, "compression",         GetRenderOptions()->m_output_exr_compression.GetAsciiString());
               CNodeSetter::SetBoolean(driverNode, "preserve_layer_name", GetRenderOptions()->m_output_exr_preserve_layer_name);            
               if (!tiled) // autocrop allowed only for scanline
                  CNodeSetter::SetBoolean(driverNode, "autocrop", GetRenderOptions()->m_output_exr_autocrop);
               else //append mode only for tile mode
                  CNodeSetter::SetBoolean(driverNode, "append", GetRenderOptions()->m_output_exr_append);

               ExportExrMetadata(driverNode);
            }
            else if (thisFb.m_driverName == L"driver_deepexr")
            {
               CNodeSetter::SetBoolean(driverNode, "tiled", GetRenderOptions()->m_output_exr_tiled);
               CNodeSetter::SetBoolean(driverNode, "subpixel_merge",       GetRenderOptions()->m_deep_subpixel_merge);
               CNodeSetter::SetBoolean(driverNode, "use_RGB_opacity",      GetRenderOptions()->m_deep_use_RGB_opacity);
               CNodeSetter::SetFloat (driverNode, "alpha_tolerance",      GetRenderOptions()->m_deep_alpha_tolerance);
               CNodeSetter::SetBoolean(driverNode, "alpha_half_precision", GetRenderOptions()->m_deep_alpha_half_precision);
               CNodeSetter::SetFloat (driverNode, "depth_tolerance",      GetRenderOptions()->m_deep_depth_tolerance);
               CNodeSetter::SetBoolean(driverNode, "depth_half_precision", GetRenderOptions()->m_deep_depth_half_precision);

               if (GetRenderOptions()->m_output_exr_tiled)
                  CNodeSetter::SetBoolean(driverNode, "append", GetRenderOptions()->m_output_exr_append);

               ExportExrMetadata(driverNode);
            }

            // Output file name
            CNodeSetter::SetString(driverNode, "filename", thisFb.m_fileName.GetAsciiString());
         }
      } // !fbFound

      // if this is a deep driver, collect the layer per driver (needed because we are looping the framebuffers == layers)
      if (masterFb.m_driverName == L"driver_deepexr")
      {
         vector <CDeepExrLayersDrivers>::iterator it;
         bool found(false);
         // check if this deep driver has already been created
         for (it=deepExrLayersDrivers.begin(); it!=deepExrLayersDrivers.end(); it++)
         {
            if (it->m_driverName == masterFb.m_fullName)
            {  // if yes, add the current layer to those belonging to the driver
               it->AddLayerAndBitDepth(thisFb.m_layerName, thisFb.m_driverBitDepth);
               found = true;
               break;
            }
         }

         if (!found) // if not, create a new entry with the driver name and the layer
            deepExrLayersDrivers.push_back(CDeepExrLayersDrivers(masterFb.m_fullName, thisFb.m_layerName, thisFb.m_driverBitDepth));
      }

      // Adding to outputs. masterFb differs from thisFb if they are both exr and share the same filename
      if (thisFb.m_layerDataType.IsEqualNoCase(L"RGB") || thisFb.m_layerDataType.IsEqualNoCase(L"RGBA"))
         AiArraySetStr(outputs, activeBuffer, CString(thisFb.m_layerName + L" " + thisFb.m_layerDataType + L" " + colorFilter + " " + masterFb.m_fullName).GetAsciiString());
      else
         AiArraySetStr(outputs, activeBuffer, CString(thisFb.m_layerName + L" " + thisFb.m_layerDataType + L" " + numericFilter + " " + masterFb.m_fullName).GetAsciiString());

      activeBuffer++;
   }

   // Setting outpus array only if there is at least one active framebuffer
   if (activeBuffer > 0)
   {
      AiNodeSetArray(in_optionsNode, "outputs", outputs);
      // set the layer arrays for the deep drivers, if any
      SetDeepExrLayers(deepExrLayersDrivers);
      return true;
   }

   AiArrayDestroy(outputs);
   return false;
}


// Load the options parameters
//
// @param in_optionsNode     the Arnold options node
// @param in_optionsProperty the Arnold rendering options property
// @param in_frame           the frame time
//
void LoadOptionsParameters(AtNode* in_optionsNode, const Property &in_arnoldOptions, double in_frame)
{
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());

   // Get the scene resolution and set it
   int width, height;
   float aspectRatio;
   CSceneUtilities::GetSceneResolution(width, height, aspectRatio);

   CNodeSetter::SetInt(in_optionsNode, "xres", width);
   CNodeSetter::SetInt(in_optionsNode, "yres", height);
   if (aspectRatio > 0.0f)
      CNodeSetter::SetFloat(in_optionsNode, "pixel_aspect_ratio", 1.0f / aspectRatio);

   // cropping
   if ((bool)ParAcc_GetValue(pass, L"CropWindowEnabled", in_frame))
   {
      int cropWidth   = ParAcc_GetValue(pass, L"CropWindowWidth",   in_frame);
      int cropHeight  = ParAcc_GetValue(pass, L"CropWindowHeight",  in_frame);
      int cropOffsetX = ParAcc_GetValue(pass, L"CropWindowOffsetX", in_frame);
      int cropOffsetY = ParAcc_GetValue(pass, L"CropWindowOffsetY", in_frame);

      CNodeSetter::SetInt(in_optionsNode, "region_min_x", AiMax(cropOffsetX, 0));
      CNodeSetter::SetInt(in_optionsNode, "region_min_y", AiMax(height-cropHeight-cropOffsetY, 0));
      CNodeSetter::SetInt(in_optionsNode, "region_max_x", AiMin(AiMax(cropWidth+cropOffsetX-1, 0), width-1));
      CNodeSetter::SetInt(in_optionsNode, "region_max_y", AiMin(AiMax(height-cropOffsetY-1, 0), height-1));
   }
   else
   {
       // default to disable cropping:
      int region_min_x(INT_MIN), region_max_x(INT_MIN), region_min_y(INT_MIN), region_max_y(INT_MIN);

      if (GetRenderOptions()->m_overscan)
      {
          region_min_x = -GetRenderOptions()->m_overscan_left;
          region_max_x = width + GetRenderOptions()->m_overscan_right - 1;
          region_min_y = -GetRenderOptions()->m_overscan_top;
          region_max_y = height + GetRenderOptions()->m_overscan_bottom - 1;
      }

      CNodeSetter::SetInt(in_optionsNode, "region_min_x", region_min_x);
      CNodeSetter::SetInt(in_optionsNode, "region_min_y", region_min_y);
      CNodeSetter::SetInt(in_optionsNode, "region_max_x", region_max_x);
      CNodeSetter::SetInt(in_optionsNode, "region_max_y", region_max_y);
   }

   // sampling
   CNodeSetter::SetInt(in_optionsNode, "AA_samples",              GetRenderOptions()->m_AA_samples);
   CNodeSetter::SetInt(in_optionsNode, "GI_diffuse_samples",      GetRenderOptions()->m_GI_diffuse_samples);
   CNodeSetter::SetInt(in_optionsNode, "GI_specular_samples",     GetRenderOptions()->m_GI_specular_samples);
   CNodeSetter::SetInt(in_optionsNode, "GI_transmission_samples", GetRenderOptions()->m_GI_transmission_samples);
   CNodeSetter::SetInt(in_optionsNode, "GI_sss_samples",          GetRenderOptions()->m_GI_sss_samples);
   CNodeSetter::SetInt(in_optionsNode, "GI_volume_samples",       GetRenderOptions()->m_GI_volume_samples);

   CNodeSetter::SetBoolean(in_optionsNode, "enable_adaptive_sampling", GetRenderOptions()->m_enable_adaptive_sampling);
   CNodeSetter::SetInt(in_optionsNode, "AA_samples_max",               GetRenderOptions()->m_AA_samples_max);
   CNodeSetter::SetFloat(in_optionsNode, "AA_adaptive_threshold",      GetRenderOptions()->m_AA_adaptive_threshold);

   CNodeSetter::SetFloat(in_optionsNode, "indirect_specular_blur", GetRenderOptions()->m_indirect_specular_blur);

   // Use sample clamp?
   if (GetRenderOptions()->m_use_sample_clamp)
   {
      CNodeSetter::SetFloat(in_optionsNode, "AA_sample_clamp", GetRenderOptions()->m_AA_sample_clamp);
      CNodeSetter::SetBoolean(in_optionsNode, "AA_sample_clamp_affects_aovs", GetRenderOptions()->m_use_sample_clamp_AOVs); 
   }

   CNodeSetter::SetFloat(in_optionsNode, "indirect_sample_clamp", GetRenderOptions()->m_indirect_sample_clamp);

   if (!GetRenderOptions()->m_lock_sampling_noise)
      CNodeSetter::SetInt(in_optionsNode, "AA_seed", (int)in_frame);

   CNodeSetter::SetBoolean(in_optionsNode, "sss_use_autobump", GetRenderOptions()->m_sss_use_autobump);

   // Subdivision
   CNodeSetter::SetByte(in_optionsNode, "max_subdivisions", (uint8_t)GetRenderOptions()->m_max_subdivisions);

   // Ray Depths
   CNodeSetter::SetInt(in_optionsNode, "GI_total_depth",        GetRenderOptions()->m_GI_total_depth); 
   CNodeSetter::SetInt(in_optionsNode, "GI_diffuse_depth",      GetRenderOptions()->m_GI_diffuse_depth);
   CNodeSetter::SetInt(in_optionsNode, "GI_specular_depth",     GetRenderOptions()->m_GI_specular_depth);
   CNodeSetter::SetInt(in_optionsNode, "GI_transmission_depth", GetRenderOptions()->m_GI_transmission_depth);
   CNodeSetter::SetInt(in_optionsNode, "GI_volume_depth",       GetRenderOptions()->m_GI_volume_depth);
   
   // Auto-transparency
   CNodeSetter::SetInt   (in_optionsNode, "auto_transparency_depth",     GetRenderOptions()->m_auto_transparency_depth); 

   CNodeSetter::SetFloat (in_optionsNode, "low_light_threshold",  GetRenderOptions()->m_low_light_threshold); 

   // Ignores
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_textures",     GetRenderOptions()->m_ignore_textures);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_shaders",      GetRenderOptions()->m_ignore_shaders);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_atmosphere",   GetRenderOptions()->m_ignore_atmosphere);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_lights",       GetRenderOptions()->m_ignore_lights);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_shadows",      GetRenderOptions()->m_ignore_shadows);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_subdivision",  GetRenderOptions()->m_ignore_subdivision);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_displacement", GetRenderOptions()->m_ignore_displacement);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_bump",         GetRenderOptions()->m_ignore_bump);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_motion_blur",  GetRenderOptions()->m_ignore_motion_blur);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_smoothing",    GetRenderOptions()->m_ignore_smoothing);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_sss",          GetRenderOptions()->m_ignore_sss);
   CNodeSetter::SetBoolean(in_optionsNode, "ignore_dof",          GetRenderOptions()->m_ignore_dof);

   // Error colors
   CNodeSetter::SetRGB(in_optionsNode, "error_color_bad_texture", GetRenderOptions()->m_error_color_bad_map.GetR(), 
                                                                  GetRenderOptions()->m_error_color_bad_map.GetG(), 
                                                                  GetRenderOptions()->m_error_color_bad_map.GetB());
   CNodeSetter::SetRGB(in_optionsNode, "error_color_bad_pixel", GetRenderOptions()->m_error_color_bad_pix.GetR(), 
                                                                GetRenderOptions()->m_error_color_bad_pix.GetG(), 
                                                                GetRenderOptions()->m_error_color_bad_pix.GetB());

   // Texture system
   CNodeSetter::SetBoolean(in_optionsNode, "texture_accept_unmipped", GetRenderOptions()->m_texture_accept_unmipped);
   CNodeSetter::SetBoolean(in_optionsNode, "texture_automip",         GetRenderOptions()->m_texture_automip);
   CNodeSetter::SetBoolean(in_optionsNode, "texture_accept_untiled",  GetRenderOptions()->m_texture_accept_untiled);

   // Tiling
   int texture_autotile = GetRenderOptions()->m_enable_autotile ? GetRenderOptions()->m_texture_autotile : 0;
   CNodeSetter::SetInt(in_optionsNode, "texture_autotile", texture_autotile);

   CNodeSetter::SetFloat(in_optionsNode, "texture_max_memory_MB", (float)GetRenderOptions()->m_texture_max_memory_MB);

   // Set the maximum number of files open
   CNodeSetter::SetInt(in_optionsNode, "texture_max_open_files", GetRenderOptions()->m_texture_max_open_files);
   CNodeSetter::SetFloat(in_optionsNode, "texture_max_sharpen",    1.5f); // #1559

   CNodeSetter::SetBoolean(in_optionsNode, "texture_per_file_stats", GetRenderOptions()->m_texture_per_file_stats);

   CNodeSetter::SetString(in_optionsNode, "bucket_scanning", GetRenderOptions()->m_bucket_scanning.GetAsciiString());
   CNodeSetter::SetInt(in_optionsNode, "bucket_size", GetRenderOptions()->m_bucket_size); 

   CNodeSetter::SetBoolean(in_optionsNode,"abort_on_error", GetRenderOptions()->m_abort_on_error); 

   bool skip_license_check = GetRenderOptions()->m_skip_license_check;
   CNodeSetter::SetBoolean(in_optionsNode,"skip_license_check", skip_license_check); 
   CNodeSetter::SetBoolean(in_optionsNode,"abort_on_license_fail", !skip_license_check && GetRenderOptions()->m_abort_on_license_fail);

   // get the procedurals search path, and check for the existence of the directories (the "true" param)
   GetRenderInstance()->GetProceduralsSearchPath().Put(CPathUtilities().GetProceduralsPath(), true);
   if (GetRenderInstance()->GetProceduralsSearchPath().GetCount() > 0)
   {
      // rebuild the full search path, (re)joining together the paths. The non existing are excluded
      CPathString translatedProceduralsSearchPath = GetRenderInstance()->GetProceduralsSearchPath().Translate();
      CNodeSetter::SetString(in_optionsNode, "procedural_searchpath", translatedProceduralsSearchPath.GetAsciiString());
   }

   // get the texture search path, and check for the existence of the directories (the "true" param)
   GetRenderInstance()->GetTexturesSearchPath().Put(CPathUtilities().GetTexturesPath(), true);
   // GetRenderInstance()->GetTexturesSearchPath().Log();
   if (GetRenderInstance()->GetTexturesSearchPath().GetCount() > 0)
   {
      // rebuild the full search path, (re)joining together the paths. The non existing are excluded
      CPathString translatedTexturesSearchPath = GetRenderInstance()->GetTexturesSearchPath().Translate();
      CNodeSetter::SetString(in_optionsNode, "texture_searchpath", translatedTexturesSearchPath.GetAsciiString());
   }

   // Asking for automatic threads or not
   int nb_threads = GetRenderOptions()->m_autodetect_threads ? 0 : GetRenderOptions()->m_threads;
   CNodeSetter::SetInt(in_optionsNode, "threads", nb_threads); 

   // #680
   LoadUserOptions(in_optionsNode, in_arnoldOptions, in_frame);
}


// Load the options node
//
// @param in_optionsProperty the Arnold rendering options property
// @param in_frame           the frame time
// @param in_flythrough      true if in flythrough mode
//
CStatus LoadOptions(const Property& in_arnoldOptions, double in_frame, bool in_flythrough)
{
   // Get Active Pass
   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());

   AtNode *optionsNode = AiUniverseGetOptions();

   // load rendering options
   LoadOptionsParameters(optionsNode, in_arnoldOptions, in_frame);

   if (!in_flythrough)
   {
      // export "frame" and "fps" (#1456)
      LoadPlayControlData(optionsNode, in_frame);
      if (!LoadFilters())
         return CStatus::Fail;
   }

   // drivers, return if at least one framebuffer is active
   if (LoadDrivers(optionsNode, pass, in_frame, in_flythrough))
      return CStatus::OK;;

   // if we're in region or export, we should go ahead normally, regardless of the missing output files
   CString renderType = GetRenderInstance()->GetRenderType();
   if (renderType == L"Region" || renderType == L"Export")
      return CStatus::OK;
   // else we're in Pass mode, so rendering to disk. If so, abort
   GetMessageQueue()->LogMsg(L"[sitoa] No active framebuffer", siWarningMsg );
   return CStatus::Abort;
}

// Post load options.subdiv_dicing_camera, because it needs the camera Arnold node
//
CStatus PostLoadOptions(const Property &in_optionsNode, double in_frame)
{
   if (GetRenderOptions()->m_use_dicing_camera)
   {
      AtNode *options = AiUniverseGetOptions();

      Camera xsiCamera(GetRenderOptions()->m_dicing_camera); 
      if (xsiCamera.IsValid()) 
      { 
         AtNode* cameraNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiCamera, in_frame);
         if (cameraNode)
            CNodeSetter::SetPointer(options, "subdiv_dicing_camera", cameraNode);
      }
   }

   return CStatus::OK;
}
