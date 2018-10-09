/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "loader/Loader.h"
#include "loader/Options.h"
#include "renderer/RendererOptions.h"
#include "renderer/Renderer.h"

#include <xsi_framebuffer.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_preferences.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_uitoolkit.h>


// Read the rendering options.
//
// @param in_cp               the rendering options property
//
void CRenderOptions::Read(const Property &in_cp)
{
   if (!in_cp.IsValid())
      return;

   // system
   m_autodetect_threads    = (bool)ParAcc_GetValue(in_cp, L"autodetect_threads",    DBL_MAX);
   m_threads               = (int) ParAcc_GetValue(in_cp, L"threads",               DBL_MAX);
   m_bucket_scanning       = ParAcc_GetValue(in_cp,       L"bucket_scanning",       DBL_MAX).GetAsText();
   m_bucket_size           = (int)ParAcc_GetValue(in_cp,  L"bucket_size",           DBL_MAX);
   m_progressive_minus3    = (bool)ParAcc_GetValue(in_cp, L"progressive_minus3",    DBL_MAX);
   m_progressive_minus2    = (bool)ParAcc_GetValue(in_cp, L"progressive_minus2",    DBL_MAX);
   m_progressive_minus1    = (bool)ParAcc_GetValue(in_cp, L"progressive_minus1",    DBL_MAX);
   m_progressive_plus1     = (bool)ParAcc_GetValue(in_cp, L"progressive_plus1",     DBL_MAX);
   
   m_ipr_rebuild_mode   = (int)ParAcc_GetValue(in_cp,  L"ipr_rebuild_mode",      DBL_MAX);

   m_skip_license_check    = (bool)ParAcc_GetValue(in_cp, L"skip_license_check",    DBL_MAX);
   m_abort_on_license_fail = (bool)ParAcc_GetValue(in_cp, L"abort_on_license_fail", DBL_MAX);
   m_abort_on_error        = (bool)ParAcc_GetValue(in_cp, L"abort_on_error",        DBL_MAX);

   m_error_color_bad_map.PutR(ParAcc_GetValue(in_cp, L"error_color_bad_mapR", DBL_MAX));
   m_error_color_bad_map.PutG(ParAcc_GetValue(in_cp, L"error_color_bad_mapG", DBL_MAX));
   m_error_color_bad_map.PutB(ParAcc_GetValue(in_cp, L"error_color_bad_mapB", DBL_MAX));
   m_error_color_bad_pix.PutR(ParAcc_GetValue(in_cp, L"error_color_bad_pixR", DBL_MAX));
   m_error_color_bad_pix.PutG(ParAcc_GetValue(in_cp, L"error_color_bad_pixG", DBL_MAX));
   m_error_color_bad_pix.PutB(ParAcc_GetValue(in_cp, L"error_color_bad_pixB", DBL_MAX));

   m_plugins_path     = ParAcc_GetValue(in_cp, L"plugins_path",     DBL_MAX).GetAsText();
   m_procedurals_path = ParAcc_GetValue(in_cp, L"procedurals_path", DBL_MAX).GetAsText();
   m_textures_path    = ParAcc_GetValue(in_cp, L"textures_path",    DBL_MAX).GetAsText();

   // output
   m_overscan        = (bool)ParAcc_GetValue(in_cp, L"overscan", DBL_MAX);
   m_overscan_top    = (int)ParAcc_GetValue(in_cp, L"overscan_top",    DBL_MAX);
   m_overscan_bottom = (int)ParAcc_GetValue(in_cp, L"overscan_bottom", DBL_MAX);
   m_overscan_left   = (int)ParAcc_GetValue(in_cp, L"overscan_left",   DBL_MAX);
   m_overscan_right  = (int)ParAcc_GetValue(in_cp, L"overscan_right",  DBL_MAX);

   m_output_driver_color_space = ParAcc_GetValue(in_cp, L"output_driver_color_space", DBL_MAX).GetAsText();

   m_dither                 = (bool)ParAcc_GetValue(in_cp, L"dither", DBL_MAX);
   m_unpremult_alpha        = (bool)ParAcc_GetValue(in_cp, L"unpremult_alpha", DBL_MAX);
   m_output_tiff_tiled      = (bool)ParAcc_GetValue(in_cp, L"output_tiff_tiled", DBL_MAX); // was a string, #1634
   m_output_tiff_compression = ParAcc_GetValue(in_cp, L"output_tiff_compression",                  DBL_MAX).GetAsText();
   m_output_tiff_append             = (bool)ParAcc_GetValue(in_cp, L"output_tiff_append",             DBL_MAX);
   m_output_exr_tiled               = (bool)ParAcc_GetValue(in_cp, L"output_exr_tiled",               DBL_MAX); // was a string, #1634
   m_output_exr_compression         =  ParAcc_GetValue(in_cp,      L"output_exr_compression",         DBL_MAX).GetAsText();
   m_output_exr_preserve_layer_name = (bool)ParAcc_GetValue(in_cp, L"output_exr_preserve_layer_name", DBL_MAX);
   m_output_exr_autocrop            = (bool)ParAcc_GetValue(in_cp, L"output_exr_autocrop",            DBL_MAX);
   m_output_exr_append              = (bool)ParAcc_GetValue(in_cp, L"output_exr_append",              DBL_MAX);

   m_deep_exr_enable              = (bool) ParAcc_GetValue(in_cp, L"deep_exr_enable", DBL_MAX);
   m_deep_subpixel_merge          = (bool) ParAcc_GetValue(in_cp, L"deep_subpixel_merge",  DBL_MAX);
   m_deep_use_RGB_opacity         = (bool) ParAcc_GetValue(in_cp, L"deep_use_RGB_opacity", DBL_MAX);
   m_deep_alpha_tolerance         = (float)ParAcc_GetValue(in_cp, L"deep_alpha_tolerance", DBL_MAX);
   m_deep_alpha_half_precision    = (bool) ParAcc_GetValue(in_cp, L"deep_alpha_half_precision", DBL_MAX);
   m_deep_depth_tolerance         = (float)ParAcc_GetValue(in_cp, L"deep_depth_tolerance", DBL_MAX);
   m_deep_depth_half_precision    = (bool) ParAcc_GetValue(in_cp, L"deep_depth_half_precision", DBL_MAX);

   for (int i=0; i<NB_MAX_LAYERS; i++)
   {
      m_deep_layer_name[i]             = ParAcc_GetValue(in_cp, L"deep_layer_name" + ((CValue)i).GetAsText(), DBL_MAX).GetAsText();
      m_deep_layer_tolerance[i]        = (float)ParAcc_GetValue(in_cp, L"deep_layer_tolerance" + ((CValue)i).GetAsText(), DBL_MAX);
      m_deep_layer_enable_filtering[i] = (bool)ParAcc_GetValue(in_cp, L"deep_layer_enable_filtering" + ((CValue)i).GetAsText(), DBL_MAX);
   }

   CString pName;
   for (int i=0; i<NB_EXR_METADATA; i++)
   {
      pName = L"exr_metadata_name" + ((CValue)i).GetAsText();
      m_exr_metadata_name[i] = ParAcc_GetValue(in_cp, pName, DBL_MAX).GetAsText();
      pName = L"exr_metadata_type" + ((CValue)i).GetAsText();
      m_exr_metadata_type[i] = (int)ParAcc_GetValue(in_cp, pName, DBL_MAX);
      pName = L"exr_metadata_value" + ((CValue)i).GetAsText();
      m_exr_metadata_value[i] = ParAcc_GetValue(in_cp, pName, DBL_MAX).GetAsText();
   }

   // sampling
   m_AA_samples              = (int)ParAcc_GetValue(in_cp, L"AA_samples",              DBL_MAX);
   m_GI_diffuse_samples      = (int)ParAcc_GetValue(in_cp, L"GI_diffuse_samples",      DBL_MAX);
   m_GI_specular_samples     = (int)ParAcc_GetValue(in_cp, L"GI_specular_samples",     DBL_MAX);
   m_GI_transmission_samples = (int)ParAcc_GetValue(in_cp, L"GI_transmission_samples", DBL_MAX);
   m_GI_sss_samples          = (int)ParAcc_GetValue(in_cp, L"GI_sss_samples",          DBL_MAX);
   m_GI_volume_samples       = (int)ParAcc_GetValue(in_cp, L"GI_volume_samples",       DBL_MAX);

   m_enable_adaptive_sampling = (bool)ParAcc_GetValue(in_cp,  L"enable_adaptive_sampling", DBL_MAX);
   m_AA_samples_max           = (int)ParAcc_GetValue(in_cp,   L"AA_samples_max",           DBL_MAX);
   m_AA_adaptive_threshold    = (float)ParAcc_GetValue(in_cp, L"AA_adaptive_threshold",    DBL_MAX);

   m_indirect_specular_blur  = (float)ParAcc_GetValue(in_cp, L"indirect_specular_blur", DBL_MAX);

   m_lock_sampling_noise = (bool)ParAcc_GetValue(in_cp, L"lock_sampling_noise", DBL_MAX);

   m_sss_use_autobump = (bool)ParAcc_GetValue(in_cp, L"sss_use_autobump", DBL_MAX);

   m_use_sample_clamp        = (bool) ParAcc_GetValue(in_cp, L"use_sample_clamp",        DBL_MAX);
   m_use_sample_clamp_AOVs   = (bool) ParAcc_GetValue(in_cp, L"use_sample_clamp_AOVs",   DBL_MAX);
   m_AA_sample_clamp         = (float)ParAcc_GetValue(in_cp, L"AA_sample_clamp",         DBL_MAX);
   
   if (ParAcc_Valid(in_cp, "indirect_sample_clamp"))
      m_indirect_sample_clamp = (float)ParAcc_GetValue(in_cp, L"indirect_sample_clamp",   DBL_MAX);

   m_output_filter           = ParAcc_GetValue(in_cp, L"output_filter", DBL_MAX).GetAsText();
   m_output_filter_width     = (float)ParAcc_GetValue(in_cp, L"output_filter_width", DBL_MAX);
   m_filter_color_AOVs = (bool)ParAcc_GetValue(in_cp,  L"filter_color_AOVs", DBL_MAX);
   m_filter_numeric_AOVs = (bool)ParAcc_GetValue(in_cp, L"filter_numeric_AOVs", DBL_MAX);

   // motion blur
   m_enable_motion_blur     = (bool)ParAcc_GetValue(in_cp, L"enable_motion_blur",    DBL_MAX);
   m_motion_step_transform  = (int) ParAcc_GetValue(in_cp, L"motion_step_transform", DBL_MAX);
   m_enable_motion_deform   = (bool)ParAcc_GetValue(in_cp, L"enable_motion_deform",  DBL_MAX);
   m_motion_step_deform     = (int) ParAcc_GetValue(in_cp, L"motion_step_deform",    DBL_MAX);
   m_exact_ice_mb           = (bool)ParAcc_GetValue(in_cp, L"exact_ice_mb",          DBL_MAX);
   
   m_motion_shutter_length       = (float)ParAcc_GetValue(in_cp, L"motion_shutter_length",       DBL_MAX);
   m_motion_shutter_custom_start = (float)ParAcc_GetValue(in_cp, L"motion_shutter_custom_start", DBL_MAX);
   m_motion_shutter_custom_end   = (float)ParAcc_GetValue(in_cp, L"motion_shutter_custom_end",   DBL_MAX);
   m_motion_shutter_onframe      = (int)ParAcc_GetValue(in_cp, L"motion_shutter_onframe", DBL_MAX); // was a string, #1634

   // subdivision
   m_max_subdivisions  = (int)ParAcc_GetValue(in_cp, L"max_subdivisions",  DBL_MAX);

   m_adaptive_error = (float)ParAcc_GetValue(in_cp, L"adaptive_error", DBL_MAX);

   m_use_dicing_camera = (bool) ParAcc_GetValue(in_cp, L"use_dicing_camera", DBL_MAX);
   m_dicing_camera     = ParAcc_GetValue(in_cp,        L"dicing_camera",     DBL_MAX); // plain CValue

   // ray depth
   m_GI_total_depth              = (int)  ParAcc_GetValue(in_cp, L"GI_total_depth",              DBL_MAX);
   m_GI_diffuse_depth            = (int)  ParAcc_GetValue(in_cp, L"GI_diffuse_depth",            DBL_MAX);
   m_GI_specular_depth           = (int)  ParAcc_GetValue(in_cp, L"GI_specular_depth",           DBL_MAX);
   m_GI_transmission_depth       = (int)  ParAcc_GetValue(in_cp, L"GI_transmission_depth",       DBL_MAX);
   m_GI_volume_depth             = (int)  ParAcc_GetValue(in_cp, L"GI_volume_depth",             DBL_MAX);
   m_auto_transparency_depth     = (int)  ParAcc_GetValue(in_cp, L"auto_transparency_depth",     DBL_MAX);
   m_low_light_threshold = (float)ParAcc_GetValue(in_cp, L"low_light_threshold", DBL_MAX);

   // textures
   m_texture_accept_unmipped = (bool)ParAcc_GetValue(in_cp,  L"texture_accept_unmipped", DBL_MAX);
   m_texture_automip         = (bool)ParAcc_GetValue(in_cp,  L"texture_automip",         DBL_MAX);
   m_texture_filter          = (int)ParAcc_GetValue(in_cp,   L"texture_filter",          DBL_MAX);
   m_texture_accept_untiled  = (bool)ParAcc_GetValue(in_cp,  L"texture_accept_untiled",  DBL_MAX);
   m_enable_autotile         = (bool)ParAcc_GetValue(in_cp,  L"enable_autotile",         DBL_MAX);
   m_texture_autotile        = (int)ParAcc_GetValue(in_cp,   L"texture_autotile",        DBL_MAX);
   m_use_existing_tx_files   = (bool)ParAcc_GetValue(in_cp,  L"use_existing_tx_files",   DBL_MAX);
   m_texture_max_memory_MB   = (int)ParAcc_GetValue(in_cp,   L"texture_max_memory_MB",   DBL_MAX);
   m_texture_max_open_files  = (int)ParAcc_GetValue(in_cp,   L"texture_max_open_files",  DBL_MAX);

   // color managers
   m_color_manager              = ParAcc_GetValue(in_cp, L"color_manager",              DBL_MAX).GetAsText();
   m_ocio_config                = ParAcc_GetValue(in_cp, L"ocio_config",                DBL_MAX).GetAsText();
   m_ocio_color_space_narrow    = ParAcc_GetValue(in_cp, L"ocio_color_space_narrow",    DBL_MAX).GetAsText();
   m_ocio_color_space_linear    = ParAcc_GetValue(in_cp, L"ocio_color_space_linear",    DBL_MAX).GetAsText();
   m_ocio_linear_chromaticities = ParAcc_GetValue(in_cp, L"ocio_linear_chromaticities", DBL_MAX).GetAsText();

   // diagnostic
   m_enable_log_console     = (bool)ParAcc_GetValue(in_cp, L"enable_log_console",     DBL_MAX);
   m_enable_log_file        = (bool)ParAcc_GetValue(in_cp, L"enable_log_file",        DBL_MAX);
   m_log_level              = (int) ParAcc_GetValue(in_cp, L"log_level",              DBL_MAX); // was a string, #1634
   m_max_log_warning_msgs   = (int) ParAcc_GetValue(in_cp,  L"max_log_warning_msgs",  DBL_MAX);
   m_texture_per_file_stats = (bool)ParAcc_GetValue(in_cp, L"texture_per_file_stats", DBL_MAX);
   m_output_file_tagdir_log = ParAcc_GetValue(in_cp,       L"output_file_tagdir_log", DBL_MAX).GetAsText();
   m_ignore_textures        = (bool)ParAcc_GetValue(in_cp, L"ignore_textures",        DBL_MAX);
   m_ignore_shaders         = (bool)ParAcc_GetValue(in_cp, L"ignore_shaders",         DBL_MAX);
   m_ignore_atmosphere      = (bool)ParAcc_GetValue(in_cp, L"ignore_atmosphere",      DBL_MAX);
   m_ignore_lights          = (bool)ParAcc_GetValue(in_cp, L"ignore_lights",          DBL_MAX);
   m_ignore_shadows         = (bool)ParAcc_GetValue(in_cp, L"ignore_shadows",         DBL_MAX);
   m_ignore_subdivision     = (bool)ParAcc_GetValue(in_cp, L"ignore_subdivision",     DBL_MAX);
   m_ignore_displacement    = (bool)ParAcc_GetValue(in_cp, L"ignore_displacement",    DBL_MAX);
   m_ignore_bump            = (bool)ParAcc_GetValue(in_cp, L"ignore_bump",            DBL_MAX);
   m_ignore_smoothing       = (bool)ParAcc_GetValue(in_cp, L"ignore_smoothing",       DBL_MAX);
   m_ignore_motion_blur     = (bool)ParAcc_GetValue(in_cp, L"ignore_motion_blur",     DBL_MAX);
   m_ignore_dof             = (bool)ParAcc_GetValue(in_cp, L"ignore_dof", DBL_MAX);
   m_ignore_sss             = (bool)ParAcc_GetValue(in_cp, L"ignore_sss", DBL_MAX);
   m_ignore_hair            = (bool)ParAcc_GetValue(in_cp, L"ignore_hair", DBL_MAX);
   m_ignore_pointclouds     = (bool)ParAcc_GetValue(in_cp, L"ignore_pointclouds", DBL_MAX);
   m_ignore_procedurals     = (bool)ParAcc_GetValue(in_cp, L"ignore_procedurals", DBL_MAX);
   m_ignore_user_options    = (bool)ParAcc_GetValue(in_cp, L"ignore_user_options", DBL_MAX);
   m_ignore_matte           = (bool)ParAcc_GetValue(in_cp, L"ignore_matte", DBL_MAX);

   // ass archive
   m_output_file_tagdir_ass = ParAcc_GetValue(in_cp,       L"output_file_tagdir_ass", DBL_MAX).GetAsText();
   m_compress_output_ass    = (bool)ParAcc_GetValue(in_cp, L"compress_output_ass",    DBL_MAX);
   m_binary_ass = (bool)ParAcc_GetValue(in_cp, L"binary_ass", DBL_MAX);
   m_save_texture_paths  = (bool)ParAcc_GetValue(in_cp, L"save_texture_paths",  DBL_MAX);
   m_save_procedural_paths = (bool)ParAcc_GetValue(in_cp, L"save_procedural_paths", DBL_MAX);
   m_use_path_translations = (bool)ParAcc_GetValue(in_cp, L"use_path_translations", DBL_MAX);
   m_open_procs = (bool)ParAcc_GetValue(in_cp, L"open_procs", DBL_MAX);
   m_output_options = (bool)ParAcc_GetValue(in_cp, L"output_options", DBL_MAX);
   m_output_drivers_filters = (bool)ParAcc_GetValue(in_cp, L"output_drivers_filters", DBL_MAX);
   m_output_geometry = (bool)ParAcc_GetValue(in_cp, L"output_geometry", DBL_MAX);
   m_output_cameras = (bool)ParAcc_GetValue(in_cp, L"output_cameras", DBL_MAX);
   m_output_lights = (bool)ParAcc_GetValue(in_cp, L"output_lights", DBL_MAX);
   m_output_shaders = (bool)ParAcc_GetValue(in_cp, L"output_shaders", DBL_MAX);
}


// Render Options and Preferences
// The render preferences used to be a js plugin. Since the 2 props are equal, let unify them
// with some CommonRenderOptions_* wrappers

// Render Options

SITOA_CALLBACK ArnoldRenderOptions_Define(CRef& in_ctxt)
{
   return CommonRenderOptions_Define(in_ctxt);
}


SITOA_CALLBACK ArnoldRenderOptions_DefineLayout(CRef& in_ctxt)
{
   return CommonRenderOptions_DefineLayout(in_ctxt);
}


SITOA_CALLBACK ArnoldRenderOptions_PPGEvent(const CRef& in_ctxt)
{
   return CommonRenderOptions_PPGEvent(in_ctxt);
}


// Render Preferences, now (js->cpp) registered by sitoa.cpp

SITOA_CALLBACK ArnoldRenderPreferences_Define(CRef& in_ctxt)
{
   return CommonRenderOptions_Define(in_ctxt);
}


SITOA_CALLBACK ArnoldRenderPreferences_DefineLayout(CRef& in_ctxt)
{
   return CommonRenderOptions_DefineLayout(in_ctxt);
}


SITOA_CALLBACK ArnoldRenderPreferences_PPGEvent(const CRef& in_ctxt)
{
   return CommonRenderOptions_PPGEvent(in_ctxt);
}


SITOA_CALLBACK CommonRenderOptions_Define(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   CustomProperty cpset(ctxt.GetSource());
   Application app;

   // Default Paths
   CString defaultPluginsPath(L""), defaultProceduralsPath(L""); // init to void for #1255
   CString defaultTexturesPath = CUtils::BuildPath(app.GetInstallationPath(siProjectPath), L"Pictures");
   CString defaultAssPath      = CUtils::BuildPath(L"[Project Path]", L"Arnold_Scenes"); // Ass Path
   CString defaultLogPath      = CUtils::BuildPath(L"[Project Path]", L"Arnold_Logs");   // Log Path

   // shaders path
   char* aux = getenv("SITOA_SHADERS_PATH");
   if (aux) // else stays ""
      defaultPluginsPath.PutAsciiString(aux);
   
   // procedurals path
   aux = getenv("SITOA_PROCEDURALS_PATH");
   if (aux) // else stays ""
      defaultProceduralsPath.PutAsciiString(aux);

   Parameter p;
   // system
   cpset.AddParameter(L"autodetect_threads",     CValue::siBool,   siPersistable, L"", L"", true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"threads",                CValue::siInt4,   siPersistable, L"", L"", 4, -AI_MAX_THREADS, AI_MAX_THREADS, 1, AI_MAX_THREADS, p);
   cpset.AddParameter(L"bucket_scanning",        CValue::siString, siPersistable, L"", L"", L"spiral", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"bucket_size",            CValue::siInt4,   siPersistable, L"", L"",  64, 16, 256, 16, 256, p);
   cpset.AddParameter(L"progressive_minus3",     CValue::siBool,   siPersistable, L"", L"",  true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"progressive_minus2",     CValue::siBool,   siPersistable, L"", L"",  true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"progressive_minus1",     CValue::siBool,   siPersistable, L"", L"",  true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"progressive_plus1",      CValue::siBool,   siPersistable, L"", L"",  true, CValue(), CValue(), CValue(), CValue(), p);
   
   cpset.AddParameter(L"ipr_rebuild_mode",       CValue::siInt4,   siPersistable, L"", L"",  eIprRebuildMode_Auto, eIprRebuildMode_Auto, eIprRebuildMode_Flythrough, eIprRebuildMode_Auto, eIprRebuildMode_Flythrough, p);
   
   cpset.AddParameter(L"skip_license_check",     CValue::siBool,   siPersistable, L"", L"",  false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"abort_on_license_fail",  CValue::siBool,   siPersistable, L"", L"",  false, CValue(), CValue(), CValue(), CValue(), p);    
   cpset.AddParameter(L"abort_on_error",         CValue::siBool,   siPersistable, L"", L"",  true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"error_color_bad_mapR",   CValue::siDouble, siPersistable, L"", L"",  1.0, 0.0, 1.0, 0.0, 1.0, p);
   cpset.AddParameter(L"error_color_bad_mapG",   CValue::siDouble, siPersistable, L"", L"",  0.0, 0.0, 1.0, 0.0, 1.0, p);
   cpset.AddParameter(L"error_color_bad_mapB",   CValue::siDouble, siPersistable, L"", L"",  0.0, 0.0, 1.0, 0.0, 1.0, p);
   cpset.AddParameter(L"error_color_bad_pixR",   CValue::siDouble, siPersistable, L"", L"",  0.0, 0.0, 1.0, 0.0, 1.0, p);
   cpset.AddParameter(L"error_color_bad_pixG",   CValue::siDouble, siPersistable, L"", L"",  1.0, 0.0, 1.0, 0.0, 1.0, p);
   cpset.AddParameter(L"error_color_bad_pixB",   CValue::siDouble, siPersistable, L"", L"",  0.0, 0.0, 1.0, 0.0, 1.0, p);
   cpset.AddParameter(L"plugins_path",           CValue::siString, siPersistable, L"", L"",  defaultPluginsPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"procedurals_path",       CValue::siString, siPersistable, L"", L"",  defaultProceduralsPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"textures_path",          CValue::siString, siPersistable, L"", L"",  defaultTexturesPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"user_options",           CValue::siString, siPersistable, L"", L"",  L"", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"resolve_tokens",         CValue::siBool,   siPersistable, L"", L"",  false, CValue(), CValue(), CValue(), CValue(), p);

   // output
   cpset.AddParameter(L"overscan",        CValue::siBool,   siPersistable, L"", L"", false,   CValue(), CValue(), CValue(), CValue(), p);
   // overscan margins
   cpset.AddParameter(L"overscan_top",    CValue::siInt4,   siPersistable | siAnimatable, L"", L"", 10, 0, 10000, 0, 10000, p);
   cpset.AddParameter(L"overscan_bottom", CValue::siInt4,   siPersistable | siAnimatable, L"", L"", 10, 0, 10000, 0, 10000, p);
   cpset.AddParameter(L"overscan_left",   CValue::siInt4,   siPersistable | siAnimatable, L"", L"", 10, 0, 10000, 0, 10000, p);
   cpset.AddParameter(L"overscan_right",  CValue::siInt4,   siPersistable | siAnimatable, L"", L"", 10, 0, 10000, 0, 10000, p);

   cpset.AddParameter(L"output_driver_color_space", CValue::siString, siPersistable, L"", L"", L"auto", CValue(), CValue(), CValue(), CValue(), p);

   cpset.AddParameter(L"dither",                         CValue::siBool,   siPersistable, L"", L"", true,   CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"unpremult_alpha",                CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_tiff_tiled",              CValue::siBool,   siPersistable, L"", L"", true,   CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_tiff_compression",        CValue::siString, siPersistable, L"", L"", L"lzw", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_tiff_append",             CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_exr_tiled",               CValue::siBool,   siPersistable, L"", L"", true,   CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_exr_compression",         CValue::siString, siPersistable, L"", L"", L"zip", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_exr_preserve_layer_name", CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_exr_autocrop",            CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_exr_append",              CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);

   // add 20 rows, showing only te first 3
   for (int i=0; i<NB_EXR_METADATA; i++)
   {
      cpset.AddParameter(L"exr_metadata_name"  + ((CValue)i).GetAsText(), CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
      if (i > 2) // show only 3 metadata
         p.PutCapabilityFlag(siNotInspectable, true);

      int defaultType = 0;
      if (i==1) defaultType = 1;      // float for the second row
      else if (i==2) defaultType = 3; // string for the third row

      cpset.AddParameter(L"exr_metadata_type"  + ((CValue)i).GetAsText(), CValue::siInt4, siPersistable, L"", L"", defaultType, 0, 4, 0, 4, p);
      if (i>2)
         p.PutCapabilityFlag(siNotInspectable, true);
      
      cpset.AddParameter(L"exr_metadata_value" + ((CValue)i).GetAsText(), CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
      if (i>2)
         p.PutCapabilityFlag(siNotInspectable, true);
   }
   
   // deep exr
   cpset.AddParameter(L"deep_exr_enable",           CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"deep_subpixel_merge",       CValue::siBool,   siPersistable, L"", L"", true,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"deep_use_RGB_opacity",      CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"deep_alpha_tolerance",      CValue::siDouble, siPersistable, L"", L"", 0.01, 0.0, 1.0, 0.0, 100000.0, p);
   cpset.AddParameter(L"deep_alpha_half_precision", CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"deep_depth_tolerance",      CValue::siDouble, siPersistable, L"", L"", 0.01, 0.0, 1.0, 0.0, 100000.0, p);
   cpset.AddParameter(L"deep_depth_half_precision", CValue::siBool,   siPersistable, L"", L"", false,  CValue(), CValue(), CValue(), CValue(), p);
   // layers' tolerance
   for (int i=0; i<NB_MAX_LAYERS; i++)
   {
      cpset.AddParameter(L"deep_layer_name" + ((CValue)i).GetAsText(), CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
      p.PutCapabilityFlag(siNotInspectable, true);
      cpset.AddParameter(L"deep_layer_tolerance" + ((CValue)i).GetAsText(), CValue::siDouble, siPersistable, L"", L"", 0.0, 0.0, 100000.0, 0.0, 1.0, p);
      p.PutCapabilityFlag(siNotInspectable, true);
      cpset.AddParameter(L"deep_layer_enable_filtering" + ((CValue)i).GetAsText(), CValue::siBool, siPersistable, L"", L"", true, CValue(), CValue(), CValue(), CValue(), p);
      p.PutCapabilityFlag(siNotInspectable, true);
   }

   // sampling
   cpset.AddParameter(L"AA_samples",              CValue::siInt4,   siPersistable, L"", L"", 3, -3, 100, 0, 10, p);
   cpset.AddParameter(L"GI_diffuse_samples",      CValue::siInt4,   siPersistable, L"", L"", 2, 0, 100, 0, 10, p);
   cpset.AddParameter(L"GI_specular_samples",     CValue::siInt4,   siPersistable, L"", L"", 2, 0, 100, 0, 10, p);
   cpset.AddParameter(L"GI_transmission_samples", CValue::siInt4,   siPersistable, L"", L"", 2, 0, 100, 0, 10, p);
   cpset.AddParameter(L"GI_sss_samples",          CValue::siInt4,   siPersistable, L"", L"", 2, 0, 100, 0, 10, p);
   cpset.AddParameter(L"GI_volume_samples",       CValue::siInt4,   siPersistable, L"", L"", 2, 0, 100, 0, 10, p);

   cpset.AddParameter(L"enable_adaptive_sampling", CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"AA_samples_max",           CValue::siInt4,   siPersistable, L"", L"", 8, -3, 100, 0, 10, p);
   cpset.AddParameter(L"AA_adaptive_threshold",    CValue::siDouble, siPersistable, L"", L"", 0.05f, 0.0f, 1.0f, 0.0f, 100.0f, p);

   cpset.AddParameter(L"indirect_specular_blur",  CValue::siDouble, siPersistable | siAnimatable, L"", L"", 1.0f, 0.0f, 2.0f, 0.0f, 100.0f, p);
   

   cpset.AddParameter(L"lock_sampling_noise",     CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"sss_use_autobump",        CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"use_sample_clamp",        CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"use_sample_clamp_AOVs",   CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"AA_sample_clamp",         CValue::siDouble, siPersistable, L"", L"", 10, 0.001, 100, 0.001, 100, p);
   cpset.AddParameter(L"indirect_sample_clamp",   CValue::siDouble, siPersistable, L"", L"", 10, 0.0, 100, 0.0, 100, p);
   cpset.AddParameter(L"output_filter",           CValue::siString, siPersistable, L"", L"", L"gaussian", 0, 10, 0, 10, p);
   cpset.AddParameter(L"output_filter_width",     CValue::siDouble, siPersistable, L"", L"", 2, 0, 100, 1, 6, p);
   cpset.AddParameter(L"filter_color_AOVs",       CValue::siBool,   siPersistable, L"", L"", true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"filter_numeric_AOVs",     CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);

   // motion blur
   cpset.AddParameter(L"enable_motion_blur",     CValue::siBool,   siPersistable,                L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"motion_step_transform",  CValue::siInt4,   siPersistable,                L"", L"", 2, 2, 200, 2, 15, p);
   cpset.AddParameter(L"enable_motion_deform",   CValue::siBool,   siPersistable,                L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"motion_step_deform",     CValue::siInt4,   siPersistable,                L"", L"", 2, 2, 200, 2, 15, p);
   cpset.AddParameter(L"exact_ice_mb",           CValue::siBool,   siPersistable,                L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"motion_shutter_length",  CValue::siDouble, siPersistable | siAnimatable, L"", L"", 0.5f ,  0, 999999, 0, 2, p);
   cpset.AddParameter(L"motion_shutter_custom_start", CValue::siDouble, siPersistable | siAnimatable, L"", L"", -0.25f , -100, 100, -100, 100, p);
   cpset.AddParameter(L"motion_shutter_custom_end",   CValue::siDouble, siPersistable | siAnimatable, L"", L"", 0.25f , -100, 100, -100, 100, p);
   cpset.AddParameter(L"motion_shutter_onframe", CValue::siInt4,   siPersistable,                L"", L"", eMbPos_Center, eMbPos_Start, eMbPos_Custom, eMbPos_Start, eMbPos_Custom, p);

   // subdivision
   cpset.AddParameter(L"max_subdivisions",  CValue::siInt4,   siPersistable, L"", L"", 255, 0, 255, 0, 255, p);
   cpset.AddParameter(L"adaptive_error",    CValue::siDouble, siPersistable, L"", L"", 0.0f, 0, 50, 0, 10, p);
   cpset.AddParameter(L"use_dicing_camera", CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"dicing_camera",     CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);

   // ray depth
   cpset.AddParameter(L"GI_total_depth",              CValue::siInt4,   siPersistable, L"", L"", 10, 0, 10000, 0, 16, p);
   cpset.AddParameter(L"GI_diffuse_depth",            CValue::siInt4,   siPersistable, L"", L"", 1, 0, 10000, 0, 16, p);
   cpset.AddParameter(L"GI_specular_depth",           CValue::siInt4,   siPersistable, L"", L"", 1, 0, 10000, 0, 16, p);   
   cpset.AddParameter(L"GI_transmission_depth",       CValue::siInt4,   siPersistable, L"", L"", 8, 0, 10000, 0, 16, p);
   cpset.AddParameter(L"GI_volume_depth",             CValue::siInt4,   siPersistable, L"", L"", 0, 0, 10000, 0, 16, p);
   cpset.AddParameter(L"auto_transparency_depth",     CValue::siInt4,   siPersistable, L"", L"", 10, 0, 10000, 0, 16, p);
   cpset.AddParameter(L"low_light_threshold",         CValue::siDouble, siPersistable, L"", L"", 0.001, 0, 10000, 0.001, 0.1, p);

   // textures
   cpset.AddParameter(L"texture_accept_unmipped", CValue::siBool,   siPersistable, L"", L"", true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"texture_automip",         CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"texture_filter",          CValue::siInt4,   siPersistable, L"", L"", AI_TEXTURE_SMART_BICUBIC, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"texture_accept_untiled",  CValue::siBool,   siPersistable, L"", L"", true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"enable_autotile",         CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"texture_autotile",        CValue::siInt4,   siPersistable, L"", L"", 64, 16, 1024, 16, 512, p);
   cpset.AddParameter(L"use_existing_tx_files",   CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"texture_max_memory_MB",   CValue::siInt4,   siPersistable, L"", L"", 2048, 128, CValue(), 128, 4096, p);
   cpset.AddParameter(L"texture_max_open_files",  CValue::siInt4,   siPersistable, L"", L"", 0, 0, 10000, 0, 2000, p);

   // color managers
   cpset.AddParameter(L"color_manager",              CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ocio_config",                CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ocio_config_message",        CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ocio_color_space_narrow",    CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ocio_color_space_linear",    CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ocio_linear_chromaticities", CValue::siString, siPersistable, L"", L"", L"", CValue(), CValue(), CValue(), CValue(), p);

   // diagnostic:
   cpset.AddParameter(L"enable_log_console",     CValue::siBool,   siPersistable, L"", L"", true, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"enable_log_file",        CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"log_level",              CValue::siInt4,   siPersistable, L"", L"", eSItoALogLevel_Warnings, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"max_log_warning_msgs",   CValue::siInt4,   siPersistable, L"", L"", 5, 0, 9999, 0, 9999, p);
   cpset.AddParameter(L"texture_per_file_stats", CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_file_tagdir_log", CValue::siString, siPersistable, L"", L"", defaultLogPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_file_dir_log",    CValue::siString, siPersistable, L"", L"", defaultLogPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_textures",        CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_shaders",         CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_atmosphere",      CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_lights",          CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_shadows",         CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_subdivision",     CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_displacement",    CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_bump",            CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_smoothing",       CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_motion_blur",     CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_dof",             CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_sss",             CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_hair",            CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_pointclouds",     CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_procedurals",     CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_user_options",    CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"ignore_matte",           CValue::siBool,   siPersistable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"show_samples",           CValue::siString, siPersistable, L"", L"", L"off", 0, 10, 0, 10, p);

   // ass archive
   cpset.AddParameter(L"output_file_tagdir_ass", CValue::siString, siPersistable, L"", L"", defaultAssPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_file_dir_ass",    CValue::siString, siPersistable, L"", L"", defaultAssPath, CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"compress_output_ass",    CValue::siBool,   siPersistable, L"", L"", false,          CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"binary_ass",             CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"save_texture_paths",     CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"save_procedural_paths",  CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"use_path_translations",  CValue::siBool,   siPersistable, L"", L"", false,          CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"open_procs",             CValue::siBool,   siPersistable, L"", L"", false,          CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_options",         CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_drivers_filters", CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_geometry",        CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_cameras",         CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_lights",          CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);
   cpset.AddParameter(L"output_shaders",         CValue::siBool,   siPersistable, L"", L"", true,           CValue(), CValue(), CValue(), CValue(), p);

   // the hidden version string saved with the scene
   cpset.AddParameter(L"sitoa_version",          CValue::siString, siPersistable, L"", L"", L"",            CValue(), CValue(), CValue(), CValue(), p);
   p.PutCapabilityFlag(siNotInspectable, true);

   // since this prop definition is shared by the render option and the render preferences,
   // let's bail out in the latter case. For render options, instead, the render preferences
   // must be cloned
   if (cpset.GetName() == L"Arnold Render Preferences")
      return CStatus::OK;

   // Getting Arnold Render Preferences to get all Default values
   // After fighting with XSI bugs some hours this is the best choice to 
   // read preferences and assign them as default values
   // If they are not found we wont do anything.
   Property renderPrefs;
   Preferences xsiPrefs = app.GetPreferences();
   xsiPrefs.GetCategories().Find(L"ArnoldRenderPreferences", renderPrefs);

   if (renderPrefs.IsValid())
   {
      CParameterRefArray paramsPrefs = renderPrefs.GetParameters();
      int nparams = paramsPrefs.GetCount();
      for (int i=0; i<nparams; i++)
      {
         Parameter paramPref = paramsPrefs[i];
         Parameter paramOption = ParAcc_GetParameter(cpset,paramPref.GetScriptName());
         if (paramOption.IsValid())
         {
            CValue prefValue;
            xsiPrefs.GetPreferenceValue(L"Arnold Render." + paramPref.GetScriptName(), prefValue);
            int dataType = paramOption.GetValueType();
            switch(dataType)
            {
               // case siFloat: //siFloat should never be the case
               case siDouble:
               {
                  double value = atof(prefValue.GetAsText().GetAsciiString());
                  cpset.PutParameterValue(paramPref.GetScriptName(), value);
                  break;
               }
               case siInt4:
               {
                  LONG value = (LONG) atoi(prefValue.GetAsText().GetAsciiString());
                  cpset.PutParameterValue(paramPref.GetScriptName(), value);
                  break;
               }
               case siString:
               {
                  cpset.PutParameterValue(paramPref.GetScriptName(), prefValue.GetAsText());
                  break;
               }
               case siBool:
               {
                  cpset.PutParameterValue(paramPref.GetScriptName(), (bool)prefValue);
                  break;
               }
               default:
                  cpset.PutParameterValue(paramPref.GetScriptName(), prefValue);
            }
         }
      }
   }

   return CStatus::OK;
}


SITOA_CALLBACK CommonRenderOptions_DefineLayout(CRef& in_ctxt)
{
   Context ctxt(in_ctxt);
   PPGLayout layout(ctxt.GetSource());
   layout.Clear();

   layout.PutAttribute(siUIHelpFile, L"https://support.solidangle.com/display/A5SItoAUG/Arnold+Render+Options");

   PPGItem item;

   layout.AddTab(L"System");
   layout.AddSpacer(5, 5);  
   layout.AddGroup(L"Multi-threading", true, 0);  
      layout.AddItem(L"autodetect_threads", L"Autodetect");
      item = layout.AddItem(L"threads", L"Number of Threads");
      item.PutAttribute(siUILabelPercentage, 100);
   layout.EndGroup();
   layout.AddGroup(L"Buckets", true, 0);
      CValueArray scanning;
      scanning.Add(L"top");     scanning.Add(L"top");
      scanning.Add(L"left");    scanning.Add(L"left");
      scanning.Add(L"random");  scanning.Add(L"random");
      scanning.Add(L"spiral");  scanning.Add(L"spiral");
      scanning.Add(L"hilbert"); scanning.Add(L"hilbert");
      layout.AddRow();
      item = layout.AddEnumControl(L"bucket_scanning", scanning, L"Scanning", siControlCombo);
      item.PutAttribute(siUIWidthPercentage, 60);
      layout.AddItem(L"bucket_size",    L"Size");
      layout.EndRow();
   layout.EndGroup();
   layout.AddGroup(L"Progressive Refinement", true, 0);
      layout.AddRow();
      layout.AddGroup();
         item = layout.AddItem(L"progressive_minus3", L"-3");
      layout.EndGroup();
      layout.AddGroup();
         item = layout.AddItem(L"progressive_minus2", L"-2");
      layout.EndGroup();
      layout.AddGroup();
         item = layout.AddItem(L"progressive_minus1", L"-1");
      layout.EndGroup();
      layout.AddGroup();
         item = layout.AddItem(L"progressive_plus1", L"1");
      layout.EndGroup();
   layout.EndRow();
   layout.EndGroup();

   layout.AddGroup(L"Scene Rebuild Mode", true, 0);
      CValueArray iprMode;
      iprMode.Add(L"Auto");        iprMode.Add(eIprRebuildMode_Auto);
      iprMode.Add(L"Always");      iprMode.Add(eIprRebuildMode_Always);
      iprMode.Add(L"Manual");      iprMode.Add(eIprRebuildMode_Manual);
      iprMode.Add(L"Fly-through"); iprMode.Add(eIprRebuildMode_Flythrough);
      item = layout.AddEnumControl(L"ipr_rebuild_mode", iprMode, L"Scene Rebuild Mode", siControlCombo);
      item.PutAttribute(siUINoLabel, true);
   layout.EndGroup();
   
   layout.AddGroup(L"Licensing", true, 0);
      layout.AddItem(L"skip_license_check", L"Skip License Check");
      layout.AddItem(L"abort_on_license_fail", L"Abort On License Fail");
   layout.EndGroup();
   layout.AddGroup(L"Error Handling", true, 0);
      layout.AddItem(L"abort_on_error",        L"Abort On Error");     
      item = layout.AddColor(L"error_color_bad_pixR",     L"NaN Error Color", false);
      item.PutAttribute(siUILabelPercentage, 70);
      item = layout.AddColor(L"error_color_bad_mapR",     L"Texture Error Color", false);
      item.PutAttribute(siUILabelPercentage, 70);
   layout.EndGroup();
   layout.AddGroup(L"Search Paths", true, 0);
      layout.AddItem(L"plugins_path",     L"Plugins",     siControlFolder);
      layout.AddItem(L"procedurals_path", L"Procedurals", siControlFolder);
      layout.AddItem(L"textures_path",    L"Textures",    siControlFolder);
   layout.EndGroup();
   layout.AddGroup(L"User Options", true, 0);
      item = layout.AddItem(L"user_options",   L"Options");
      item = layout.AddItem(L"resolve_tokens",  L"Resolve Tokens");
   layout.EndGroup();

   layout.AddGroup(L"Reset Options", true, 0);
      item = layout.AddButton(L"ResetToDefault", L"Reset All The Parameters To Their Default Value");
   layout.EndGroup();

   layout.AddTab(L"Output");

   layout.AddGroup(L"Overscan (top, bottom, left, right)", true, 0);
      item = layout.AddItem(L"overscan", L"Enable ");
      layout.AddRow();
          item = layout.AddItem(L"overscan_top",    L"");
          item.PutAttribute(siUINoSlider, true);
          item.PutAttribute(siUINoLabel, true);
          item = layout.AddItem(L"overscan_bottom", L"");
          item.PutAttribute(siUINoSlider, true);
          item.PutAttribute(siUINoLabel, true);
          item = layout.AddItem(L"overscan_left",   L"");
          item.PutAttribute(siUINoSlider, true);
          item.PutAttribute(siUINoLabel, true);
          item = layout.AddItem(L"overscan_right",  L"");
          item.PutAttribute(siUINoSlider, true);
          item.PutAttribute(siUINoLabel, true);
      layout.EndRow();
   layout.EndGroup();

   layout.AddGroup(L" Color Space ", true, 0);
      item = layout.AddItem(L"output_driver_color_space", L"");
      item.PutAttribute(siUINoLabel, true);
   layout.EndGroup();

   layout.AddGroup(L"TIFF", true, 0);
      layout.AddItem(L"unpremult_alpha", L"Unpremultiplied Alpha");
      CValueArray output_tiff_tiled;
      output_tiff_tiled.Add(L"scanline");    output_tiff_tiled.Add(0);
      output_tiff_tiled.Add(L"tiled");       output_tiff_tiled.Add(1);
      item = layout.AddEnumControl(L"output_tiff_tiled", output_tiff_tiled, L"Format", siControlCombo);
      item.PutAttribute(siUILabelMinPixels, 150);
      item.PutAttribute(siUILabelPercentage, 60);
      CValueArray output_tiff_compression;
      output_tiff_compression.Add(L"none");       output_tiff_compression.Add(L"none");
      output_tiff_compression.Add(L"lzw");        output_tiff_compression.Add(L"lzw");
      output_tiff_compression.Add(L"ccittrle");   output_tiff_compression.Add(L"ccittrle");
      output_tiff_compression.Add(L"zip");        output_tiff_compression.Add(L"zip");
      output_tiff_compression.Add(L"packbits");        output_tiff_compression.Add(L"packbits");
      item = layout.AddEnumControl(L"output_tiff_compression", output_tiff_compression, L"Compression", siControlCombo);
      item.PutAttribute(siUILabelMinPixels, 150);
      item.PutAttribute(siUILabelPercentage, 60);
      layout.AddItem(L"output_tiff_append", L"Append");
   layout.EndGroup();
   layout.AddGroup(L"EXR", true, 0);
      CValueArray output_exr_tiled;
      output_exr_tiled.Add(L"scanline");    output_exr_tiled.Add(0);
      output_exr_tiled.Add(L"tiled");       output_exr_tiled.Add(1);
      item = layout.AddEnumControl(L"output_exr_tiled", output_exr_tiled, L"Format", siControlCombo);
      item.PutAttribute(siUILabelMinPixels, 150);
      item.PutAttribute(siUILabelPercentage, 60);
      CValueArray output_exr_compression;
      output_exr_compression.Add(L"none");  output_exr_compression.Add(L"none");
      output_exr_compression.Add(L"piz");   output_exr_compression.Add(L"piz");
      output_exr_compression.Add(L"pxr24"); output_exr_compression.Add(L"pxr24");
      output_exr_compression.Add(L"rle");   output_exr_compression.Add(L"rle");
      output_exr_compression.Add(L"zip");   output_exr_compression.Add(L"zip");
      output_exr_compression.Add(L"zips");  output_exr_compression.Add(L"zips");
      output_exr_compression.Add(L"b44");   output_exr_compression.Add(L"b44");
      output_exr_compression.Add(L"b44a");  output_exr_compression.Add(L"b44a");
      item = layout.AddEnumControl(L"output_exr_compression", output_exr_compression, L"Compression", siControlCombo);
      item.PutAttribute(siUILabelMinPixels, 150);
      item.PutAttribute(siUILabelPercentage, 60);
      layout.AddItem(L"output_exr_preserve_layer_name", L"Preserve Layer Name");
      layout.AddItem(L"output_exr_autocrop", L"Autocrop");
      layout.AddItem(L"output_exr_append",   L"Append");

      layout.AddGroup(L"Metadata Name/Type/Value)", true, 0);
         CValueArray metaDataTypes;
         metaDataTypes.Add(L"INT");      metaDataTypes.Add(0);
         metaDataTypes.Add(L"FLOAT");    metaDataTypes.Add(1);
         metaDataTypes.Add(L"VECTOR2");  metaDataTypes.Add(2);
         metaDataTypes.Add(L"STRING");   metaDataTypes.Add(3);
         metaDataTypes.Add(L"MATRIX");   metaDataTypes.Add(4);

         for (int i=0; i<NB_EXR_METADATA; i++)
         {
            layout.AddRow();
               item = layout.AddItem(L"exr_metadata_name" + ((CValue)i).GetAsText(), L"");
               item.PutAttribute(siUINoLabel, true);
               item.PutWidthPercentage(36);
               item = layout.AddEnumControl(L"exr_metadata_type" + ((CValue)i).GetAsText(), metaDataTypes, L"", siControlCombo);
               item.PutAttribute(siUINoLabel, true);
               item.PutWidthPercentage(28);
               item = layout.AddItem(L"exr_metadata_value" + ((CValue)i).GetAsText(), L"");
               item.PutAttribute(siUINoLabel, true);
               item.PutWidthPercentage(36);
            layout.EndRow();
         }

         layout.AddSpacer();

         layout.AddRow(); // buttom for adding/removing metadata rows
            item = layout.AddButton(L"AddMetadata",    L"Add");
            item.PutAttribute(siUICX, 140);
            item = layout.AddButton(L"RemoveMetadata", L"Remove");
            item.PutAttribute(siUICX, 140);
         layout.EndRow();

      layout.EndGroup();
      
   layout.EndGroup();

   layout.AddGroup(L"Deep EXR", true, 0);
      layout.AddItem(L"deep_exr_enable", L"Enable");
      layout.AddItem(L"deep_subpixel_merge",  L"Subpixel Merge");
      layout.AddItem(L"deep_use_RGB_opacity", L"Use RGB Opacity");

      layout.AddGroup(L"Alpha", true, 0);
         layout.AddRow();
            item = layout.AddItem(L"deep_alpha_tolerance", L"Tolerance"); 
            item.PutAttribute(siUINoSlider, true);
            item = layout.AddItem(L"deep_alpha_half_precision", L"Half Precision");
         layout.EndRow();
      layout.EndGroup();

      layout.AddGroup(L"Depth", true, 0);
         layout.AddRow();
            item = layout.AddItem(L"deep_depth_tolerance", L"Tolerance"); 
            item.PutAttribute(siUINoSlider, true);
            item = layout.AddItem(L"deep_depth_half_precision", L"Half Precision"); 
         layout.EndRow();
      layout.EndGroup();
   
      layout.AddGroup(L"Layers Tolerance / Filtering", true, 0);
         for (int i=0; i<NB_MAX_LAYERS; i++) // place them here, the inspectability is defined dynamically
         {
            layout.AddRow();
               item = layout.AddItem(L"deep_layer_tolerance" + ((CValue)i).GetAsText(), L"");
               item.PutWidthPercentage(90);
               item = layout.AddItem(L"deep_layer_enable_filtering" + ((CValue)i).GetAsText(), L"");
               item.PutWidthPercentage(10);
               item.PutAttribute(siUINoLabel, true);
            layout.EndRow();
         }
      layout.EndGroup();

   layout.EndGroup();         

   item = layout.AddItem(L"dither", L"Dither LDR Images");
   item.PutAttribute(siUILabelPercentage, 110);

   layout.AddTab(L"Sampling");
   layout.AddGroup(L"Samples");
      item = layout.AddItem(L"AA_samples", L"Camera (AA)");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"GI_diffuse_samples", L"Diffuse");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"GI_specular_samples", L"Specular");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"GI_transmission_samples", L"Transmission");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"GI_sss_samples", L"SSS");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"GI_volume_samples", L"Volume");
      item.PutAttribute(siUILabelPercentage, 100);
   layout.EndGroup();

   layout.AddGroup(L"Adaptive Sampling");
      layout.AddItem(L"enable_adaptive_sampling",  L"Enable");
      item = layout.AddItem(L"AA_samples_max", L"Max. Camera (AA)");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"AA_adaptive_threshold", L"Adaptive Threshold");
      item.PutAttribute(siUILabelPercentage, 100);
   layout.EndGroup();

   item = layout.AddItem(L"indirect_specular_blur",  L"Indirect Specular Blur");
   item.PutAttribute(siUILabelPercentage, 70);
   
   layout.AddItem(L"lock_sampling_noise",  L"Lock Sampling Pattern");
   layout.AddItem(L"sss_use_autobump",  L"Use Autobump in SSS");
   layout.AddGroup(L"Clamping", true, 0);
      layout.AddRow();
         layout.AddItem(L"use_sample_clamp",     L"Clamp Sample Values");
         layout.AddItem(L"use_sample_clamp_AOVs", L"Affect AOVs");
      layout.EndRow();
      item = layout.AddItem(L"AA_sample_clamp",       L"Max. Value");
      item.PutAttribute(siUILabelPercentage, 100);
      item = layout.AddItem(L"indirect_sample_clamp", L"Indirect Sample Clamp");
      item.PutAttribute(siUILabelPercentage, 100);
   layout.EndGroup();

   layout.AddGroup(L"Pixel Filtering", true, 0);
      CValueArray filters;
      filters.Add(L"blackman_harris"); filters.Add(L"blackman_harris");
      filters.Add(L"box");        filters.Add(L"box");
      filters.Add(L"catmull-rom");filters.Add(L"catrom");
      filters.Add(L"contour");filters.Add(L"contour");
      filters.Add(L"gaussian");   filters.Add(L"gaussian");
      filters.Add(L"mitchell-netravali"); filters.Add(L"mitnet");
      filters.Add(L"sinc"); filters.Add(L"sinc");
      filters.Add(L"triangle");   filters.Add(L"triangle");       
      filters.Add(L"variance");   filters.Add(L"variance");
      layout.AddEnumControl(L"output_filter", filters, L"Type", siControlCombo);    
      layout.AddItem(L"output_filter_width", L"Width"); 
      layout.AddRow();
         layout.AddItem(L"filter_color_AOVs",  L"Filter Color AOVs");
         layout.AddItem(L"filter_numeric_AOVs",  L"Filter Numeric AOVs");
      layout.EndRow();
   layout.EndGroup();

   layout.AddTab(L"Motion Blur");
   layout.AddRow();
      item = layout.AddItem(L"enable_motion_blur",    L"Transformation");
      item = layout.AddItem(L"motion_step_transform", L"Keys");
      item.PutAttribute(siUINoSlider, true);
   layout.EndRow();
   layout.AddRow();
      item = layout.AddItem(L"enable_motion_deform", L"Deformation");
      item = layout.AddItem(L"motion_step_deform",   L"Keys");
      item.PutAttribute(siUINoSlider, true);
   layout.EndRow();
   item = layout.AddItem(L"exact_ice_mb",     L"Exact ICE Blur");

   layout.AddGroup(L"Geometry Shutter", true, 0); 
      CValueArray onFrame, shutterType;
      onFrame.Add(L"Start on Frame");  onFrame.Add(eMbPos_Start);
      onFrame.Add(L"Center on Frame"); onFrame.Add(eMbPos_Center);
      onFrame.Add(L"End on Frame");    onFrame.Add(eMbPos_End);
      onFrame.Add(L"Custom");          onFrame.Add(eMbPos_Custom);
      item = layout.AddEnumControl(L"motion_shutter_onframe", onFrame, L"Position", siControlCombo);
      item = layout.AddItem(L"motion_shutter_length", L"Length");

      layout.AddRow();
         item = layout.AddItem(L"motion_shutter_custom_start", L"Start");
         item.PutAttribute(siUINoSlider, true);
         item = layout.AddItem(L"motion_shutter_custom_end", L"End");
         item.PutAttribute(siUINoSlider, true);
      layout.EndRow();
   layout.EndGroup();

   layout.AddTab(L"Subdivision");

   layout.AddGroup(L"Max. Subdivisions", true, 0);
      item = layout.AddItem(L"max_subdivisions", L"Max. Subdivisions");
      item.PutAttribute(siUINoLabel, true);
   layout.EndGroup();
   
   layout.AddGroup(L"Adaptive", true, 0);
      item = layout.AddItem(L"adaptive_error", L"Adaptive Error");
      item.PutAttribute(siUILabelPercentage, 70);
      item = layout.AddItem(L"use_dicing_camera", L"Use Alternate Dicing Camera");
      item.PutAttribute(siUILabelPercentage, 70);
      item = layout.AddEnumControl(L"dicing_camera", CValueArray(), L"Dicing Camera", siControlCombo);
      item.PutAttribute(siUILabelPercentage, 70);
   layout.EndGroup();

   layout.AddTab(L"Ray Depth");
   layout.AddGroup(L"Total", true, 0);
      item = layout.AddItem(L"GI_total_depth", L"Total");
      item.PutAttribute(siUINoLabel, true);
   layout.EndGroup();
   layout.AddGroup(L"Ray Type", true, 0);
      item = layout.AddItem(L"GI_diffuse_depth",      L"Diffuse");
      item.PutAttribute(siUILabelPercentage, 70);
      item = layout.AddItem(L"GI_specular_depth",     L"Specular");
      item.PutAttribute(siUILabelPercentage, 70);
      item = layout.AddItem(L"GI_transmission_depth", L"Transmission");
      item.PutAttribute(siUILabelPercentage, 70);
      item = layout.AddItem(L"GI_volume_depth",       L"Volume");
      item.PutAttribute(siUILabelPercentage, 70);
   layout.EndGroup();

   item = layout.AddItem(L"auto_transparency_depth", L"Auto Transp. Depth");
   item.PutAttribute(siUILabelPercentage, 70);
   item = layout.AddItem(L"low_light_threshold", L"Low Light Threshold");
   item.PutAttribute(siUILabelPercentage, 70);

   layout.AddTab(L"Textures");
   layout.AddGroup(L"Filtering", true, 0);
      layout.AddItem(L"texture_accept_unmipped", L"Accept Unmipped Textures");
      layout.AddItem(L"texture_automip",         L"Auto-mipmap");
      CValueArray textFilters;
      textFilters.Add(L"Closest");  textFilters.Add(AI_TEXTURE_CLOSEST);
      textFilters.Add(L"Bilinear"); textFilters.Add(AI_TEXTURE_BILINEAR);
      textFilters.Add(L"Bicubic");  textFilters.Add(AI_TEXTURE_BICUBIC);
      textFilters.Add(L"Smart Bicubic");  textFilters.Add(AI_TEXTURE_SMART_BICUBIC);
      item = layout.AddEnumControl(L"texture_filter", textFilters, L"Filter", siControlCombo);
      item.PutAttribute(siUILabelMinPixels, 195);
      item.PutAttribute(siUILabelPercentage, 90);
   layout.EndGroup();
   layout.AddGroup(L"Tiling", true, 0);
      layout.AddItem(L"texture_accept_untiled", L"Accept Untiled Textures");
      layout.AddRow();
      layout.AddItem(L"enable_autotile",        L"Auto-tile");
      item = layout.AddItem(L"texture_autotile", L"Tile Size");
      item.PutAttribute(siUINoSlider, true);
      item.PutAttribute(siUILabelMinPixels, 100);
      item.PutAttribute(siUILabelPercentage, 90);
      layout.EndRow();
      layout.AddItem(L"use_existing_tx_files", L"Use Existing .tx Textures");
   layout.EndGroup();
   layout.AddGroup(L"Caching", true, 0);
      item = layout.AddItem(L"texture_max_memory_MB",  L"Cache Size (MB)");
      item.PutAttribute(siUINoSlider, true);
      item.PutAttribute(siUILabelMinPixels, 100);
      item.PutAttribute(siUILabelPercentage, 90);
      layout.AddRow();
      item = layout.AddItem(L"texture_max_open_files", L"Max. Open Textures");
      item.PutAttribute(siUILabelPercentage, 90);
      layout.EndRow();
   layout.EndGroup();

   layout.AddTab(L"Color Management");
   layout.AddGroup(L"Color Manager");
      CValueArray color_managers;
      color_managers.Add(L"None"); color_managers.Add(L"");
      color_managers.Add(L"OCIO"); color_managers.Add(L"color_manager_ocio");
      item = layout.AddEnumControl(L"color_manager", color_managers, L"Color Manager", siControlCombo);
      item.PutAttribute(siUINoLabel, true);
   layout.EndGroup();
   layout.AddGroup(L"OCIO");
      layout.AddGroup(L"Config");
         item = layout.AddItem(L"ocio_config", L"Config", siControlFilePath);
         item.PutAttribute(siUINoLabel, true);
         //item.PutAttribute(siUILabelMinPixels, 40);
         item = layout.AddItem(L"ocio_config_message", L"", siControlStatic);
      layout.EndGroup();
      CValueArray colorSpaces(2);
      colorSpaces[0] = L""; colorSpaces[1] = L"";
      layout.AddGroup(L"sRGB Color Space");
         item = layout.AddEnumControl(L"ocio_color_space_narrow", colorSpaces, L"sRGB Color Space", siControlCombo);
         item.PutAttribute(siUINoLabel, true);
      layout.EndGroup();
      layout.AddGroup(L"Rendering Color Space");
         item = layout.AddEnumControl(L"ocio_color_space_linear", colorSpaces, L"Rendering Color Space", siControlCombo);
         item.PutAttribute(siUINoLabel, true);
         item = layout.AddItem(L"ocio_linear_chromaticities", L"Chromaticities");
         item.PutAttribute(siUILabelMinPixels, 80);
         item.PutAttribute(siUILabelPercentage, 25);
      layout.EndGroup();
   layout.EndGroup();

   layout.AddTab(L"Diagnostics");
   layout.AddGroup(L"Logs");
      layout.AddRow();
      layout.AddItem(L"enable_log_console",  L"Console");
      layout.AddItem(L"enable_log_file",     L"File");
      layout.EndRow();
      CValueArray logLevel;
      logLevel.Add(L"Errors");     logLevel.Add(eSItoALogLevel_Errors);
      logLevel.Add(L"Warnings");   logLevel.Add(eSItoALogLevel_Warnings);
      logLevel.Add(L"Info");       logLevel.Add(eSItoALogLevel_Info);
      logLevel.Add(L"Debug");      logLevel.Add(eSItoALogLevel_Debug);
      item = layout.AddEnumControl(L"log_level", logLevel, L"Verbosity", siControlCombo);
      item.PutWidthPercentage(100L);

      item = layout.AddItem(L"texture_per_file_stats", L"Detailed Texture Statistics");      

      item = layout.AddItem(L"max_log_warning_msgs", L"Max. Warning Messages");
      item.PutLabelMinPixels(250);
      item.PutAttribute(siUINoSlider, true);

      layout.AddSpacer(5, 5);
      layout.AddGroup(L"Output Path");
         item = layout.AddItem(L"output_file_tagdir_log", L"Directory", siControlFolder);
         item.PutAttribute(siUINoLabel, true);
      layout.EndGroup();
      layout.AddGroup(L"Resolved Path");
         item = layout.AddItem(L"output_file_dir_log", L"Directory");
         item.PutAttribute(siUINoLabel, true);
      layout.EndGroup();
   layout.EndGroup();
   layout.AddGroup(L"Ignore", true, 0);
      layout.AddItem(L"ignore_textures",     L"Texture Maps");
      layout.AddItem(L"ignore_shaders",      L"Shaders");
      layout.AddItem(L"ignore_atmosphere",   L"Atmosphere Shaders");
      layout.AddItem(L"ignore_lights",       L"Lights");
      layout.AddItem(L"ignore_shadows",      L"Shadows");
      layout.AddItem(L"ignore_subdivision",  L"Subdivision");
      layout.AddItem(L"ignore_displacement", L"Displacement");
      layout.AddItem(L"ignore_bump",         L"Bump");
      layout.AddItem(L"ignore_smoothing",    L"Normal Smoothing");
      layout.AddItem(L"ignore_motion_blur",  L"Motion Blur");
      layout.AddItem(L"ignore_dof",          L"Depth of Field");	  
      layout.AddItem(L"ignore_sss",          L"Sub-Surface Scattering");
      layout.AddItem(L"ignore_hair",         L"Hair");
      layout.AddItem(L"ignore_pointclouds",  L"ICE Point Clouds");
      layout.AddItem(L"ignore_procedurals",  L"Procedurals");
      layout.AddItem(L"ignore_user_options", L"User Options");
      layout.AddItem(L"ignore_matte",        L"Matte Properties");
   layout.EndGroup();

   layout.AddTab(L"ASS Archives");
      layout.AddGroup(L"Output Path");
         item = layout.AddItem(L"output_file_tagdir_ass", L"Directory", siControlFolder);
         item.PutAttribute(siUINoLabel, true);
      layout.EndGroup();
      layout.AddGroup(L"Resolved Path");
         item = layout.AddItem(L"output_file_dir_ass", L"Directory");
         item.PutAttribute(siUINoLabel, true);
      layout.EndGroup();
      layout.AddGroup(L"Options");
         layout.AddItem(L"compress_output_ass",     L"gzip Compression (.ass.gz)");
         layout.AddItem(L"binary_ass",              L"Binary-encode ASS Files");
         layout.AddItem(L"save_texture_paths",      L"Absolute Texture Paths");
         layout.AddItem(L"save_procedural_paths",   L"Absolute Procedural Paths");
         layout.AddItem(L"use_path_translations",   L"Translate Paths");
         layout.AddItem(L"open_procs", L"Expand Procedurals");
      layout.EndGroup();
      layout.AddGroup(L"Node Types");
         layout.AddItem(L"output_options", L"Options");
         layout.AddItem(L"output_drivers_filters", L"Drivers/Filters");
         layout.AddItem(L"output_geometry", L"Geometry");
         layout.AddItem(L"output_cameras", L"Cameras");
         layout.AddItem(L"output_lights", L"Lights");
         layout.AddItem(L"output_shaders", L"Shaders");
      layout.EndGroup();
      layout.AddRow();
      item = layout.AddButton(L"ExportASS", L"Export Frame");
      item.PutAttribute(siUICX, 140);
      item = layout.AddButton(L"ExportAnimation", L"Export Animation");
      item.PutAttribute(siUICX, 140);
      layout.EndRow();

      layout.AddItem(L"sitoa_version", L"SItoA Version");

   return CStatus::OK;
}


SITOA_CALLBACK CommonRenderOptions_PPGEvent(const CRef& in_ctxt)
{
   Application app;
   
   PPGEventContext ctxt(in_ctxt);
   PPGEventContext::PPGEvent eventID = ctxt.GetEventID();

   // Read the rendering options, else the output ass path is not resolved.
   // This is also needed in case the shaders search path was edited, and we need to update the
   // shaderdef menu.

   // The custom property is the ctxt source, or its parent, in case a parameter was changed
   CustomProperty cpset = ctxt.GetSource();
   if (!cpset.IsValid())
   {
      Parameter paramChanged = ctxt.GetSource();
      cpset = paramChanged.GetParent();
   }
   GetRenderOptions()->Read(Property(cpset));

   if (eventID == PPGEventContext::siOnInit)
   {
      // This event meant that the UI was just created.
      // It gives us a chance to set some parameter values.
      // We could even change the layout completely at this point.
      // For this event Source() of the event is the CustomProperty object

      CustomProperty cpset = ctxt.GetSource();

      MotionBlurTabLogic(cpset);
      SamplingTabLogic(cpset);
      SystemTabLogic(cpset);
      OutputTabLogic(cpset);
      TexturesTabLogic(cpset);
      ColorManagersTabLogic(cpset, ctxt);
      SubdivisionTabLogic(cpset);
      DiagnosticsTabLogic(cpset);
      AssOutputTabLogic(cpset);

      Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());

      CRefArray allFrameBuffers = pass.GetFramebuffers();
      CRefArray exrFrameBuffers;
      // collect the exr framebuffers
      for (LONG i=0; i<allFrameBuffers.GetCount(); i++)
      {
         CFrameBuffer fb(Framebuffer(allFrameBuffers[i]), DBL_MAX, true);
         if (fb.IsExr())
            exrFrameBuffers.Add(allFrameBuffers[i]);
      }

      LONG nbExrBuffers = exrFrameBuffers.GetCount();

      PPGLayout layout = cpset.GetPPGLayout();
      // unhide as many layer line as exr buffers
      for (LONG i=0; i<NB_MAX_LAYERS; i++)
      {
         CString tolerance       = L"deep_layer_tolerance" + ((CValue)i).GetAsText();
         CString name            = L"deep_layer_name" + ((CValue)i).GetAsText();
         CString enableFiltering = L"deep_layer_enable_filtering" + ((CValue)i).GetAsText();
         Parameter toleranceParam       = ParAcc_GetParameter(cpset, tolerance);
         Parameter nameParam            = ParAcc_GetParameter(cpset, name);
         Parameter enableFilteringParam = ParAcc_GetParameter(cpset, enableFiltering);
         // hide the others
         if (i >= nbExrBuffers)
         {
            toleranceParam.PutCapabilityFlag(siNotInspectable, true);
            nameParam.PutValue(L"");
            enableFilteringParam.PutCapabilityFlag(siNotInspectable, true);
            enableFilteringParam.PutValue(true);
            continue;
         }

         CFrameBuffer fb(Framebuffer(exrFrameBuffers[i]), DBL_MAX, false);
         // fb.Log();
         // go unhide the i-th line
         toleranceParam.PutCapabilityFlag(siNotInspectable, false);
         PPGItem item = layout.GetItem(tolerance);
         item.PutLabel(fb.m_name); // set the label to the Softimage framebuffer name...
         item.PutAttribute(siUINoSlider, true);
         item.PutAttribute(siUILabelMinPixels, 195);
         item.PutAttribute(siUILabelPercentage, 90);
         // ... but the value of the hidden string to the Arnold layer name 
         // This value will be read when exporting the tolerances for the deepexr driver
         nameParam.PutValue(fb.m_layerName); 

         enableFilteringParam.PutCapabilityFlag(siNotInspectable, false);
      }

      ctxt.PutAttribute(L"Refresh", true);
   }
   else if (eventID == PPGEventContext::siButtonClicked) // buttons
   {
      CValue buttonPressed = ctxt.GetAttribute(L"Button");
      CString buttonName = buttonPressed.GetAsText(); 
      CustomProperty cpset = ctxt.GetSource();

      // Get render options from active pass.
      Pass pass(app.GetActiveProject().GetActiveScene().GetActivePass());
      Property arnoldOptions(pass.GetProperties().GetItem(L"Arnold Render Options"));

      if (buttonName.IsEqualNoCase(L"ExportASS"))
      {
         CRefArray propertiesArray = app.GetActiveProject().GetProperties();
         Property playctrl(propertiesArray.GetItem(L"Play Control"));
         LONG currentFrame = atol(ParAcc_GetValue(playctrl, L"Current", DBL_MAX).GetAsText().GetAsciiString());

         GetRenderInstance()->SetRenderType(L"Export");
         GetRenderInstance()->DestroyScene(false);

         LoadScene(arnoldOptions, L"Export", currentFrame, currentFrame, 1, false, true);
      }
      else if (buttonName.IsEqualNoCase(L"ExportAnimation"))
      {
         CLongArray frames = pass.GetFrames();
         LONG frameStep = frames.GetCount() > 1 ? frames[1] - frames[0] : 1; // #1194

         LoadScene(arnoldOptions, L"Export", frames[0], frames[frames.GetCount()-1], frameStep, false, true);            
      }
      else if (buttonName.IsEqualNoCase(L"ResetToDefault"))
      {
         LONG okPressed;
         if (app.GetUIToolkit().MsgBox(L"Are You Sure ?", siMsgOkCancel, L"Reset Options", okPressed) == CStatus::OK)
         {
            if (okPressed == 1)
               ResetToDefault(cpset, ctxt);
         }
      }
      else if (buttonName.IsEqualNoCase(L"AddMetadata"))
      {
         // add a bottom metadata row below the last active one
         for (LONG i=0; i<NB_EXR_METADATA; i++)
         {
            Parameter p = cpset.GetParameter(L"exr_metadata_name" + ((CValue)i).GetAsText());
            LONG cap = p.GetCapabilities();
            if (cap & siNotInspectable) // is hidden
            {  // show it
               p.PutCapabilityFlag(siNotInspectable, false);
               p = cpset.GetParameter(L"exr_metadata_type"  + ((CValue)i).GetAsText());
               p.PutCapabilityFlag(siNotInspectable, false);
               p = cpset.GetParameter(L"exr_metadata_value" + ((CValue)i).GetAsText());
               p.PutCapabilityFlag(siNotInspectable, false);
               // disable "Add" if we reached 20 metadata
               cpset.GetPPGLayout().GetItem(L"AddMetadata").PutAttribute(siUIButtonDisable, i==NB_EXR_METADATA-1);
               // enable "Remove"
               cpset.GetPPGLayout().GetItem(L"RemoveMetadata").PutAttribute(siUIButtonDisable, false);

               ctxt.PutAttribute(L"Refresh", true); // refresh the ppg
               break;
            }
         }
      }
      else if (buttonName.IsEqualNoCase(L"RemoveMetadata"))
      {
         // remove the bottom metadata row
         for (LONG i=NB_EXR_METADATA-1; i>=0; i--)
         {
            Parameter p = cpset.GetParameter(L"exr_metadata_name" + ((CValue)i).GetAsText());
            LONG cap = p.GetCapabilities();
            if (!(cap & siNotInspectable)) // is showing
            {  // clean and hide it
               p.PutValue(L"");
               p.PutCapabilityFlag(siNotInspectable, true);
               p = cpset.GetParameter(L"exr_metadata_type"  + ((CValue)i).GetAsText());
               p.PutCapabilityFlag(siNotInspectable, true);
               p = cpset.GetParameter(L"exr_metadata_value" + ((CValue)i).GetAsText());
               p.PutValue(L"");
               p.PutCapabilityFlag(siNotInspectable, true);
               // enable "Add"
               cpset.GetPPGLayout().GetItem(L"AddMetadata").PutAttribute(siUIButtonDisable, false);
               // disable "Remove" if there are no metadata
               cpset.GetPPGLayout().GetItem(L"RemoveMetadata").PutAttribute(siUIButtonDisable, i==0);

               ctxt.PutAttribute(L"Refresh", true); // refresh the ppg
               break;
            }
         }
      }
   }
   else if (eventID == PPGEventContext::siParameterChange)
   {
      // For this event the Source of the event is the parameter itself
      Parameter paramChanged = ctxt.GetSource();
      CString paramName = paramChanged.GetScriptName(); 
      CustomProperty cpset = paramChanged.GetParent();

      if (paramName == L"enable_motion_blur"     || 
          paramName == L"enable_motion_deform"   ||
          paramName == L"motion_shutter_onframe")
         MotionBlurTabLogic(cpset);

      else if (paramName == L"enable_adaptive_sampling" ||
               paramName == L"use_sample_clamp" ||
               paramName == L"output_filter")
         SamplingTabLogic(cpset);

      else if (paramName == L"autodetect_threads")
         SystemTabLogic(cpset);

      else if (paramName == L"overscan" || 
               paramName == L"output_tiff_tiled" ||
               paramName == L"output_exr_tiled"  ||
               paramName == L"deep_exr_enable")
         OutputTabLogic(cpset);

      else if (paramName == L"enable_autotile" || 
               paramName == L"texture_accept_untiled")
         TexturesTabLogic(cpset);

      else if (paramName == L"color_manager" ||
               paramName == L"ocio_config" ||
               paramName == L"ocio_color_space_linear")
         ColorManagersTabLogic(cpset, ctxt);

      else if (paramName == L"use_dicing_camera")
         SubdivisionTabLogic(cpset);

      else if (paramName == L"enable_log_file" ||
               paramName == L"log_level"       || 
               paramName == L"output_file_tagdir_log")
         DiagnosticsTabLogic(cpset);

      else if (paramName == L"output_file_tagdir_ass" || 
               paramName == L"compress_output_ass")
         AssOutputTabLogic(cpset);

      else if (paramName == L"skip_license_check")
      {
         bool skipLicenseCheck = (bool)ParAcc_GetValue(cpset, L"skip_license_check", DBL_MAX);
         ParAcc_GetParameter(cpset, L"abort_on_license_fail").PutCapabilityFlag(siReadOnly, skipLicenseCheck);                     
      }
      else if (paramName == L"plugins_path")
      {
         GetRenderInstance()->ShaderDefSet().Clear();
         CString plugin_origin_path = CPathUtilities().GetShadersPath();
         GetRenderInstance()->ShaderDefSet().Load(plugin_origin_path);
      }
   }

   return CStatus::OK;
}


// Logic for the motion blur tab
//
// @param in_cp       The arnold rendering options property
//
void MotionBlurTabLogic(CustomProperty &in_cp)
{
   // Enabling / Disabling Blur settings
   bool transfOn = (bool)ParAcc_GetValue(in_cp, L"enable_motion_blur",   DBL_MAX);
   bool defOn    = (bool)ParAcc_GetValue(in_cp, L"enable_motion_deform", DBL_MAX);
   bool transfOrDefOn = transfOn || defOn;
   int  onFrame = (int)ParAcc_GetValue(in_cp, L"motion_shutter_onframe", DBL_MAX);
   bool customOn = transfOrDefOn && (onFrame == eMbPos_Custom);
   bool lengthOn = transfOrDefOn && (onFrame != eMbPos_Custom);

   ParAcc_GetParameter(in_cp, L"motion_step_transform").PutCapabilityFlag(siReadOnly, !transfOn);
   ParAcc_GetParameter(in_cp, L"motion_step_deform").PutCapabilityFlag(siReadOnly, !defOn); 
   ParAcc_GetParameter(in_cp, L"exact_ice_mb").PutCapabilityFlag(siReadOnly, !defOn); 

   ParAcc_GetParameter(in_cp, L"motion_shutter_length").PutCapabilityFlag(siReadOnly, !lengthOn); 
   ParAcc_GetParameter(in_cp, L"motion_shutter_custom_start").PutCapabilityFlag(siReadOnly, !customOn); 
   ParAcc_GetParameter(in_cp, L"motion_shutter_custom_end").PutCapabilityFlag(siReadOnly, !customOn); 
   ParAcc_GetParameter(in_cp, L"motion_shutter_onframe").PutCapabilityFlag(siReadOnly, !transfOrDefOn); 
}


// Logic for the sampling tab
//
// @param in_cp       The arnold rendering options property
//
void SamplingTabLogic(CustomProperty &in_cp)
{
   // adaptive sampling
   bool adaptive = (bool)ParAcc_GetValue(in_cp, L"enable_adaptive_Sampling", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"AA_samples_max").PutCapabilityFlag(siReadOnly, !adaptive);
   ParAcc_GetParameter(in_cp, L"AA_adaptive_threshold").PutCapabilityFlag(siReadOnly, !adaptive);

   // Only some filter nodes have a width attribute
   CString filter = ParAcc_GetValue(in_cp, L"output_filter", DBL_MAX).GetAsText();

   bool enableWidth = filter.IsEqualNoCase(L"gaussian") || filter.IsEqualNoCase(L"triangle") ||
                      filter.IsEqualNoCase(L"variance") || filter.IsEqualNoCase(L"blackman_harris") ||
                      filter.IsEqualNoCase(L"contour") || filter.IsEqualNoCase(L"sinc");

   ParAcc_GetParameter(in_cp, L"output_filter_width").PutCapabilityFlag(siReadOnly, !enableWidth);

   // sample clamp
   bool clamp = (bool)ParAcc_GetValue(in_cp, L"use_sample_clamp", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"use_sample_clamp_AOVs").PutCapabilityFlag(siReadOnly, !clamp);
   ParAcc_GetParameter(in_cp, L"AA_sample_clamp").PutCapabilityFlag(siReadOnly, !clamp);
}


// Logic for the system tab
//
// @param in_cp       The arnold rendering options property
//
void SystemTabLogic(CustomProperty &in_cp)
{
   bool autoDetect = (bool)ParAcc_GetValue(in_cp, L"autodetect_threads", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"threads").PutCapabilityFlag(siReadOnly, autoDetect);
}


// Logic for the output tab
//
// @param in_cp       The arnold rendering options property
//
void OutputTabLogic(CustomProperty &in_cp)
{
   bool overscan = (bool)ParAcc_GetValue(in_cp, L"overscan", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"overscan_top").PutCapabilityFlag(siReadOnly, !overscan);
   ParAcc_GetParameter(in_cp, L"overscan_bottom").PutCapabilityFlag(siReadOnly, !overscan);
   ParAcc_GetParameter(in_cp, L"overscan_left").PutCapabilityFlag(siReadOnly, !overscan);
   ParAcc_GetParameter(in_cp, L"overscan_right").PutCapabilityFlag(siReadOnly, !overscan);

   bool exrTiled  = (bool)ParAcc_GetValue(in_cp, L"output_exr_tiled", DBL_MAX);
   bool tiffTiled = (bool)ParAcc_GetValue(in_cp, L"output_tiff_tiled", DBL_MAX);
   bool deepExr = deepExr = (bool)ParAcc_GetValue(in_cp, L"deep_exr_enable", DBL_MAX);

   ParAcc_GetParameter(in_cp, L"output_tiff_append").PutCapabilityFlag(siReadOnly, !tiffTiled);

   ParAcc_GetParameter(in_cp, L"output_exr_autocrop").PutCapabilityFlag(siReadOnly, exrTiled || deepExr);
   ParAcc_GetParameter(in_cp, L"output_exr_append").PutCapabilityFlag(siReadOnly, !exrTiled);

   ParAcc_GetParameter(in_cp, L"output_exr_compression").PutCapabilityFlag(siReadOnly, deepExr);
   ParAcc_GetParameter(in_cp, L"output_exr_preserve_layer_name").PutCapabilityFlag(siReadOnly, deepExr);
   ParAcc_GetParameter(in_cp, L"deep_subpixel_merge").PutCapabilityFlag(siReadOnly, !deepExr);
   ParAcc_GetParameter(in_cp, L"deep_subpixel_merge").PutCapabilityFlag(siReadOnly, !deepExr);
   ParAcc_GetParameter(in_cp, L"deep_use_RGB_opacity").PutCapabilityFlag(siReadOnly, !deepExr);
   ParAcc_GetParameter(in_cp, L"deep_alpha_tolerance").PutCapabilityFlag(siReadOnly, !deepExr);
   ParAcc_GetParameter(in_cp, L"deep_alpha_half_precision").PutCapabilityFlag(siReadOnly, !deepExr);
   ParAcc_GetParameter(in_cp, L"deep_depth_tolerance").PutCapabilityFlag(siReadOnly, !deepExr);
   ParAcc_GetParameter(in_cp, L"deep_depth_half_precision").PutCapabilityFlag(siReadOnly, !deepExr);
   for (int i=0; i<NB_MAX_LAYERS; i++)
   {
      ParAcc_GetParameter(in_cp, L"deep_layer_tolerance"        + ((CValue)i).GetAsText()).PutCapabilityFlag(siReadOnly, !deepExr);
      ParAcc_GetParameter(in_cp, L"deep_layer_enable_filtering" + ((CValue)i).GetAsText()).PutCapabilityFlag(siReadOnly, !deepExr);
   }
}


// Logic for the textures tab
//
// @param in_cp       The arnold rendering options property
//
void TexturesTabLogic(CustomProperty &in_cp)
{
   bool acceptUntiled = (bool)ParAcc_GetValue(in_cp, L"texture_accept_untiled", DBL_MAX);
   bool autotile = (bool)ParAcc_GetValue(in_cp, L"enable_autotile", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"enable_autotile").PutCapabilityFlag(siReadOnly, !acceptUntiled); 
   ParAcc_GetParameter(in_cp, L"texture_autotile").PutCapabilityFlag(siReadOnly, (!acceptUntiled || (acceptUntiled && !autotile))); 
}


// Logic for the color managers tab
// https://github.com/Autodesk/sitoa/issues/31
//
// @param in_cp       The arnold rendering options property
// @param in_ctxt     The arnold rendering options PPGEvent
//
void ColorManagersTabLogic(CustomProperty &in_cp, PPGEventContext &in_ctxt)
{
   Parameter paramChanged = in_ctxt.GetSource();
   CString paramName = paramChanged.GetScriptName(); 

   // OCIO color manager
   bool ocioManager = (bool)(ParAcc_GetValue(in_cp, L"color_manager", DBL_MAX) == L"color_manager_ocio");
   bool useOcioDefaultRenderingSpace = (bool)(ParAcc_GetValue(in_cp, L"ocio_color_space_linear", DBL_MAX) == L"");
   bool hasOcioEnv = (bool)(getenv("OCIO") != NULL);
   CString ocioConfig = ParAcc_GetValue(in_cp, L"ocio_config", DBL_MAX);
   bool ocioLoaded = false;

   ParAcc_GetParameter(in_cp, L"ocio_config").PutCapabilityFlag(siReadOnly, !ocioManager);
   ParAcc_GetParameter(in_cp, L"ocio_config_message").PutCapabilityFlag(siReadOnly, !ocioManager);
   ParAcc_GetParameter(in_cp, L"ocio_color_space_narrow").PutCapabilityFlag(siReadOnly, !ocioManager);
   ParAcc_GetParameter(in_cp, L"ocio_color_space_linear").PutCapabilityFlag(siReadOnly, !ocioManager);
   ParAcc_GetParameter(in_cp, L"ocio_linear_chromaticities").PutCapabilityFlag(siReadOnly, (!ocioManager || useOcioDefaultRenderingSpace));


   // don't do the heavy UI update if just the rendering color space has changed
   if (paramName != L"ocio_color_space_linear") {
      if (ocioManager) {
         if (hasOcioEnv && ocioConfig == L"") {
            in_cp.PutParameterValue(L"ocio_config_message", CString(L"Using OCIO config from environment."));
            ocioLoaded = true;
         }
         else if (ocioConfig != L"") {
            in_cp.PutParameterValue(L"ocio_config_message", CString(L"Using the specified OCIO config."));
            ocioLoaded = true;
         }
         else
            in_cp.PutParameterValue(L"ocio_config_message", CString(L"No OCIO in environment.\nLoad a config manually to use OCIO."));
      }
      else
         in_cp.PutParameterValue(L"ocio_config_message", CString(L""));

      if (ocioLoaded) {
         // init strings to get default colorspaces
         AtString defaultsRGB;
         AtString defaultLinear;
         CValueArray colorSpaces(2);
         colorSpaces[0] = L""; colorSpaces[1] = L"";  // init first items

         // we need to have an arnold universe with the ocio node so that we can get all the color spaces
         bool defaultUniverseExist = AiUniverseIsActive();
         AtUniverse* ocioUniverse;
         if (defaultUniverseExist)
            ocioUniverse = AiUniverse();
         else
            AiBegin();

         AtNode* ocioNode = AiNode("color_manager_ocio");
         CNodeSetter::SetString(ocioNode, "config", GetRenderOptions()->m_ocio_config.GetAsciiString());

         int numColorSpaces = AiColorManagerGetNumColorSpaces(ocioNode);
         if (numColorSpaces > 0) {
            // get all colorspaces in the current OCIO config
            colorSpaces.Resize((numColorSpaces+1)*2);
            CString colorSpace;

            for (LONG i=0; i<numColorSpaces; i++) {
               colorSpace = CString(AiColorManagerGetColorSpaceNameByIndex(ocioNode, i));
               colorSpaces[i*2+2] = colorSpace;
               colorSpaces[i*2+3] = colorSpace;
            }

            // get the default color spaces
            AiColorManagerGetDefaults(ocioNode, defaultsRGB, defaultLinear);

         }
         else {
            in_cp.PutParameterValue(L"ocio_config_message", CString(L"Error: No color spaces found in current config!"));
         }

         // destroy the universe
         if (defaultUniverseExist)
            AiUniverseDestroy(ocioUniverse);
         else
            AiEnd();

         // update the PPGs
         PPGLayout layout = in_cp.GetPPGLayout();
         PPGItem item;

         // add the default sRGB color space
         if (defaultsRGB)
            colorSpaces[0] = L"Auto (" + CString(defaultsRGB) + ")";
         item = layout.GetItem(L"ocio_color_space_narrow");
         item.PutUIItems(colorSpaces);

         // add the default linear color space
         if (defaultLinear)
            colorSpaces[0] = L"Auto (" + CString(defaultLinear) + ")";
         item = layout.GetItem(L"ocio_color_space_linear");
         item.PutUIItems(colorSpaces);

         // redraw the PPG so the new Enum items are showing
         in_ctxt.PutAttribute(L"Refresh", true);
      }
   }
}


// Logic for the subdivision tab
//
// @param in_cp       The arnold rendering options property
//
void SubdivisionTabLogic(CustomProperty &in_cp)
{
   bool useDicingCamera = (bool)ParAcc_GetValue(in_cp, L"use_dicing_camera", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"dicing_camera").PutCapabilityFlag(siReadOnly, !useDicingCamera);

   // Dicing camera selection
   CValueArray cameras;
   CRefArray camerasArray = Application().GetActiveSceneRoot().FindChildren(L"", siCameraPrimType, CStringArray(), true);

   for (LONG i=0; i<camerasArray.GetCount(); i++)
   {
      Camera xsiCamera(camerasArray[i]);
      // (label, value) pair (#1489)
      cameras.Add(xsiCamera.GetFullName());
      cameras.Add(xsiCamera.GetFullName());
   }
     
   in_cp.GetPPGLayout().GetItem(L"dicing_camera").PutUIItems(cameras);

   if (ParAcc_GetValue(in_cp, L"dicing_camera", DBL_MAX).GetAsText().IsEmpty())
   {
      CString cameraName(in_cp.GetPPGLayout().GetItem(L"dicing_camera").GetUIItems()[0].GetAsText());
      in_cp.PutParameterValue(L"dicing_camera", cameraName);
   }
}


// Logic for the diagnostics tab
//
// @param in_cp       The arnold rendering options property
//
void DiagnosticsTabLogic(CustomProperty &in_cp)
{
   // Output log path
   bool logfile = (bool)ParAcc_GetValue(in_cp, L"enable_log_file", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"output_file_tagdir_log").PutCapabilityFlag(siReadOnly, !logfile); 

   in_cp.PutParameterValue(L"output_file_dir_log", CPathUtilities().GetOutputLogPath());

   int log_level = (int)ParAcc_GetValue(in_cp, L"log_level", DBL_MAX);
   ParAcc_GetParameter(in_cp, L"texture_per_file_stats").PutCapabilityFlag(siReadOnly, log_level != eSItoALogLevel_Debug); 
}


// Logic for the ass archives tab
//
// @param in_cp       The arnold rendering options property
//
void AssOutputTabLogic(CustomProperty &in_cp)
{
   double frame = CTimeUtilities().GetCurrentFrame();

   CString outputAsspath = CPathUtilities().GetOutputAssPath(); // this resolves the tokens
   CString fileName = CPathUtilities().GetOutputExportFileName(true, true, frame);

   in_cp.PutParameterValue(L"output_file_dir_ass", outputAsspath + CUtils::Slash() + fileName);
   // Paths Translations (disabled if linktab not defined)
   ParAcc_GetParameter(in_cp, L"use_path_translations").PutCapabilityFlag(siReadOnly, getenv("SITOA_LINKTAB_LOCATION") == NULL); 
   // Resolved Paths (ReadOnly)
   ParAcc_GetParameter(in_cp, L"output_file_dir_ass").PutCapabilityFlag(siReadOnly, true); 
}


// Reset the default values of all the parameters
//
// @param in_cp       The arnold rendering options property
// @param in_ctxt     The arnold rendering options PPGEvent
//
void ResetToDefault(CustomProperty &in_cp, PPGEventContext &in_ctxt)
{
   CParameterRefArray params = in_cp.GetParameters();
   for (LONG i=0; i<params.GetCount(); i++)
   {
      Parameter p(params[i]);
      p.PutValue(p.GetDefault());
   }
   // restore the logic, according to the default values (#1532)
   MotionBlurTabLogic(in_cp);
   SamplingTabLogic(in_cp);
   SystemTabLogic(in_cp);
   OutputTabLogic(in_cp);
   TexturesTabLogic(in_cp);
   ColorManagersTabLogic(in_cp, in_ctxt);
   SubdivisionTabLogic(in_cp);
   DiagnosticsTabLogic(in_cp);
   AssOutputTabLogic(in_cp);
}
