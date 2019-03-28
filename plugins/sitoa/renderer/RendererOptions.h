/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "sitoa.h"

#include <xsi_ppgeventcontext.h>
#include <xsi_customproperty.h>
#include <xsi_color4f.h>
#include <xsi_utils.h>

#include <ai_texture.h>

using namespace XSI;
using namespace XSI::MATH;


enum eIprRebuildMode
{
   eIprRebuildMode_Auto = 0,
   eIprRebuildMode_Always,
   eIprRebuildMode_Manual,
   eIprRebuildMode_Flythrough
};


#define NB_EXR_METADATA 20  // max number of exr metadata
#define NB_MAX_LAYERS   50  // max number of deep exr layers


// Class for reading the rendering options
// Let's keep the parameters listed as they show in the property
//
class CRenderOptions
{
public:
   // system
   bool     m_autodetect_threads;
   int      m_threads;
   CString  m_render_device;
   CString  m_render_device_fallback;
   CString  m_gpu_default_names;
   int      m_gpu_default_min_memory_MB;
   CString  m_bucket_scanning;
   int      m_bucket_size;
   bool     m_progressive_minus3;
   bool     m_progressive_minus2;
   bool     m_progressive_minus1;
   bool     m_progressive_plus1;

   int      m_ipr_rebuild_mode;

   bool     m_skip_license_check;
   bool     m_abort_on_license_fail;
   bool     m_abort_on_error;
   CColor4f m_error_color_bad_map;
   CColor4f m_error_color_bad_pix;
   CString  m_plugins_path;
   CString  m_procedurals_path;
   CString  m_textures_path;
   // m_user_options, m_resolve_tokens: no, treated by LoadUserOptions depending on the input property

   // output
   bool    m_overscan;
   int     m_overscan_top, m_overscan_bottom, m_overscan_left, m_overscan_right;

   CString m_output_driver_color_space;

   bool    m_dither;
   bool    m_unpremult_alpha;
   bool    m_output_tiff_tiled;
   CString m_output_tiff_compression;
   bool    m_output_tiff_append;
   bool    m_output_exr_tiled;
   CString m_output_exr_compression;
   bool    m_output_exr_preserve_layer_name;
   bool    m_output_exr_autocrop;
   bool    m_output_exr_append;

   bool    m_deep_exr_enable;
   bool    m_deep_subpixel_merge;
   bool    m_deep_use_RGB_opacity;
   float   m_deep_alpha_tolerance;
   bool    m_deep_alpha_half_precision;
   float   m_deep_depth_tolerance;
   bool    m_deep_depth_half_precision;
   CString m_deep_layer_name[NB_MAX_LAYERS];
   float   m_deep_layer_tolerance[NB_MAX_LAYERS];
   bool    m_deep_layer_enable_filtering[NB_MAX_LAYERS];

   CString m_exr_metadata_name[NB_EXR_METADATA];
   int     m_exr_metadata_type[NB_EXR_METADATA];
   CString m_exr_metadata_value[NB_EXR_METADATA];

   // sampling
   int     m_AA_samples;
   int     m_GI_diffuse_samples;
   int     m_GI_specular_samples;
   int     m_GI_transmission_samples;
   int     m_GI_sss_samples;
   int     m_GI_volume_samples;
   bool     m_enable_progressive_render;

   bool    m_enable_adaptive_sampling;
   int     m_AA_samples_max;
   float   m_AA_adaptive_threshold;

   float   m_indirect_specular_blur;
   bool    m_lock_sampling_noise;
   bool    m_sss_use_autobump;

   bool    m_use_sample_clamp;
   bool    m_use_sample_clamp_AOVs;
   float   m_AA_sample_clamp;
   float   m_indirect_sample_clamp;
   CString m_output_filter;
   float   m_output_filter_width;
   bool    m_filter_color_AOVs;
   bool    m_filter_numeric_AOVs;

   // motion blur
   bool    m_enable_motion_blur;
   int     m_motion_step_transform;
   bool    m_enable_motion_deform;
   int     m_motion_step_deform;
   bool    m_exact_ice_mb;
   bool    m_ignore_motion_blur;
   float   m_motion_shutter_length;
   float   m_motion_shutter_custom_start;
   float   m_motion_shutter_custom_end;
   int     m_motion_shutter_onframe;

   // subdivision
   int    m_max_subdivisions;
   float  m_adaptive_error;
   bool   m_use_dicing_camera;
   CValue m_dicing_camera;

   // ray depth
   int     m_GI_total_depth;
   int     m_GI_diffuse_depth;
   int     m_GI_specular_depth;
   int     m_GI_transmission_depth;
   int     m_GI_volume_depth;

   int     m_auto_transparency_depth;
   float   m_low_light_threshold;

   // textures
   bool  m_texture_accept_unmipped;
   bool  m_texture_automip;
   int   m_texture_filter;
   bool  m_texture_accept_untiled;
   bool  m_enable_autotile;
   int   m_texture_autotile;
   bool  m_use_existing_tx_files;
   int   m_texture_max_memory_MB;
   int   m_texture_max_open_files;

   // color managers
   CString m_color_manager;
   CString m_ocio_config;
   CString m_ocio_color_space_narrow;
   CString m_ocio_color_space_linear;
   CString m_ocio_linear_chromaticities;

   // diagnostic
   bool         m_enable_log_console;
   bool         m_enable_log_file;
   unsigned int m_log_level;
   int          m_max_log_warning_msgs;
   bool         m_texture_per_file_stats;
   CString      m_output_file_tagdir_log;
   // output_file_dir_log; not read, just used to display the path

   bool         m_enable_stats;
   CString      m_stats_file;
   int          m_stats_mode;
   bool         m_enable_profile;
   CString      m_profile_file;

   bool         m_ignore_textures;
   bool         m_ignore_shaders;
   bool         m_ignore_atmosphere;
   bool         m_ignore_lights;
   bool         m_ignore_shadows;
   bool         m_ignore_subdivision;
   bool         m_ignore_displacement;
   bool         m_ignore_bump;
   bool         m_ignore_smoothing;
   bool         m_ignore_motion;
   bool         m_ignore_dof;
   bool         m_ignore_sss;
   bool         m_ignore_hair;
   bool         m_ignore_pointclouds;
   bool         m_ignore_procedurals;
   bool         m_ignore_user_options;
   bool         m_ignore_matte;

   // ass archive
   CString m_output_file_tagdir_ass;
   // output_file_dir_ass; not read, just used to display the path
   bool m_compress_output_ass;
   bool m_binary_ass;
   bool m_save_texture_paths;
   bool m_save_procedural_paths;
   bool m_use_path_translations;
   bool m_open_procs;
   bool m_output_options;
   bool m_output_drivers_filters;
   bool m_output_geometry;
   bool m_output_cameras;
   bool m_output_lights;
   bool m_output_shaders;

   // denoiser
   bool m_use_optix_on_main;
   bool m_only_show_denoise;
   bool m_output_denoising_aovs;

   //////////////////////////////////////

   void Read(const Property &in_cp);

   // In the constructor, let's initialize the params to the default value,
   // or, if added along the way, to the value they should have when loading old scence
   CRenderOptions():
      // system
      m_autodetect_threads(true),
      m_threads(4),
      m_render_device(L"CPU"),
      m_render_device_fallback(L"error"),
      m_gpu_default_names(L"*"),
      m_gpu_default_min_memory_MB(512),
      m_bucket_scanning(L"spiral"),
      m_bucket_size(64),
      m_progressive_minus3(true),
      m_progressive_minus2(true),
      m_progressive_minus1(true),
      m_progressive_plus1(true),
      
      m_ipr_rebuild_mode(eIprRebuildMode_Auto),

      m_skip_license_check(false),
      m_abort_on_license_fail(false),
      m_abort_on_error(true),
      m_error_color_bad_map(1.0f, 0.0f, 0.0f, 0.0f),
      m_error_color_bad_pix(0.0f, 1.0f, 0.0f, 0.0f),
      m_plugins_path(L""),
      m_procedurals_path(L""),
      m_textures_path(L""),

      // output
      m_overscan(false),
      m_overscan_top(INT_MIN),
      m_overscan_bottom(INT_MIN),
      m_overscan_left(INT_MIN),
      m_overscan_right(INT_MIN),

      m_output_driver_color_space(L"auto"),

      m_dither(true),
      m_unpremult_alpha(false),
      m_output_tiff_tiled(true),
      m_output_tiff_compression(L"lzw"),
      m_output_tiff_append(false),
      m_output_exr_tiled(true),
      m_output_exr_compression(L"zip"),
      m_output_exr_preserve_layer_name(false),
      m_output_exr_autocrop(false),
      m_output_exr_append(false),
      
      // deep exr
      m_deep_exr_enable(false),
      m_deep_subpixel_merge(false),
      m_deep_use_RGB_opacity(false),
      m_deep_alpha_tolerance(0.01f),
      m_deep_alpha_half_precision(false),
      m_deep_depth_tolerance(0.01f),
      m_deep_depth_half_precision(false),

      // sampling
      m_AA_samples(3),
      m_GI_diffuse_samples(2),
      m_GI_specular_samples(2),
      m_GI_transmission_samples(2),
      m_GI_sss_samples(2),
      m_GI_volume_samples(2),
      m_enable_progressive_render(false),

      m_enable_adaptive_sampling(false),
      m_AA_samples_max(8),
      m_AA_adaptive_threshold(0.05f),

      m_indirect_specular_blur(1.0f),

      m_lock_sampling_noise(false),
      m_sss_use_autobump(false),
      m_use_sample_clamp(false),
      m_use_sample_clamp_AOVs(false),
      m_AA_sample_clamp(10.0f),
      m_indirect_sample_clamp(10.0f),
      m_output_filter(L"gaussian"),
      m_output_filter_width(2.0),
      m_filter_color_AOVs(true),
      m_filter_numeric_AOVs(true), // not the default

      // motion blur
      m_enable_motion_blur(false),
      m_motion_step_transform(2),
      m_enable_motion_deform(false),
      m_motion_step_deform(2),
      m_exact_ice_mb(false),
      // new mb
      m_ignore_motion_blur(false),
      m_motion_shutter_length(0.5f),
      m_motion_shutter_custom_start(-0.25f),
      m_motion_shutter_custom_end(0.25f),

      m_motion_shutter_onframe(0),

      // subdivision
      m_max_subdivisions(999),
      m_adaptive_error(0.0f),
      m_use_dicing_camera(false),
      m_dicing_camera(CValue()), // not getting the CString here

      // ray depth
      m_GI_total_depth(10),
      m_GI_diffuse_depth(1),
      m_GI_specular_depth(1),
      m_GI_transmission_depth(8),
      m_GI_volume_depth(0),
      m_auto_transparency_depth(10),
      m_low_light_threshold(0.001f),

      // textures
      m_texture_accept_unmipped(true),
      m_texture_automip(false),
      m_texture_filter(AI_TEXTURE_SMART_BICUBIC),
      m_texture_accept_untiled(true),
      m_enable_autotile(false),
      m_texture_autotile(64),
      m_use_existing_tx_files(false),
      m_texture_max_memory_MB(2048),
      m_texture_max_open_files(100),

      // color managers
      m_color_manager(L"none"),
      m_ocio_config(L""),
      m_ocio_color_space_narrow(L""),
      m_ocio_color_space_linear(L""),
      m_ocio_linear_chromaticities(L""),

      // diagnostic
      m_enable_log_console(true),
      m_enable_log_file(false),
      m_log_level(1), // warnings
      m_max_log_warning_msgs(5),
      m_texture_per_file_stats(false),
      m_output_file_tagdir_log(CUtils::BuildPath(L"[Project Path]", L"Arnold_Logs")),

      m_enable_stats(false),
      m_stats_file(CUtils::BuildPath(L"[Project Path]", L"Arnold_Logs", L"[Scene]_[Pass].[Frame].stats.json")),
      m_stats_mode(1),
      m_enable_profile(false),
      m_profile_file(CUtils::BuildPath(L"[Project Path]", L"Arnold_Logs", L"[Scene]_[Pass].[Frame].profile_[Host].json")),

      m_ignore_textures(false),
      m_ignore_shaders(false),
      m_ignore_atmosphere(false),
      m_ignore_lights(false),
      m_ignore_shadows(false),
      m_ignore_subdivision(false),
      m_ignore_displacement(false),
      m_ignore_bump(false),
      m_ignore_smoothing(false),
      m_ignore_motion(false),
      m_ignore_dof(false),
      m_ignore_sss(false),
      m_ignore_hair(false),
      m_ignore_pointclouds(false),
      m_ignore_procedurals(false),
      m_ignore_user_options(false),
      m_ignore_matte(false),

      // ass archive
      m_output_file_tagdir_ass(L""), // this to be reviewed, see CommonRenderOptions_Define
      m_compress_output_ass(false),
      m_binary_ass(true),
      m_save_texture_paths(true),
      m_save_procedural_paths(true),
      m_use_path_translations(false),
      m_open_procs(false),
      // for the 6 following, default is true, but initialized to false for very old scenes
      m_output_options(false), 
      m_output_drivers_filters(false),
      m_output_geometry(false),
      m_output_cameras(false),
      m_output_lights(false),
      m_output_shaders(false),

      // denoiser
      m_use_optix_on_main(false),
      m_only_show_denoise(true),
      m_output_denoising_aovs(false)

   {
      for (LONG i=0; i<NB_MAX_LAYERS; i++)
      {
         m_deep_layer_name[i]             = L"";
         m_deep_layer_tolerance[i]        = 0.01f;
         m_deep_layer_enable_filtering[i] = true;
      }
   }
};


// Render options and preferences
// The render preferences used to be a js plugin. Since the 2 props are equal, let unify them
// with some CommonRenderOptions_* wrappers
SITOA_CALLBACK CommonRenderOptions_Define(CRef& in_ctxt);
SITOA_CALLBACK CommonRenderOptions_DefineLayout(CRef& in_ctxt);
SITOA_CALLBACK CommonRenderOptions_PPGEvent(const CRef& in_ctxt);

// Logic for the motion blur parameters
void MotionBlurTabLogic(CustomProperty &in_cp);
// Logic for the dof tab
void DepthOfFieldTabLogic(CustomProperty &in_cp);
// Logic for the sampling tab
void SamplingTabLogic(CustomProperty &in_cp);
// Logic for the system tab
void SystemTabLogic(CustomProperty &in_cp);
// Logic for the output tab
void OutputTabLogic(CustomProperty &in_cp);
// Logic for the textures tab
void TexturesTabLogic(CustomProperty &in_cp);
// Logic for the color managers tab
void ColorManagersTabLogic(CustomProperty &in_cp, PPGEventContext &in_ctxt);
// Logic for the subdivision tab
void SubdivisionTabLogic(CustomProperty &in_cp);
// Logic for the diagnostics tab
void DiagnosticsTabLogic(CustomProperty &in_cp);
// Logic for the ass archives tab
void AssOutputTabLogic(CustomProperty &in_cp);
// Logic for the denoiser tab
void DenoiserTabLogic(CustomProperty &in_cp);

// Reset the default values of all the parameters
void ResetToDefault(CustomProperty &in_cp, PPGEventContext &in_ctxt);


