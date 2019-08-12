/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

// To have the shaders browsable in the rendertree, I found no solution other
// than registering the shaders here, with a void definition. The definition is
// completely implemented in ShaderDef.cpp

function XSILoadPlugin( in_reg )
{
   if (Application.plugins("Arnold Tools") == null)
      LoadPlugin(XSIUtils.BuildPath(in_reg.OriginPath, "ArnoldTools.js"));
   
   var h = SItoAToolHelper();
   h.SetPluginInfo(in_reg, "Arnold");   

   in_reg.RegisterShader("abs", 1, 0);
   in_reg.RegisterShader("add", 1, 0);
   in_reg.RegisterShader("ambient_occlusion", 1, 0);
   in_reg.RegisterShader("aov_read_float", 1, 0);
   in_reg.RegisterShader("aov_read_int", 1, 0);
   in_reg.RegisterShader("aov_read_rgb", 1, 0);
   in_reg.RegisterShader("aov_read_rgba", 1, 0);
   in_reg.RegisterShader("aov_write_float", 1, 0);
   in_reg.RegisterShader("aov_write_int", 1, 0);
   in_reg.RegisterShader("aov_write_rgb", 1, 0);
   in_reg.RegisterShader("aov_write_rgba", 1, 0);
   in_reg.RegisterShader("atan", 1, 0);
   in_reg.RegisterShader("atmosphere_volume", 1, 0);
   in_reg.RegisterShader("blackbody", 1, 0);
   in_reg.RegisterShader("bump2d", 1, 0);
   in_reg.RegisterShader("bump3d", 1, 0);
   in_reg.RegisterShader("cache", 1, 0);
   in_reg.RegisterShader("camera_projection", 1, 0);
   in_reg.RegisterShader("car_paint", 1, 0);
   in_reg.RegisterShader("cell_noise", 1, 0);
   in_reg.RegisterShader("checkerboard", 1, 0);
   in_reg.RegisterShader("clamp", 1, 0);
   in_reg.RegisterShader("clip_geo", 1, 0);
   in_reg.RegisterShader("closure", 1, 0); // SItoA
   in_reg.RegisterShader("color_convert", 1, 0);
   in_reg.RegisterShader("color_correct", 1, 0);
   in_reg.RegisterShader("color_jitter", 1, 0);
   in_reg.RegisterShader("compare", 1, 0);
   in_reg.RegisterShader("complement", 1, 0);
   in_reg.RegisterShader("complex_ior", 1, 0); // deprecated
   in_reg.RegisterShader("composite", 1, 0);
   in_reg.RegisterShader("cross", 1, 0);
   in_reg.RegisterShader("cryptomatte", 1, 0);
   in_reg.RegisterShader("curvature", 1, 0);
   in_reg.RegisterShader("divide", 1, 0);
   in_reg.RegisterShader("dot", 1, 0);
   in_reg.RegisterShader("exp", 1, 0);
   in_reg.RegisterShader("facing_ratio", 1, 0);
   in_reg.RegisterShader("flakes", 1, 0);
   in_reg.RegisterShader("flat", 1, 0);
   in_reg.RegisterShader("float_to_int", 1, 0);
   in_reg.RegisterShader("float_to_matrix", 1, 0);
   in_reg.RegisterShader("float_to_rgb", 1, 0);
   in_reg.RegisterShader("float_to_rgba", 1, 0);
   in_reg.RegisterShader("fog", 1, 0);
   in_reg.RegisterShader("fraction", 1, 0);
   in_reg.RegisterShader("hair", 1, 0); // deprecated
   in_reg.RegisterShader("image", 1, 0);
   in_reg.RegisterShader("is_finite", 1, 0);
   in_reg.RegisterShader("lambert", 1, 0);
   in_reg.RegisterShader("layer_float", 1, 0);
   in_reg.RegisterShader("layer_rgba", 1, 0);
   in_reg.RegisterShader("layer_shader", 1, 0);
   in_reg.RegisterShader("length", 1, 0);
   in_reg.RegisterShader("log", 1, 0);
   in_reg.RegisterShader("matrix_interpolate", 1, 0);
   in_reg.RegisterShader("matrix_multiply_vector", 1, 0);
   in_reg.RegisterShader("matrix_transform", 1, 0);
   in_reg.RegisterShader("matte", 1, 0);
   in_reg.RegisterShader("max", 1, 0);
   in_reg.RegisterShader("min", 1, 0);
   in_reg.RegisterShader("mix_rgba", 1, 0);
   in_reg.RegisterShader("mix_shader", 1, 0);
   in_reg.RegisterShader("modulo", 1, 0);
   in_reg.RegisterShader("motion_vector", 1, 0);
   in_reg.RegisterShader("multiply", 1, 0);
   in_reg.RegisterShader("negate", 1, 0);
   in_reg.RegisterShader("noise", 1, 0);
   in_reg.RegisterShader("normal_map", 1, 0);
   in_reg.RegisterShader("normalize", 1, 0);
   in_reg.RegisterShader("osl", 1, 0);
   in_reg.RegisterShader("passthrough", 1, 0);
   in_reg.RegisterShader("physical_sky", 1, 0);
   in_reg.RegisterShader("pow", 1, 0);
   in_reg.RegisterShader("ramp_float", 1, 0);
   in_reg.RegisterShader("ramp_rgb", 1, 0);
   in_reg.RegisterShader("random", 1, 0);
   in_reg.RegisterShader("range", 1, 0);
   in_reg.RegisterShader("ray_switch_rgba", 1, 0);
   in_reg.RegisterShader("ray_switch_shader", 1, 0);
   in_reg.RegisterShader("reciprocal", 1, 0);
   in_reg.RegisterShader("rgb_to_float", 1, 0);
   in_reg.RegisterShader("rgb_to_vector", 1, 0);
   in_reg.RegisterShader("rgba_to_float", 1, 0);
   in_reg.RegisterShader("round_corners", 1, 0);
   in_reg.RegisterShader("shadow_matte", 1, 0);
   in_reg.RegisterShader("shuffle", 1, 0);
   in_reg.RegisterShader("sign", 1, 0);
   in_reg.RegisterShader("skin", 1, 0); // deprecated
   in_reg.RegisterShader("sky", 1, 0); // deprecated
   in_reg.RegisterShader("space_transform", 1, 0);
   in_reg.RegisterShader("sqrt", 1, 0);
   in_reg.RegisterShader("standard", 1, 0); // deprecated
   in_reg.RegisterShader("standard_hair", 1, 0);
   in_reg.RegisterShader("standard_surface", 1, 0);
   in_reg.RegisterShader("standard_volume", 1, 0);
   in_reg.RegisterShader("state_float", 1, 0);
   in_reg.RegisterShader("state_int", 1, 0);
   in_reg.RegisterShader("state_vector", 1, 0);
   in_reg.RegisterShader("subtract", 1, 0);
   in_reg.RegisterShader("switch_rgba", 1, 0);
   in_reg.RegisterShader("switch_shader", 1, 0);
   in_reg.RegisterShader("thin_film", 1, 0);
   in_reg.RegisterShader("toon", 1, 0);
   in_reg.RegisterShader("trace_set", 1, 0);
   in_reg.RegisterShader("trigo", 1, 0);
   in_reg.RegisterShader("triplanar", 1, 0);
   in_reg.RegisterShader("two_sided", 1, 0);
   in_reg.RegisterShader("user_data_float", 1, 0);
   in_reg.RegisterShader("user_data_int", 1, 0);
   in_reg.RegisterShader("user_data_rgb", 1, 0);
   in_reg.RegisterShader("user_data_rgba", 1, 0);
   in_reg.RegisterShader("user_data_string", 1, 0);
   in_reg.RegisterShader("utility", 1, 0);
   in_reg.RegisterShader("uv_projection", 1, 0);
   in_reg.RegisterShader("uv_transform", 1, 0);
   in_reg.RegisterShader("vector_displacement", 1, 0); // extra, clone of vector_map with float output
   in_reg.RegisterShader("vector_map", 1, 0);
   in_reg.RegisterShader("vector_to_rgb", 1, 0);
   in_reg.RegisterShader("volume_collector", 1, 0); // deprecated
   in_reg.RegisterShader("volume_sample_float", 1, 0);
   in_reg.RegisterShader("volume_sample_rgb", 1, 0);
   in_reg.RegisterShader("wireframe", 1, 0);
   // operators
   in_reg.RegisterShader("disable", 1, 0);
   in_reg.RegisterShader("collection", 1, 0);
   in_reg.RegisterShader("include_graph", 1, 0);
   in_reg.RegisterShader("materialx", 1, 0);
   in_reg.RegisterShader("merge", 1, 0);
   in_reg.RegisterShader("operator", 1, 0);
   in_reg.RegisterShader("set_parameter", 1, 0);
   in_reg.RegisterShader("set_transform", 1, 0);
   in_reg.RegisterShader("switch_operator", 1, 0);

   // in_reg.Help = "https://support.solidangle.com/display/A5SItoAUG/Shaders";

   return true;
}

function XSIUnloadPlugin(in_reg) { return true; }

function Arnold_abs_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_abs_1_0_Define(in_ctxt) { return true; }
function Arnold_add_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_add_1_0_Define(in_ctxt) { return true; }
function Arnold_ambient_occlusion_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_ambient_occlusion_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_read_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_read_float_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_read_int_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_read_int_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_read_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_read_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_read_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_read_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_write_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_write_float_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_write_int_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_write_int_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_write_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_write_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_aov_write_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_aov_write_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_atan_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_atan_1_0_Define(in_ctxt) { return true; }
function Arnold_atmosphere_volume_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_atmosphere_volume_1_0_Define(in_ctxt) { return true; }
function Arnold_blackbody_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_blackbody_1_0_Define(in_ctxt) { return true; }
function Arnold_bump2d_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_bump2d_1_0_Define(in_ctxt) { return true; }
function Arnold_bump3d_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_bump3d_1_0_Define(in_ctxt) { return true; }
function Arnold_cache_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_cache_1_0_Define(in_ctxt) { return true; }
function Arnold_camera_projection_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_camera_projection_1_0_Define(in_ctxt) { return true; }
function Arnold_car_paint_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_car_paint_1_0_Define(in_ctxt) { return true; }
function Arnold_cell_noise_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_cell_noise_1_0_Define(in_ctxt) { return true; }
function Arnold_checkerboard_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_checkerboard_1_0_Define(in_ctxt) { return true; }
function Arnold_clamp_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_clamp_1_0_Define(in_ctxt) { return true; }
function Arnold_clip_geo_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_clip_geo_1_0_Define(in_ctxt) { return true; }
function Arnold_closure_1_0_DefineInfo(in_ctxt) { return true; } // SItoA
function Arnold_closure_1_0_Define(in_ctxt) { return true; }     // SItoA
function Arnold_color_convert_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_color_convert_1_0_Define(in_ctxt) { return true; }
function Arnold_color_correct_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_color_correct_1_0_Define(in_ctxt) { return true; }
function Arnold_color_jitter_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_color_jitter_1_0_Define(in_ctxt) { return true; }
function Arnold_compare_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_compare_1_0_Define(in_ctxt) { return true; }
function Arnold_complement_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_complement_1_0_Define(in_ctxt) { return true; }
function Arnold_complex_ior_1_0_DefineInfo(in_ctxt) { return true; } // deprecated
function Arnold_complex_ior_1_0_Define(in_ctxt) { return true; }     // deprecated
function Arnold_composite_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_composite_1_0_Define(in_ctxt) { return true; }
function Arnold_cross_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_cross_1_0_Define(in_ctxt) { return true; }
function Arnold_cryptomatte_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_cryptomatte_1_0_Define(in_ctxt) { return true; }
function Arnold_curvature_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_curvature_1_0_Define(in_ctxt) { return true; }
function Arnold_divide_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_divide_1_0_Define(in_ctxt) { return true; }
function Arnold_dot_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_dot_1_0_Define(in_ctxt) { return true; }
function Arnold_exp_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_exp_1_0_Define(in_ctxt) { return true; }
function Arnold_facing_ratio_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_facing_ratio_1_0_Define(in_ctxt) { return true; }
function Arnold_flakes_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_flakes_1_0_Define(in_ctxt) { return true; }
function Arnold_flat_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_flat_1_0_Define(in_ctxt) { return true; }
function Arnold_float_to_int_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_float_to_int_1_0_Define(in_ctxt) { return true; }
function Arnold_float_to_matrix_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_float_to_matrix_1_0_Define(in_ctxt) { return true; }
function Arnold_float_to_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_float_to_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_float_to_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_float_to_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_fog_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_fog_1_0_Define(in_ctxt) { return true; }
function Arnold_fraction_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_fraction_1_0_Define(in_ctxt) { return true; }
function Arnold_hair_1_0_DefineInfo(in_ctxt) { return true; } // deprecated
function Arnold_hair_1_0_Define(in_ctxt) { return true; }     // deprecated
function Arnold_image_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_image_1_0_Define(in_ctxt) { return true; }
function Arnold_is_finite_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_is_finite_1_0_Define(in_ctxt) { return true; }
function Arnold_lambert_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_lambert_1_0_Define(in_ctxt) { return true; }
function Arnold_layer_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_layer_float_1_0_Define(in_ctxt) { return true; }
function Arnold_layer_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_layer_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_layer_shader_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_layer_shader_1_0_Define(in_ctxt) { return true; }
function Arnold_length_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_length_1_0_Define(in_ctxt) { return true; }
function Arnold_log_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_log_1_0_Define(in_ctxt) { return true; }
function Arnold_matrix_interpolate_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_matrix_interpolate_1_0_Define(in_ctxt) { return true; }
function Arnold_matrix_multiply_vector_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_matrix_multiply_vector_1_0_Define(in_ctxt) { return true; }
function Arnold_matrix_transform_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_matrix_transform_1_0_Define(in_ctxt) { return true; }
function Arnold_matte_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_matte_1_0_Define(in_ctxt) { return true; }
function Arnold_max_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_max_1_0_Define(in_ctxt) { return true; }
function Arnold_min_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_min_1_0_Define(in_ctxt) { return true; }
function Arnold_mix_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_mix_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_mix_shader_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_mix_shader_1_0_Define(in_ctxt) { return true; }
function Arnold_modulo_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_modulo_1_0_Define(in_ctxt) { return true; }
function Arnold_motion_vector_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_motion_vector_1_0_Define(in_ctxt) { return true; }
function Arnold_multiply_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_multiply_1_0_Define(in_ctxt) { return true; }
function Arnold_negate_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_negate_1_0_Define(in_ctxt) { return true; }
function Arnold_noise_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_noise_1_0_Define(in_ctxt) { return true; }
function Arnold_normal_map_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_normal_map_1_0_Define(in_ctxt) { return true; }
function Arnold_normalize_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_normalize_1_0_Define(in_ctxt) { return true; }
function Arnold_osl_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_osl_1_0_Define(in_ctxt) { return true; }
function Arnold_passthrough_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_passthrough_1_0_Define(in_ctxt) { return true; }
// function Arnold_physical_sky_1_0_DefineInfo(in_ctxt) { return true; }
// function Arnold_physical_sky_1_0_Define(in_ctxt) { return true; }
function Arnold_pow_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_pow_1_0_Define(in_ctxt) { return true; }
function Arnold_ramp_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_ramp_float_1_0_Define(in_ctxt) { return true; }
function Arnold_ramp_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_ramp_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_random_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_random_1_0_Define(in_ctxt) { return true; }
function Arnold_range_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_range_1_0_Define(in_ctxt) { return true; }
function Arnold_ray_switch_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_ray_switch_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_ray_switch_shader_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_ray_switch_shader_1_0_Define(in_ctxt) { return true; }
function Arnold_reciprocal_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_reciprocal_1_0_Define(in_ctxt) { return true; }
function Arnold_rgb_to_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_rgb_to_float_1_0_Define(in_ctxt) { return true; }
function Arnold_rgb_to_vector_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_rgb_to_vector_1_0_Define(in_ctxt) { return true; }
function Arnold_rgba_to_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_rgba_to_float_1_0_Define(in_ctxt) { return true; }
function Arnold_round_corners_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_round_corners_1_0_Define(in_ctxt) { return true; }
function Arnold_shadow_matte_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_shadow_matte_1_0_Define(in_ctxt) { return true; }
function Arnold_shuffle_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_shuffle_1_0_Define(in_ctxt) { return true; }
function Arnold_sign_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_sign_1_0_Define(in_ctxt) { return true; }
function Arnold_skin_1_0_DefineInfo(in_ctxt) { return true; } // deprecated
function Arnold_skin_1_0_Define(in_ctxt) { return true; }     // deprecated
function Arnold_sky_1_0_DefineInfo(in_ctxt) { return true; } // deprecated
function Arnold_sky_1_0_Define(in_ctxt) { return true; }     // deprecated
function Arnold_space_transform_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_space_transform_1_0_Define(in_ctxt) { return true; }
function Arnold_sqrt_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_sqrt_1_0_Define(in_ctxt) { return true; }
function Arnold_standard_1_0_DefineInfo(in_ctxt) { return true; } // deprecated
function Arnold_standard_1_0_Define(in_ctxt) { return true; }     // deprecated
function Arnold_standard_hair_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_standard_hair_1_0_Define(in_ctxt) { return true; }
function Arnold_standard_surface_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_standard_surface_1_0_Define(in_ctxt) { return true; }
function Arnold_standard_volume_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_standard_volume_1_0_Define(in_ctxt) { return true; }
function Arnold_state_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_state_float_1_0_Define(in_ctxt) { return true; }
function Arnold_state_int_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_state_int_1_0_Define(in_ctxt) { return true; }
function Arnold_state_vector_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_state_vector_1_0_Define(in_ctxt) { return true; }
function Arnold_subtract_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_subtract_1_0_Define(in_ctxt) { return true; }
function Arnold_switch_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_switch_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_switch_shader_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_switch_shader_1_0_Define(in_ctxt) { return true; }
function Arnold_thin_film_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_thin_film_1_0_Define(in_ctxt) { return true; }
function Arnold_toon_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_toon_1_0_Define(in_ctxt) { return true; }
function Arnold_trace_set_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_trace_set_1_0_Define(in_ctxt) { return true; }
function Arnold_trigo_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_trigo_1_0_Define(in_ctxt) { return true; }
function Arnold_triplanar_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_triplanar_1_0_Define(in_ctxt) { return true; }
function Arnold_two_sided_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_two_sided_1_0_Define(in_ctxt) { return true; }
function Arnold_user_data_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_user_data_float_1_0_Define(in_ctxt) { return true; }
function Arnold_user_data_int_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_user_data_int_1_0_Define(in_ctxt) { return true; }
function Arnold_user_data_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_user_data_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_user_data_rgba_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_user_data_rgba_1_0_Define(in_ctxt) { return true; }
function Arnold_user_data_string_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_user_data_string_1_0_Define(in_ctxt) { return true; }
function Arnold_utility_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_utility_1_0_Define(in_ctxt) { return true; }
function Arnold_uv_projection_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_uv_projection_1_0_Define(in_ctxt) { return true; }
function Arnold_uv_transform_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_uv_transform_1_0_Define(in_ctxt) { return true; }
function Arnold_vector_displacement_1_0_DefineInfo(in_ctxt) { return true; } // extra, clone of vector_map with float output
function Arnold_vector_displacement_1_0_Define(in_ctxt) { return true; }     // extra, clone of vector_map with float output
function Arnold_vector_map_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_vector_map_1_0_Define(in_ctxt) { return true; }
function Arnold_vector_to_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_vector_to_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_volume_collector_1_0_DefineInfo(in_ctxt) { return true; } // deprecated
function Arnold_volume_collector_1_0_Define(in_ctxt) { return true; }     // deprecated
function Arnold_volume_sample_float_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_volume_sample_float_1_0_Define(in_ctxt) { return true; }
function Arnold_volume_sample_rgb_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_volume_sample_rgb_1_0_Define(in_ctxt) { return true; }
function Arnold_wireframe_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_wireframe_1_0_Define(in_ctxt) { return true; }

// operators
function Arnold_collection_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_collection_1_0_Define(in_ctxt) { return true; }
function Arnold_disable_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_disable_1_0_Define(in_ctxt) { return true; }
function Arnold_include_graph_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_include_graph_1_0_Define(in_ctxt) { return true; }
function Arnold_materialx_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_materialx_1_0_Define(in_ctxt) { return true; }
function Arnold_merge_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_merge_1_0_Define(in_ctxt) { return true; }
//function Arnold_set_parameter_1_0_DefineInfo(in_ctxt) { return true; }
//function Arnold_set_parameter_1_0_Define(in_ctxt) { return true; }
function Arnold_set_transform_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_set_transform_1_0_Define(in_ctxt) { return true; }
function Arnold_switch_operator_1_0_DefineInfo(in_ctxt) { return true; }
function Arnold_switch_operator_1_0_Define(in_ctxt) { return true; }


///////////////////////////////////////////////////
/////////////// shaders that require a dedicated UI
///////////////////////////////////////////////////

function Arnold_physical_sky_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function Arnold_physical_sky_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyTexture);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   h.AddScalar (params, "turbidity",     3.0, 1.0, 10.0, 1.0, 10.0, true, false, true);
   h.AddColor3 (params, "ground_albedo", 0.1, 0.1, 0.1, true, false, true);
   h.AddScalar (params, "elevation",     45.0, 0.0, 180.0, 0.0, 180.0, true, false, true);
   h.AddScalar (params, "azimuth",       90.0, 0.0, 360.0, 0.0, 360.0, true, false, true);
   h.AddBoolean(params, "enable_sun",    true, true, false, true);
   h.AddScalar (params, "sun_size",      0.51, 0.0, 180.0, 0.0, 5.0, true, false, true);
   h.AddScalar (params, "intensity",     1.0, 0.0, 10.0, 0.0, 10.0, true, false, true);
   h.AddColor3 (params, "sky_tint",      1.0, 1.0, 1.0, true, false, true);
   h.AddColor3 (params, "sun_tint",      1.0, 1.0, 1.0, true, false, true);
   h.AddVector3(params, "X",             1.0, 0.0, 0.0, -1.0, 1.0, -1.0, 1.0);
   h.AddVector3(params, "Y",             0.0, 1.0, 0.0, -1.0, 1.0, -1.0, 1.0);
   h.AddVector3(params, "Z",             0.0, 0.0, 1.0, -1.0, 1.0, -1.0, 1.0);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);
   
   physical_sky_Layout(shaderDef.PPGLayout);

   return true;
}

function physical_sky_Layout(in_layout)
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/SItoAUG/Physical+Sky");    

   item = in_layout.AddItem("turbidity",     "Turbidity");
   item = in_layout.AddItem("ground_albedo", "Ground Albedo");
   item = in_layout.AddItem("elevation",     "Elevation");
   item = in_layout.AddItem("azimuth",       "Azimuth");
   item = in_layout.AddItem("intensity",     "Intensity");
   item = in_layout.AddItem("sky_tint",      "Sky Tint");
   item = in_layout.AddItem("enable_sun",    "Enable Sun");
   item = in_layout.AddItem("sun_tint",      "Sun Tint");
   item = in_layout.AddItem("sun_size",      "Sun Size");
   in_layout.AddGroup("Orientation");
      in_layout.AddGroup("");
         item = in_layout.AddItem("X", "X");
         item = in_layout.AddItem("Y", "Y");
         item = in_layout.AddItem("Z", "Z");
      in_layout.EndGroup();
      in_layout.AddRow();
         item = in_layout.AddButton("SetExpression",    "Set Expression");
         item = in_layout.AddButton("RemoveExpression", "Remove Expression");
      in_layout.EndRow();
   in_layout.EndGroup();

   in_layout.SetAttribute(siUILogicPrefix, "physical_sky_");
}

function physical_sky_OnInit()
{
   physical_sky_enable_sun_OnChanged();
}

function physical_sky_enable_sun_OnChanged()
{
   PPG.sun_size.Enable(PPG.enable_sun.Value);   
   PPG.sun_tint.Enable(PPG.enable_sun.Value);   
}

// Xx = cy*cz;
// Xy = cy*sz;
// Xz = -sy;
// Yx = sx*sy*cz - cx*sz;
// Yy = sx*sy*sz + cx*cz;
// Yz = sx*cy;
// Zx = cx*sy*cz + sx*sz;
// Zy = cx*sy*sz - sx*cz;
// Zz = cx*cy;
function physical_sky_SetExpression_OnClicked() 
{
   var aRtn = PickObject("Pick Object", "Pick Object");
   var button = aRtn.Value("ButtonPressed");
   if (button == 1) // LMB
   {
      var obj  = aRtn.Value("PickedElement");
      var rotx = obj.fullname + ".kine.global.rotx";
      var roty = obj.fullname + ".kine.global.roty";
      var rotz = obj.fullname + ".kine.global.rotz";
      var pset = PPG.Inspected.Item(0);
      var s;

      s = "cos(" + roty + ")*cos(" + rotz + ")";
      SetExpr(pset + ".X.x", s);
      s = "cos(" + roty + ")*sin(" + rotz + ")";;
      SetExpr(pset + ".X.y", s);
      s = "-sin(" + roty + ")";
      SetExpr(pset + ".X.z", s);
      s = "sin(" + rotx + ")*sin(" + roty + ")*cos(" + rotz + ") - cos(" + rotx + ")*sin(" + rotz + ")";
      SetExpr(pset + ".Y.x", s);
      s = "sin(" + rotx + ")*sin(" + roty + ")*sin(" + rotz + ") + cos(" + rotx + ")*cos(" + rotz + ")";
      SetExpr(pset + ".Y.y", s);
      s = "sin(" + rotx + ")*cos(" + roty + ")";
      SetExpr(pset + ".Y.z", s);
      s = "cos(" + rotx + ")*sin(" + roty + ")*cos(" + rotz + ") + sin(" + rotx + ")*sin(" + rotz + ")";
      SetExpr(pset + ".Z.x", s);
      s = "cos(" + rotx + ")*sin(" + roty + ")*sin(" + rotz + ") - sin(" + rotx + ")*cos(" + rotz + ")";
      SetExpr(pset + ".Z.y", s);
      s = "cos(" + rotx + ")*cos(" + roty + ")";
      SetExpr(pset + ".Z.z", s);
   }
}

function physical_sky_RemoveExpression_OnClicked() 
{
   var pset = PPG.Inspected.Item(0);
   RemoveAnimation(pset + ".X.x", null, null, null, null, null);
   RemoveAnimation(pset + ".X.y", null, null, null, null, null);
   RemoveAnimation(pset + ".X.z", null, null, null, null, null);
   RemoveAnimation(pset + ".Y.x", null, null, null, null, null);
   RemoveAnimation(pset + ".Y.y", null, null, null, null, null);
   RemoveAnimation(pset + ".Y.z", null, null, null, null, null);
   RemoveAnimation(pset + ".Z.x", null, null, null, null, null);
   RemoveAnimation(pset + ".Z.y", null, null, null, null, null);
   RemoveAnimation(pset + ".Z.z", null, null, null, null, null);
}

function Arnold_operator_1_0_DefineInfo(in_ctxt)
{
   in_ctxt.SetAttribute("DisplayName", "operator");
   in_ctxt.SetAttribute("Category", "Arnold/Operators");
   return true;
}

function Arnold_operator_1_0_Define(in_ctxt)
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyTexture);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   h.AddNode(params, "operator");

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);

   return true;
}
