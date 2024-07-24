//
//   Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_GARCH_GLAPI_H
#define PXR_IMAGING_GARCH_GLAPI_H

#if !defined(__gl_h_)
#define __gl_h_
#if !defined(__gl3_h_)
#define __gl3_h_

#include "pxr/pxr.h"
#include "pxr/imaging/garch/api.h"

#include "pxr/imaging/garch/khrplatform.h"

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef khronos_int8_t GLbyte;
typedef khronos_uint8_t GLubyte;
typedef khronos_int16_t GLshort;
typedef khronos_uint16_t GLushort;
typedef int GLint;
typedef unsigned int GLuint;
typedef khronos_int32_t GLclampx;
typedef int GLsizei;
typedef khronos_float_t GLfloat;
typedef khronos_float_t GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void *GLeglClientBufferEXT;
typedef void *GLeglImageOES;
typedef char GLchar;
typedef char GLcharARB;
#ifdef __APPLE__
typedef void *GLhandleARB;
#else
typedef unsigned int GLhandleARB;
#endif
typedef khronos_uint16_t GLhalf;
typedef khronos_uint16_t GLhalfARB;
typedef khronos_int32_t GLfixed;
typedef khronos_intptr_t GLintptr;
typedef khronos_intptr_t GLintptrARB;
typedef khronos_ssize_t GLsizeiptr;
typedef khronos_ssize_t GLsizeiptrARB;
typedef khronos_int64_t GLint64;
typedef khronos_int64_t GLint64EXT;
typedef khronos_uint64_t GLuint64;
typedef khronos_uint64_t GLuint64EXT;
typedef struct __GLsync *GLsync;
struct _cl_context;
struct _cl_event;
typedef void (*GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (*GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (*GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (*GLDEBUGPROCAMD)(GLuint id,GLenum category,GLenum severity,GLsizei length,const GLchar *message,void *userParam);
typedef unsigned short GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;
typedef void (*GLVULKANPROCNV)(void);


#define GL_VERSION_1_0 1
#define GL_VERSION_1_1 1
#define GL_VERSION_1_2 1
#define GL_VERSION_1_3 1
#define GL_VERSION_1_4 1
#define GL_VERSION_1_5 1
#define GL_VERSION_2_0 1
#define GL_VERSION_2_1 1
#define GL_VERSION_3_0 1
#define GL_VERSION_3_1 1
#define GL_VERSION_3_2 1
#define GL_VERSION_3_3 1
#define GL_VERSION_4_0 1
#define GL_VERSION_4_1 1
#define GL_VERSION_4_2 1
#define GL_VERSION_4_3 1
#define GL_VERSION_4_4 1
#define GL_VERSION_4_5 1
#define GL_VERSION_4_6 1


#define GL_AMD_blend_minmax_factor 1
#define GL_AMD_conservative_depth 1
#define GL_AMD_debug_output 1
#define GL_AMD_depth_clamp_separate 1
#define GL_AMD_draw_buffers_blend 1
#define GL_AMD_framebuffer_multisample_advanced 1
#define GL_AMD_framebuffer_sample_positions 1
#define GL_AMD_gcn_shader 1
#define GL_AMD_gpu_shader_half_float 1
#define GL_AMD_gpu_shader_int16 1
#define GL_AMD_gpu_shader_int64 1
#define GL_AMD_interleaved_elements 1
#define GL_AMD_multi_draw_indirect 1
#define GL_AMD_name_gen_delete 1
#define GL_AMD_occlusion_query_event 1
#define GL_AMD_performance_monitor 1
#define GL_AMD_pinned_memory 1
#define GL_AMD_query_buffer_object 1
#define GL_AMD_sample_positions 1
#define GL_AMD_seamless_cubemap_per_texture 1
#define GL_AMD_shader_atomic_counter_ops 1
#define GL_AMD_shader_ballot 1
#define GL_AMD_shader_gpu_shader_half_float_fetch 1
#define GL_AMD_shader_image_load_store_lod 1
#define GL_AMD_shader_stencil_export 1
#define GL_AMD_shader_trinary_minmax 1
#define GL_AMD_shader_explicit_vertex_parameter 1
#define GL_AMD_sparse_texture 1
#define GL_AMD_stencil_operation_extended 1
#define GL_AMD_texture_gather_bias_lod 1
#define GL_AMD_texture_texture4 1
#define GL_AMD_transform_feedback3_lines_triangles 1
#define GL_AMD_transform_feedback4 1
#define GL_AMD_vertex_shader_layer 1
#define GL_AMD_vertex_shader_tessellator 1
#define GL_AMD_vertex_shader_viewport_index 1
#define GL_APPLE_aux_depth_stencil 1
#define GL_APPLE_client_storage 1
#define GL_APPLE_element_array 1
#define GL_APPLE_fence 1
#define GL_APPLE_float_pixels 1
#define GL_APPLE_flush_buffer_range 1
#define GL_APPLE_object_purgeable 1
#define GL_APPLE_rgb_422 1
#define GL_APPLE_row_bytes 1
#define GL_APPLE_specular_vector 1
#define GL_APPLE_texture_range 1
#define GL_APPLE_transform_hint 1
#define GL_APPLE_vertex_array_object 1
#define GL_APPLE_vertex_array_range 1
#define GL_APPLE_vertex_program_evaluators 1
#define GL_APPLE_ycbcr_422 1
#define GL_ARB_ES2_compatibility 1
#define GL_ARB_ES3_1_compatibility 1
#define GL_ARB_ES3_2_compatibility 1
#define GL_ARB_ES3_compatibility 1
#define GL_ARB_arrays_of_arrays 1
#define GL_ARB_base_instance 1
#define GL_ARB_bindless_texture 1
#define GL_ARB_blend_func_extended 1
#define GL_ARB_buffer_storage 1
#define GL_ARB_cl_event 1
#define GL_ARB_clear_buffer_object 1
#define GL_ARB_clear_texture 1
#define GL_ARB_clip_control 1
#define GL_ARB_color_buffer_float 1
#define GL_ARB_compatibility 1
#define GL_ARB_compressed_texture_pixel_storage 1
#define GL_ARB_compute_shader 1
#define GL_ARB_compute_variable_group_size 1
#define GL_ARB_conditional_render_inverted 1
#define GL_ARB_conservative_depth 1
#define GL_ARB_copy_buffer 1
#define GL_ARB_copy_image 1
#define GL_ARB_cull_distance 1
#define GL_ARB_debug_output 1
#define GL_ARB_depth_buffer_float 1
#define GL_ARB_depth_clamp 1
#define GL_ARB_depth_texture 1
#define GL_ARB_derivative_control 1
#define GL_ARB_direct_state_access 1
#define GL_ARB_draw_buffers 1
#define GL_ARB_draw_buffers_blend 1
#define GL_ARB_draw_elements_base_vertex 1
#define GL_ARB_draw_indirect 1
#define GL_ARB_draw_instanced 1
#define GL_ARB_enhanced_layouts 1
#define GL_ARB_explicit_attrib_location 1
#define GL_ARB_explicit_uniform_location 1
#define GL_ARB_fragment_coord_conventions 1
#define GL_ARB_fragment_layer_viewport 1
#define GL_ARB_fragment_program 1
#define GL_ARB_fragment_program_shadow 1
#define GL_ARB_fragment_shader 1
#define GL_ARB_fragment_shader_interlock 1
#define GL_ARB_framebuffer_no_attachments 1
#define GL_ARB_framebuffer_object 1
#define GL_ARB_framebuffer_sRGB 1
#define GL_ARB_geometry_shader4 1
#define GL_ARB_get_program_binary 1
#define GL_ARB_get_texture_sub_image 1
#define GL_ARB_gl_spirv 1
#define GL_ARB_gpu_shader5 1
#define GL_ARB_gpu_shader_fp64 1
#define GL_ARB_gpu_shader_int64 1
#define GL_ARB_half_float_pixel 1
#define GL_ARB_half_float_vertex 1
#define GL_ARB_imaging 1
#define GL_ARB_indirect_parameters 1
#define GL_ARB_instanced_arrays 1
#define GL_ARB_internalformat_query 1
#define GL_ARB_internalformat_query2 1
#define GL_ARB_invalidate_subdata 1
#define GL_ARB_map_buffer_alignment 1
#define GL_ARB_map_buffer_range 1
#define GL_ARB_matrix_palette 1
#define GL_ARB_multi_bind 1
#define GL_ARB_multi_draw_indirect 1
#define GL_ARB_multisample 1
#define GL_ARB_multitexture 1
#define GL_ARB_occlusion_query 1
#define GL_ARB_occlusion_query2 1
#define GL_ARB_parallel_shader_compile 1
#define GL_ARB_pipeline_statistics_query 1
#define GL_ARB_pixel_buffer_object 1
#define GL_ARB_point_parameters 1
#define GL_ARB_point_sprite 1
#define GL_ARB_polygon_offset_clamp 1
#define GL_ARB_post_depth_coverage 1
#define GL_ARB_program_interface_query 1
#define GL_ARB_provoking_vertex 1
#define GL_ARB_query_buffer_object 1
#define GL_ARB_robust_buffer_access_behavior 1
#define GL_ARB_robustness 1
#define GL_ARB_robustness_isolation 1
#define GL_ARB_sample_locations 1
#define GL_ARB_sample_shading 1
#define GL_ARB_sampler_objects 1
#define GL_ARB_seamless_cube_map 1
#define GL_ARB_seamless_cubemap_per_texture 1
#define GL_ARB_separate_shader_objects 1
#define GL_ARB_shader_atomic_counter_ops 1
#define GL_ARB_shader_atomic_counters 1
#define GL_ARB_shader_ballot 1
#define GL_ARB_shader_bit_encoding 1
#define GL_ARB_shader_clock 1
#define GL_ARB_shader_draw_parameters 1
#define GL_ARB_shader_group_vote 1
#define GL_ARB_shader_image_load_store 1
#define GL_ARB_shader_image_size 1
#define GL_ARB_shader_objects 1
#define GL_ARB_shader_precision 1
#define GL_ARB_shader_stencil_export 1
#define GL_ARB_shader_storage_buffer_object 1
#define GL_ARB_shader_subroutine 1
#define GL_ARB_shader_texture_image_samples 1
#define GL_ARB_shader_texture_lod 1
#define GL_ARB_shader_viewport_layer_array 1
#define GL_ARB_shading_language_100 1
#define GL_ARB_shading_language_420pack 1
#define GL_ARB_shading_language_include 1
#define GL_ARB_shading_language_packing 1
#define GL_ARB_shadow 1
#define GL_ARB_shadow_ambient 1
#define GL_ARB_sparse_buffer 1
#define GL_ARB_sparse_texture 1
#define GL_ARB_sparse_texture2 1
#define GL_ARB_sparse_texture_clamp 1
#define GL_ARB_spirv_extensions 1
#define GL_ARB_stencil_texturing 1
#define GL_ARB_sync 1
#define GL_ARB_tessellation_shader 1
#define GL_ARB_texture_barrier 1
#define GL_ARB_texture_border_clamp 1
#define GL_ARB_texture_buffer_object 1
#define GL_ARB_texture_buffer_object_rgb32 1
#define GL_ARB_texture_buffer_range 1
#define GL_ARB_texture_compression 1
#define GL_ARB_texture_compression_bptc 1
#define GL_ARB_texture_compression_rgtc 1
#define GL_ARB_texture_cube_map 1
#define GL_ARB_texture_cube_map_array 1
#define GL_ARB_texture_env_add 1
#define GL_ARB_texture_env_combine 1
#define GL_ARB_texture_env_crossbar 1
#define GL_ARB_texture_env_dot3 1
#define GL_ARB_texture_filter_anisotropic 1
#define GL_ARB_texture_filter_minmax 1
#define GL_ARB_texture_float 1
#define GL_ARB_texture_gather 1
#define GL_ARB_texture_mirror_clamp_to_edge 1
#define GL_ARB_texture_mirrored_repeat 1
#define GL_ARB_texture_multisample 1
#define GL_ARB_texture_non_power_of_two 1
#define GL_ARB_texture_query_levels 1
#define GL_ARB_texture_query_lod 1
#define GL_ARB_texture_rectangle 1
#define GL_ARB_texture_rg 1
#define GL_ARB_texture_rgb10_a2ui 1
#define GL_ARB_texture_stencil8 1
#define GL_ARB_texture_storage 1
#define GL_ARB_texture_storage_multisample 1
#define GL_ARB_texture_swizzle 1
#define GL_ARB_texture_view 1
#define GL_ARB_timer_query 1
#define GL_ARB_transform_feedback2 1
#define GL_ARB_transform_feedback3 1
#define GL_ARB_transform_feedback_instanced 1
#define GL_ARB_transform_feedback_overflow_query 1
#define GL_ARB_transpose_matrix 1
#define GL_ARB_uniform_buffer_object 1
#define GL_ARB_vertex_array_bgra 1
#define GL_ARB_vertex_array_object 1
#define GL_ARB_vertex_attrib_64bit 1
#define GL_ARB_vertex_attrib_binding 1
#define GL_ARB_vertex_blend 1
#define GL_ARB_vertex_buffer_object 1
#define GL_ARB_vertex_program 1
#define GL_ARB_vertex_shader 1
#define GL_ARB_vertex_type_10f_11f_11f_rev 1
#define GL_ARB_vertex_type_2_10_10_10_rev 1
#define GL_ARB_viewport_array 1
#define GL_ARB_window_pos 1
#define GL_EXT_422_pixels 1
#define GL_EXT_EGL_image_storage 1
#define GL_EXT_EGL_sync 1
#define GL_EXT_abgr 1
#define GL_EXT_bgra 1
#define GL_EXT_bindable_uniform 1
#define GL_EXT_blend_color 1
#define GL_EXT_blend_equation_separate 1
#define GL_EXT_blend_func_separate 1
#define GL_EXT_blend_logic_op 1
#define GL_EXT_blend_minmax 1
#define GL_EXT_blend_subtract 1
#define GL_EXT_clip_volume_hint 1
#define GL_EXT_cmyka 1
#define GL_EXT_color_subtable 1
#define GL_EXT_compiled_vertex_array 1
#define GL_EXT_convolution 1
#define GL_EXT_coordinate_frame 1
#define GL_EXT_copy_texture 1
#define GL_EXT_cull_vertex 1
#define GL_EXT_debug_label 1
#define GL_EXT_debug_marker 1
#define GL_EXT_depth_bounds_test 1
#define GL_EXT_direct_state_access 1
#define GL_EXT_draw_buffers2 1
#define GL_EXT_draw_instanced 1
#define GL_EXT_draw_range_elements 1
#define GL_EXT_external_buffer 1
#define GL_EXT_fog_coord 1
#define GL_EXT_framebuffer_blit 1
#define GL_EXT_framebuffer_multisample 1
#define GL_EXT_framebuffer_multisample_blit_scaled 1
#define GL_EXT_framebuffer_object 1
#define GL_EXT_framebuffer_sRGB 1
#define GL_EXT_geometry_shader4 1
#define GL_EXT_gpu_program_parameters 1
#define GL_EXT_gpu_shader4 1
#define GL_EXT_histogram 1
#define GL_EXT_index_array_formats 1
#define GL_EXT_index_func 1
#define GL_EXT_index_material 1
#define GL_EXT_index_texture 1
#define GL_EXT_light_texture 1
#define GL_EXT_memory_object 1
#define GL_EXT_memory_object_fd 1
#define GL_EXT_memory_object_win32 1
#define GL_EXT_misc_attribute 1
#define GL_EXT_multi_draw_arrays 1
#define GL_EXT_multisample 1
#define GL_EXT_multiview_tessellation_geometry_shader 1
#define GL_EXT_multiview_texture_multisample 1
#define GL_EXT_multiview_timer_query 1
#define GL_EXT_packed_depth_stencil 1
#define GL_EXT_packed_float 1
#define GL_EXT_packed_pixels 1
#define GL_EXT_paletted_texture 1
#define GL_EXT_pixel_buffer_object 1
#define GL_EXT_pixel_transform 1
#define GL_EXT_pixel_transform_color_table 1
#define GL_EXT_point_parameters 1
#define GL_EXT_polygon_offset 1
#define GL_EXT_polygon_offset_clamp 1
#define GL_EXT_post_depth_coverage 1
#define GL_EXT_provoking_vertex 1
#define GL_EXT_raster_multisample 1
#define GL_EXT_rescale_normal 1
#define GL_EXT_semaphore 1
#define GL_EXT_semaphore_fd 1
#define GL_EXT_semaphore_win32 1
#define GL_EXT_secondary_color 1
#define GL_EXT_separate_shader_objects 1
#define GL_EXT_separate_specular_color 1
#define GL_EXT_shader_framebuffer_fetch 1
#define GL_EXT_shader_framebuffer_fetch_non_coherent 1
#define GL_EXT_shader_image_load_formatted 1
#define GL_EXT_shader_image_load_store 1
#define GL_EXT_shader_integer_mix 1
#define GL_EXT_shadow_funcs 1
#define GL_EXT_shared_texture_palette 1
#define GL_EXT_sparse_texture2 1
#define GL_EXT_stencil_clear_tag 1
#define GL_EXT_stencil_two_side 1
#define GL_EXT_stencil_wrap 1
#define GL_EXT_subtexture 1
#define GL_EXT_texture 1
#define GL_EXT_texture3D 1
#define GL_EXT_texture_array 1
#define GL_EXT_texture_buffer_object 1
#define GL_EXT_texture_compression_latc 1
#define GL_EXT_texture_compression_rgtc 1
#define GL_EXT_texture_compression_s3tc 1
#define GL_EXT_texture_cube_map 1
#define GL_EXT_texture_env_add 1
#define GL_EXT_texture_env_combine 1
#define GL_EXT_texture_env_dot3 1
#define GL_EXT_texture_filter_anisotropic 1
#define GL_EXT_texture_filter_minmax 1
#define GL_EXT_texture_integer 1
#define GL_EXT_texture_lod_bias 1
#define GL_EXT_texture_mirror_clamp 1
#define GL_EXT_texture_object 1
#define GL_EXT_texture_perturb_normal 1
#define GL_EXT_texture_sRGB 1
#define GL_EXT_texture_sRGB_R8 1
#define GL_EXT_texture_sRGB_decode 1
#define GL_EXT_texture_shared_exponent 1
#define GL_EXT_texture_snorm 1
#define GL_EXT_texture_swizzle 1
#define GL_NV_timeline_semaphore 1
#define GL_EXT_timer_query 1
#define GL_EXT_transform_feedback 1
#define GL_EXT_vertex_array 1
#define GL_EXT_vertex_array_bgra 1
#define GL_EXT_vertex_attrib_64bit 1
#define GL_EXT_vertex_shader 1
#define GL_EXT_vertex_weighting 1
#define GL_EXT_win32_keyed_mutex 1
#define GL_EXT_window_rectangles 1
#define GL_EXT_x11_sync_object 1
#define GL_INTEL_conservative_rasterization 1
#define GL_INTEL_fragment_shader_ordering 1
#define GL_INTEL_framebuffer_CMAA 1
#define GL_INTEL_map_texture 1
#define GL_INTEL_blackhole_render 1
#define GL_INTEL_parallel_arrays 1
#define GL_INTEL_performance_query 1
#define GL_KHR_blend_equation_advanced 1
#define GL_KHR_blend_equation_advanced_coherent 1
#define GL_KHR_context_flush_control 1
#define GL_KHR_debug 1
#define GL_KHR_no_error 1
#define GL_KHR_robust_buffer_access_behavior 1
#define GL_KHR_robustness 1
#define GL_KHR_shader_subgroup 1
#define GL_KHR_texture_compression_astc_hdr 1
#define GL_KHR_texture_compression_astc_ldr 1
#define GL_KHR_texture_compression_astc_sliced_3d 1
#define GL_KHR_parallel_shader_compile 1
#define GL_NVX_blend_equation_advanced_multi_draw_buffers 1
#define GL_NVX_conditional_render 1
#define GL_NVX_gpu_memory_info 1
#define GL_NVX_linked_gpu_multicast 1
#define GL_NV_alpha_to_coverage_dither_control 1
#define GL_NV_bindless_multi_draw_indirect 1
#define GL_NV_bindless_multi_draw_indirect_count 1
#define GL_NV_bindless_texture 1
#define GL_NV_blend_equation_advanced 1
#define GL_NV_blend_equation_advanced_coherent 1
#define GL_NV_blend_minmax_factor 1
#define GL_NV_blend_square 1
#define GL_NV_clip_space_w_scaling 1
#define GL_NV_command_list 1
#define GL_NV_compute_program5 1
#define GL_NV_compute_shader_derivatives 1
#define GL_NV_conditional_render 1
#define GL_NV_conservative_raster 1
#define GL_NV_conservative_raster_dilate 1
#define GL_NV_conservative_raster_pre_snap 1
#define GL_NV_conservative_raster_pre_snap_triangles 1
#define GL_NV_conservative_raster_underestimation 1
#define GL_NV_copy_depth_to_color 1
#define GL_NV_copy_image 1
#define GL_NV_deep_texture3D 1
#define GL_NV_depth_buffer_float 1
#define GL_NV_depth_clamp 1
#define GL_NV_draw_texture 1
#define GL_NV_draw_vulkan_image 1
#define GL_NV_evaluators 1
#define GL_NV_explicit_multisample 1
#define GL_NV_fence 1
#define GL_NV_fill_rectangle 1
#define GL_NV_float_buffer 1
#define GL_NV_fog_distance 1
#define GL_NV_fragment_coverage_to_color 1
#define GL_NV_fragment_program 1
#define GL_NV_fragment_program2 1
#define GL_NV_fragment_program4 1
#define GL_NV_fragment_program_option 1
#define GL_NV_fragment_shader_barycentric 1
#define GL_NV_fragment_shader_interlock 1
#define GL_NV_framebuffer_mixed_samples 1
#define GL_NV_framebuffer_multisample_coverage 1
#define GL_NV_geometry_program4 1
#define GL_NV_geometry_shader4 1
#define GL_NV_geometry_shader_passthrough 1
#define GL_NV_gpu_program4 1
#define GL_NV_gpu_program5 1
#define GL_NV_gpu_program5_mem_extended 1
#define GL_NV_gpu_shader5 1
#define GL_NV_half_float 1
#define GL_NV_internalformat_sample_query 1
#define GL_NV_light_max_exponent 1
#define GL_NV_gpu_multicast 1
#define GL_NVX_gpu_multicast2 1
#define GL_NVX_progress_fence 1
#define GL_NV_memory_attachment 1
#define GL_NV_memory_object_sparse 1
#define GL_NV_mesh_shader 1
#define GL_NV_multisample_coverage 1
#define GL_NV_multisample_filter_hint 1
#define GL_NV_occlusion_query 1
#define GL_NV_packed_depth_stencil 1
#define GL_NV_parameter_buffer_object 1
#define GL_NV_parameter_buffer_object2 1
#define GL_NV_path_rendering 1
#define GL_NV_path_rendering_shared_edge 1
#define GL_NV_pixel_data_range 1
#define GL_NV_point_sprite 1
#define GL_NV_present_video 1
#define GL_NV_primitive_restart 1
#define GL_NV_query_resource 1
#define GL_NV_query_resource_tag 1
#define GL_NV_register_combiners 1
#define GL_NV_register_combiners2 1
#define GL_NV_representative_fragment_test 1
#define GL_NV_robustness_video_memory_purge 1
#define GL_NV_sample_locations 1
#define GL_NV_sample_mask_override_coverage 1
#define GL_NV_scissor_exclusive 1
#define GL_NV_shader_atomic_counters 1
#define GL_NV_shader_atomic_float 1
#define GL_NV_shader_atomic_float64 1
#define GL_NV_shader_atomic_fp16_vector 1
#define GL_NV_shader_atomic_int64 1
#define GL_NV_shader_buffer_load 1
#define GL_NV_shader_buffer_store 1
#define GL_NV_shader_storage_buffer_object 1
#define GL_NV_shader_subgroup_partitioned 1
#define GL_NV_shader_texture_footprint 1
#define GL_NV_shader_thread_group 1
#define GL_NV_shader_thread_shuffle 1
#define GL_NV_shading_rate_image 1
#define GL_NV_stereo_view_rendering 1
#define GL_NV_tessellation_program5 1
#define GL_NV_texgen_emboss 1
#define GL_NV_texgen_reflection 1
#define GL_NV_texture_barrier 1
#define GL_NV_texture_compression_vtc 1
#define GL_NV_texture_env_combine4 1
#define GL_NV_texture_expand_normal 1
#define GL_NV_texture_multisample 1
#define GL_NV_texture_rectangle 1
#define GL_NV_texture_rectangle_compressed 1
#define GL_NV_texture_shader 1
#define GL_NV_texture_shader2 1
#define GL_NV_texture_shader3 1
#define GL_NV_transform_feedback 1
#define GL_NV_transform_feedback2 1
#define GL_NV_uniform_buffer_unified_memory 1
#define GL_NV_vdpau_interop 1
#define GL_NV_vdpau_interop2 1
#define GL_NV_vertex_array_range 1
#define GL_NV_vertex_array_range2 1
#define GL_NV_vertex_attrib_integer_64bit 1
#define GL_NV_vertex_buffer_unified_memory 1
#define GL_NV_vertex_program 1
#define GL_NV_vertex_program1_1 1
#define GL_NV_vertex_program2 1
#define GL_NV_vertex_program2_option 1
#define GL_NV_vertex_program3 1
#define GL_NV_vertex_program4 1
#define GL_NV_video_capture 1
#define GL_NV_viewport_array2 1
#define GL_NV_viewport_swizzle 1
#define GL_EXT_texture_shadow_lod 1


#define GL_CURRENT_BIT                                                0x00000001
#define GL_POINT_BIT                                                  0x00000002
#define GL_LINE_BIT                                                   0x00000004
#define GL_POLYGON_BIT                                                0x00000008
#define GL_POLYGON_STIPPLE_BIT                                        0x00000010
#define GL_PIXEL_MODE_BIT                                             0x00000020
#define GL_LIGHTING_BIT                                               0x00000040
#define GL_FOG_BIT                                                    0x00000080
#define GL_DEPTH_BUFFER_BIT                                           0x00000100
#define GL_ACCUM_BUFFER_BIT                                           0x00000200
#define GL_STENCIL_BUFFER_BIT                                         0x00000400
#define GL_VIEWPORT_BIT                                               0x00000800
#define GL_TRANSFORM_BIT                                              0x00001000
#define GL_ENABLE_BIT                                                 0x00002000
#define GL_COLOR_BUFFER_BIT                                           0x00004000
#define GL_HINT_BIT                                                   0x00008000
#define GL_EVAL_BIT                                                   0x00010000
#define GL_LIST_BIT                                                   0x00020000
#define GL_TEXTURE_BIT                                                0x00040000
#define GL_SCISSOR_BIT                                                0x00080000
#define GL_MULTISAMPLE_BIT                                            0x20000000
#define GL_MULTISAMPLE_BIT_ARB                                        0x20000000
#define GL_MULTISAMPLE_BIT_EXT                                        0x20000000
#define GL_ALL_ATTRIB_BITS                                            0xFFFFFFFF
#define GL_DYNAMIC_STORAGE_BIT                                        0x0100
#define GL_CLIENT_STORAGE_BIT                                         0x0200
#define GL_SPARSE_STORAGE_BIT_ARB                                     0x0400
#define GL_LGPU_SEPARATE_STORAGE_BIT_NVX                              0x0800
#define GL_PER_GPU_STORAGE_BIT_NV                                     0x0800
#define GL_CLIENT_PIXEL_STORE_BIT                                     0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT                                    0x00000002
#define GL_CLIENT_ALL_ATTRIB_BITS                                     0xFFFFFFFF
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT                        0x00000001
#define GL_CONTEXT_FLAG_DEBUG_BIT                                     0x00000002
#define GL_CONTEXT_FLAG_DEBUG_BIT_KHR                                 0x00000002
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT                             0x00000004
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB                         0x00000004
#define GL_CONTEXT_FLAG_NO_ERROR_BIT                                  0x00000008
#define GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR                              0x00000008
#define GL_CONTEXT_CORE_PROFILE_BIT                                   0x00000001
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT                          0x00000002
#define GL_MAP_READ_BIT                                               0x0001
#define GL_MAP_WRITE_BIT                                              0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT                                   0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT                                  0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT                                     0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT                                     0x0020
#define GL_MAP_PERSISTENT_BIT                                         0x0040
#define GL_MAP_COHERENT_BIT                                           0x0080
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT                            0x00000001
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT_EXT                        0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT                                  0x00000002
#define GL_ELEMENT_ARRAY_BARRIER_BIT_EXT                              0x00000002
#define GL_UNIFORM_BARRIER_BIT                                        0x00000004
#define GL_UNIFORM_BARRIER_BIT_EXT                                    0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT                                  0x00000008
#define GL_TEXTURE_FETCH_BARRIER_BIT_EXT                              0x00000008
#define GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV                        0x00000010
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT                            0x00000020
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT_EXT                        0x00000020
#define GL_COMMAND_BARRIER_BIT                                        0x00000040
#define GL_COMMAND_BARRIER_BIT_EXT                                    0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT                                   0x00000080
#define GL_PIXEL_BUFFER_BARRIER_BIT_EXT                               0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT                                 0x00000100
#define GL_TEXTURE_UPDATE_BARRIER_BIT_EXT                             0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT                                  0x00000200
#define GL_BUFFER_UPDATE_BARRIER_BIT_EXT                              0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT                                    0x00000400
#define GL_FRAMEBUFFER_BARRIER_BIT_EXT                                0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT                             0x00000800
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT_EXT                         0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT                                 0x00001000
#define GL_ATOMIC_COUNTER_BARRIER_BIT_EXT                             0x00001000
#define GL_SHADER_STORAGE_BARRIER_BIT                                 0x00002000
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT                           0x00004000
#define GL_QUERY_BUFFER_BARRIER_BIT                                   0x00008000
#define GL_ALL_BARRIER_BITS                                           0xFFFFFFFF
#define GL_ALL_BARRIER_BITS_EXT                                       0xFFFFFFFF
#define GL_QUERY_DEPTH_PASS_EVENT_BIT_AMD                             0x00000001
#define GL_QUERY_DEPTH_FAIL_EVENT_BIT_AMD                             0x00000002
#define GL_QUERY_STENCIL_FAIL_EVENT_BIT_AMD                           0x00000004
#define GL_QUERY_DEPTH_BOUNDS_FAIL_EVENT_BIT_AMD                      0x00000008
#define GL_QUERY_ALL_EVENT_BITS_AMD                                   0xFFFFFFFF
#define GL_SYNC_FLUSH_COMMANDS_BIT                                    0x00000001
#define GL_VERTEX_SHADER_BIT                                          0x00000001
#define GL_VERTEX_SHADER_BIT_EXT                                      0x00000001
#define GL_FRAGMENT_SHADER_BIT                                        0x00000002
#define GL_FRAGMENT_SHADER_BIT_EXT                                    0x00000002
#define GL_GEOMETRY_SHADER_BIT                                        0x00000004
#define GL_TESS_CONTROL_SHADER_BIT                                    0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT                                 0x00000010
#define GL_COMPUTE_SHADER_BIT                                         0x00000020
#define GL_MESH_SHADER_BIT_NV                                         0x00000040
#define GL_TASK_SHADER_BIT_NV                                         0x00000080
#define GL_ALL_SHADER_BITS                                            0xFFFFFFFF
#define GL_ALL_SHADER_BITS_EXT                                        0xFFFFFFFF
#define GL_SUBGROUP_FEATURE_BASIC_BIT_KHR                             0x00000001
#define GL_SUBGROUP_FEATURE_VOTE_BIT_KHR                              0x00000002
#define GL_SUBGROUP_FEATURE_ARITHMETIC_BIT_KHR                        0x00000004
#define GL_SUBGROUP_FEATURE_BALLOT_BIT_KHR                            0x00000008
#define GL_SUBGROUP_FEATURE_SHUFFLE_BIT_KHR                           0x00000010
#define GL_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT_KHR                  0x00000020
#define GL_SUBGROUP_FEATURE_CLUSTERED_BIT_KHR                         0x00000040
#define GL_SUBGROUP_FEATURE_QUAD_BIT_KHR                              0x00000080
#define GL_SUBGROUP_FEATURE_PARTITIONED_BIT_NV                        0x00000100
#define GL_TEXTURE_STORAGE_SPARSE_BIT_AMD                             0x00000001
#define GL_BOLD_BIT_NV                                                0x01
#define GL_ITALIC_BIT_NV                                              0x02
#define GL_GLYPH_WIDTH_BIT_NV                                         0x01
#define GL_GLYPH_HEIGHT_BIT_NV                                        0x02
#define GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV                          0x04
#define GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV                          0x08
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV                    0x10
#define GL_GLYPH_VERTICAL_BEARING_X_BIT_NV                            0x20
#define GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV                            0x40
#define GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV                      0x80
#define GL_GLYPH_HAS_KERNING_BIT_NV                                   0x100
#define GL_FONT_X_MIN_BOUNDS_BIT_NV                                   0x00010000
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV                                   0x00020000
#define GL_FONT_X_MAX_BOUNDS_BIT_NV                                   0x00040000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV                                   0x00080000
#define GL_FONT_UNITS_PER_EM_BIT_NV                                   0x00100000
#define GL_FONT_ASCENDER_BIT_NV                                       0x00200000
#define GL_FONT_DESCENDER_BIT_NV                                      0x00400000
#define GL_FONT_HEIGHT_BIT_NV                                         0x00800000
#define GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV                              0x01000000
#define GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV                             0x02000000
#define GL_FONT_UNDERLINE_POSITION_BIT_NV                             0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV                            0x08000000
#define GL_FONT_HAS_KERNING_BIT_NV                                    0x10000000
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV                              0x20000000
#define GL_PERFQUERY_SINGLE_CONTEXT_INTEL                             0x00000000
#define GL_PERFQUERY_GLOBAL_CONTEXT_INTEL                             0x00000001
#define GL_TERMINATE_SEQUENCE_COMMAND_NV                              0x0000
#define GL_NOP_COMMAND_NV                                             0x0001
#define GL_DRAW_ELEMENTS_COMMAND_NV                                   0x0002
#define GL_DRAW_ARRAYS_COMMAND_NV                                     0x0003
#define GL_DRAW_ELEMENTS_STRIP_COMMAND_NV                             0x0004
#define GL_DRAW_ARRAYS_STRIP_COMMAND_NV                               0x0005
#define GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV                         0x0006
#define GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV                           0x0007
#define GL_ELEMENT_ADDRESS_COMMAND_NV                                 0x0008
#define GL_ATTRIBUTE_ADDRESS_COMMAND_NV                               0x0009
#define GL_UNIFORM_ADDRESS_COMMAND_NV                                 0x000A
#define GL_BLEND_COLOR_COMMAND_NV                                     0x000B
#define GL_STENCIL_REF_COMMAND_NV                                     0x000C
#define GL_LINE_WIDTH_COMMAND_NV                                      0x000D
#define GL_POLYGON_OFFSET_COMMAND_NV                                  0x000E
#define GL_ALPHA_REF_COMMAND_NV                                       0x000F
#define GL_VIEWPORT_COMMAND_NV                                        0x0010
#define GL_SCISSOR_COMMAND_NV                                         0x0011
#define GL_FRONT_FACE_COMMAND_NV                                      0x0012
#define GL_LAYOUT_DEFAULT_INTEL                                       0
#define GL_LAYOUT_LINEAR_INTEL                                        1
#define GL_LAYOUT_LINEAR_CPU_CACHED_INTEL                             2
#define GL_CLOSE_PATH_NV                                              0x00
#define GL_MOVE_TO_NV                                                 0x02
#define GL_RELATIVE_MOVE_TO_NV                                        0x03
#define GL_LINE_TO_NV                                                 0x04
#define GL_RELATIVE_LINE_TO_NV                                        0x05
#define GL_HORIZONTAL_LINE_TO_NV                                      0x06
#define GL_RELATIVE_HORIZONTAL_LINE_TO_NV                             0x07
#define GL_VERTICAL_LINE_TO_NV                                        0x08
#define GL_RELATIVE_VERTICAL_LINE_TO_NV                               0x09
#define GL_QUADRATIC_CURVE_TO_NV                                      0x0A
#define GL_RELATIVE_QUADRATIC_CURVE_TO_NV                             0x0B
#define GL_CUBIC_CURVE_TO_NV                                          0x0C
#define GL_RELATIVE_CUBIC_CURVE_TO_NV                                 0x0D
#define GL_SMOOTH_QUADRATIC_CURVE_TO_NV                               0x0E
#define GL_RELATIVE_SMOOTH_QUADRATIC_CURVE_TO_NV                      0x0F
#define GL_SMOOTH_CUBIC_CURVE_TO_NV                                   0x10
#define GL_RELATIVE_SMOOTH_CUBIC_CURVE_TO_NV                          0x11
#define GL_SMALL_CCW_ARC_TO_NV                                        0x12
#define GL_RELATIVE_SMALL_CCW_ARC_TO_NV                               0x13
#define GL_SMALL_CW_ARC_TO_NV                                         0x14
#define GL_RELATIVE_SMALL_CW_ARC_TO_NV                                0x15
#define GL_LARGE_CCW_ARC_TO_NV                                        0x16
#define GL_RELATIVE_LARGE_CCW_ARC_TO_NV                               0x17
#define GL_LARGE_CW_ARC_TO_NV                                         0x18
#define GL_RELATIVE_LARGE_CW_ARC_TO_NV                                0x19
#define GL_CONIC_CURVE_TO_NV                                          0x1A
#define GL_RELATIVE_CONIC_CURVE_TO_NV                                 0x1B
#define GL_SHARED_EDGE_NV                                             0xC0
#define GL_ROUNDED_RECT_NV                                            0xE8
#define GL_RELATIVE_ROUNDED_RECT_NV                                   0xE9
#define GL_ROUNDED_RECT2_NV                                           0xEA
#define GL_RELATIVE_ROUNDED_RECT2_NV                                  0xEB
#define GL_ROUNDED_RECT4_NV                                           0xEC
#define GL_RELATIVE_ROUNDED_RECT4_NV                                  0xED
#define GL_ROUNDED_RECT8_NV                                           0xEE
#define GL_RELATIVE_ROUNDED_RECT8_NV                                  0xEF
#define GL_RESTART_PATH_NV                                            0xF0
#define GL_DUP_FIRST_CUBIC_CURVE_TO_NV                                0xF2
#define GL_DUP_LAST_CUBIC_CURVE_TO_NV                                 0xF4
#define GL_RECT_NV                                                    0xF6
#define GL_RELATIVE_RECT_NV                                           0xF7
#define GL_CIRCULAR_CCW_ARC_TO_NV                                     0xF8
#define GL_CIRCULAR_CW_ARC_TO_NV                                      0xFA
#define GL_CIRCULAR_TANGENT_ARC_TO_NV                                 0xFC
#define GL_ARC_TO_NV                                                  0xFE
#define GL_RELATIVE_ARC_TO_NV                                         0xFF
#define GL_NEXT_BUFFER_NV                                             -2
#define GL_SKIP_COMPONENTS4_NV                                        -3
#define GL_SKIP_COMPONENTS3_NV                                        -4
#define GL_SKIP_COMPONENTS2_NV                                        -5
#define GL_SKIP_COMPONENTS1_NV                                        -6
#define GL_FALSE                                                      0
#define GL_NO_ERROR                                                   0
#define GL_ZERO                                                       0
#define GL_NONE                                                       0
#define GL_TRUE                                                       1
#define GL_ONE                                                        1
#define GL_INVALID_INDEX                                              0xFFFFFFFF
#define GL_ALL_PIXELS_AMD                                             0xFFFFFFFF
#define GL_TIMEOUT_IGNORED                                            0xFFFFFFFFFFFFFFFF
#define GL_UUID_SIZE_EXT                                              16
#define GL_LUID_SIZE_EXT                                              8
#define GL_POINTS                                                     0x0000
#define GL_LINES                                                      0x0001
#define GL_LINE_LOOP                                                  0x0002
#define GL_LINE_STRIP                                                 0x0003
#define GL_TRIANGLES                                                  0x0004
#define GL_TRIANGLE_STRIP                                             0x0005
#define GL_TRIANGLE_FAN                                               0x0006
#define GL_QUADS                                                      0x0007
#define GL_QUAD_STRIP                                                 0x0008
#define GL_POLYGON                                                    0x0009
#define GL_LINES_ADJACENCY                                            0x000A
#define GL_LINES_ADJACENCY_ARB                                        0x000A
#define GL_LINES_ADJACENCY_EXT                                        0x000A
#define GL_LINE_STRIP_ADJACENCY                                       0x000B
#define GL_LINE_STRIP_ADJACENCY_ARB                                   0x000B
#define GL_LINE_STRIP_ADJACENCY_EXT                                   0x000B
#define GL_TRIANGLES_ADJACENCY                                        0x000C
#define GL_TRIANGLES_ADJACENCY_ARB                                    0x000C
#define GL_TRIANGLES_ADJACENCY_EXT                                    0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY                                   0x000D
#define GL_TRIANGLE_STRIP_ADJACENCY_ARB                               0x000D
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT                               0x000D
#define GL_PATCHES                                                    0x000E
#define GL_ACCUM                                                      0x0100
#define GL_LOAD                                                       0x0101
#define GL_RETURN                                                     0x0102
#define GL_MULT                                                       0x0103
#define GL_ADD                                                        0x0104
#define GL_NEVER                                                      0x0200
#define GL_LESS                                                       0x0201
#define GL_EQUAL                                                      0x0202
#define GL_LEQUAL                                                     0x0203
#define GL_GREATER                                                    0x0204
#define GL_NOTEQUAL                                                   0x0205
#define GL_GEQUAL                                                     0x0206
#define GL_ALWAYS                                                     0x0207
#define GL_SRC_COLOR                                                  0x0300
#define GL_ONE_MINUS_SRC_COLOR                                        0x0301
#define GL_SRC_ALPHA                                                  0x0302
#define GL_ONE_MINUS_SRC_ALPHA                                        0x0303
#define GL_DST_ALPHA                                                  0x0304
#define GL_ONE_MINUS_DST_ALPHA                                        0x0305
#define GL_DST_COLOR                                                  0x0306
#define GL_ONE_MINUS_DST_COLOR                                        0x0307
#define GL_SRC_ALPHA_SATURATE                                         0x0308
#define GL_FRONT_LEFT                                                 0x0400
#define GL_FRONT_RIGHT                                                0x0401
#define GL_BACK_LEFT                                                  0x0402
#define GL_BACK_RIGHT                                                 0x0403
#define GL_FRONT                                                      0x0404
#define GL_BACK                                                       0x0405
#define GL_LEFT                                                       0x0406
#define GL_RIGHT                                                      0x0407
#define GL_FRONT_AND_BACK                                             0x0408
#define GL_AUX0                                                       0x0409
#define GL_AUX1                                                       0x040A
#define GL_AUX2                                                       0x040B
#define GL_AUX3                                                       0x040C
#define GL_INVALID_ENUM                                               0x0500
#define GL_INVALID_VALUE                                              0x0501
#define GL_INVALID_OPERATION                                          0x0502
#define GL_STACK_OVERFLOW                                             0x0503
#define GL_STACK_OVERFLOW_KHR                                         0x0503
#define GL_STACK_UNDERFLOW                                            0x0504
#define GL_STACK_UNDERFLOW_KHR                                        0x0504
#define GL_OUT_OF_MEMORY                                              0x0505
#define GL_INVALID_FRAMEBUFFER_OPERATION                              0x0506
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT                          0x0506
#define GL_CONTEXT_LOST                                               0x0507
#define GL_CONTEXT_LOST_KHR                                           0x0507
#define GL_2D                                                         0x0600
#define GL_3D                                                         0x0601
#define GL_3D_COLOR                                                   0x0602
#define GL_3D_COLOR_TEXTURE                                           0x0603
#define GL_4D_COLOR_TEXTURE                                           0x0604
#define GL_PASS_THROUGH_TOKEN                                         0x0700
#define GL_POINT_TOKEN                                                0x0701
#define GL_LINE_TOKEN                                                 0x0702
#define GL_POLYGON_TOKEN                                              0x0703
#define GL_BITMAP_TOKEN                                               0x0704
#define GL_DRAW_PIXEL_TOKEN                                           0x0705
#define GL_COPY_PIXEL_TOKEN                                           0x0706
#define GL_LINE_RESET_TOKEN                                           0x0707
#define GL_EXP                                                        0x0800
#define GL_EXP2                                                       0x0801
#define GL_CW                                                         0x0900
#define GL_CCW                                                        0x0901
#define GL_COEFF                                                      0x0A00
#define GL_ORDER                                                      0x0A01
#define GL_DOMAIN                                                     0x0A02
#define GL_CURRENT_COLOR                                              0x0B00
#define GL_CURRENT_INDEX                                              0x0B01
#define GL_CURRENT_NORMAL                                             0x0B02
#define GL_CURRENT_TEXTURE_COORDS                                     0x0B03
#define GL_CURRENT_RASTER_COLOR                                       0x0B04
#define GL_CURRENT_RASTER_INDEX                                       0x0B05
#define GL_CURRENT_RASTER_TEXTURE_COORDS                              0x0B06
#define GL_CURRENT_RASTER_POSITION                                    0x0B07
#define GL_CURRENT_RASTER_POSITION_VALID                              0x0B08
#define GL_CURRENT_RASTER_DISTANCE                                    0x0B09
#define GL_POINT_SMOOTH                                               0x0B10
#define GL_POINT_SIZE                                                 0x0B11
#define GL_POINT_SIZE_RANGE                                           0x0B12
#define GL_SMOOTH_POINT_SIZE_RANGE                                    0x0B12
#define GL_POINT_SIZE_GRANULARITY                                     0x0B13
#define GL_SMOOTH_POINT_SIZE_GRANULARITY                              0x0B13
#define GL_LINE_SMOOTH                                                0x0B20
#define GL_LINE_WIDTH                                                 0x0B21
#define GL_LINE_WIDTH_RANGE                                           0x0B22
#define GL_SMOOTH_LINE_WIDTH_RANGE                                    0x0B22
#define GL_LINE_WIDTH_GRANULARITY                                     0x0B23
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY                              0x0B23
#define GL_LINE_STIPPLE                                               0x0B24
#define GL_LINE_STIPPLE_PATTERN                                       0x0B25
#define GL_LINE_STIPPLE_REPEAT                                        0x0B26
#define GL_LIST_MODE                                                  0x0B30
#define GL_MAX_LIST_NESTING                                           0x0B31
#define GL_LIST_BASE                                                  0x0B32
#define GL_LIST_INDEX                                                 0x0B33
#define GL_POLYGON_MODE                                               0x0B40
#define GL_POLYGON_SMOOTH                                             0x0B41
#define GL_POLYGON_STIPPLE                                            0x0B42
#define GL_EDGE_FLAG                                                  0x0B43
#define GL_CULL_FACE                                                  0x0B44
#define GL_CULL_FACE_MODE                                             0x0B45
#define GL_FRONT_FACE                                                 0x0B46
#define GL_LIGHTING                                                   0x0B50
#define GL_LIGHT_MODEL_LOCAL_VIEWER                                   0x0B51
#define GL_LIGHT_MODEL_TWO_SIDE                                       0x0B52
#define GL_LIGHT_MODEL_AMBIENT                                        0x0B53
#define GL_SHADE_MODEL                                                0x0B54
#define GL_COLOR_MATERIAL_FACE                                        0x0B55
#define GL_COLOR_MATERIAL_PARAMETER                                   0x0B56
#define GL_COLOR_MATERIAL                                             0x0B57
#define GL_FOG                                                        0x0B60
#define GL_FOG_INDEX                                                  0x0B61
#define GL_FOG_DENSITY                                                0x0B62
#define GL_FOG_START                                                  0x0B63
#define GL_FOG_END                                                    0x0B64
#define GL_FOG_MODE                                                   0x0B65
#define GL_FOG_COLOR                                                  0x0B66
#define GL_DEPTH_RANGE                                                0x0B70
#define GL_DEPTH_TEST                                                 0x0B71
#define GL_DEPTH_WRITEMASK                                            0x0B72
#define GL_DEPTH_CLEAR_VALUE                                          0x0B73
#define GL_DEPTH_FUNC                                                 0x0B74
#define GL_ACCUM_CLEAR_VALUE                                          0x0B80
#define GL_STENCIL_TEST                                               0x0B90
#define GL_STENCIL_CLEAR_VALUE                                        0x0B91
#define GL_STENCIL_FUNC                                               0x0B92
#define GL_STENCIL_VALUE_MASK                                         0x0B93
#define GL_STENCIL_FAIL                                               0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL                                    0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS                                    0x0B96
#define GL_STENCIL_REF                                                0x0B97
#define GL_STENCIL_WRITEMASK                                          0x0B98
#define GL_MATRIX_MODE                                                0x0BA0
#define GL_NORMALIZE                                                  0x0BA1
#define GL_VIEWPORT                                                   0x0BA2
#define GL_MODELVIEW_STACK_DEPTH                                      0x0BA3
#define GL_MODELVIEW0_STACK_DEPTH_EXT                                 0x0BA3
#define GL_PATH_MODELVIEW_STACK_DEPTH_NV                              0x0BA3
#define GL_PROJECTION_STACK_DEPTH                                     0x0BA4
#define GL_PATH_PROJECTION_STACK_DEPTH_NV                             0x0BA4
#define GL_TEXTURE_STACK_DEPTH                                        0x0BA5
#define GL_MODELVIEW_MATRIX                                           0x0BA6
#define GL_MODELVIEW0_MATRIX_EXT                                      0x0BA6
#define GL_PATH_MODELVIEW_MATRIX_NV                                   0x0BA6
#define GL_PROJECTION_MATRIX                                          0x0BA7
#define GL_PATH_PROJECTION_MATRIX_NV                                  0x0BA7
#define GL_TEXTURE_MATRIX                                             0x0BA8
#define GL_ATTRIB_STACK_DEPTH                                         0x0BB0
#define GL_CLIENT_ATTRIB_STACK_DEPTH                                  0x0BB1
#define GL_ALPHA_TEST                                                 0x0BC0
#define GL_ALPHA_TEST_FUNC                                            0x0BC1
#define GL_ALPHA_TEST_REF                                             0x0BC2
#define GL_DITHER                                                     0x0BD0
#define GL_BLEND_DST                                                  0x0BE0
#define GL_BLEND_SRC                                                  0x0BE1
#define GL_BLEND                                                      0x0BE2
#define GL_LOGIC_OP_MODE                                              0x0BF0
#define GL_INDEX_LOGIC_OP                                             0x0BF1
#define GL_LOGIC_OP                                                   0x0BF1
#define GL_COLOR_LOGIC_OP                                             0x0BF2
#define GL_AUX_BUFFERS                                                0x0C00
#define GL_DRAW_BUFFER                                                0x0C01
#define GL_READ_BUFFER                                                0x0C02
#define GL_SCISSOR_BOX                                                0x0C10
#define GL_SCISSOR_TEST                                               0x0C11
#define GL_INDEX_CLEAR_VALUE                                          0x0C20
#define GL_INDEX_WRITEMASK                                            0x0C21
#define GL_COLOR_CLEAR_VALUE                                          0x0C22
#define GL_COLOR_WRITEMASK                                            0x0C23
#define GL_INDEX_MODE                                                 0x0C30
#define GL_RGBA_MODE                                                  0x0C31
#define GL_DOUBLEBUFFER                                               0x0C32
#define GL_STEREO                                                     0x0C33
#define GL_RENDER_MODE                                                0x0C40
#define GL_PERSPECTIVE_CORRECTION_HINT                                0x0C50
#define GL_POINT_SMOOTH_HINT                                          0x0C51
#define GL_LINE_SMOOTH_HINT                                           0x0C52
#define GL_POLYGON_SMOOTH_HINT                                        0x0C53
#define GL_FOG_HINT                                                   0x0C54
#define GL_TEXTURE_GEN_S                                              0x0C60
#define GL_TEXTURE_GEN_T                                              0x0C61
#define GL_TEXTURE_GEN_R                                              0x0C62
#define GL_TEXTURE_GEN_Q                                              0x0C63
#define GL_PIXEL_MAP_I_TO_I                                           0x0C70
#define GL_PIXEL_MAP_S_TO_S                                           0x0C71
#define GL_PIXEL_MAP_I_TO_R                                           0x0C72
#define GL_PIXEL_MAP_I_TO_G                                           0x0C73
#define GL_PIXEL_MAP_I_TO_B                                           0x0C74
#define GL_PIXEL_MAP_I_TO_A                                           0x0C75
#define GL_PIXEL_MAP_R_TO_R                                           0x0C76
#define GL_PIXEL_MAP_G_TO_G                                           0x0C77
#define GL_PIXEL_MAP_B_TO_B                                           0x0C78
#define GL_PIXEL_MAP_A_TO_A                                           0x0C79
#define GL_PIXEL_MAP_I_TO_I_SIZE                                      0x0CB0
#define GL_PIXEL_MAP_S_TO_S_SIZE                                      0x0CB1
#define GL_PIXEL_MAP_I_TO_R_SIZE                                      0x0CB2
#define GL_PIXEL_MAP_I_TO_G_SIZE                                      0x0CB3
#define GL_PIXEL_MAP_I_TO_B_SIZE                                      0x0CB4
#define GL_PIXEL_MAP_I_TO_A_SIZE                                      0x0CB5
#define GL_PIXEL_MAP_R_TO_R_SIZE                                      0x0CB6
#define GL_PIXEL_MAP_G_TO_G_SIZE                                      0x0CB7
#define GL_PIXEL_MAP_B_TO_B_SIZE                                      0x0CB8
#define GL_PIXEL_MAP_A_TO_A_SIZE                                      0x0CB9
#define GL_UNPACK_SWAP_BYTES                                          0x0CF0
#define GL_UNPACK_LSB_FIRST                                           0x0CF1
#define GL_UNPACK_ROW_LENGTH                                          0x0CF2
#define GL_UNPACK_SKIP_ROWS                                           0x0CF3
#define GL_UNPACK_SKIP_PIXELS                                         0x0CF4
#define GL_UNPACK_ALIGNMENT                                           0x0CF5
#define GL_PACK_SWAP_BYTES                                            0x0D00
#define GL_PACK_LSB_FIRST                                             0x0D01
#define GL_PACK_ROW_LENGTH                                            0x0D02
#define GL_PACK_SKIP_ROWS                                             0x0D03
#define GL_PACK_SKIP_PIXELS                                           0x0D04
#define GL_PACK_ALIGNMENT                                             0x0D05
#define GL_MAP_COLOR                                                  0x0D10
#define GL_MAP_STENCIL                                                0x0D11
#define GL_INDEX_SHIFT                                                0x0D12
#define GL_INDEX_OFFSET                                               0x0D13
#define GL_RED_SCALE                                                  0x0D14
#define GL_RED_BIAS                                                   0x0D15
#define GL_ZOOM_X                                                     0x0D16
#define GL_ZOOM_Y                                                     0x0D17
#define GL_GREEN_SCALE                                                0x0D18
#define GL_GREEN_BIAS                                                 0x0D19
#define GL_BLUE_SCALE                                                 0x0D1A
#define GL_BLUE_BIAS                                                  0x0D1B
#define GL_ALPHA_SCALE                                                0x0D1C
#define GL_ALPHA_BIAS                                                 0x0D1D
#define GL_DEPTH_SCALE                                                0x0D1E
#define GL_DEPTH_BIAS                                                 0x0D1F
#define GL_MAX_EVAL_ORDER                                             0x0D30
#define GL_MAX_LIGHTS                                                 0x0D31
#define GL_MAX_CLIP_PLANES                                            0x0D32
#define GL_MAX_CLIP_DISTANCES                                         0x0D32
#define GL_MAX_TEXTURE_SIZE                                           0x0D33
#define GL_MAX_PIXEL_MAP_TABLE                                        0x0D34
#define GL_MAX_ATTRIB_STACK_DEPTH                                     0x0D35
#define GL_MAX_MODELVIEW_STACK_DEPTH                                  0x0D36
#define GL_PATH_MAX_MODELVIEW_STACK_DEPTH_NV                          0x0D36
#define GL_MAX_NAME_STACK_DEPTH                                       0x0D37
#define GL_MAX_PROJECTION_STACK_DEPTH                                 0x0D38
#define GL_PATH_MAX_PROJECTION_STACK_DEPTH_NV                         0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH                                    0x0D39
#define GL_MAX_VIEWPORT_DIMS                                          0x0D3A
#define GL_MAX_CLIENT_ATTRIB_STACK_DEPTH                              0x0D3B
#define GL_SUBPIXEL_BITS                                              0x0D50
#define GL_INDEX_BITS                                                 0x0D51
#define GL_RED_BITS                                                   0x0D52
#define GL_GREEN_BITS                                                 0x0D53
#define GL_BLUE_BITS                                                  0x0D54
#define GL_ALPHA_BITS                                                 0x0D55
#define GL_DEPTH_BITS                                                 0x0D56
#define GL_STENCIL_BITS                                               0x0D57
#define GL_ACCUM_RED_BITS                                             0x0D58
#define GL_ACCUM_GREEN_BITS                                           0x0D59
#define GL_ACCUM_BLUE_BITS                                            0x0D5A
#define GL_ACCUM_ALPHA_BITS                                           0x0D5B
#define GL_NAME_STACK_DEPTH                                           0x0D70
#define GL_AUTO_NORMAL                                                0x0D80
#define GL_MAP1_COLOR_4                                               0x0D90
#define GL_MAP1_INDEX                                                 0x0D91
#define GL_MAP1_NORMAL                                                0x0D92
#define GL_MAP1_TEXTURE_COORD_1                                       0x0D93
#define GL_MAP1_TEXTURE_COORD_2                                       0x0D94
#define GL_MAP1_TEXTURE_COORD_3                                       0x0D95
#define GL_MAP1_TEXTURE_COORD_4                                       0x0D96
#define GL_MAP1_VERTEX_3                                              0x0D97
#define GL_MAP1_VERTEX_4                                              0x0D98
#define GL_MAP2_COLOR_4                                               0x0DB0
#define GL_MAP2_INDEX                                                 0x0DB1
#define GL_MAP2_NORMAL                                                0x0DB2
#define GL_MAP2_TEXTURE_COORD_1                                       0x0DB3
#define GL_MAP2_TEXTURE_COORD_2                                       0x0DB4
#define GL_MAP2_TEXTURE_COORD_3                                       0x0DB5
#define GL_MAP2_TEXTURE_COORD_4                                       0x0DB6
#define GL_MAP2_VERTEX_3                                              0x0DB7
#define GL_MAP2_VERTEX_4                                              0x0DB8
#define GL_MAP1_GRID_DOMAIN                                           0x0DD0
#define GL_MAP1_GRID_SEGMENTS                                         0x0DD1
#define GL_MAP2_GRID_DOMAIN                                           0x0DD2
#define GL_MAP2_GRID_SEGMENTS                                         0x0DD3
#define GL_TEXTURE_1D                                                 0x0DE0
#define GL_TEXTURE_2D                                                 0x0DE1
#define GL_FEEDBACK_BUFFER_POINTER                                    0x0DF0
#define GL_FEEDBACK_BUFFER_SIZE                                       0x0DF1
#define GL_FEEDBACK_BUFFER_TYPE                                       0x0DF2
#define GL_SELECTION_BUFFER_POINTER                                   0x0DF3
#define GL_SELECTION_BUFFER_SIZE                                      0x0DF4
#define GL_TEXTURE_WIDTH                                              0x1000
#define GL_TEXTURE_HEIGHT                                             0x1001
#define GL_TEXTURE_INTERNAL_FORMAT                                    0x1003
#define GL_TEXTURE_COMPONENTS                                         0x1003
#define GL_TEXTURE_BORDER_COLOR                                       0x1004
#define GL_TEXTURE_BORDER                                             0x1005
#define GL_TEXTURE_TARGET                                             0x1006
#define GL_DONT_CARE                                                  0x1100
#define GL_FASTEST                                                    0x1101
#define GL_NICEST                                                     0x1102
#define GL_AMBIENT                                                    0x1200
#define GL_DIFFUSE                                                    0x1201
#define GL_SPECULAR                                                   0x1202
#define GL_POSITION                                                   0x1203
#define GL_SPOT_DIRECTION                                             0x1204
#define GL_SPOT_EXPONENT                                              0x1205
#define GL_SPOT_CUTOFF                                                0x1206
#define GL_CONSTANT_ATTENUATION                                       0x1207
#define GL_LINEAR_ATTENUATION                                         0x1208
#define GL_QUADRATIC_ATTENUATION                                      0x1209
#define GL_COMPILE                                                    0x1300
#define GL_COMPILE_AND_EXECUTE                                        0x1301
#define GL_BYTE                                                       0x1400
#define GL_UNSIGNED_BYTE                                              0x1401
#define GL_SHORT                                                      0x1402
#define GL_UNSIGNED_SHORT                                             0x1403
#define GL_INT                                                        0x1404
#define GL_UNSIGNED_INT                                               0x1405
#define GL_FLOAT                                                      0x1406
#define GL_2_BYTES                                                    0x1407
#define GL_2_BYTES_NV                                                 0x1407
#define GL_3_BYTES                                                    0x1408
#define GL_3_BYTES_NV                                                 0x1408
#define GL_4_BYTES                                                    0x1409
#define GL_4_BYTES_NV                                                 0x1409
#define GL_DOUBLE                                                     0x140A
#define GL_HALF_FLOAT                                                 0x140B
#define GL_HALF_FLOAT_ARB                                             0x140B
#define GL_HALF_FLOAT_NV                                              0x140B
#define GL_HALF_APPLE                                                 0x140B
#define GL_FIXED                                                      0x140C
#define GL_INT64_ARB                                                  0x140E
#define GL_INT64_NV                                                   0x140E
#define GL_UNSIGNED_INT64_ARB                                         0x140F
#define GL_UNSIGNED_INT64_NV                                          0x140F
#define GL_CLEAR                                                      0x1500
#define GL_AND                                                        0x1501
#define GL_AND_REVERSE                                                0x1502
#define GL_COPY                                                       0x1503
#define GL_AND_INVERTED                                               0x1504
#define GL_NOOP                                                       0x1505
#define GL_XOR                                                        0x1506
#define GL_XOR_NV                                                     0x1506
#define GL_OR                                                         0x1507
#define GL_NOR                                                        0x1508
#define GL_EQUIV                                                      0x1509
#define GL_INVERT                                                     0x150A
#define GL_OR_REVERSE                                                 0x150B
#define GL_COPY_INVERTED                                              0x150C
#define GL_OR_INVERTED                                                0x150D
#define GL_NAND                                                       0x150E
#define GL_SET                                                        0x150F
#define GL_EMISSION                                                   0x1600
#define GL_SHININESS                                                  0x1601
#define GL_AMBIENT_AND_DIFFUSE                                        0x1602
#define GL_COLOR_INDEXES                                              0x1603
#define GL_MODELVIEW                                                  0x1700
#define GL_MODELVIEW0_ARB                                             0x1700
#define GL_MODELVIEW0_EXT                                             0x1700
#define GL_PATH_MODELVIEW_NV                                          0x1700
#define GL_PROJECTION                                                 0x1701
#define GL_PATH_PROJECTION_NV                                         0x1701
#define GL_TEXTURE                                                    0x1702
#define GL_COLOR                                                      0x1800
#define GL_DEPTH                                                      0x1801
#define GL_STENCIL                                                    0x1802
#define GL_COLOR_INDEX                                                0x1900
#define GL_STENCIL_INDEX                                              0x1901
#define GL_DEPTH_COMPONENT                                            0x1902
#define GL_RED                                                        0x1903
#define GL_RED_NV                                                     0x1903
#define GL_GREEN                                                      0x1904
#define GL_GREEN_NV                                                   0x1904
#define GL_BLUE                                                       0x1905
#define GL_BLUE_NV                                                    0x1905
#define GL_ALPHA                                                      0x1906
#define GL_RGB                                                        0x1907
#define GL_RGBA                                                       0x1908
#define GL_LUMINANCE                                                  0x1909
#define GL_LUMINANCE_ALPHA                                            0x190A
#define GL_BITMAP                                                     0x1A00
#define GL_POINT                                                      0x1B00
#define GL_LINE                                                       0x1B01
#define GL_FILL                                                       0x1B02
#define GL_RENDER                                                     0x1C00
#define GL_FEEDBACK                                                   0x1C01
#define GL_SELECT                                                     0x1C02
#define GL_FLAT                                                       0x1D00
#define GL_SMOOTH                                                     0x1D01
#define GL_KEEP                                                       0x1E00
#define GL_REPLACE                                                    0x1E01
#define GL_INCR                                                       0x1E02
#define GL_DECR                                                       0x1E03
#define GL_VENDOR                                                     0x1F00
#define GL_RENDERER                                                   0x1F01
#define GL_VERSION                                                    0x1F02
#define GL_EXTENSIONS                                                 0x1F03
#define GL_S                                                          0x2000
#define GL_T                                                          0x2001
#define GL_R                                                          0x2002
#define GL_Q                                                          0x2003
#define GL_MODULATE                                                   0x2100
#define GL_DECAL                                                      0x2101
#define GL_TEXTURE_ENV_MODE                                           0x2200
#define GL_TEXTURE_ENV_COLOR                                          0x2201
#define GL_TEXTURE_ENV                                                0x2300
#define GL_EYE_LINEAR                                                 0x2400
#define GL_EYE_LINEAR_NV                                              0x2400
#define GL_OBJECT_LINEAR                                              0x2401
#define GL_OBJECT_LINEAR_NV                                           0x2401
#define GL_SPHERE_MAP                                                 0x2402
#define GL_TEXTURE_GEN_MODE                                           0x2500
#define GL_OBJECT_PLANE                                               0x2501
#define GL_EYE_PLANE                                                  0x2502
#define GL_NEAREST                                                    0x2600
#define GL_LINEAR                                                     0x2601
#define GL_NEAREST_MIPMAP_NEAREST                                     0x2700
#define GL_LINEAR_MIPMAP_NEAREST                                      0x2701
#define GL_NEAREST_MIPMAP_LINEAR                                      0x2702
#define GL_LINEAR_MIPMAP_LINEAR                                       0x2703
#define GL_TEXTURE_MAG_FILTER                                         0x2800
#define GL_TEXTURE_MIN_FILTER                                         0x2801
#define GL_TEXTURE_WRAP_S                                             0x2802
#define GL_TEXTURE_WRAP_T                                             0x2803
#define GL_CLAMP                                                      0x2900
#define GL_REPEAT                                                     0x2901
#define GL_POLYGON_OFFSET_UNITS                                       0x2A00
#define GL_POLYGON_OFFSET_POINT                                       0x2A01
#define GL_POLYGON_OFFSET_LINE                                        0x2A02
#define GL_R3_G3_B2                                                   0x2A10
#define GL_V2F                                                        0x2A20
#define GL_V3F                                                        0x2A21
#define GL_C4UB_V2F                                                   0x2A22
#define GL_C4UB_V3F                                                   0x2A23
#define GL_C3F_V3F                                                    0x2A24
#define GL_N3F_V3F                                                    0x2A25
#define GL_C4F_N3F_V3F                                                0x2A26
#define GL_T2F_V3F                                                    0x2A27
#define GL_T4F_V4F                                                    0x2A28
#define GL_T2F_C4UB_V3F                                               0x2A29
#define GL_T2F_C3F_V3F                                                0x2A2A
#define GL_T2F_N3F_V3F                                                0x2A2B
#define GL_T2F_C4F_N3F_V3F                                            0x2A2C
#define GL_T4F_C4F_N3F_V4F                                            0x2A2D
#define GL_CLIP_PLANE0                                                0x3000
#define GL_CLIP_DISTANCE0                                             0x3000
#define GL_CLIP_PLANE1                                                0x3001
#define GL_CLIP_DISTANCE1                                             0x3001
#define GL_CLIP_PLANE2                                                0x3002
#define GL_CLIP_DISTANCE2                                             0x3002
#define GL_CLIP_PLANE3                                                0x3003
#define GL_CLIP_DISTANCE3                                             0x3003
#define GL_CLIP_PLANE4                                                0x3004
#define GL_CLIP_DISTANCE4                                             0x3004
#define GL_CLIP_PLANE5                                                0x3005
#define GL_CLIP_DISTANCE5                                             0x3005
#define GL_CLIP_DISTANCE6                                             0x3006
#define GL_CLIP_DISTANCE7                                             0x3007
#define GL_LIGHT0                                                     0x4000
#define GL_LIGHT1                                                     0x4001
#define GL_LIGHT2                                                     0x4002
#define GL_LIGHT3                                                     0x4003
#define GL_LIGHT4                                                     0x4004
#define GL_LIGHT5                                                     0x4005
#define GL_LIGHT6                                                     0x4006
#define GL_LIGHT7                                                     0x4007
#define GL_ABGR_EXT                                                   0x8000
#define GL_CONSTANT_COLOR                                             0x8001
#define GL_CONSTANT_COLOR_EXT                                         0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR                                   0x8002
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT                               0x8002
#define GL_CONSTANT_ALPHA                                             0x8003
#define GL_CONSTANT_ALPHA_EXT                                         0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA                                   0x8004
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT                               0x8004
#define GL_BLEND_COLOR                                                0x8005
#define GL_BLEND_COLOR_EXT                                            0x8005
#define GL_FUNC_ADD                                                   0x8006
#define GL_FUNC_ADD_EXT                                               0x8006
#define GL_MIN                                                        0x8007
#define GL_MIN_EXT                                                    0x8007
#define GL_MAX                                                        0x8008
#define GL_MAX_EXT                                                    0x8008
#define GL_BLEND_EQUATION                                             0x8009
#define GL_BLEND_EQUATION_EXT                                         0x8009
#define GL_BLEND_EQUATION_RGB                                         0x8009
#define GL_BLEND_EQUATION_RGB_EXT                                     0x8009
#define GL_FUNC_SUBTRACT                                              0x800A
#define GL_FUNC_SUBTRACT_EXT                                          0x800A
#define GL_FUNC_REVERSE_SUBTRACT                                      0x800B
#define GL_FUNC_REVERSE_SUBTRACT_EXT                                  0x800B
#define GL_CMYK_EXT                                                   0x800C
#define GL_CMYKA_EXT                                                  0x800D
#define GL_PACK_CMYK_HINT_EXT                                         0x800E
#define GL_UNPACK_CMYK_HINT_EXT                                       0x800F
#define GL_CONVOLUTION_1D                                             0x8010
#define GL_CONVOLUTION_1D_EXT                                         0x8010
#define GL_CONVOLUTION_2D                                             0x8011
#define GL_CONVOLUTION_2D_EXT                                         0x8011
#define GL_SEPARABLE_2D                                               0x8012
#define GL_SEPARABLE_2D_EXT                                           0x8012
#define GL_CONVOLUTION_BORDER_MODE                                    0x8013
#define GL_CONVOLUTION_BORDER_MODE_EXT                                0x8013
#define GL_CONVOLUTION_FILTER_SCALE                                   0x8014
#define GL_CONVOLUTION_FILTER_SCALE_EXT                               0x8014
#define GL_CONVOLUTION_FILTER_BIAS                                    0x8015
#define GL_CONVOLUTION_FILTER_BIAS_EXT                                0x8015
#define GL_REDUCE                                                     0x8016
#define GL_REDUCE_EXT                                                 0x8016
#define GL_CONVOLUTION_FORMAT                                         0x8017
#define GL_CONVOLUTION_FORMAT_EXT                                     0x8017
#define GL_CONVOLUTION_WIDTH                                          0x8018
#define GL_CONVOLUTION_WIDTH_EXT                                      0x8018
#define GL_CONVOLUTION_HEIGHT                                         0x8019
#define GL_CONVOLUTION_HEIGHT_EXT                                     0x8019
#define GL_MAX_CONVOLUTION_WIDTH                                      0x801A
#define GL_MAX_CONVOLUTION_WIDTH_EXT                                  0x801A
#define GL_MAX_CONVOLUTION_HEIGHT                                     0x801B
#define GL_MAX_CONVOLUTION_HEIGHT_EXT                                 0x801B
#define GL_POST_CONVOLUTION_RED_SCALE                                 0x801C
#define GL_POST_CONVOLUTION_RED_SCALE_EXT                             0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE                               0x801D
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT                           0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE                                0x801E
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT                            0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE                               0x801F
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT                           0x801F
#define GL_POST_CONVOLUTION_RED_BIAS                                  0x8020
#define GL_POST_CONVOLUTION_RED_BIAS_EXT                              0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS                                0x8021
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT                            0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS                                 0x8022
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT                             0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS                                0x8023
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT                            0x8023
#define GL_HISTOGRAM                                                  0x8024
#define GL_HISTOGRAM_EXT                                              0x8024
#define GL_PROXY_HISTOGRAM                                            0x8025
#define GL_PROXY_HISTOGRAM_EXT                                        0x8025
#define GL_HISTOGRAM_WIDTH                                            0x8026
#define GL_HISTOGRAM_WIDTH_EXT                                        0x8026
#define GL_HISTOGRAM_FORMAT                                           0x8027
#define GL_HISTOGRAM_FORMAT_EXT                                       0x8027
#define GL_HISTOGRAM_RED_SIZE                                         0x8028
#define GL_HISTOGRAM_RED_SIZE_EXT                                     0x8028
#define GL_HISTOGRAM_GREEN_SIZE                                       0x8029
#define GL_HISTOGRAM_GREEN_SIZE_EXT                                   0x8029
#define GL_HISTOGRAM_BLUE_SIZE                                        0x802A
#define GL_HISTOGRAM_BLUE_SIZE_EXT                                    0x802A
#define GL_HISTOGRAM_ALPHA_SIZE                                       0x802B
#define GL_HISTOGRAM_ALPHA_SIZE_EXT                                   0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE                                   0x802C
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT                               0x802C
#define GL_HISTOGRAM_SINK                                             0x802D
#define GL_HISTOGRAM_SINK_EXT                                         0x802D
#define GL_MINMAX                                                     0x802E
#define GL_MINMAX_EXT                                                 0x802E
#define GL_MINMAX_FORMAT                                              0x802F
#define GL_MINMAX_FORMAT_EXT                                          0x802F
#define GL_MINMAX_SINK                                                0x8030
#define GL_MINMAX_SINK_EXT                                            0x8030
#define GL_TABLE_TOO_LARGE_EXT                                        0x8031
#define GL_TABLE_TOO_LARGE                                            0x8031
#define GL_UNSIGNED_BYTE_3_3_2                                        0x8032
#define GL_UNSIGNED_BYTE_3_3_2_EXT                                    0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4                                     0x8033
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT                                 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1                                     0x8034
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT                                 0x8034
#define GL_UNSIGNED_INT_8_8_8_8                                       0x8035
#define GL_UNSIGNED_INT_8_8_8_8_EXT                                   0x8035
#define GL_UNSIGNED_INT_10_10_10_2                                    0x8036
#define GL_UNSIGNED_INT_10_10_10_2_EXT                                0x8036
#define GL_POLYGON_OFFSET_EXT                                         0x8037
#define GL_POLYGON_OFFSET_FILL                                        0x8037
#define GL_POLYGON_OFFSET_FACTOR                                      0x8038
#define GL_POLYGON_OFFSET_FACTOR_EXT                                  0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT                                    0x8039
#define GL_RESCALE_NORMAL                                             0x803A
#define GL_RESCALE_NORMAL_EXT                                         0x803A
#define GL_ALPHA4                                                     0x803B
#define GL_ALPHA4_EXT                                                 0x803B
#define GL_ALPHA8                                                     0x803C
#define GL_ALPHA8_EXT                                                 0x803C
#define GL_ALPHA12                                                    0x803D
#define GL_ALPHA12_EXT                                                0x803D
#define GL_ALPHA16                                                    0x803E
#define GL_ALPHA16_EXT                                                0x803E
#define GL_LUMINANCE4                                                 0x803F
#define GL_LUMINANCE4_EXT                                             0x803F
#define GL_LUMINANCE8                                                 0x8040
#define GL_LUMINANCE8_EXT                                             0x8040
#define GL_LUMINANCE12                                                0x8041
#define GL_LUMINANCE12_EXT                                            0x8041
#define GL_LUMINANCE16                                                0x8042
#define GL_LUMINANCE16_EXT                                            0x8042
#define GL_LUMINANCE4_ALPHA4                                          0x8043
#define GL_LUMINANCE4_ALPHA4_EXT                                      0x8043
#define GL_LUMINANCE6_ALPHA2                                          0x8044
#define GL_LUMINANCE6_ALPHA2_EXT                                      0x8044
#define GL_LUMINANCE8_ALPHA8                                          0x8045
#define GL_LUMINANCE8_ALPHA8_EXT                                      0x8045
#define GL_LUMINANCE12_ALPHA4                                         0x8046
#define GL_LUMINANCE12_ALPHA4_EXT                                     0x8046
#define GL_LUMINANCE12_ALPHA12                                        0x8047
#define GL_LUMINANCE12_ALPHA12_EXT                                    0x8047
#define GL_LUMINANCE16_ALPHA16                                        0x8048
#define GL_LUMINANCE16_ALPHA16_EXT                                    0x8048
#define GL_INTENSITY                                                  0x8049
#define GL_INTENSITY_EXT                                              0x8049
#define GL_INTENSITY4                                                 0x804A
#define GL_INTENSITY4_EXT                                             0x804A
#define GL_INTENSITY8                                                 0x804B
#define GL_INTENSITY8_EXT                                             0x804B
#define GL_INTENSITY12                                                0x804C
#define GL_INTENSITY12_EXT                                            0x804C
#define GL_INTENSITY16                                                0x804D
#define GL_INTENSITY16_EXT                                            0x804D
#define GL_RGB2_EXT                                                   0x804E
#define GL_RGB4                                                       0x804F
#define GL_RGB4_EXT                                                   0x804F
#define GL_RGB5                                                       0x8050
#define GL_RGB5_EXT                                                   0x8050
#define GL_RGB8                                                       0x8051
#define GL_RGB8_EXT                                                   0x8051
#define GL_RGB10                                                      0x8052
#define GL_RGB10_EXT                                                  0x8052
#define GL_RGB12                                                      0x8053
#define GL_RGB12_EXT                                                  0x8053
#define GL_RGB16                                                      0x8054
#define GL_RGB16_EXT                                                  0x8054
#define GL_RGBA2                                                      0x8055
#define GL_RGBA2_EXT                                                  0x8055
#define GL_RGBA4                                                      0x8056
#define GL_RGBA4_EXT                                                  0x8056
#define GL_RGB5_A1                                                    0x8057
#define GL_RGB5_A1_EXT                                                0x8057
#define GL_RGBA8                                                      0x8058
#define GL_RGBA8_EXT                                                  0x8058
#define GL_RGB10_A2                                                   0x8059
#define GL_RGB10_A2_EXT                                               0x8059
#define GL_RGBA12                                                     0x805A
#define GL_RGBA12_EXT                                                 0x805A
#define GL_RGBA16                                                     0x805B
#define GL_RGBA16_EXT                                                 0x805B
#define GL_TEXTURE_RED_SIZE                                           0x805C
#define GL_TEXTURE_RED_SIZE_EXT                                       0x805C
#define GL_TEXTURE_GREEN_SIZE                                         0x805D
#define GL_TEXTURE_GREEN_SIZE_EXT                                     0x805D
#define GL_TEXTURE_BLUE_SIZE                                          0x805E
#define GL_TEXTURE_BLUE_SIZE_EXT                                      0x805E
#define GL_TEXTURE_ALPHA_SIZE                                         0x805F
#define GL_TEXTURE_ALPHA_SIZE_EXT                                     0x805F
#define GL_TEXTURE_LUMINANCE_SIZE                                     0x8060
#define GL_TEXTURE_LUMINANCE_SIZE_EXT                                 0x8060
#define GL_TEXTURE_INTENSITY_SIZE                                     0x8061
#define GL_TEXTURE_INTENSITY_SIZE_EXT                                 0x8061
#define GL_REPLACE_EXT                                                0x8062
#define GL_PROXY_TEXTURE_1D                                           0x8063
#define GL_PROXY_TEXTURE_1D_EXT                                       0x8063
#define GL_PROXY_TEXTURE_2D                                           0x8064
#define GL_PROXY_TEXTURE_2D_EXT                                       0x8064
#define GL_TEXTURE_TOO_LARGE_EXT                                      0x8065
#define GL_TEXTURE_PRIORITY                                           0x8066
#define GL_TEXTURE_PRIORITY_EXT                                       0x8066
#define GL_TEXTURE_RESIDENT                                           0x8067
#define GL_TEXTURE_RESIDENT_EXT                                       0x8067
#define GL_TEXTURE_1D_BINDING_EXT                                     0x8068
#define GL_TEXTURE_BINDING_1D                                         0x8068
#define GL_TEXTURE_2D_BINDING_EXT                                     0x8069
#define GL_TEXTURE_BINDING_2D                                         0x8069
#define GL_TEXTURE_3D_BINDING_EXT                                     0x806A
#define GL_TEXTURE_BINDING_3D                                         0x806A
#define GL_PACK_SKIP_IMAGES                                           0x806B
#define GL_PACK_SKIP_IMAGES_EXT                                       0x806B
#define GL_PACK_IMAGE_HEIGHT                                          0x806C
#define GL_PACK_IMAGE_HEIGHT_EXT                                      0x806C
#define GL_UNPACK_SKIP_IMAGES                                         0x806D
#define GL_UNPACK_SKIP_IMAGES_EXT                                     0x806D
#define GL_UNPACK_IMAGE_HEIGHT                                        0x806E
#define GL_UNPACK_IMAGE_HEIGHT_EXT                                    0x806E
#define GL_TEXTURE_3D                                                 0x806F
#define GL_TEXTURE_3D_EXT                                             0x806F
#define GL_PROXY_TEXTURE_3D                                           0x8070
#define GL_PROXY_TEXTURE_3D_EXT                                       0x8070
#define GL_TEXTURE_DEPTH                                              0x8071
#define GL_TEXTURE_DEPTH_EXT                                          0x8071
#define GL_TEXTURE_WRAP_R                                             0x8072
#define GL_TEXTURE_WRAP_R_EXT                                         0x8072
#define GL_MAX_3D_TEXTURE_SIZE                                        0x8073
#define GL_MAX_3D_TEXTURE_SIZE_EXT                                    0x8073
#define GL_VERTEX_ARRAY                                               0x8074
#define GL_VERTEX_ARRAY_EXT                                           0x8074
#define GL_VERTEX_ARRAY_KHR                                           0x8074
#define GL_NORMAL_ARRAY                                               0x8075
#define GL_NORMAL_ARRAY_EXT                                           0x8075
#define GL_COLOR_ARRAY                                                0x8076
#define GL_COLOR_ARRAY_EXT                                            0x8076
#define GL_INDEX_ARRAY                                                0x8077
#define GL_INDEX_ARRAY_EXT                                            0x8077
#define GL_TEXTURE_COORD_ARRAY                                        0x8078
#define GL_TEXTURE_COORD_ARRAY_EXT                                    0x8078
#define GL_EDGE_FLAG_ARRAY                                            0x8079
#define GL_EDGE_FLAG_ARRAY_EXT                                        0x8079
#define GL_VERTEX_ARRAY_SIZE                                          0x807A
#define GL_VERTEX_ARRAY_SIZE_EXT                                      0x807A
#define GL_VERTEX_ARRAY_TYPE                                          0x807B
#define GL_VERTEX_ARRAY_TYPE_EXT                                      0x807B
#define GL_VERTEX_ARRAY_STRIDE                                        0x807C
#define GL_VERTEX_ARRAY_STRIDE_EXT                                    0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT                                     0x807D
#define GL_NORMAL_ARRAY_TYPE                                          0x807E
#define GL_NORMAL_ARRAY_TYPE_EXT                                      0x807E
#define GL_NORMAL_ARRAY_STRIDE                                        0x807F
#define GL_NORMAL_ARRAY_STRIDE_EXT                                    0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT                                     0x8080
#define GL_COLOR_ARRAY_SIZE                                           0x8081
#define GL_COLOR_ARRAY_SIZE_EXT                                       0x8081
#define GL_COLOR_ARRAY_TYPE                                           0x8082
#define GL_COLOR_ARRAY_TYPE_EXT                                       0x8082
#define GL_COLOR_ARRAY_STRIDE                                         0x8083
#define GL_COLOR_ARRAY_STRIDE_EXT                                     0x8083
#define GL_COLOR_ARRAY_COUNT_EXT                                      0x8084
#define GL_INDEX_ARRAY_TYPE                                           0x8085
#define GL_INDEX_ARRAY_TYPE_EXT                                       0x8085
#define GL_INDEX_ARRAY_STRIDE                                         0x8086
#define GL_INDEX_ARRAY_STRIDE_EXT                                     0x8086
#define GL_INDEX_ARRAY_COUNT_EXT                                      0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE                                   0x8088
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT                               0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE                                   0x8089
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT                               0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE                                 0x808A
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT                             0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT                              0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE                                     0x808C
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT                                 0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT                                  0x808D
#define GL_VERTEX_ARRAY_POINTER                                       0x808E
#define GL_VERTEX_ARRAY_POINTER_EXT                                   0x808E
#define GL_NORMAL_ARRAY_POINTER                                       0x808F
#define GL_NORMAL_ARRAY_POINTER_EXT                                   0x808F
#define GL_COLOR_ARRAY_POINTER                                        0x8090
#define GL_COLOR_ARRAY_POINTER_EXT                                    0x8090
#define GL_INDEX_ARRAY_POINTER                                        0x8091
#define GL_INDEX_ARRAY_POINTER_EXT                                    0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER                                0x8092
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT                            0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER                                    0x8093
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT                                0x8093
#define GL_MULTISAMPLE                                                0x809D
#define GL_MULTISAMPLE_ARB                                            0x809D
#define GL_MULTISAMPLE_EXT                                            0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE                                   0x809E
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB                               0x809E
#define GL_SAMPLE_ALPHA_TO_MASK_EXT                                   0x809E
#define GL_SAMPLE_ALPHA_TO_ONE                                        0x809F
#define GL_SAMPLE_ALPHA_TO_ONE_ARB                                    0x809F
#define GL_SAMPLE_ALPHA_TO_ONE_EXT                                    0x809F
#define GL_SAMPLE_COVERAGE                                            0x80A0
#define GL_SAMPLE_COVERAGE_ARB                                        0x80A0
#define GL_SAMPLE_MASK_EXT                                            0x80A0
#define GL_1PASS_EXT                                                  0x80A1
#define GL_2PASS_0_EXT                                                0x80A2
#define GL_2PASS_1_EXT                                                0x80A3
#define GL_4PASS_0_EXT                                                0x80A4
#define GL_4PASS_1_EXT                                                0x80A5
#define GL_4PASS_2_EXT                                                0x80A6
#define GL_4PASS_3_EXT                                                0x80A7
#define GL_SAMPLE_BUFFERS                                             0x80A8
#define GL_SAMPLE_BUFFERS_ARB                                         0x80A8
#define GL_SAMPLE_BUFFERS_EXT                                         0x80A8
#define GL_SAMPLES                                                    0x80A9
#define GL_SAMPLES_ARB                                                0x80A9
#define GL_SAMPLES_EXT                                                0x80A9
#define GL_SAMPLE_COVERAGE_VALUE                                      0x80AA
#define GL_SAMPLE_COVERAGE_VALUE_ARB                                  0x80AA
#define GL_SAMPLE_MASK_VALUE_EXT                                      0x80AA
#define GL_SAMPLE_COVERAGE_INVERT                                     0x80AB
#define GL_SAMPLE_COVERAGE_INVERT_ARB                                 0x80AB
#define GL_SAMPLE_MASK_INVERT_EXT                                     0x80AB
#define GL_SAMPLE_PATTERN_EXT                                         0x80AC
#define GL_COLOR_MATRIX                                               0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH                                   0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH                               0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE                                0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE                              0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE                               0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE                              0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS                                 0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS                               0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS                                0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS                               0x80BB
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB                             0x80BF
#define GL_BLEND_DST_RGB                                              0x80C8
#define GL_BLEND_DST_RGB_EXT                                          0x80C8
#define GL_BLEND_SRC_RGB                                              0x80C9
#define GL_BLEND_SRC_RGB_EXT                                          0x80C9
#define GL_BLEND_DST_ALPHA                                            0x80CA
#define GL_BLEND_DST_ALPHA_EXT                                        0x80CA
#define GL_BLEND_SRC_ALPHA                                            0x80CB
#define GL_BLEND_SRC_ALPHA_EXT                                        0x80CB
#define GL_422_EXT                                                    0x80CC
#define GL_422_REV_EXT                                                0x80CD
#define GL_422_AVERAGE_EXT                                            0x80CE
#define GL_422_REV_AVERAGE_EXT                                        0x80CF
#define GL_COLOR_TABLE                                                0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE                               0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE                              0x80D2
#define GL_PROXY_COLOR_TABLE                                          0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE                         0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE                        0x80D5
#define GL_COLOR_TABLE_SCALE                                          0x80D6
#define GL_COLOR_TABLE_BIAS                                           0x80D7
#define GL_COLOR_TABLE_FORMAT                                         0x80D8
#define GL_COLOR_TABLE_WIDTH                                          0x80D9
#define GL_COLOR_TABLE_RED_SIZE                                       0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE                                     0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE                                      0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE                                     0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE                                 0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE                                 0x80DF
#define GL_BGR                                                        0x80E0
#define GL_BGR_EXT                                                    0x80E0
#define GL_BGRA                                                       0x80E1
#define GL_BGRA_EXT                                                   0x80E1
#define GL_COLOR_INDEX1_EXT                                           0x80E2
#define GL_COLOR_INDEX2_EXT                                           0x80E3
#define GL_COLOR_INDEX4_EXT                                           0x80E4
#define GL_COLOR_INDEX8_EXT                                           0x80E5
#define GL_COLOR_INDEX12_EXT                                          0x80E6
#define GL_COLOR_INDEX16_EXT                                          0x80E7
#define GL_MAX_ELEMENTS_VERTICES                                      0x80E8
#define GL_MAX_ELEMENTS_VERTICES_EXT                                  0x80E8
#define GL_MAX_ELEMENTS_INDICES                                       0x80E9
#define GL_MAX_ELEMENTS_INDICES_EXT                                   0x80E9
#define GL_TEXTURE_INDEX_SIZE_EXT                                     0x80ED
#define GL_PARAMETER_BUFFER                                           0x80EE
#define GL_PARAMETER_BUFFER_ARB                                       0x80EE
#define GL_PARAMETER_BUFFER_BINDING                                   0x80EF
#define GL_PARAMETER_BUFFER_BINDING_ARB                               0x80EF
#define GL_CLIP_VOLUME_CLIPPING_HINT_EXT                              0x80F0
#define GL_POINT_SIZE_MIN                                             0x8126
#define GL_POINT_SIZE_MIN_ARB                                         0x8126
#define GL_POINT_SIZE_MIN_EXT                                         0x8126
#define GL_POINT_SIZE_MAX                                             0x8127
#define GL_POINT_SIZE_MAX_ARB                                         0x8127
#define GL_POINT_SIZE_MAX_EXT                                         0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE                                  0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_ARB                              0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT                              0x8128
#define GL_DISTANCE_ATTENUATION_EXT                                   0x8129
#define GL_POINT_DISTANCE_ATTENUATION                                 0x8129
#define GL_POINT_DISTANCE_ATTENUATION_ARB                             0x8129
#define GL_CLAMP_TO_BORDER                                            0x812D
#define GL_CLAMP_TO_BORDER_ARB                                        0x812D
#define GL_CLAMP_TO_EDGE                                              0x812F
#define GL_TEXTURE_MIN_LOD                                            0x813A
#define GL_TEXTURE_MAX_LOD                                            0x813B
#define GL_TEXTURE_BASE_LEVEL                                         0x813C
#define GL_TEXTURE_MAX_LEVEL                                          0x813D
#define GL_CONSTANT_BORDER                                            0x8151
#define GL_REPLICATE_BORDER                                           0x8153
#define GL_CONVOLUTION_BORDER_COLOR                                   0x8154
#define GL_GENERATE_MIPMAP                                            0x8191
#define GL_GENERATE_MIPMAP_HINT                                       0x8192
#define GL_DEPTH_COMPONENT16                                          0x81A5
#define GL_DEPTH_COMPONENT16_ARB                                      0x81A5
#define GL_DEPTH_COMPONENT24                                          0x81A6
#define GL_DEPTH_COMPONENT24_ARB                                      0x81A6
#define GL_DEPTH_COMPONENT32                                          0x81A7
#define GL_DEPTH_COMPONENT32_ARB                                      0x81A7
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT                               0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT                               0x81A9
#define GL_CULL_VERTEX_EXT                                            0x81AA
#define GL_CULL_VERTEX_EYE_POSITION_EXT                               0x81AB
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT                            0x81AC
#define GL_IUI_V2F_EXT                                                0x81AD
#define GL_IUI_V3F_EXT                                                0x81AE
#define GL_IUI_N3F_V2F_EXT                                            0x81AF
#define GL_IUI_N3F_V3F_EXT                                            0x81B0
#define GL_T2F_IUI_V2F_EXT                                            0x81B1
#define GL_T2F_IUI_V3F_EXT                                            0x81B2
#define GL_T2F_IUI_N3F_V2F_EXT                                        0x81B3
#define GL_T2F_IUI_N3F_V3F_EXT                                        0x81B4
#define GL_INDEX_TEST_EXT                                             0x81B5
#define GL_INDEX_TEST_FUNC_EXT                                        0x81B6
#define GL_INDEX_TEST_REF_EXT                                         0x81B7
#define GL_INDEX_MATERIAL_EXT                                         0x81B8
#define GL_INDEX_MATERIAL_PARAMETER_EXT                               0x81B9
#define GL_INDEX_MATERIAL_FACE_EXT                                    0x81BA
#define GL_LIGHT_MODEL_COLOR_CONTROL                                  0x81F8
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT                              0x81F8
#define GL_SINGLE_COLOR                                               0x81F9
#define GL_SINGLE_COLOR_EXT                                           0x81F9
#define GL_SEPARATE_SPECULAR_COLOR                                    0x81FA
#define GL_SEPARATE_SPECULAR_COLOR_EXT                                0x81FA
#define GL_SHARED_TEXTURE_PALETTE_EXT                                 0x81FB
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING                      0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE                      0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE                            0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE                          0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE                           0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE                          0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE                          0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE                        0x8217
#define GL_FRAMEBUFFER_DEFAULT                                        0x8218
#define GL_FRAMEBUFFER_UNDEFINED                                      0x8219
#define GL_DEPTH_STENCIL_ATTACHMENT                                   0x821A
#define GL_MAJOR_VERSION                                              0x821B
#define GL_MINOR_VERSION                                              0x821C
#define GL_NUM_EXTENSIONS                                             0x821D
#define GL_CONTEXT_FLAGS                                              0x821E
#define GL_BUFFER_IMMUTABLE_STORAGE                                   0x821F
#define GL_BUFFER_STORAGE_FLAGS                                       0x8220
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED                    0x8221
#define GL_INDEX                                                      0x8222
#define GL_COMPRESSED_RED                                             0x8225
#define GL_COMPRESSED_RG                                              0x8226
#define GL_RG                                                         0x8227
#define GL_RG_INTEGER                                                 0x8228
#define GL_R8                                                         0x8229
#define GL_R16                                                        0x822A
#define GL_RG8                                                        0x822B
#define GL_RG16                                                       0x822C
#define GL_R16F                                                       0x822D
#define GL_R32F                                                       0x822E
#define GL_RG16F                                                      0x822F
#define GL_RG32F                                                      0x8230
#define GL_R8I                                                        0x8231
#define GL_R8UI                                                       0x8232
#define GL_R16I                                                       0x8233
#define GL_R16UI                                                      0x8234
#define GL_R32I                                                       0x8235
#define GL_R32UI                                                      0x8236
#define GL_RG8I                                                       0x8237
#define GL_RG8UI                                                      0x8238
#define GL_RG16I                                                      0x8239
#define GL_RG16UI                                                     0x823A
#define GL_RG32I                                                      0x823B
#define GL_RG32UI                                                     0x823C
#define GL_SYNC_CL_EVENT_ARB                                          0x8240
#define GL_SYNC_CL_EVENT_COMPLETE_ARB                                 0x8241
#define GL_DEBUG_OUTPUT_SYNCHRONOUS                                   0x8242
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB                               0x8242
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR                               0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH                           0x8243
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB                       0x8243
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_KHR                       0x8243
#define GL_DEBUG_CALLBACK_FUNCTION                                    0x8244
#define GL_DEBUG_CALLBACK_FUNCTION_ARB                                0x8244
#define GL_DEBUG_CALLBACK_FUNCTION_KHR                                0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM                                  0x8245
#define GL_DEBUG_CALLBACK_USER_PARAM_ARB                              0x8245
#define GL_DEBUG_CALLBACK_USER_PARAM_KHR                              0x8245
#define GL_DEBUG_SOURCE_API                                           0x8246
#define GL_DEBUG_SOURCE_API_ARB                                       0x8246
#define GL_DEBUG_SOURCE_API_KHR                                       0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM                                 0x8247
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB                             0x8247
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR                             0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER                               0x8248
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB                           0x8248
#define GL_DEBUG_SOURCE_SHADER_COMPILER_KHR                           0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY                                   0x8249
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB                               0x8249
#define GL_DEBUG_SOURCE_THIRD_PARTY_KHR                               0x8249
#define GL_DEBUG_SOURCE_APPLICATION                                   0x824A
#define GL_DEBUG_SOURCE_APPLICATION_ARB                               0x824A
#define GL_DEBUG_SOURCE_APPLICATION_KHR                               0x824A
#define GL_DEBUG_SOURCE_OTHER                                         0x824B
#define GL_DEBUG_SOURCE_OTHER_ARB                                     0x824B
#define GL_DEBUG_SOURCE_OTHER_KHR                                     0x824B
#define GL_DEBUG_TYPE_ERROR                                           0x824C
#define GL_DEBUG_TYPE_ERROR_ARB                                       0x824C
#define GL_DEBUG_TYPE_ERROR_KHR                                       0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR                             0x824D
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB                         0x824D
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR                         0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR                              0x824E
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB                          0x824E
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR                          0x824E
#define GL_DEBUG_TYPE_PORTABILITY                                     0x824F
#define GL_DEBUG_TYPE_PORTABILITY_ARB                                 0x824F
#define GL_DEBUG_TYPE_PORTABILITY_KHR                                 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE                                     0x8250
#define GL_DEBUG_TYPE_PERFORMANCE_ARB                                 0x8250
#define GL_DEBUG_TYPE_PERFORMANCE_KHR                                 0x8250
#define GL_DEBUG_TYPE_OTHER                                           0x8251
#define GL_DEBUG_TYPE_OTHER_ARB                                       0x8251
#define GL_DEBUG_TYPE_OTHER_KHR                                       0x8251
#define GL_LOSE_CONTEXT_ON_RESET                                      0x8252
#define GL_LOSE_CONTEXT_ON_RESET_ARB                                  0x8252
#define GL_LOSE_CONTEXT_ON_RESET_KHR                                  0x8252
#define GL_GUILTY_CONTEXT_RESET                                       0x8253
#define GL_GUILTY_CONTEXT_RESET_ARB                                   0x8253
#define GL_GUILTY_CONTEXT_RESET_KHR                                   0x8253
#define GL_INNOCENT_CONTEXT_RESET                                     0x8254
#define GL_INNOCENT_CONTEXT_RESET_ARB                                 0x8254
#define GL_INNOCENT_CONTEXT_RESET_KHR                                 0x8254
#define GL_UNKNOWN_CONTEXT_RESET                                      0x8255
#define GL_UNKNOWN_CONTEXT_RESET_ARB                                  0x8255
#define GL_UNKNOWN_CONTEXT_RESET_KHR                                  0x8255
#define GL_RESET_NOTIFICATION_STRATEGY                                0x8256
#define GL_RESET_NOTIFICATION_STRATEGY_ARB                            0x8256
#define GL_RESET_NOTIFICATION_STRATEGY_KHR                            0x8256
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT                            0x8257
#define GL_PROGRAM_SEPARABLE                                          0x8258
#define GL_PROGRAM_SEPARABLE_EXT                                      0x8258
#define GL_ACTIVE_PROGRAM                                             0x8259
#define GL_ACTIVE_PROGRAM_EXT                                         0x8259
#define GL_PROGRAM_PIPELINE_BINDING                                   0x825A
#define GL_PROGRAM_PIPELINE_BINDING_EXT                               0x825A
#define GL_MAX_VIEWPORTS                                              0x825B
#define GL_VIEWPORT_SUBPIXEL_BITS                                     0x825C
#define GL_VIEWPORT_BOUNDS_RANGE                                      0x825D
#define GL_LAYER_PROVOKING_VERTEX                                     0x825E
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX                            0x825F
#define GL_UNDEFINED_VERTEX                                           0x8260
#define GL_NO_RESET_NOTIFICATION                                      0x8261
#define GL_NO_RESET_NOTIFICATION_ARB                                  0x8261
#define GL_NO_RESET_NOTIFICATION_KHR                                  0x8261
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE                             0x8262
#define GL_MAX_COMPUTE_UNIFORM_COMPONENTS                             0x8263
#define GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS                         0x8264
#define GL_MAX_COMPUTE_ATOMIC_COUNTERS                                0x8265
#define GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS                    0x8266
#define GL_COMPUTE_WORK_GROUP_SIZE                                    0x8267
#define GL_DEBUG_TYPE_MARKER                                          0x8268
#define GL_DEBUG_TYPE_MARKER_KHR                                      0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP                                      0x8269
#define GL_DEBUG_TYPE_PUSH_GROUP_KHR                                  0x8269
#define GL_DEBUG_TYPE_POP_GROUP                                       0x826A
#define GL_DEBUG_TYPE_POP_GROUP_KHR                                   0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION                                0x826B
#define GL_DEBUG_SEVERITY_NOTIFICATION_KHR                            0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH                                0x826C
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH_KHR                            0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH                                    0x826D
#define GL_DEBUG_GROUP_STACK_DEPTH_KHR                                0x826D
#define GL_MAX_UNIFORM_LOCATIONS                                      0x826E
#define GL_INTERNALFORMAT_SUPPORTED                                   0x826F
#define GL_INTERNALFORMAT_PREFERRED                                   0x8270
#define GL_INTERNALFORMAT_RED_SIZE                                    0x8271
#define GL_INTERNALFORMAT_GREEN_SIZE                                  0x8272
#define GL_INTERNALFORMAT_BLUE_SIZE                                   0x8273
#define GL_INTERNALFORMAT_ALPHA_SIZE                                  0x8274
#define GL_INTERNALFORMAT_DEPTH_SIZE                                  0x8275
#define GL_INTERNALFORMAT_STENCIL_SIZE                                0x8276
#define GL_INTERNALFORMAT_SHARED_SIZE                                 0x8277
#define GL_INTERNALFORMAT_RED_TYPE                                    0x8278
#define GL_INTERNALFORMAT_GREEN_TYPE                                  0x8279
#define GL_INTERNALFORMAT_BLUE_TYPE                                   0x827A
#define GL_INTERNALFORMAT_ALPHA_TYPE                                  0x827B
#define GL_INTERNALFORMAT_DEPTH_TYPE                                  0x827C
#define GL_INTERNALFORMAT_STENCIL_TYPE                                0x827D
#define GL_MAX_WIDTH                                                  0x827E
#define GL_MAX_HEIGHT                                                 0x827F
#define GL_MAX_DEPTH                                                  0x8280
#define GL_MAX_LAYERS                                                 0x8281
#define GL_MAX_COMBINED_DIMENSIONS                                    0x8282
#define GL_COLOR_COMPONENTS                                           0x8283
#define GL_DEPTH_COMPONENTS                                           0x8284
#define GL_STENCIL_COMPONENTS                                         0x8285
#define GL_COLOR_RENDERABLE                                           0x8286
#define GL_DEPTH_RENDERABLE                                           0x8287
#define GL_STENCIL_RENDERABLE                                         0x8288
#define GL_FRAMEBUFFER_RENDERABLE                                     0x8289
#define GL_FRAMEBUFFER_RENDERABLE_LAYERED                             0x828A
#define GL_FRAMEBUFFER_BLEND                                          0x828B
#define GL_READ_PIXELS                                                0x828C
#define GL_READ_PIXELS_FORMAT                                         0x828D
#define GL_READ_PIXELS_TYPE                                           0x828E
#define GL_TEXTURE_IMAGE_FORMAT                                       0x828F
#define GL_TEXTURE_IMAGE_TYPE                                         0x8290
#define GL_GET_TEXTURE_IMAGE_FORMAT                                   0x8291
#define GL_GET_TEXTURE_IMAGE_TYPE                                     0x8292
#define GL_MIPMAP                                                     0x8293
#define GL_MANUAL_GENERATE_MIPMAP                                     0x8294
#define GL_AUTO_GENERATE_MIPMAP                                       0x8295
#define GL_COLOR_ENCODING                                             0x8296
#define GL_SRGB_READ                                                  0x8297
#define GL_SRGB_WRITE                                                 0x8298
#define GL_SRGB_DECODE_ARB                                            0x8299
#define GL_FILTER                                                     0x829A
#define GL_VERTEX_TEXTURE                                             0x829B
#define GL_TESS_CONTROL_TEXTURE                                       0x829C
#define GL_TESS_EVALUATION_TEXTURE                                    0x829D
#define GL_GEOMETRY_TEXTURE                                           0x829E
#define GL_FRAGMENT_TEXTURE                                           0x829F
#define GL_COMPUTE_TEXTURE                                            0x82A0
#define GL_TEXTURE_SHADOW                                             0x82A1
#define GL_TEXTURE_GATHER                                             0x82A2
#define GL_TEXTURE_GATHER_SHADOW                                      0x82A3
#define GL_SHADER_IMAGE_LOAD                                          0x82A4
#define GL_SHADER_IMAGE_STORE                                         0x82A5
#define GL_SHADER_IMAGE_ATOMIC                                        0x82A6
#define GL_IMAGE_TEXEL_SIZE                                           0x82A7
#define GL_IMAGE_COMPATIBILITY_CLASS                                  0x82A8
#define GL_IMAGE_PIXEL_FORMAT                                         0x82A9
#define GL_IMAGE_PIXEL_TYPE                                           0x82AA
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST                        0x82AC
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST                      0x82AD
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE                       0x82AE
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE                     0x82AF
#define GL_TEXTURE_COMPRESSED_BLOCK_WIDTH                             0x82B1
#define GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT                            0x82B2
#define GL_TEXTURE_COMPRESSED_BLOCK_SIZE                              0x82B3
#define GL_CLEAR_BUFFER                                               0x82B4
#define GL_TEXTURE_VIEW                                               0x82B5
#define GL_VIEW_COMPATIBILITY_CLASS                                   0x82B6
#define GL_FULL_SUPPORT                                               0x82B7
#define GL_CAVEAT_SUPPORT                                             0x82B8
#define GL_IMAGE_CLASS_4_X_32                                         0x82B9
#define GL_IMAGE_CLASS_2_X_32                                         0x82BA
#define GL_IMAGE_CLASS_1_X_32                                         0x82BB
#define GL_IMAGE_CLASS_4_X_16                                         0x82BC
#define GL_IMAGE_CLASS_2_X_16                                         0x82BD
#define GL_IMAGE_CLASS_1_X_16                                         0x82BE
#define GL_IMAGE_CLASS_4_X_8                                          0x82BF
#define GL_IMAGE_CLASS_2_X_8                                          0x82C0
#define GL_IMAGE_CLASS_1_X_8                                          0x82C1
#define GL_IMAGE_CLASS_11_11_10                                       0x82C2
#define GL_IMAGE_CLASS_10_10_10_2                                     0x82C3
#define GL_VIEW_CLASS_128_BITS                                        0x82C4
#define GL_VIEW_CLASS_96_BITS                                         0x82C5
#define GL_VIEW_CLASS_64_BITS                                         0x82C6
#define GL_VIEW_CLASS_48_BITS                                         0x82C7
#define GL_VIEW_CLASS_32_BITS                                         0x82C8
#define GL_VIEW_CLASS_24_BITS                                         0x82C9
#define GL_VIEW_CLASS_16_BITS                                         0x82CA
#define GL_VIEW_CLASS_8_BITS                                          0x82CB
#define GL_VIEW_CLASS_S3TC_DXT1_RGB                                   0x82CC
#define GL_VIEW_CLASS_S3TC_DXT1_RGBA                                  0x82CD
#define GL_VIEW_CLASS_S3TC_DXT3_RGBA                                  0x82CE
#define GL_VIEW_CLASS_S3TC_DXT5_RGBA                                  0x82CF
#define GL_VIEW_CLASS_RGTC1_RED                                       0x82D0
#define GL_VIEW_CLASS_RGTC2_RG                                        0x82D1
#define GL_VIEW_CLASS_BPTC_UNORM                                      0x82D2
#define GL_VIEW_CLASS_BPTC_FLOAT                                      0x82D3
#define GL_VERTEX_ATTRIB_BINDING                                      0x82D4
#define GL_VERTEX_ATTRIB_RELATIVE_OFFSET                              0x82D5
#define GL_VERTEX_BINDING_DIVISOR                                     0x82D6
#define GL_VERTEX_BINDING_OFFSET                                      0x82D7
#define GL_VERTEX_BINDING_STRIDE                                      0x82D8
#define GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET                          0x82D9
#define GL_MAX_VERTEX_ATTRIB_BINDINGS                                 0x82DA
#define GL_TEXTURE_VIEW_MIN_LEVEL                                     0x82DB
#define GL_TEXTURE_VIEW_NUM_LEVELS                                    0x82DC
#define GL_TEXTURE_VIEW_MIN_LAYER                                     0x82DD
#define GL_TEXTURE_VIEW_NUM_LAYERS                                    0x82DE
#define GL_TEXTURE_IMMUTABLE_LEVELS                                   0x82DF
#define GL_BUFFER                                                     0x82E0
#define GL_BUFFER_KHR                                                 0x82E0
#define GL_SHADER                                                     0x82E1
#define GL_SHADER_KHR                                                 0x82E1
#define GL_PROGRAM                                                    0x82E2
#define GL_PROGRAM_KHR                                                0x82E2
#define GL_QUERY                                                      0x82E3
#define GL_QUERY_KHR                                                  0x82E3
#define GL_PROGRAM_PIPELINE                                           0x82E4
#define GL_PROGRAM_PIPELINE_KHR                                       0x82E4
#define GL_MAX_VERTEX_ATTRIB_STRIDE                                   0x82E5
#define GL_SAMPLER                                                    0x82E6
#define GL_SAMPLER_KHR                                                0x82E6
#define GL_DISPLAY_LIST                                               0x82E7
#define GL_MAX_LABEL_LENGTH                                           0x82E8
#define GL_MAX_LABEL_LENGTH_KHR                                       0x82E8
#define GL_NUM_SHADING_LANGUAGE_VERSIONS                              0x82E9
#define GL_QUERY_TARGET                                               0x82EA
#define GL_TRANSFORM_FEEDBACK_OVERFLOW                                0x82EC
#define GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB                            0x82EC
#define GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW                         0x82ED
#define GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB                     0x82ED
#define GL_VERTICES_SUBMITTED                                         0x82EE
#define GL_VERTICES_SUBMITTED_ARB                                     0x82EE
#define GL_PRIMITIVES_SUBMITTED                                       0x82EF
#define GL_PRIMITIVES_SUBMITTED_ARB                                   0x82EF
#define GL_VERTEX_SHADER_INVOCATIONS                                  0x82F0
#define GL_VERTEX_SHADER_INVOCATIONS_ARB                              0x82F0
#define GL_TESS_CONTROL_SHADER_PATCHES                                0x82F1
#define GL_TESS_CONTROL_SHADER_PATCHES_ARB                            0x82F1
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS                         0x82F2
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB                     0x82F2
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED                         0x82F3
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB                     0x82F3
#define GL_FRAGMENT_SHADER_INVOCATIONS                                0x82F4
#define GL_FRAGMENT_SHADER_INVOCATIONS_ARB                            0x82F4
#define GL_COMPUTE_SHADER_INVOCATIONS                                 0x82F5
#define GL_COMPUTE_SHADER_INVOCATIONS_ARB                             0x82F5
#define GL_CLIPPING_INPUT_PRIMITIVES                                  0x82F6
#define GL_CLIPPING_INPUT_PRIMITIVES_ARB                              0x82F6
#define GL_CLIPPING_OUTPUT_PRIMITIVES                                 0x82F7
#define GL_CLIPPING_OUTPUT_PRIMITIVES_ARB                             0x82F7
#define GL_SPARSE_BUFFER_PAGE_SIZE_ARB                                0x82F8
#define GL_MAX_CULL_DISTANCES                                         0x82F9
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES                       0x82FA
#define GL_CONTEXT_RELEASE_BEHAVIOR                                   0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_KHR                               0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH                             0x82FC
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR                         0x82FC
#define GL_PIXEL_TRANSFORM_2D_EXT                                     0x8330
#define GL_PIXEL_MAG_FILTER_EXT                                       0x8331
#define GL_PIXEL_MIN_FILTER_EXT                                       0x8332
#define GL_PIXEL_CUBIC_WEIGHT_EXT                                     0x8333
#define GL_CUBIC_EXT                                                  0x8334
#define GL_AVERAGE_EXT                                                0x8335
#define GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT                         0x8336
#define GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT                     0x8337
#define GL_PIXEL_TRANSFORM_2D_MATRIX_EXT                              0x8338
#define GL_FRAGMENT_MATERIAL_EXT                                      0x8349
#define GL_FRAGMENT_NORMAL_EXT                                        0x834A
#define GL_FRAGMENT_COLOR_EXT                                         0x834C
#define GL_ATTENUATION_EXT                                            0x834D
#define GL_SHADOW_ATTENUATION_EXT                                     0x834E
#define GL_TEXTURE_APPLICATION_MODE_EXT                               0x834F
#define GL_TEXTURE_LIGHT_EXT                                          0x8350
#define GL_TEXTURE_MATERIAL_FACE_EXT                                  0x8351
#define GL_TEXTURE_MATERIAL_PARAMETER_EXT                             0x8352
#define GL_UNSIGNED_BYTE_2_3_3_REV                                    0x8362
#define GL_UNSIGNED_SHORT_5_6_5                                       0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV                                   0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV                                 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV                                 0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV                                   0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV                                0x8368
#define GL_MIRRORED_REPEAT                                            0x8370
#define GL_MIRRORED_REPEAT_ARB                                        0x8370
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                               0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                              0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                              0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                              0x83F3
#define GL_PARALLEL_ARRAYS_INTEL                                      0x83F4
#define GL_VERTEX_ARRAY_PARALLEL_POINTERS_INTEL                       0x83F5
#define GL_NORMAL_ARRAY_PARALLEL_POINTERS_INTEL                       0x83F6
#define GL_COLOR_ARRAY_PARALLEL_POINTERS_INTEL                        0x83F7
#define GL_TEXTURE_COORD_ARRAY_PARALLEL_POINTERS_INTEL                0x83F8
#define GL_PERFQUERY_DONOT_FLUSH_INTEL                                0x83F9
#define GL_PERFQUERY_FLUSH_INTEL                                      0x83FA
#define GL_PERFQUERY_WAIT_INTEL                                       0x83FB
#define GL_BLACKHOLE_RENDER_INTEL                                     0x83FC
#define GL_CONSERVATIVE_RASTERIZATION_INTEL                           0x83FE
#define GL_TEXTURE_MEMORY_LAYOUT_INTEL                                0x83FF
#define GL_TANGENT_ARRAY_EXT                                          0x8439
#define GL_BINORMAL_ARRAY_EXT                                         0x843A
#define GL_CURRENT_TANGENT_EXT                                        0x843B
#define GL_CURRENT_BINORMAL_EXT                                       0x843C
#define GL_TANGENT_ARRAY_TYPE_EXT                                     0x843E
#define GL_TANGENT_ARRAY_STRIDE_EXT                                   0x843F
#define GL_BINORMAL_ARRAY_TYPE_EXT                                    0x8440
#define GL_BINORMAL_ARRAY_STRIDE_EXT                                  0x8441
#define GL_TANGENT_ARRAY_POINTER_EXT                                  0x8442
#define GL_BINORMAL_ARRAY_POINTER_EXT                                 0x8443
#define GL_MAP1_TANGENT_EXT                                           0x8444
#define GL_MAP2_TANGENT_EXT                                           0x8445
#define GL_MAP1_BINORMAL_EXT                                          0x8446
#define GL_MAP2_BINORMAL_EXT                                          0x8447
#define GL_FOG_COORDINATE_SOURCE                                      0x8450
#define GL_FOG_COORDINATE_SOURCE_EXT                                  0x8450
#define GL_FOG_COORD_SRC                                              0x8450
#define GL_FOG_COORDINATE                                             0x8451
#define GL_FOG_COORD                                                  0x8451
#define GL_FOG_COORDINATE_EXT                                         0x8451
#define GL_FRAGMENT_DEPTH                                             0x8452
#define GL_FRAGMENT_DEPTH_EXT                                         0x8452
#define GL_CURRENT_FOG_COORDINATE                                     0x8453
#define GL_CURRENT_FOG_COORD                                          0x8453
#define GL_CURRENT_FOG_COORDINATE_EXT                                 0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE                                  0x8454
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT                              0x8454
#define GL_FOG_COORD_ARRAY_TYPE                                       0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE                                0x8455
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT                            0x8455
#define GL_FOG_COORD_ARRAY_STRIDE                                     0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER                               0x8456
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT                           0x8456
#define GL_FOG_COORD_ARRAY_POINTER                                    0x8456
#define GL_FOG_COORDINATE_ARRAY                                       0x8457
#define GL_FOG_COORDINATE_ARRAY_EXT                                   0x8457
#define GL_FOG_COORD_ARRAY                                            0x8457
#define GL_COLOR_SUM                                                  0x8458
#define GL_COLOR_SUM_ARB                                              0x8458
#define GL_COLOR_SUM_EXT                                              0x8458
#define GL_CURRENT_SECONDARY_COLOR                                    0x8459
#define GL_CURRENT_SECONDARY_COLOR_EXT                                0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE                                 0x845A
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT                             0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE                                 0x845B
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT                             0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE                               0x845C
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT                           0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER                              0x845D
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT                          0x845D
#define GL_SECONDARY_COLOR_ARRAY                                      0x845E
#define GL_SECONDARY_COLOR_ARRAY_EXT                                  0x845E
#define GL_CURRENT_RASTER_SECONDARY_COLOR                             0x845F
#define GL_ALIASED_POINT_SIZE_RANGE                                   0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE                                   0x846E
#define GL_TEXTURE0                                                   0x84C0
#define GL_TEXTURE0_ARB                                               0x84C0
#define GL_TEXTURE1                                                   0x84C1
#define GL_TEXTURE1_ARB                                               0x84C1
#define GL_TEXTURE2                                                   0x84C2
#define GL_TEXTURE2_ARB                                               0x84C2
#define GL_TEXTURE3                                                   0x84C3
#define GL_TEXTURE3_ARB                                               0x84C3
#define GL_TEXTURE4                                                   0x84C4
#define GL_TEXTURE4_ARB                                               0x84C4
#define GL_TEXTURE5                                                   0x84C5
#define GL_TEXTURE5_ARB                                               0x84C5
#define GL_TEXTURE6                                                   0x84C6
#define GL_TEXTURE6_ARB                                               0x84C6
#define GL_TEXTURE7                                                   0x84C7
#define GL_TEXTURE7_ARB                                               0x84C7
#define GL_TEXTURE8                                                   0x84C8
#define GL_TEXTURE8_ARB                                               0x84C8
#define GL_TEXTURE9                                                   0x84C9
#define GL_TEXTURE9_ARB                                               0x84C9
#define GL_TEXTURE10                                                  0x84CA
#define GL_TEXTURE10_ARB                                              0x84CA
#define GL_TEXTURE11                                                  0x84CB
#define GL_TEXTURE11_ARB                                              0x84CB
#define GL_TEXTURE12                                                  0x84CC
#define GL_TEXTURE12_ARB                                              0x84CC
#define GL_TEXTURE13                                                  0x84CD
#define GL_TEXTURE13_ARB                                              0x84CD
#define GL_TEXTURE14                                                  0x84CE
#define GL_TEXTURE14_ARB                                              0x84CE
#define GL_TEXTURE15                                                  0x84CF
#define GL_TEXTURE15_ARB                                              0x84CF
#define GL_TEXTURE16                                                  0x84D0
#define GL_TEXTURE16_ARB                                              0x84D0
#define GL_TEXTURE17                                                  0x84D1
#define GL_TEXTURE17_ARB                                              0x84D1
#define GL_TEXTURE18                                                  0x84D2
#define GL_TEXTURE18_ARB                                              0x84D2
#define GL_TEXTURE19                                                  0x84D3
#define GL_TEXTURE19_ARB                                              0x84D3
#define GL_TEXTURE20                                                  0x84D4
#define GL_TEXTURE20_ARB                                              0x84D4
#define GL_TEXTURE21                                                  0x84D5
#define GL_TEXTURE21_ARB                                              0x84D5
#define GL_TEXTURE22                                                  0x84D6
#define GL_TEXTURE22_ARB                                              0x84D6
#define GL_TEXTURE23                                                  0x84D7
#define GL_TEXTURE23_ARB                                              0x84D7
#define GL_TEXTURE24                                                  0x84D8
#define GL_TEXTURE24_ARB                                              0x84D8
#define GL_TEXTURE25                                                  0x84D9
#define GL_TEXTURE25_ARB                                              0x84D9
#define GL_TEXTURE26                                                  0x84DA
#define GL_TEXTURE26_ARB                                              0x84DA
#define GL_TEXTURE27                                                  0x84DB
#define GL_TEXTURE27_ARB                                              0x84DB
#define GL_TEXTURE28                                                  0x84DC
#define GL_TEXTURE28_ARB                                              0x84DC
#define GL_TEXTURE29                                                  0x84DD
#define GL_TEXTURE29_ARB                                              0x84DD
#define GL_TEXTURE30                                                  0x84DE
#define GL_TEXTURE30_ARB                                              0x84DE
#define GL_TEXTURE31                                                  0x84DF
#define GL_TEXTURE31_ARB                                              0x84DF
#define GL_ACTIVE_TEXTURE                                             0x84E0
#define GL_ACTIVE_TEXTURE_ARB                                         0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE                                      0x84E1
#define GL_CLIENT_ACTIVE_TEXTURE_ARB                                  0x84E1
#define GL_MAX_TEXTURE_UNITS                                          0x84E2
#define GL_MAX_TEXTURE_UNITS_ARB                                      0x84E2
#define GL_TRANSPOSE_MODELVIEW_MATRIX                                 0x84E3
#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB                             0x84E3
#define GL_PATH_TRANSPOSE_MODELVIEW_MATRIX_NV                         0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX                                0x84E4
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB                            0x84E4
#define GL_PATH_TRANSPOSE_PROJECTION_MATRIX_NV                        0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX                                   0x84E5
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB                               0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX                                     0x84E6
#define GL_TRANSPOSE_COLOR_MATRIX_ARB                                 0x84E6
#define GL_SUBTRACT                                                   0x84E7
#define GL_SUBTRACT_ARB                                               0x84E7
#define GL_MAX_RENDERBUFFER_SIZE                                      0x84E8
#define GL_MAX_RENDERBUFFER_SIZE_EXT                                  0x84E8
#define GL_COMPRESSED_ALPHA                                           0x84E9
#define GL_COMPRESSED_ALPHA_ARB                                       0x84E9
#define GL_COMPRESSED_LUMINANCE                                       0x84EA
#define GL_COMPRESSED_LUMINANCE_ARB                                   0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA                                 0x84EB
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB                             0x84EB
#define GL_COMPRESSED_INTENSITY                                       0x84EC
#define GL_COMPRESSED_INTENSITY_ARB                                   0x84EC
#define GL_COMPRESSED_RGB                                             0x84ED
#define GL_COMPRESSED_RGB_ARB                                         0x84ED
#define GL_COMPRESSED_RGBA                                            0x84EE
#define GL_COMPRESSED_RGBA_ARB                                        0x84EE
#define GL_TEXTURE_COMPRESSION_HINT                                   0x84EF
#define GL_TEXTURE_COMPRESSION_HINT_ARB                               0x84EF
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER            0x84F0
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER         0x84F1
#define GL_ALL_COMPLETED_NV                                           0x84F2
#define GL_FENCE_STATUS_NV                                            0x84F3
#define GL_FENCE_CONDITION_NV                                         0x84F4
#define GL_TEXTURE_RECTANGLE                                          0x84F5
#define GL_TEXTURE_RECTANGLE_ARB                                      0x84F5
#define GL_TEXTURE_RECTANGLE_NV                                       0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE                                  0x84F6
#define GL_TEXTURE_BINDING_RECTANGLE_ARB                              0x84F6
#define GL_TEXTURE_BINDING_RECTANGLE_NV                               0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE                                    0x84F7
#define GL_PROXY_TEXTURE_RECTANGLE_ARB                                0x84F7
#define GL_PROXY_TEXTURE_RECTANGLE_NV                                 0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE                                 0x84F8
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB                             0x84F8
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV                              0x84F8
#define GL_DEPTH_STENCIL                                              0x84F9
#define GL_DEPTH_STENCIL_EXT                                          0x84F9
#define GL_DEPTH_STENCIL_NV                                           0x84F9
#define GL_UNSIGNED_INT_24_8                                          0x84FA
#define GL_UNSIGNED_INT_24_8_EXT                                      0x84FA
#define GL_UNSIGNED_INT_24_8_NV                                       0x84FA
#define GL_MAX_TEXTURE_LOD_BIAS                                       0x84FD
#define GL_MAX_TEXTURE_LOD_BIAS_EXT                                   0x84FD
#define GL_TEXTURE_MAX_ANISOTROPY                                     0x84FE
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                                 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY                                 0x84FF
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT                             0x84FF
#define GL_TEXTURE_FILTER_CONTROL                                     0x8500
#define GL_TEXTURE_FILTER_CONTROL_EXT                                 0x8500
#define GL_TEXTURE_LOD_BIAS                                           0x8501
#define GL_TEXTURE_LOD_BIAS_EXT                                       0x8501
#define GL_MODELVIEW1_STACK_DEPTH_EXT                                 0x8502
#define GL_COMBINE4_NV                                                0x8503
#define GL_MAX_SHININESS_NV                                           0x8504
#define GL_MAX_SPOT_EXPONENT_NV                                       0x8505
#define GL_MODELVIEW1_MATRIX_EXT                                      0x8506
#define GL_INCR_WRAP                                                  0x8507
#define GL_INCR_WRAP_EXT                                              0x8507
#define GL_DECR_WRAP                                                  0x8508
#define GL_DECR_WRAP_EXT                                              0x8508
#define GL_VERTEX_WEIGHTING_EXT                                       0x8509
#define GL_MODELVIEW1_ARB                                             0x850A
#define GL_MODELVIEW1_EXT                                             0x850A
#define GL_CURRENT_VERTEX_WEIGHT_EXT                                  0x850B
#define GL_VERTEX_WEIGHT_ARRAY_EXT                                    0x850C
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT                               0x850D
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT                               0x850E
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT                             0x850F
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT                            0x8510
#define GL_NORMAL_MAP                                                 0x8511
#define GL_NORMAL_MAP_ARB                                             0x8511
#define GL_NORMAL_MAP_EXT                                             0x8511
#define GL_NORMAL_MAP_NV                                              0x8511
#define GL_REFLECTION_MAP                                             0x8512
#define GL_REFLECTION_MAP_ARB                                         0x8512
#define GL_REFLECTION_MAP_EXT                                         0x8512
#define GL_REFLECTION_MAP_NV                                          0x8512
#define GL_TEXTURE_CUBE_MAP                                           0x8513
#define GL_TEXTURE_CUBE_MAP_ARB                                       0x8513
#define GL_TEXTURE_CUBE_MAP_EXT                                       0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP                                   0x8514
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB                               0x8514
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT                               0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X                                0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB                            0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT                            0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X                                0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB                            0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT                            0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y                                0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB                            0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT                            0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                                0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB                            0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT                            0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z                                0x8519
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB                            0x8519
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT                            0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                                0x851A
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB                            0x851A
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT                            0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP                                     0x851B
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB                                 0x851B
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT                                 0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE                                  0x851C
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB                              0x851C
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT                              0x851C
#define GL_VERTEX_ARRAY_RANGE_APPLE                                   0x851D
#define GL_VERTEX_ARRAY_RANGE_NV                                      0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_APPLE                            0x851E
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV                               0x851E
#define GL_VERTEX_ARRAY_RANGE_VALID_NV                                0x851F
#define GL_VERTEX_ARRAY_STORAGE_HINT_APPLE                            0x851F
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV                          0x8520
#define GL_VERTEX_ARRAY_RANGE_POINTER_APPLE                           0x8521
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV                              0x8521
#define GL_REGISTER_COMBINERS_NV                                      0x8522
#define GL_VARIABLE_A_NV                                              0x8523
#define GL_VARIABLE_B_NV                                              0x8524
#define GL_VARIABLE_C_NV                                              0x8525
#define GL_VARIABLE_D_NV                                              0x8526
#define GL_VARIABLE_E_NV                                              0x8527
#define GL_VARIABLE_F_NV                                              0x8528
#define GL_VARIABLE_G_NV                                              0x8529
#define GL_CONSTANT_COLOR0_NV                                         0x852A
#define GL_CONSTANT_COLOR1_NV                                         0x852B
#define GL_PRIMARY_COLOR_NV                                           0x852C
#define GL_SECONDARY_COLOR_NV                                         0x852D
#define GL_SPARE0_NV                                                  0x852E
#define GL_SPARE1_NV                                                  0x852F
#define GL_DISCARD_NV                                                 0x8530
#define GL_E_TIMES_F_NV                                               0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV                             0x8532
#define GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV                        0x8533
#define GL_MULTISAMPLE_FILTER_HINT_NV                                 0x8534
#define GL_PER_STAGE_CONSTANTS_NV                                     0x8535
#define GL_UNSIGNED_IDENTITY_NV                                       0x8536
#define GL_UNSIGNED_INVERT_NV                                         0x8537
#define GL_EXPAND_NORMAL_NV                                           0x8538
#define GL_EXPAND_NEGATE_NV                                           0x8539
#define GL_HALF_BIAS_NORMAL_NV                                        0x853A
#define GL_HALF_BIAS_NEGATE_NV                                        0x853B
#define GL_SIGNED_IDENTITY_NV                                         0x853C
#define GL_SIGNED_NEGATE_NV                                           0x853D
#define GL_SCALE_BY_TWO_NV                                            0x853E
#define GL_SCALE_BY_FOUR_NV                                           0x853F
#define GL_SCALE_BY_ONE_HALF_NV                                       0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV                               0x8541
#define GL_COMBINER_INPUT_NV                                          0x8542
#define GL_COMBINER_MAPPING_NV                                        0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV                                0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV                                 0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV                                 0x8546
#define GL_COMBINER_MUX_SUM_NV                                        0x8547
#define GL_COMBINER_SCALE_NV                                          0x8548
#define GL_COMBINER_BIAS_NV                                           0x8549
#define GL_COMBINER_AB_OUTPUT_NV                                      0x854A
#define GL_COMBINER_CD_OUTPUT_NV                                      0x854B
#define GL_COMBINER_SUM_OUTPUT_NV                                     0x854C
#define GL_MAX_GENERAL_COMBINERS_NV                                   0x854D
#define GL_NUM_GENERAL_COMBINERS_NV                                   0x854E
#define GL_COLOR_SUM_CLAMP_NV                                         0x854F
#define GL_COMBINER0_NV                                               0x8550
#define GL_COMBINER1_NV                                               0x8551
#define GL_COMBINER2_NV                                               0x8552
#define GL_COMBINER3_NV                                               0x8553
#define GL_COMBINER4_NV                                               0x8554
#define GL_COMBINER5_NV                                               0x8555
#define GL_COMBINER6_NV                                               0x8556
#define GL_COMBINER7_NV                                               0x8557
#define GL_PRIMITIVE_RESTART_NV                                       0x8558
#define GL_PRIMITIVE_RESTART_INDEX_NV                                 0x8559
#define GL_FOG_DISTANCE_MODE_NV                                       0x855A
#define GL_EYE_RADIAL_NV                                              0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV                                      0x855C
#define GL_EMBOSS_LIGHT_NV                                            0x855D
#define GL_EMBOSS_CONSTANT_NV                                         0x855E
#define GL_EMBOSS_MAP_NV                                              0x855F
#define GL_COMBINE                                                    0x8570
#define GL_COMBINE_ARB                                                0x8570
#define GL_COMBINE_EXT                                                0x8570
#define GL_COMBINE_RGB                                                0x8571
#define GL_COMBINE_RGB_ARB                                            0x8571
#define GL_COMBINE_RGB_EXT                                            0x8571
#define GL_COMBINE_ALPHA                                              0x8572
#define GL_COMBINE_ALPHA_ARB                                          0x8572
#define GL_COMBINE_ALPHA_EXT                                          0x8572
#define GL_RGB_SCALE                                                  0x8573
#define GL_RGB_SCALE_ARB                                              0x8573
#define GL_RGB_SCALE_EXT                                              0x8573
#define GL_ADD_SIGNED                                                 0x8574
#define GL_ADD_SIGNED_ARB                                             0x8574
#define GL_ADD_SIGNED_EXT                                             0x8574
#define GL_INTERPOLATE                                                0x8575
#define GL_INTERPOLATE_ARB                                            0x8575
#define GL_INTERPOLATE_EXT                                            0x8575
#define GL_CONSTANT                                                   0x8576
#define GL_CONSTANT_ARB                                               0x8576
#define GL_CONSTANT_EXT                                               0x8576
#define GL_CONSTANT_NV                                                0x8576
#define GL_PRIMARY_COLOR                                              0x8577
#define GL_PRIMARY_COLOR_ARB                                          0x8577
#define GL_PRIMARY_COLOR_EXT                                          0x8577
#define GL_PREVIOUS                                                   0x8578
#define GL_PREVIOUS_ARB                                               0x8578
#define GL_PREVIOUS_EXT                                               0x8578
#define GL_SOURCE0_RGB                                                0x8580
#define GL_SOURCE0_RGB_ARB                                            0x8580
#define GL_SOURCE0_RGB_EXT                                            0x8580
#define GL_SRC0_RGB                                                   0x8580
#define GL_SOURCE1_RGB                                                0x8581
#define GL_SOURCE1_RGB_ARB                                            0x8581
#define GL_SOURCE1_RGB_EXT                                            0x8581
#define GL_SRC1_RGB                                                   0x8581
#define GL_SOURCE2_RGB                                                0x8582
#define GL_SOURCE2_RGB_ARB                                            0x8582
#define GL_SOURCE2_RGB_EXT                                            0x8582
#define GL_SRC2_RGB                                                   0x8582
#define GL_SOURCE3_RGB_NV                                             0x8583
#define GL_SOURCE0_ALPHA                                              0x8588
#define GL_SOURCE0_ALPHA_ARB                                          0x8588
#define GL_SOURCE0_ALPHA_EXT                                          0x8588
#define GL_SRC0_ALPHA                                                 0x8588
#define GL_SOURCE1_ALPHA                                              0x8589
#define GL_SOURCE1_ALPHA_ARB                                          0x8589
#define GL_SOURCE1_ALPHA_EXT                                          0x8589
#define GL_SRC1_ALPHA                                                 0x8589
#define GL_SOURCE2_ALPHA                                              0x858A
#define GL_SOURCE2_ALPHA_ARB                                          0x858A
#define GL_SOURCE2_ALPHA_EXT                                          0x858A
#define GL_SRC2_ALPHA                                                 0x858A
#define GL_SOURCE3_ALPHA_NV                                           0x858B
#define GL_OPERAND0_RGB                                               0x8590
#define GL_OPERAND0_RGB_ARB                                           0x8590
#define GL_OPERAND0_RGB_EXT                                           0x8590
#define GL_OPERAND1_RGB                                               0x8591
#define GL_OPERAND1_RGB_ARB                                           0x8591
#define GL_OPERAND1_RGB_EXT                                           0x8591
#define GL_OPERAND2_RGB                                               0x8592
#define GL_OPERAND2_RGB_ARB                                           0x8592
#define GL_OPERAND2_RGB_EXT                                           0x8592
#define GL_OPERAND3_RGB_NV                                            0x8593
#define GL_OPERAND0_ALPHA                                             0x8598
#define GL_OPERAND0_ALPHA_ARB                                         0x8598
#define GL_OPERAND0_ALPHA_EXT                                         0x8598
#define GL_OPERAND1_ALPHA                                             0x8599
#define GL_OPERAND1_ALPHA_ARB                                         0x8599
#define GL_OPERAND1_ALPHA_EXT                                         0x8599
#define GL_OPERAND2_ALPHA                                             0x859A
#define GL_OPERAND2_ALPHA_ARB                                         0x859A
#define GL_OPERAND2_ALPHA_EXT                                         0x859A
#define GL_OPERAND3_ALPHA_NV                                          0x859B
#define GL_PERTURB_EXT                                                0x85AE
#define GL_TEXTURE_NORMAL_EXT                                         0x85AF
#define GL_LIGHT_MODEL_SPECULAR_VECTOR_APPLE                          0x85B0
#define GL_TRANSFORM_HINT_APPLE                                       0x85B1
#define GL_UNPACK_CLIENT_STORAGE_APPLE                                0x85B2
#define GL_BUFFER_OBJECT_APPLE                                        0x85B3
#define GL_STORAGE_CLIENT_APPLE                                       0x85B4
#define GL_VERTEX_ARRAY_BINDING                                       0x85B5
#define GL_VERTEX_ARRAY_BINDING_APPLE                                 0x85B5
#define GL_TEXTURE_RANGE_LENGTH_APPLE                                 0x85B7
#define GL_TEXTURE_RANGE_POINTER_APPLE                                0x85B8
#define GL_YCBCR_422_APPLE                                            0x85B9
#define GL_UNSIGNED_SHORT_8_8_APPLE                                   0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_APPLE                               0x85BB
#define GL_TEXTURE_STORAGE_HINT_APPLE                                 0x85BC
#define GL_STORAGE_PRIVATE_APPLE                                      0x85BD
#define GL_STORAGE_CACHED_APPLE                                       0x85BE
#define GL_STORAGE_SHARED_APPLE                                       0x85BF
#define GL_VERTEX_PROGRAM_ARB                                         0x8620
#define GL_VERTEX_PROGRAM_NV                                          0x8620
#define GL_VERTEX_STATE_PROGRAM_NV                                    0x8621
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED                                0x8622
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB                            0x8622
#define GL_ATTRIB_ARRAY_SIZE_NV                                       0x8623
#define GL_VERTEX_ATTRIB_ARRAY_SIZE                                   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB                               0x8623
#define GL_ATTRIB_ARRAY_STRIDE_NV                                     0x8624
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE                                 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB                             0x8624
#define GL_ATTRIB_ARRAY_TYPE_NV                                       0x8625
#define GL_VERTEX_ATTRIB_ARRAY_TYPE                                   0x8625
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB                               0x8625
#define GL_CURRENT_ATTRIB_NV                                          0x8626
#define GL_CURRENT_VERTEX_ATTRIB                                      0x8626
#define GL_CURRENT_VERTEX_ATTRIB_ARB                                  0x8626
#define GL_PROGRAM_LENGTH_ARB                                         0x8627
#define GL_PROGRAM_LENGTH_NV                                          0x8627
#define GL_PROGRAM_STRING_ARB                                         0x8628
#define GL_PROGRAM_STRING_NV                                          0x8628
#define GL_MODELVIEW_PROJECTION_NV                                    0x8629
#define GL_IDENTITY_NV                                                0x862A
#define GL_INVERSE_NV                                                 0x862B
#define GL_TRANSPOSE_NV                                               0x862C
#define GL_INVERSE_TRANSPOSE_NV                                       0x862D
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB                         0x862E
#define GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV                            0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB                                   0x862F
#define GL_MAX_TRACK_MATRICES_NV                                      0x862F
#define GL_MATRIX0_NV                                                 0x8630
#define GL_MATRIX1_NV                                                 0x8631
#define GL_MATRIX2_NV                                                 0x8632
#define GL_MATRIX3_NV                                                 0x8633
#define GL_MATRIX4_NV                                                 0x8634
#define GL_MATRIX5_NV                                                 0x8635
#define GL_MATRIX6_NV                                                 0x8636
#define GL_MATRIX7_NV                                                 0x8637
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB                             0x8640
#define GL_CURRENT_MATRIX_STACK_DEPTH_NV                              0x8640
#define GL_CURRENT_MATRIX_ARB                                         0x8641
#define GL_CURRENT_MATRIX_NV                                          0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE                                  0x8642
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB                              0x8642
#define GL_VERTEX_PROGRAM_POINT_SIZE_NV                               0x8642
#define GL_PROGRAM_POINT_SIZE                                         0x8642
#define GL_PROGRAM_POINT_SIZE_ARB                                     0x8642
#define GL_PROGRAM_POINT_SIZE_EXT                                     0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE                                    0x8643
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB                                0x8643
#define GL_VERTEX_PROGRAM_TWO_SIDE_NV                                 0x8643
#define GL_PROGRAM_PARAMETER_NV                                       0x8644
#define GL_ATTRIB_ARRAY_POINTER_NV                                    0x8645
#define GL_VERTEX_ATTRIB_ARRAY_POINTER                                0x8645
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB                            0x8645
#define GL_PROGRAM_TARGET_NV                                          0x8646
#define GL_PROGRAM_RESIDENT_NV                                        0x8647
#define GL_TRACK_MATRIX_NV                                            0x8648
#define GL_TRACK_MATRIX_TRANSFORM_NV                                  0x8649
#define GL_VERTEX_PROGRAM_BINDING_NV                                  0x864A
#define GL_PROGRAM_ERROR_POSITION_ARB                                 0x864B
#define GL_PROGRAM_ERROR_POSITION_NV                                  0x864B
#define GL_OFFSET_TEXTURE_RECTANGLE_NV                                0x864C
#define GL_OFFSET_TEXTURE_RECTANGLE_SCALE_NV                          0x864D
#define GL_DOT_PRODUCT_TEXTURE_RECTANGLE_NV                           0x864E
#define GL_DEPTH_CLAMP                                                0x864F
#define GL_DEPTH_CLAMP_NV                                             0x864F
#define GL_VERTEX_ATTRIB_ARRAY0_NV                                    0x8650
#define GL_VERTEX_ATTRIB_ARRAY1_NV                                    0x8651
#define GL_VERTEX_ATTRIB_ARRAY2_NV                                    0x8652
#define GL_VERTEX_ATTRIB_ARRAY3_NV                                    0x8653
#define GL_VERTEX_ATTRIB_ARRAY4_NV                                    0x8654
#define GL_VERTEX_ATTRIB_ARRAY5_NV                                    0x8655
#define GL_VERTEX_ATTRIB_ARRAY6_NV                                    0x8656
#define GL_VERTEX_ATTRIB_ARRAY7_NV                                    0x8657
#define GL_VERTEX_ATTRIB_ARRAY8_NV                                    0x8658
#define GL_VERTEX_ATTRIB_ARRAY9_NV                                    0x8659
#define GL_VERTEX_ATTRIB_ARRAY10_NV                                   0x865A
#define GL_VERTEX_ATTRIB_ARRAY11_NV                                   0x865B
#define GL_VERTEX_ATTRIB_ARRAY12_NV                                   0x865C
#define GL_VERTEX_ATTRIB_ARRAY13_NV                                   0x865D
#define GL_VERTEX_ATTRIB_ARRAY14_NV                                   0x865E
#define GL_VERTEX_ATTRIB_ARRAY15_NV                                   0x865F
#define GL_MAP1_VERTEX_ATTRIB0_4_NV                                   0x8660
#define GL_MAP1_VERTEX_ATTRIB1_4_NV                                   0x8661
#define GL_MAP1_VERTEX_ATTRIB2_4_NV                                   0x8662
#define GL_MAP1_VERTEX_ATTRIB3_4_NV                                   0x8663
#define GL_MAP1_VERTEX_ATTRIB4_4_NV                                   0x8664
#define GL_MAP1_VERTEX_ATTRIB5_4_NV                                   0x8665
#define GL_MAP1_VERTEX_ATTRIB6_4_NV                                   0x8666
#define GL_MAP1_VERTEX_ATTRIB7_4_NV                                   0x8667
#define GL_MAP1_VERTEX_ATTRIB8_4_NV                                   0x8668
#define GL_MAP1_VERTEX_ATTRIB9_4_NV                                   0x8669
#define GL_MAP1_VERTEX_ATTRIB10_4_NV                                  0x866A
#define GL_MAP1_VERTEX_ATTRIB11_4_NV                                  0x866B
#define GL_MAP1_VERTEX_ATTRIB12_4_NV                                  0x866C
#define GL_MAP1_VERTEX_ATTRIB13_4_NV                                  0x866D
#define GL_MAP1_VERTEX_ATTRIB14_4_NV                                  0x866E
#define GL_MAP1_VERTEX_ATTRIB15_4_NV                                  0x866F
#define GL_MAP2_VERTEX_ATTRIB0_4_NV                                   0x8670
#define GL_MAP2_VERTEX_ATTRIB1_4_NV                                   0x8671
#define GL_MAP2_VERTEX_ATTRIB2_4_NV                                   0x8672
#define GL_MAP2_VERTEX_ATTRIB3_4_NV                                   0x8673
#define GL_MAP2_VERTEX_ATTRIB4_4_NV                                   0x8674
#define GL_MAP2_VERTEX_ATTRIB5_4_NV                                   0x8675
#define GL_MAP2_VERTEX_ATTRIB6_4_NV                                   0x8676
#define GL_MAP2_VERTEX_ATTRIB7_4_NV                                   0x8677
#define GL_PROGRAM_BINDING_ARB                                        0x8677
#define GL_MAP2_VERTEX_ATTRIB8_4_NV                                   0x8678
#define GL_MAP2_VERTEX_ATTRIB9_4_NV                                   0x8679
#define GL_MAP2_VERTEX_ATTRIB10_4_NV                                  0x867A
#define GL_MAP2_VERTEX_ATTRIB11_4_NV                                  0x867B
#define GL_MAP2_VERTEX_ATTRIB12_4_NV                                  0x867C
#define GL_MAP2_VERTEX_ATTRIB13_4_NV                                  0x867D
#define GL_MAP2_VERTEX_ATTRIB14_4_NV                                  0x867E
#define GL_MAP2_VERTEX_ATTRIB15_4_NV                                  0x867F
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE                              0x86A0
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB                          0x86A0
#define GL_TEXTURE_COMPRESSED                                         0x86A1
#define GL_TEXTURE_COMPRESSED_ARB                                     0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS                             0x86A2
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB                         0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS                                 0x86A3
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB                             0x86A3
#define GL_MAX_VERTEX_UNITS_ARB                                       0x86A4
#define GL_ACTIVE_VERTEX_UNITS_ARB                                    0x86A5
#define GL_WEIGHT_SUM_UNITY_ARB                                       0x86A6
#define GL_VERTEX_BLEND_ARB                                           0x86A7
#define GL_CURRENT_WEIGHT_ARB                                         0x86A8
#define GL_WEIGHT_ARRAY_TYPE_ARB                                      0x86A9
#define GL_WEIGHT_ARRAY_STRIDE_ARB                                    0x86AA
#define GL_WEIGHT_ARRAY_SIZE_ARB                                      0x86AB
#define GL_WEIGHT_ARRAY_POINTER_ARB                                   0x86AC
#define GL_WEIGHT_ARRAY_ARB                                           0x86AD
#define GL_DOT3_RGB                                                   0x86AE
#define GL_DOT3_RGB_ARB                                               0x86AE
#define GL_DOT3_RGBA                                                  0x86AF
#define GL_DOT3_RGBA_ARB                                              0x86AF
#define GL_EVAL_2D_NV                                                 0x86C0
#define GL_EVAL_TRIANGULAR_2D_NV                                      0x86C1
#define GL_MAP_TESSELLATION_NV                                        0x86C2
#define GL_MAP_ATTRIB_U_ORDER_NV                                      0x86C3
#define GL_MAP_ATTRIB_V_ORDER_NV                                      0x86C4
#define GL_EVAL_FRACTIONAL_TESSELLATION_NV                            0x86C5
#define GL_EVAL_VERTEX_ATTRIB0_NV                                     0x86C6
#define GL_EVAL_VERTEX_ATTRIB1_NV                                     0x86C7
#define GL_EVAL_VERTEX_ATTRIB2_NV                                     0x86C8
#define GL_EVAL_VERTEX_ATTRIB3_NV                                     0x86C9
#define GL_EVAL_VERTEX_ATTRIB4_NV                                     0x86CA
#define GL_EVAL_VERTEX_ATTRIB5_NV                                     0x86CB
#define GL_EVAL_VERTEX_ATTRIB6_NV                                     0x86CC
#define GL_EVAL_VERTEX_ATTRIB7_NV                                     0x86CD
#define GL_EVAL_VERTEX_ATTRIB8_NV                                     0x86CE
#define GL_EVAL_VERTEX_ATTRIB9_NV                                     0x86CF
#define GL_EVAL_VERTEX_ATTRIB10_NV                                    0x86D0
#define GL_EVAL_VERTEX_ATTRIB11_NV                                    0x86D1
#define GL_EVAL_VERTEX_ATTRIB12_NV                                    0x86D2
#define GL_EVAL_VERTEX_ATTRIB13_NV                                    0x86D3
#define GL_EVAL_VERTEX_ATTRIB14_NV                                    0x86D4
#define GL_EVAL_VERTEX_ATTRIB15_NV                                    0x86D5
#define GL_MAX_MAP_TESSELLATION_NV                                    0x86D6
#define GL_MAX_RATIONAL_EVAL_ORDER_NV                                 0x86D7
#define GL_MAX_PROGRAM_PATCH_ATTRIBS_NV                               0x86D8
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV                       0x86D9
#define GL_UNSIGNED_INT_S8_S8_8_8_NV                                  0x86DA
#define GL_UNSIGNED_INT_8_8_S8_S8_REV_NV                              0x86DB
#define GL_DSDT_MAG_INTENSITY_NV                                      0x86DC
#define GL_SHADER_CONSISTENT_NV                                       0x86DD
#define GL_TEXTURE_SHADER_NV                                          0x86DE
#define GL_SHADER_OPERATION_NV                                        0x86DF
#define GL_CULL_MODES_NV                                              0x86E0
#define GL_OFFSET_TEXTURE_MATRIX_NV                                   0x86E1
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV                                0x86E1
#define GL_OFFSET_TEXTURE_SCALE_NV                                    0x86E2
#define GL_OFFSET_TEXTURE_2D_SCALE_NV                                 0x86E2
#define GL_OFFSET_TEXTURE_BIAS_NV                                     0x86E3
#define GL_OFFSET_TEXTURE_2D_BIAS_NV                                  0x86E3
#define GL_PREVIOUS_TEXTURE_INPUT_NV                                  0x86E4
#define GL_CONST_EYE_NV                                               0x86E5
#define GL_PASS_THROUGH_NV                                            0x86E6
#define GL_CULL_FRAGMENT_NV                                           0x86E7
#define GL_OFFSET_TEXTURE_2D_NV                                       0x86E8
#define GL_DEPENDENT_AR_TEXTURE_2D_NV                                 0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV                                 0x86EA
#define GL_SURFACE_STATE_NV                                           0x86EB
#define GL_DOT_PRODUCT_NV                                             0x86EC
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV                               0x86ED
#define GL_DOT_PRODUCT_TEXTURE_2D_NV                                  0x86EE
#define GL_DOT_PRODUCT_TEXTURE_3D_NV                                  0x86EF
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV                            0x86F0
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV                            0x86F1
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV                            0x86F2
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV                  0x86F3
#define GL_HILO_NV                                                    0x86F4
#define GL_DSDT_NV                                                    0x86F5
#define GL_DSDT_MAG_NV                                                0x86F6
#define GL_DSDT_MAG_VIB_NV                                            0x86F7
#define GL_HILO16_NV                                                  0x86F8
#define GL_SIGNED_HILO_NV                                             0x86F9
#define GL_SIGNED_HILO16_NV                                           0x86FA
#define GL_SIGNED_RGBA_NV                                             0x86FB
#define GL_SIGNED_RGBA8_NV                                            0x86FC
#define GL_SURFACE_REGISTERED_NV                                      0x86FD
#define GL_SIGNED_RGB_NV                                              0x86FE
#define GL_SIGNED_RGB8_NV                                             0x86FF
#define GL_SURFACE_MAPPED_NV                                          0x8700
#define GL_SIGNED_LUMINANCE_NV                                        0x8701
#define GL_SIGNED_LUMINANCE8_NV                                       0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV                                  0x8703
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV                                0x8704
#define GL_SIGNED_ALPHA_NV                                            0x8705
#define GL_SIGNED_ALPHA8_NV                                           0x8706
#define GL_SIGNED_INTENSITY_NV                                        0x8707
#define GL_SIGNED_INTENSITY8_NV                                       0x8708
#define GL_DSDT8_NV                                                   0x8709
#define GL_DSDT8_MAG8_NV                                              0x870A
#define GL_DSDT8_MAG8_INTENSITY8_NV                                   0x870B
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV                               0x870C
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV                             0x870D
#define GL_HI_SCALE_NV                                                0x870E
#define GL_LO_SCALE_NV                                                0x870F
#define GL_DS_SCALE_NV                                                0x8710
#define GL_DT_SCALE_NV                                                0x8711
#define GL_MAGNITUDE_SCALE_NV                                         0x8712
#define GL_VIBRANCE_SCALE_NV                                          0x8713
#define GL_HI_BIAS_NV                                                 0x8714
#define GL_LO_BIAS_NV                                                 0x8715
#define GL_DS_BIAS_NV                                                 0x8716
#define GL_DT_BIAS_NV                                                 0x8717
#define GL_MAGNITUDE_BIAS_NV                                          0x8718
#define GL_VIBRANCE_BIAS_NV                                           0x8719
#define GL_TEXTURE_BORDER_VALUES_NV                                   0x871A
#define GL_TEXTURE_HI_SIZE_NV                                         0x871B
#define GL_TEXTURE_LO_SIZE_NV                                         0x871C
#define GL_TEXTURE_DS_SIZE_NV                                         0x871D
#define GL_TEXTURE_DT_SIZE_NV                                         0x871E
#define GL_TEXTURE_MAG_SIZE_NV                                        0x871F
#define GL_MODELVIEW2_ARB                                             0x8722
#define GL_MODELVIEW3_ARB                                             0x8723
#define GL_MODELVIEW4_ARB                                             0x8724
#define GL_MODELVIEW5_ARB                                             0x8725
#define GL_MODELVIEW6_ARB                                             0x8726
#define GL_MODELVIEW7_ARB                                             0x8727
#define GL_MODELVIEW8_ARB                                             0x8728
#define GL_MODELVIEW9_ARB                                             0x8729
#define GL_MODELVIEW10_ARB                                            0x872A
#define GL_MODELVIEW11_ARB                                            0x872B
#define GL_MODELVIEW12_ARB                                            0x872C
#define GL_MODELVIEW13_ARB                                            0x872D
#define GL_MODELVIEW14_ARB                                            0x872E
#define GL_MODELVIEW15_ARB                                            0x872F
#define GL_MODELVIEW16_ARB                                            0x8730
#define GL_MODELVIEW17_ARB                                            0x8731
#define GL_MODELVIEW18_ARB                                            0x8732
#define GL_MODELVIEW19_ARB                                            0x8733
#define GL_MODELVIEW20_ARB                                            0x8734
#define GL_MODELVIEW21_ARB                                            0x8735
#define GL_MODELVIEW22_ARB                                            0x8736
#define GL_MODELVIEW23_ARB                                            0x8737
#define GL_MODELVIEW24_ARB                                            0x8738
#define GL_MODELVIEW25_ARB                                            0x8739
#define GL_MODELVIEW26_ARB                                            0x873A
#define GL_MODELVIEW27_ARB                                            0x873B
#define GL_MODELVIEW28_ARB                                            0x873C
#define GL_MODELVIEW29_ARB                                            0x873D
#define GL_MODELVIEW30_ARB                                            0x873E
#define GL_MODELVIEW31_ARB                                            0x873F
#define GL_DOT3_RGB_EXT                                               0x8740
#define GL_DOT3_RGBA_EXT                                              0x8741
#define GL_PROGRAM_BINARY_LENGTH                                      0x8741
#define GL_MIRROR_CLAMP_EXT                                           0x8742
#define GL_MIRROR_CLAMP_TO_EDGE                                       0x8743
#define GL_MIRROR_CLAMP_TO_EDGE_EXT                                   0x8743
#define GL_SET_AMD                                                    0x874A
#define GL_REPLACE_VALUE_AMD                                          0x874B
#define GL_STENCIL_OP_VALUE_AMD                                       0x874C
#define GL_STENCIL_BACK_OP_VALUE_AMD                                  0x874D
#define GL_VERTEX_ATTRIB_ARRAY_LONG                                   0x874E
#define GL_OCCLUSION_QUERY_EVENT_MASK_AMD                             0x874F
#define GL_BUFFER_SIZE                                                0x8764
#define GL_BUFFER_SIZE_ARB                                            0x8764
#define GL_BUFFER_USAGE                                               0x8765
#define GL_BUFFER_USAGE_ARB                                           0x8765
#define GL_VERTEX_SHADER_EXT                                          0x8780
#define GL_VERTEX_SHADER_BINDING_EXT                                  0x8781
#define GL_OP_INDEX_EXT                                               0x8782
#define GL_OP_NEGATE_EXT                                              0x8783
#define GL_OP_DOT3_EXT                                                0x8784
#define GL_OP_DOT4_EXT                                                0x8785
#define GL_OP_MUL_EXT                                                 0x8786
#define GL_OP_ADD_EXT                                                 0x8787
#define GL_OP_MADD_EXT                                                0x8788
#define GL_OP_FRAC_EXT                                                0x8789
#define GL_OP_MAX_EXT                                                 0x878A
#define GL_OP_MIN_EXT                                                 0x878B
#define GL_OP_SET_GE_EXT                                              0x878C
#define GL_OP_SET_LT_EXT                                              0x878D
#define GL_OP_CLAMP_EXT                                               0x878E
#define GL_OP_FLOOR_EXT                                               0x878F
#define GL_OP_ROUND_EXT                                               0x8790
#define GL_OP_EXP_BASE_2_EXT                                          0x8791
#define GL_OP_LOG_BASE_2_EXT                                          0x8792
#define GL_OP_POWER_EXT                                               0x8793
#define GL_OP_RECIP_EXT                                               0x8794
#define GL_OP_RECIP_SQRT_EXT                                          0x8795
#define GL_OP_SUB_EXT                                                 0x8796
#define GL_OP_CROSS_PRODUCT_EXT                                       0x8797
#define GL_OP_MULTIPLY_MATRIX_EXT                                     0x8798
#define GL_OP_MOV_EXT                                                 0x8799
#define GL_OUTPUT_VERTEX_EXT                                          0x879A
#define GL_OUTPUT_COLOR0_EXT                                          0x879B
#define GL_OUTPUT_COLOR1_EXT                                          0x879C
#define GL_OUTPUT_TEXTURE_COORD0_EXT                                  0x879D
#define GL_OUTPUT_TEXTURE_COORD1_EXT                                  0x879E
#define GL_OUTPUT_TEXTURE_COORD2_EXT                                  0x879F
#define GL_OUTPUT_TEXTURE_COORD3_EXT                                  0x87A0
#define GL_OUTPUT_TEXTURE_COORD4_EXT                                  0x87A1
#define GL_OUTPUT_TEXTURE_COORD5_EXT                                  0x87A2
#define GL_OUTPUT_TEXTURE_COORD6_EXT                                  0x87A3
#define GL_OUTPUT_TEXTURE_COORD7_EXT                                  0x87A4
#define GL_OUTPUT_TEXTURE_COORD8_EXT                                  0x87A5
#define GL_OUTPUT_TEXTURE_COORD9_EXT                                  0x87A6
#define GL_OUTPUT_TEXTURE_COORD10_EXT                                 0x87A7
#define GL_OUTPUT_TEXTURE_COORD11_EXT                                 0x87A8
#define GL_OUTPUT_TEXTURE_COORD12_EXT                                 0x87A9
#define GL_OUTPUT_TEXTURE_COORD13_EXT                                 0x87AA
#define GL_OUTPUT_TEXTURE_COORD14_EXT                                 0x87AB
#define GL_OUTPUT_TEXTURE_COORD15_EXT                                 0x87AC
#define GL_OUTPUT_TEXTURE_COORD16_EXT                                 0x87AD
#define GL_OUTPUT_TEXTURE_COORD17_EXT                                 0x87AE
#define GL_OUTPUT_TEXTURE_COORD18_EXT                                 0x87AF
#define GL_OUTPUT_TEXTURE_COORD19_EXT                                 0x87B0
#define GL_OUTPUT_TEXTURE_COORD20_EXT                                 0x87B1
#define GL_OUTPUT_TEXTURE_COORD21_EXT                                 0x87B2
#define GL_OUTPUT_TEXTURE_COORD22_EXT                                 0x87B3
#define GL_OUTPUT_TEXTURE_COORD23_EXT                                 0x87B4
#define GL_OUTPUT_TEXTURE_COORD24_EXT                                 0x87B5
#define GL_OUTPUT_TEXTURE_COORD25_EXT                                 0x87B6
#define GL_OUTPUT_TEXTURE_COORD26_EXT                                 0x87B7
#define GL_OUTPUT_TEXTURE_COORD27_EXT                                 0x87B8
#define GL_OUTPUT_TEXTURE_COORD28_EXT                                 0x87B9
#define GL_OUTPUT_TEXTURE_COORD29_EXT                                 0x87BA
#define GL_OUTPUT_TEXTURE_COORD30_EXT                                 0x87BB
#define GL_OUTPUT_TEXTURE_COORD31_EXT                                 0x87BC
#define GL_OUTPUT_FOG_EXT                                             0x87BD
#define GL_SCALAR_EXT                                                 0x87BE
#define GL_VECTOR_EXT                                                 0x87BF
#define GL_MATRIX_EXT                                                 0x87C0
#define GL_VARIANT_EXT                                                0x87C1
#define GL_INVARIANT_EXT                                              0x87C2
#define GL_LOCAL_CONSTANT_EXT                                         0x87C3
#define GL_LOCAL_EXT                                                  0x87C4
#define GL_MAX_VERTEX_SHADER_INSTRUCTIONS_EXT                         0x87C5
#define GL_MAX_VERTEX_SHADER_VARIANTS_EXT                             0x87C6
#define GL_MAX_VERTEX_SHADER_INVARIANTS_EXT                           0x87C7
#define GL_MAX_VERTEX_SHADER_LOCAL_CONSTANTS_EXT                      0x87C8
#define GL_MAX_VERTEX_SHADER_LOCALS_EXT                               0x87C9
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_INSTRUCTIONS_EXT               0x87CA
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_VARIANTS_EXT                   0x87CB
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCAL_CONSTANTS_EXT            0x87CC
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_INVARIANTS_EXT                 0x87CD
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCALS_EXT                     0x87CE
#define GL_VERTEX_SHADER_INSTRUCTIONS_EXT                             0x87CF
#define GL_VERTEX_SHADER_VARIANTS_EXT                                 0x87D0
#define GL_VERTEX_SHADER_INVARIANTS_EXT                               0x87D1
#define GL_VERTEX_SHADER_LOCAL_CONSTANTS_EXT                          0x87D2
#define GL_VERTEX_SHADER_LOCALS_EXT                                   0x87D3
#define GL_VERTEX_SHADER_OPTIMIZED_EXT                                0x87D4
#define GL_X_EXT                                                      0x87D5
#define GL_Y_EXT                                                      0x87D6
#define GL_Z_EXT                                                      0x87D7
#define GL_W_EXT                                                      0x87D8
#define GL_NEGATIVE_X_EXT                                             0x87D9
#define GL_NEGATIVE_Y_EXT                                             0x87DA
#define GL_NEGATIVE_Z_EXT                                             0x87DB
#define GL_NEGATIVE_W_EXT                                             0x87DC
#define GL_ZERO_EXT                                                   0x87DD
#define GL_ONE_EXT                                                    0x87DE
#define GL_NEGATIVE_ONE_EXT                                           0x87DF
#define GL_NORMALIZED_RANGE_EXT                                       0x87E0
#define GL_FULL_RANGE_EXT                                             0x87E1
#define GL_CURRENT_VERTEX_EXT                                         0x87E2
#define GL_MVP_MATRIX_EXT                                             0x87E3
#define GL_VARIANT_VALUE_EXT                                          0x87E4
#define GL_VARIANT_DATATYPE_EXT                                       0x87E5
#define GL_VARIANT_ARRAY_STRIDE_EXT                                   0x87E6
#define GL_VARIANT_ARRAY_TYPE_EXT                                     0x87E7
#define GL_VARIANT_ARRAY_EXT                                          0x87E8
#define GL_VARIANT_ARRAY_POINTER_EXT                                  0x87E9
#define GL_INVARIANT_VALUE_EXT                                        0x87EA
#define GL_INVARIANT_DATATYPE_EXT                                     0x87EB
#define GL_LOCAL_CONSTANT_VALUE_EXT                                   0x87EC
#define GL_LOCAL_CONSTANT_DATATYPE_EXT                                0x87ED
#define GL_NUM_PROGRAM_BINARY_FORMATS                                 0x87FE
#define GL_PROGRAM_BINARY_FORMATS                                     0x87FF
#define GL_STENCIL_BACK_FUNC                                          0x8800
#define GL_STENCIL_BACK_FAIL                                          0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL                               0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS                               0x8803
#define GL_FRAGMENT_PROGRAM_ARB                                       0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB                               0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB                               0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB                               0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB                        0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB                        0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB                        0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB                           0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB                           0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB                           0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB                    0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB                    0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB                    0x8810
#define GL_RGBA32F                                                    0x8814
#define GL_RGBA32F_ARB                                                0x8814
#define GL_RGBA_FLOAT32_APPLE                                         0x8814
#define GL_RGB32F                                                     0x8815
#define GL_RGB32F_ARB                                                 0x8815
#define GL_RGB_FLOAT32_APPLE                                          0x8815
#define GL_ALPHA32F_ARB                                               0x8816
#define GL_ALPHA_FLOAT32_APPLE                                        0x8816
#define GL_INTENSITY32F_ARB                                           0x8817
#define GL_INTENSITY_FLOAT32_APPLE                                    0x8817
#define GL_LUMINANCE32F_ARB                                           0x8818
#define GL_LUMINANCE_FLOAT32_APPLE                                    0x8818
#define GL_LUMINANCE_ALPHA32F_ARB                                     0x8819
#define GL_LUMINANCE_ALPHA_FLOAT32_APPLE                              0x8819
#define GL_RGBA16F                                                    0x881A
#define GL_RGBA16F_ARB                                                0x881A
#define GL_RGBA_FLOAT16_APPLE                                         0x881A
#define GL_RGB16F                                                     0x881B
#define GL_RGB16F_ARB                                                 0x881B
#define GL_RGB_FLOAT16_APPLE                                          0x881B
#define GL_ALPHA16F_ARB                                               0x881C
#define GL_ALPHA_FLOAT16_APPLE                                        0x881C
#define GL_INTENSITY16F_ARB                                           0x881D
#define GL_INTENSITY_FLOAT16_APPLE                                    0x881D
#define GL_LUMINANCE16F_ARB                                           0x881E
#define GL_LUMINANCE_FLOAT16_APPLE                                    0x881E
#define GL_LUMINANCE_ALPHA16F_ARB                                     0x881F
#define GL_LUMINANCE_ALPHA_FLOAT16_APPLE                              0x881F
#define GL_RGBA_FLOAT_MODE_ARB                                        0x8820
#define GL_MAX_DRAW_BUFFERS                                           0x8824
#define GL_MAX_DRAW_BUFFERS_ARB                                       0x8824
#define GL_DRAW_BUFFER0                                               0x8825
#define GL_DRAW_BUFFER0_ARB                                           0x8825
#define GL_DRAW_BUFFER1                                               0x8826
#define GL_DRAW_BUFFER1_ARB                                           0x8826
#define GL_DRAW_BUFFER2                                               0x8827
#define GL_DRAW_BUFFER2_ARB                                           0x8827
#define GL_DRAW_BUFFER3                                               0x8828
#define GL_DRAW_BUFFER3_ARB                                           0x8828
#define GL_DRAW_BUFFER4                                               0x8829
#define GL_DRAW_BUFFER4_ARB                                           0x8829
#define GL_DRAW_BUFFER5                                               0x882A
#define GL_DRAW_BUFFER5_ARB                                           0x882A
#define GL_DRAW_BUFFER6                                               0x882B
#define GL_DRAW_BUFFER6_ARB                                           0x882B
#define GL_DRAW_BUFFER7                                               0x882C
#define GL_DRAW_BUFFER7_ARB                                           0x882C
#define GL_DRAW_BUFFER8                                               0x882D
#define GL_DRAW_BUFFER8_ARB                                           0x882D
#define GL_DRAW_BUFFER9                                               0x882E
#define GL_DRAW_BUFFER9_ARB                                           0x882E
#define GL_DRAW_BUFFER10                                              0x882F
#define GL_DRAW_BUFFER10_ARB                                          0x882F
#define GL_DRAW_BUFFER11                                              0x8830
#define GL_DRAW_BUFFER11_ARB                                          0x8830
#define GL_DRAW_BUFFER12                                              0x8831
#define GL_DRAW_BUFFER12_ARB                                          0x8831
#define GL_DRAW_BUFFER13                                              0x8832
#define GL_DRAW_BUFFER13_ARB                                          0x8832
#define GL_DRAW_BUFFER14                                              0x8833
#define GL_DRAW_BUFFER14_ARB                                          0x8833
#define GL_DRAW_BUFFER15                                              0x8834
#define GL_DRAW_BUFFER15_ARB                                          0x8834
#define GL_BLEND_EQUATION_ALPHA                                       0x883D
#define GL_BLEND_EQUATION_ALPHA_EXT                                   0x883D
#define GL_SUBSAMPLE_DISTANCE_AMD                                     0x883F
#define GL_MATRIX_PALETTE_ARB                                         0x8840
#define GL_MAX_MATRIX_PALETTE_STACK_DEPTH_ARB                         0x8841
#define GL_MAX_PALETTE_MATRICES_ARB                                   0x8842
#define GL_CURRENT_PALETTE_MATRIX_ARB                                 0x8843
#define GL_MATRIX_INDEX_ARRAY_ARB                                     0x8844
#define GL_CURRENT_MATRIX_INDEX_ARB                                   0x8845
#define GL_MATRIX_INDEX_ARRAY_SIZE_ARB                                0x8846
#define GL_MATRIX_INDEX_ARRAY_TYPE_ARB                                0x8847
#define GL_MATRIX_INDEX_ARRAY_STRIDE_ARB                              0x8848
#define GL_MATRIX_INDEX_ARRAY_POINTER_ARB                             0x8849
#define GL_TEXTURE_DEPTH_SIZE                                         0x884A
#define GL_TEXTURE_DEPTH_SIZE_ARB                                     0x884A
#define GL_DEPTH_TEXTURE_MODE                                         0x884B
#define GL_DEPTH_TEXTURE_MODE_ARB                                     0x884B
#define GL_TEXTURE_COMPARE_MODE                                       0x884C
#define GL_TEXTURE_COMPARE_MODE_ARB                                   0x884C
#define GL_TEXTURE_COMPARE_FUNC                                       0x884D
#define GL_TEXTURE_COMPARE_FUNC_ARB                                   0x884D
#define GL_COMPARE_R_TO_TEXTURE                                       0x884E
#define GL_COMPARE_R_TO_TEXTURE_ARB                                   0x884E
#define GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT                           0x884E
#define GL_COMPARE_REF_TO_TEXTURE                                     0x884E
#define GL_TEXTURE_CUBE_MAP_SEAMLESS                                  0x884F
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV                            0x8850
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_SCALE_NV                      0x8851
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_NV                     0x8852
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_SCALE_NV               0x8853
#define GL_OFFSET_HILO_TEXTURE_2D_NV                                  0x8854
#define GL_OFFSET_HILO_TEXTURE_RECTANGLE_NV                           0x8855
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_2D_NV                       0x8856
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_RECTANGLE_NV                0x8857
#define GL_DEPENDENT_HILO_TEXTURE_2D_NV                               0x8858
#define GL_DEPENDENT_RGB_TEXTURE_3D_NV                                0x8859
#define GL_DEPENDENT_RGB_TEXTURE_CUBE_MAP_NV                          0x885A
#define GL_DOT_PRODUCT_PASS_THROUGH_NV                                0x885B
#define GL_DOT_PRODUCT_TEXTURE_1D_NV                                  0x885C
#define GL_DOT_PRODUCT_AFFINE_DEPTH_REPLACE_NV                        0x885D
#define GL_HILO8_NV                                                   0x885E
#define GL_SIGNED_HILO8_NV                                            0x885F
#define GL_FORCE_BLUE_TO_ONE_NV                                       0x8860
#define GL_POINT_SPRITE                                               0x8861
#define GL_POINT_SPRITE_ARB                                           0x8861
#define GL_POINT_SPRITE_NV                                            0x8861
#define GL_COORD_REPLACE                                              0x8862
#define GL_COORD_REPLACE_ARB                                          0x8862
#define GL_COORD_REPLACE_NV                                           0x8862
#define GL_POINT_SPRITE_R_MODE_NV                                     0x8863
#define GL_PIXEL_COUNTER_BITS_NV                                      0x8864
#define GL_QUERY_COUNTER_BITS                                         0x8864
#define GL_QUERY_COUNTER_BITS_ARB                                     0x8864
#define GL_CURRENT_OCCLUSION_QUERY_ID_NV                              0x8865
#define GL_CURRENT_QUERY                                              0x8865
#define GL_CURRENT_QUERY_ARB                                          0x8865
#define GL_PIXEL_COUNT_NV                                             0x8866
#define GL_QUERY_RESULT                                               0x8866
#define GL_QUERY_RESULT_ARB                                           0x8866
#define GL_PIXEL_COUNT_AVAILABLE_NV                                   0x8867
#define GL_QUERY_RESULT_AVAILABLE                                     0x8867
#define GL_QUERY_RESULT_AVAILABLE_ARB                                 0x8867
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV                   0x8868
#define GL_MAX_VERTEX_ATTRIBS                                         0x8869
#define GL_MAX_VERTEX_ATTRIBS_ARB                                     0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED                             0x886A
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB                         0x886A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS                          0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS                       0x886D
#define GL_DEPTH_STENCIL_TO_RGBA_NV                                   0x886E
#define GL_DEPTH_STENCIL_TO_BGRA_NV                                   0x886F
#define GL_FRAGMENT_PROGRAM_NV                                        0x8870
#define GL_MAX_TEXTURE_COORDS                                         0x8871
#define GL_MAX_TEXTURE_COORDS_ARB                                     0x8871
#define GL_MAX_TEXTURE_COORDS_NV                                      0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS                                    0x8872
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB                                0x8872
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV                                 0x8872
#define GL_FRAGMENT_PROGRAM_BINDING_NV                                0x8873
#define GL_PROGRAM_ERROR_STRING_ARB                                   0x8874
#define GL_PROGRAM_ERROR_STRING_NV                                    0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB                                   0x8875
#define GL_PROGRAM_FORMAT_ARB                                         0x8876
#define GL_WRITE_PIXEL_DATA_RANGE_NV                                  0x8878
#define GL_READ_PIXEL_DATA_RANGE_NV                                   0x8879
#define GL_WRITE_PIXEL_DATA_RANGE_LENGTH_NV                           0x887A
#define GL_READ_PIXEL_DATA_RANGE_LENGTH_NV                            0x887B
#define GL_WRITE_PIXEL_DATA_RANGE_POINTER_NV                          0x887C
#define GL_READ_PIXEL_DATA_RANGE_POINTER_NV                           0x887D
#define GL_GEOMETRY_SHADER_INVOCATIONS                                0x887F
#define GL_FLOAT_R_NV                                                 0x8880
#define GL_FLOAT_RG_NV                                                0x8881
#define GL_FLOAT_RGB_NV                                               0x8882
#define GL_FLOAT_RGBA_NV                                              0x8883
#define GL_FLOAT_R16_NV                                               0x8884
#define GL_FLOAT_R32_NV                                               0x8885
#define GL_FLOAT_RG16_NV                                              0x8886
#define GL_FLOAT_RG32_NV                                              0x8887
#define GL_FLOAT_RGB16_NV                                             0x8888
#define GL_FLOAT_RGB32_NV                                             0x8889
#define GL_FLOAT_RGBA16_NV                                            0x888A
#define GL_FLOAT_RGBA32_NV                                            0x888B
#define GL_TEXTURE_FLOAT_COMPONENTS_NV                                0x888C
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV                                 0x888D
#define GL_FLOAT_RGBA_MODE_NV                                         0x888E
#define GL_TEXTURE_UNSIGNED_REMAP_MODE_NV                             0x888F
#define GL_DEPTH_BOUNDS_TEST_EXT                                      0x8890
#define GL_DEPTH_BOUNDS_EXT                                           0x8891
#define GL_ARRAY_BUFFER                                               0x8892
#define GL_ARRAY_BUFFER_ARB                                           0x8892
#define GL_ELEMENT_ARRAY_BUFFER                                       0x8893
#define GL_ELEMENT_ARRAY_BUFFER_ARB                                   0x8893
#define GL_ARRAY_BUFFER_BINDING                                       0x8894
#define GL_ARRAY_BUFFER_BINDING_ARB                                   0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING                               0x8895
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB                           0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING                                0x8896
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB                            0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING                                0x8897
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB                            0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING                                 0x8898
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB                             0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING                                 0x8899
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB                             0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING                         0x889A
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB                     0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING                             0x889B
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB                         0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING                       0x889C
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB                   0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB                    0x889D
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING                        0x889D
#define GL_FOG_COORD_ARRAY_BUFFER_BINDING                             0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING                                0x889E
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB                            0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING                         0x889F
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB                     0x889F
#define GL_PROGRAM_INSTRUCTIONS_ARB                                   0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB                               0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB                            0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB                        0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB                                    0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB                                0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB                             0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB                         0x88A7
#define GL_PROGRAM_PARAMETERS_ARB                                     0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB                                 0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB                              0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB                          0x88AB
#define GL_PROGRAM_ATTRIBS_ARB                                        0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB                                    0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB                                 0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB                             0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB                              0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB                          0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB                       0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB                   0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB                           0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB                             0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB                            0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB                               0x88B7
#define GL_READ_ONLY                                                  0x88B8
#define GL_READ_ONLY_ARB                                              0x88B8
#define GL_WRITE_ONLY                                                 0x88B9
#define GL_WRITE_ONLY_ARB                                             0x88B9
#define GL_READ_WRITE                                                 0x88BA
#define GL_READ_WRITE_ARB                                             0x88BA
#define GL_BUFFER_ACCESS                                              0x88BB
#define GL_BUFFER_ACCESS_ARB                                          0x88BB
#define GL_BUFFER_MAPPED                                              0x88BC
#define GL_BUFFER_MAPPED_ARB                                          0x88BC
#define GL_BUFFER_MAP_POINTER                                         0x88BD
#define GL_BUFFER_MAP_POINTER_ARB                                     0x88BD
#define GL_WRITE_DISCARD_NV                                           0x88BE
#define GL_TIME_ELAPSED                                               0x88BF
#define GL_TIME_ELAPSED_EXT                                           0x88BF
#define GL_MATRIX0_ARB                                                0x88C0
#define GL_MATRIX1_ARB                                                0x88C1
#define GL_MATRIX2_ARB                                                0x88C2
#define GL_MATRIX3_ARB                                                0x88C3
#define GL_MATRIX4_ARB                                                0x88C4
#define GL_MATRIX5_ARB                                                0x88C5
#define GL_MATRIX6_ARB                                                0x88C6
#define GL_MATRIX7_ARB                                                0x88C7
#define GL_MATRIX8_ARB                                                0x88C8
#define GL_MATRIX9_ARB                                                0x88C9
#define GL_MATRIX10_ARB                                               0x88CA
#define GL_MATRIX11_ARB                                               0x88CB
#define GL_MATRIX12_ARB                                               0x88CC
#define GL_MATRIX13_ARB                                               0x88CD
#define GL_MATRIX14_ARB                                               0x88CE
#define GL_MATRIX15_ARB                                               0x88CF
#define GL_MATRIX16_ARB                                               0x88D0
#define GL_MATRIX17_ARB                                               0x88D1
#define GL_MATRIX18_ARB                                               0x88D2
#define GL_MATRIX19_ARB                                               0x88D3
#define GL_MATRIX20_ARB                                               0x88D4
#define GL_MATRIX21_ARB                                               0x88D5
#define GL_MATRIX22_ARB                                               0x88D6
#define GL_MATRIX23_ARB                                               0x88D7
#define GL_MATRIX24_ARB                                               0x88D8
#define GL_MATRIX25_ARB                                               0x88D9
#define GL_MATRIX26_ARB                                               0x88DA
#define GL_MATRIX27_ARB                                               0x88DB
#define GL_MATRIX28_ARB                                               0x88DC
#define GL_MATRIX29_ARB                                               0x88DD
#define GL_MATRIX30_ARB                                               0x88DE
#define GL_MATRIX31_ARB                                               0x88DF
#define GL_STREAM_DRAW                                                0x88E0
#define GL_STREAM_DRAW_ARB                                            0x88E0
#define GL_STREAM_READ                                                0x88E1
#define GL_STREAM_READ_ARB                                            0x88E1
#define GL_STREAM_COPY                                                0x88E2
#define GL_STREAM_COPY_ARB                                            0x88E2
#define GL_STATIC_DRAW                                                0x88E4
#define GL_STATIC_DRAW_ARB                                            0x88E4
#define GL_STATIC_READ                                                0x88E5
#define GL_STATIC_READ_ARB                                            0x88E5
#define GL_STATIC_COPY                                                0x88E6
#define GL_STATIC_COPY_ARB                                            0x88E6
#define GL_DYNAMIC_DRAW                                               0x88E8
#define GL_DYNAMIC_DRAW_ARB                                           0x88E8
#define GL_DYNAMIC_READ                                               0x88E9
#define GL_DYNAMIC_READ_ARB                                           0x88E9
#define GL_DYNAMIC_COPY                                               0x88EA
#define GL_DYNAMIC_COPY_ARB                                           0x88EA
#define GL_PIXEL_PACK_BUFFER                                          0x88EB
#define GL_PIXEL_PACK_BUFFER_ARB                                      0x88EB
#define GL_PIXEL_PACK_BUFFER_EXT                                      0x88EB
#define GL_PIXEL_UNPACK_BUFFER                                        0x88EC
#define GL_PIXEL_UNPACK_BUFFER_ARB                                    0x88EC
#define GL_PIXEL_UNPACK_BUFFER_EXT                                    0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING                                  0x88ED
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB                              0x88ED
#define GL_PIXEL_PACK_BUFFER_BINDING_EXT                              0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING                                0x88EF
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB                            0x88EF
#define GL_PIXEL_UNPACK_BUFFER_BINDING_EXT                            0x88EF
#define GL_DEPTH24_STENCIL8                                           0x88F0
#define GL_DEPTH24_STENCIL8_EXT                                       0x88F0
#define GL_TEXTURE_STENCIL_SIZE                                       0x88F1
#define GL_TEXTURE_STENCIL_SIZE_EXT                                   0x88F1
#define GL_STENCIL_TAG_BITS_EXT                                       0x88F2
#define GL_STENCIL_CLEAR_TAG_VALUE_EXT                                0x88F3
#define GL_MAX_PROGRAM_EXEC_INSTRUCTIONS_NV                           0x88F4
#define GL_MAX_PROGRAM_CALL_DEPTH_NV                                  0x88F5
#define GL_MAX_PROGRAM_IF_DEPTH_NV                                    0x88F6
#define GL_MAX_PROGRAM_LOOP_DEPTH_NV                                  0x88F7
#define GL_MAX_PROGRAM_LOOP_COUNT_NV                                  0x88F8
#define GL_SRC1_COLOR                                                 0x88F9
#define GL_ONE_MINUS_SRC1_COLOR                                       0x88FA
#define GL_ONE_MINUS_SRC1_ALPHA                                       0x88FB
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS                               0x88FC
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER                                0x88FD
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_EXT                            0x88FD
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_NV                             0x88FD
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR                                0x88FE
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB                            0x88FE
#define GL_MAX_ARRAY_TEXTURE_LAYERS                                   0x88FF
#define GL_MAX_ARRAY_TEXTURE_LAYERS_EXT                               0x88FF
#define GL_MIN_PROGRAM_TEXEL_OFFSET                                   0x8904
#define GL_MIN_PROGRAM_TEXEL_OFFSET_EXT                               0x8904
#define GL_MIN_PROGRAM_TEXEL_OFFSET_NV                                0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET                                   0x8905
#define GL_MAX_PROGRAM_TEXEL_OFFSET_EXT                               0x8905
#define GL_MAX_PROGRAM_TEXEL_OFFSET_NV                                0x8905
#define GL_PROGRAM_ATTRIB_COMPONENTS_NV                               0x8906
#define GL_PROGRAM_RESULT_COMPONENTS_NV                               0x8907
#define GL_MAX_PROGRAM_ATTRIB_COMPONENTS_NV                           0x8908
#define GL_MAX_PROGRAM_RESULT_COMPONENTS_NV                           0x8909
#define GL_STENCIL_TEST_TWO_SIDE_EXT                                  0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT                                    0x8911
#define GL_MIRROR_CLAMP_TO_BORDER_EXT                                 0x8912
#define GL_SAMPLES_PASSED                                             0x8914
#define GL_SAMPLES_PASSED_ARB                                         0x8914
#define GL_GEOMETRY_VERTICES_OUT                                      0x8916
#define GL_GEOMETRY_INPUT_TYPE                                        0x8917
#define GL_GEOMETRY_OUTPUT_TYPE                                       0x8918
#define GL_SAMPLER_BINDING                                            0x8919
#define GL_CLAMP_VERTEX_COLOR                                         0x891A
#define GL_CLAMP_VERTEX_COLOR_ARB                                     0x891A
#define GL_CLAMP_FRAGMENT_COLOR                                       0x891B
#define GL_CLAMP_FRAGMENT_COLOR_ARB                                   0x891B
#define GL_CLAMP_READ_COLOR                                           0x891C
#define GL_CLAMP_READ_COLOR_ARB                                       0x891C
#define GL_FIXED_ONLY                                                 0x891D
#define GL_FIXED_ONLY_ARB                                             0x891D
#define GL_TESS_CONTROL_PROGRAM_NV                                    0x891E
#define GL_TESS_EVALUATION_PROGRAM_NV                                 0x891F
#define GL_VERTEX_ATTRIB_MAP1_APPLE                                   0x8A00
#define GL_VERTEX_ATTRIB_MAP2_APPLE                                   0x8A01
#define GL_VERTEX_ATTRIB_MAP1_SIZE_APPLE                              0x8A02
#define GL_VERTEX_ATTRIB_MAP1_COEFF_APPLE                             0x8A03
#define GL_VERTEX_ATTRIB_MAP1_ORDER_APPLE                             0x8A04
#define GL_VERTEX_ATTRIB_MAP1_DOMAIN_APPLE                            0x8A05
#define GL_VERTEX_ATTRIB_MAP2_SIZE_APPLE                              0x8A06
#define GL_VERTEX_ATTRIB_MAP2_COEFF_APPLE                             0x8A07
#define GL_VERTEX_ATTRIB_MAP2_ORDER_APPLE                             0x8A08
#define GL_VERTEX_ATTRIB_MAP2_DOMAIN_APPLE                            0x8A09
#define GL_DRAW_PIXELS_APPLE                                          0x8A0A
#define GL_FENCE_APPLE                                                0x8A0B
#define GL_ELEMENT_ARRAY_APPLE                                        0x8A0C
#define GL_ELEMENT_ARRAY_TYPE_APPLE                                   0x8A0D
#define GL_ELEMENT_ARRAY_POINTER_APPLE                                0x8A0E
#define GL_COLOR_FLOAT_APPLE                                          0x8A0F
#define GL_UNIFORM_BUFFER                                             0x8A11
#define GL_BUFFER_SERIALIZED_MODIFY_APPLE                             0x8A12
#define GL_BUFFER_FLUSHING_UNMAP_APPLE                                0x8A13
#define GL_AUX_DEPTH_STENCIL_APPLE                                    0x8A14
#define GL_PACK_ROW_BYTES_APPLE                                       0x8A15
#define GL_UNPACK_ROW_BYTES_APPLE                                     0x8A16
#define GL_RELEASED_APPLE                                             0x8A19
#define GL_VOLATILE_APPLE                                             0x8A1A
#define GL_RETAINED_APPLE                                             0x8A1B
#define GL_UNDEFINED_APPLE                                            0x8A1C
#define GL_PURGEABLE_APPLE                                            0x8A1D
#define GL_RGB_422_APPLE                                              0x8A1F
#define GL_UNIFORM_BUFFER_BINDING                                     0x8A28
#define GL_UNIFORM_BUFFER_START                                       0x8A29
#define GL_UNIFORM_BUFFER_SIZE                                        0x8A2A
#define GL_MAX_VERTEX_UNIFORM_BLOCKS                                  0x8A2B
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS                                0x8A2C
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS                                0x8A2D
#define GL_MAX_COMBINED_UNIFORM_BLOCKS                                0x8A2E
#define GL_MAX_UNIFORM_BUFFER_BINDINGS                                0x8A2F
#define GL_MAX_UNIFORM_BLOCK_SIZE                                     0x8A30
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS                     0x8A31
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS                   0x8A32
#define GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS                   0x8A33
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT                            0x8A34
#define GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH                       0x8A35
#define GL_ACTIVE_UNIFORM_BLOCKS                                      0x8A36
#define GL_UNIFORM_TYPE                                               0x8A37
#define GL_UNIFORM_SIZE                                               0x8A38
#define GL_UNIFORM_NAME_LENGTH                                        0x8A39
#define GL_UNIFORM_BLOCK_INDEX                                        0x8A3A
#define GL_UNIFORM_OFFSET                                             0x8A3B
#define GL_UNIFORM_ARRAY_STRIDE                                       0x8A3C
#define GL_UNIFORM_MATRIX_STRIDE                                      0x8A3D
#define GL_UNIFORM_IS_ROW_MAJOR                                       0x8A3E
#define GL_UNIFORM_BLOCK_BINDING                                      0x8A3F
#define GL_UNIFORM_BLOCK_DATA_SIZE                                    0x8A40
#define GL_UNIFORM_BLOCK_NAME_LENGTH                                  0x8A41
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS                              0x8A42
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES                       0x8A43
#define GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER                  0x8A44
#define GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER                0x8A45
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER                0x8A46
#define GL_TEXTURE_SRGB_DECODE_EXT                                    0x8A48
#define GL_DECODE_EXT                                                 0x8A49
#define GL_SKIP_DECODE_EXT                                            0x8A4A
#define GL_PROGRAM_PIPELINE_OBJECT_EXT                                0x8A4F
#define GL_RGB_RAW_422_APPLE                                          0x8A51
#define GL_FRAGMENT_SHADER_DISCARDS_SAMPLES_EXT                       0x8A52
#define GL_FRAGMENT_SHADER                                            0x8B30
#define GL_FRAGMENT_SHADER_ARB                                        0x8B30
#define GL_VERTEX_SHADER                                              0x8B31
#define GL_VERTEX_SHADER_ARB                                          0x8B31
#define GL_PROGRAM_OBJECT_ARB                                         0x8B40
#define GL_PROGRAM_OBJECT_EXT                                         0x8B40
#define GL_SHADER_OBJECT_ARB                                          0x8B48
#define GL_SHADER_OBJECT_EXT                                          0x8B48
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS                            0x8B49
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB                        0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS                              0x8B4A
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB                          0x8B4A
#define GL_MAX_VARYING_FLOATS                                         0x8B4B
#define GL_MAX_VARYING_COMPONENTS                                     0x8B4B
#define GL_MAX_VARYING_COMPONENTS_EXT                                 0x8B4B
#define GL_MAX_VARYING_FLOATS_ARB                                     0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS                             0x8B4C
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB                         0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS                           0x8B4D
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB                       0x8B4D
#define GL_OBJECT_TYPE_ARB                                            0x8B4E
#define GL_SHADER_TYPE                                                0x8B4F
#define GL_OBJECT_SUBTYPE_ARB                                         0x8B4F
#define GL_FLOAT_VEC2                                                 0x8B50
#define GL_FLOAT_VEC2_ARB                                             0x8B50
#define GL_FLOAT_VEC3                                                 0x8B51
#define GL_FLOAT_VEC3_ARB                                             0x8B51
#define GL_FLOAT_VEC4                                                 0x8B52
#define GL_FLOAT_VEC4_ARB                                             0x8B52
#define GL_INT_VEC2                                                   0x8B53
#define GL_INT_VEC2_ARB                                               0x8B53
#define GL_INT_VEC3                                                   0x8B54
#define GL_INT_VEC3_ARB                                               0x8B54
#define GL_INT_VEC4                                                   0x8B55
#define GL_INT_VEC4_ARB                                               0x8B55
#define GL_BOOL                                                       0x8B56
#define GL_BOOL_ARB                                                   0x8B56
#define GL_BOOL_VEC2                                                  0x8B57
#define GL_BOOL_VEC2_ARB                                              0x8B57
#define GL_BOOL_VEC3                                                  0x8B58
#define GL_BOOL_VEC3_ARB                                              0x8B58
#define GL_BOOL_VEC4                                                  0x8B59
#define GL_BOOL_VEC4_ARB                                              0x8B59
#define GL_FLOAT_MAT2                                                 0x8B5A
#define GL_FLOAT_MAT2_ARB                                             0x8B5A
#define GL_FLOAT_MAT3                                                 0x8B5B
#define GL_FLOAT_MAT3_ARB                                             0x8B5B
#define GL_FLOAT_MAT4                                                 0x8B5C
#define GL_FLOAT_MAT4_ARB                                             0x8B5C
#define GL_SAMPLER_1D                                                 0x8B5D
#define GL_SAMPLER_1D_ARB                                             0x8B5D
#define GL_SAMPLER_2D                                                 0x8B5E
#define GL_SAMPLER_2D_ARB                                             0x8B5E
#define GL_SAMPLER_3D                                                 0x8B5F
#define GL_SAMPLER_3D_ARB                                             0x8B5F
#define GL_SAMPLER_CUBE                                               0x8B60
#define GL_SAMPLER_CUBE_ARB                                           0x8B60
#define GL_SAMPLER_1D_SHADOW                                          0x8B61
#define GL_SAMPLER_1D_SHADOW_ARB                                      0x8B61
#define GL_SAMPLER_2D_SHADOW                                          0x8B62
#define GL_SAMPLER_2D_SHADOW_ARB                                      0x8B62
#define GL_SAMPLER_2D_RECT                                            0x8B63
#define GL_SAMPLER_2D_RECT_ARB                                        0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW                                     0x8B64
#define GL_SAMPLER_2D_RECT_SHADOW_ARB                                 0x8B64
#define GL_FLOAT_MAT2x3                                               0x8B65
#define GL_FLOAT_MAT2x4                                               0x8B66
#define GL_FLOAT_MAT3x2                                               0x8B67
#define GL_FLOAT_MAT3x4                                               0x8B68
#define GL_FLOAT_MAT4x2                                               0x8B69
#define GL_FLOAT_MAT4x3                                               0x8B6A
#define GL_DELETE_STATUS                                              0x8B80
#define GL_OBJECT_DELETE_STATUS_ARB                                   0x8B80
#define GL_COMPILE_STATUS                                             0x8B81
#define GL_OBJECT_COMPILE_STATUS_ARB                                  0x8B81
#define GL_LINK_STATUS                                                0x8B82
#define GL_OBJECT_LINK_STATUS_ARB                                     0x8B82
#define GL_VALIDATE_STATUS                                            0x8B83
#define GL_OBJECT_VALIDATE_STATUS_ARB                                 0x8B83
#define GL_INFO_LOG_LENGTH                                            0x8B84
#define GL_OBJECT_INFO_LOG_LENGTH_ARB                                 0x8B84
#define GL_ATTACHED_SHADERS                                           0x8B85
#define GL_OBJECT_ATTACHED_OBJECTS_ARB                                0x8B85
#define GL_ACTIVE_UNIFORMS                                            0x8B86
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB                                 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH                                  0x8B87
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB                       0x8B87
#define GL_SHADER_SOURCE_LENGTH                                       0x8B88
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB                            0x8B88
#define GL_ACTIVE_ATTRIBUTES                                          0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB                               0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH                                0x8B8A
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB                     0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT                            0x8B8B
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB                        0x8B8B
#define GL_SHADING_LANGUAGE_VERSION                                   0x8B8C
#define GL_SHADING_LANGUAGE_VERSION_ARB                               0x8B8C
#define GL_CURRENT_PROGRAM                                            0x8B8D
#define GL_IMPLEMENTATION_COLOR_READ_TYPE                             0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT                           0x8B9B
#define GL_COUNTER_TYPE_AMD                                           0x8BC0
#define GL_COUNTER_RANGE_AMD                                          0x8BC1
#define GL_UNSIGNED_INT64_AMD                                         0x8BC2
#define GL_PERCENTAGE_AMD                                             0x8BC3
#define GL_PERFMON_RESULT_AVAILABLE_AMD                               0x8BC4
#define GL_PERFMON_RESULT_SIZE_AMD                                    0x8BC5
#define GL_PERFMON_RESULT_AMD                                         0x8BC6
#define GL_TEXTURE_RED_TYPE                                           0x8C10
#define GL_TEXTURE_RED_TYPE_ARB                                       0x8C10
#define GL_TEXTURE_GREEN_TYPE                                         0x8C11
#define GL_TEXTURE_GREEN_TYPE_ARB                                     0x8C11
#define GL_TEXTURE_BLUE_TYPE                                          0x8C12
#define GL_TEXTURE_BLUE_TYPE_ARB                                      0x8C12
#define GL_TEXTURE_ALPHA_TYPE                                         0x8C13
#define GL_TEXTURE_ALPHA_TYPE_ARB                                     0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE                                     0x8C14
#define GL_TEXTURE_LUMINANCE_TYPE_ARB                                 0x8C14
#define GL_TEXTURE_INTENSITY_TYPE                                     0x8C15
#define GL_TEXTURE_INTENSITY_TYPE_ARB                                 0x8C15
#define GL_TEXTURE_DEPTH_TYPE                                         0x8C16
#define GL_TEXTURE_DEPTH_TYPE_ARB                                     0x8C16
#define GL_UNSIGNED_NORMALIZED                                        0x8C17
#define GL_UNSIGNED_NORMALIZED_ARB                                    0x8C17
#define GL_TEXTURE_1D_ARRAY                                           0x8C18
#define GL_TEXTURE_1D_ARRAY_EXT                                       0x8C18
#define GL_PROXY_TEXTURE_1D_ARRAY                                     0x8C19
#define GL_PROXY_TEXTURE_1D_ARRAY_EXT                                 0x8C19
#define GL_TEXTURE_2D_ARRAY                                           0x8C1A
#define GL_TEXTURE_2D_ARRAY_EXT                                       0x8C1A
#define GL_PROXY_TEXTURE_2D_ARRAY                                     0x8C1B
#define GL_PROXY_TEXTURE_2D_ARRAY_EXT                                 0x8C1B
#define GL_TEXTURE_BINDING_1D_ARRAY                                   0x8C1C
#define GL_TEXTURE_BINDING_1D_ARRAY_EXT                               0x8C1C
#define GL_TEXTURE_BINDING_2D_ARRAY                                   0x8C1D
#define GL_TEXTURE_BINDING_2D_ARRAY_EXT                               0x8C1D
#define GL_GEOMETRY_PROGRAM_NV                                        0x8C26
#define GL_MAX_PROGRAM_OUTPUT_VERTICES_NV                             0x8C27
#define GL_MAX_PROGRAM_TOTAL_OUTPUT_COMPONENTS_NV                     0x8C28
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS                           0x8C29
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB                       0x8C29
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT                       0x8C29
#define GL_TEXTURE_BUFFER                                             0x8C2A
#define GL_TEXTURE_BUFFER_ARB                                         0x8C2A
#define GL_TEXTURE_BUFFER_EXT                                         0x8C2A
#define GL_TEXTURE_BUFFER_BINDING                                     0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE                                    0x8C2B
#define GL_MAX_TEXTURE_BUFFER_SIZE_ARB                                0x8C2B
#define GL_MAX_TEXTURE_BUFFER_SIZE_EXT                                0x8C2B
#define GL_TEXTURE_BINDING_BUFFER                                     0x8C2C
#define GL_TEXTURE_BINDING_BUFFER_ARB                                 0x8C2C
#define GL_TEXTURE_BINDING_BUFFER_EXT                                 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING                          0x8C2D
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB                      0x8C2D
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT                      0x8C2D
#define GL_TEXTURE_BUFFER_FORMAT_ARB                                  0x8C2E
#define GL_TEXTURE_BUFFER_FORMAT_EXT                                  0x8C2E
#define GL_ANY_SAMPLES_PASSED                                         0x8C2F
#define GL_SAMPLE_SHADING                                             0x8C36
#define GL_SAMPLE_SHADING_ARB                                         0x8C36
#define GL_MIN_SAMPLE_SHADING_VALUE                                   0x8C37
#define GL_MIN_SAMPLE_SHADING_VALUE_ARB                               0x8C37
#define GL_R11F_G11F_B10F                                             0x8C3A
#define GL_R11F_G11F_B10F_EXT                                         0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV                               0x8C3B
#define GL_UNSIGNED_INT_10F_11F_11F_REV_EXT                           0x8C3B
#define GL_RGBA_SIGNED_COMPONENTS_EXT                                 0x8C3C
#define GL_RGB9_E5                                                    0x8C3D
#define GL_RGB9_E5_EXT                                                0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV                                   0x8C3E
#define GL_UNSIGNED_INT_5_9_9_9_REV_EXT                               0x8C3E
#define GL_TEXTURE_SHARED_SIZE                                        0x8C3F
#define GL_TEXTURE_SHARED_SIZE_EXT                                    0x8C3F
#define GL_SRGB                                                       0x8C40
#define GL_SRGB_EXT                                                   0x8C40
#define GL_SRGB8                                                      0x8C41
#define GL_SRGB8_EXT                                                  0x8C41
#define GL_SRGB_ALPHA                                                 0x8C42
#define GL_SRGB_ALPHA_EXT                                             0x8C42
#define GL_SRGB8_ALPHA8                                               0x8C43
#define GL_SRGB8_ALPHA8_EXT                                           0x8C43
#define GL_SLUMINANCE_ALPHA                                           0x8C44
#define GL_SLUMINANCE_ALPHA_EXT                                       0x8C44
#define GL_SLUMINANCE8_ALPHA8                                         0x8C45
#define GL_SLUMINANCE8_ALPHA8_EXT                                     0x8C45
#define GL_SLUMINANCE                                                 0x8C46
#define GL_SLUMINANCE_EXT                                             0x8C46
#define GL_SLUMINANCE8                                                0x8C47
#define GL_SLUMINANCE8_EXT                                            0x8C47
#define GL_COMPRESSED_SRGB                                            0x8C48
#define GL_COMPRESSED_SRGB_EXT                                        0x8C48
#define GL_COMPRESSED_SRGB_ALPHA                                      0x8C49
#define GL_COMPRESSED_SRGB_ALPHA_EXT                                  0x8C49
#define GL_COMPRESSED_SLUMINANCE                                      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_EXT                                  0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA                                0x8C4B
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT                            0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                              0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT                        0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT                        0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT                        0x8C4F
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT                             0x8C70
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT                      0x8C71
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT                       0x8C72
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT                0x8C73
#define GL_TESS_CONTROL_PROGRAM_PARAMETER_BUFFER_NV                   0x8C74
#define GL_TESS_EVALUATION_PROGRAM_PARAMETER_BUFFER_NV                0x8C75
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH                      0x8C76
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH_EXT                  0x8C76
#define GL_BACK_PRIMARY_COLOR_NV                                      0x8C77
#define GL_BACK_SECONDARY_COLOR_NV                                    0x8C78
#define GL_TEXTURE_COORD_NV                                           0x8C79
#define GL_CLIP_DISTANCE_NV                                           0x8C7A
#define GL_VERTEX_ID_NV                                               0x8C7B
#define GL_PRIMITIVE_ID_NV                                            0x8C7C
#define GL_GENERIC_ATTRIB_NV                                          0x8C7D
#define GL_TRANSFORM_FEEDBACK_ATTRIBS_NV                              0x8C7E
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE                             0x8C7F
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE_EXT                         0x8C7F
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE_NV                          0x8C7F
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS                 0x8C80
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT             0x8C80
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_NV              0x8C80
#define GL_ACTIVE_VARYINGS_NV                                         0x8C81
#define GL_ACTIVE_VARYING_MAX_LENGTH_NV                               0x8C82
#define GL_TRANSFORM_FEEDBACK_VARYINGS                                0x8C83
#define GL_TRANSFORM_FEEDBACK_VARYINGS_EXT                            0x8C83
#define GL_TRANSFORM_FEEDBACK_VARYINGS_NV                             0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_START                            0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_START_EXT                        0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_START_NV                         0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE                             0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_EXT                         0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_NV                          0x8C85
#define GL_TRANSFORM_FEEDBACK_RECORD_NV                               0x8C86
#define GL_PRIMITIVES_GENERATED                                       0x8C87
#define GL_PRIMITIVES_GENERATED_EXT                                   0x8C87
#define GL_PRIMITIVES_GENERATED_NV                                    0x8C87
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN                      0x8C88
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT                  0x8C88
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_NV                   0x8C88
#define GL_RASTERIZER_DISCARD                                         0x8C89
#define GL_RASTERIZER_DISCARD_EXT                                     0x8C89
#define GL_RASTERIZER_DISCARD_NV                                      0x8C89
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS              0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT          0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_NV           0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS                    0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT                0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_NV                 0x8C8B
#define GL_INTERLEAVED_ATTRIBS                                        0x8C8C
#define GL_INTERLEAVED_ATTRIBS_EXT                                    0x8C8C
#define GL_INTERLEAVED_ATTRIBS_NV                                     0x8C8C
#define GL_SEPARATE_ATTRIBS                                           0x8C8D
#define GL_SEPARATE_ATTRIBS_EXT                                       0x8C8D
#define GL_SEPARATE_ATTRIBS_NV                                        0x8C8D
#define GL_TRANSFORM_FEEDBACK_BUFFER                                  0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_EXT                              0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_NV                               0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING                          0x8C8F
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_EXT                      0x8C8F
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_NV                       0x8C8F
#define GL_POINT_SPRITE_COORD_ORIGIN                                  0x8CA0
#define GL_LOWER_LEFT                                                 0x8CA1
#define GL_UPPER_LEFT                                                 0x8CA2
#define GL_STENCIL_BACK_REF                                           0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK                                    0x8CA4
#define GL_STENCIL_BACK_WRITEMASK                                     0x8CA5
#define GL_DRAW_FRAMEBUFFER_BINDING                                   0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT                               0x8CA6
#define GL_FRAMEBUFFER_BINDING                                        0x8CA6
#define GL_FRAMEBUFFER_BINDING_EXT                                    0x8CA6
#define GL_RENDERBUFFER_BINDING                                       0x8CA7
#define GL_RENDERBUFFER_BINDING_EXT                                   0x8CA7
#define GL_READ_FRAMEBUFFER                                           0x8CA8
#define GL_READ_FRAMEBUFFER_EXT                                       0x8CA8
#define GL_DRAW_FRAMEBUFFER                                           0x8CA9
#define GL_DRAW_FRAMEBUFFER_EXT                                       0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING                                   0x8CAA
#define GL_READ_FRAMEBUFFER_BINDING_EXT                               0x8CAA
#define GL_RENDERBUFFER_COVERAGE_SAMPLES_NV                           0x8CAB
#define GL_RENDERBUFFER_SAMPLES                                       0x8CAB
#define GL_RENDERBUFFER_SAMPLES_EXT                                   0x8CAB
#define GL_DEPTH_COMPONENT32F                                         0x8CAC
#define GL_DEPTH32F_STENCIL8                                          0x8CAD
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE                         0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT                     0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME                         0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT                     0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL                       0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT                   0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE               0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT           0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT              0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER                       0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT                   0x8CD4
#define GL_FRAMEBUFFER_COMPLETE                                       0x8CD5
#define GL_FRAMEBUFFER_COMPLETE_EXT                                   0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT                          0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT                      0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT                  0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT              0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT                      0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                         0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER                         0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT                     0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER                         0x8CDC
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT                     0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED                                    0x8CDD
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                                0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS                                      0x8CDF
#define GL_MAX_COLOR_ATTACHMENTS_EXT                                  0x8CDF
#define GL_COLOR_ATTACHMENT0                                          0x8CE0
#define GL_COLOR_ATTACHMENT0_EXT                                      0x8CE0
#define GL_COLOR_ATTACHMENT1                                          0x8CE1
#define GL_COLOR_ATTACHMENT1_EXT                                      0x8CE1
#define GL_COLOR_ATTACHMENT2                                          0x8CE2
#define GL_COLOR_ATTACHMENT2_EXT                                      0x8CE2
#define GL_COLOR_ATTACHMENT3                                          0x8CE3
#define GL_COLOR_ATTACHMENT3_EXT                                      0x8CE3
#define GL_COLOR_ATTACHMENT4                                          0x8CE4
#define GL_COLOR_ATTACHMENT4_EXT                                      0x8CE4
#define GL_COLOR_ATTACHMENT5                                          0x8CE5
#define GL_COLOR_ATTACHMENT5_EXT                                      0x8CE5
#define GL_COLOR_ATTACHMENT6                                          0x8CE6
#define GL_COLOR_ATTACHMENT6_EXT                                      0x8CE6
#define GL_COLOR_ATTACHMENT7                                          0x8CE7
#define GL_COLOR_ATTACHMENT7_EXT                                      0x8CE7
#define GL_COLOR_ATTACHMENT8                                          0x8CE8
#define GL_COLOR_ATTACHMENT8_EXT                                      0x8CE8
#define GL_COLOR_ATTACHMENT9                                          0x8CE9
#define GL_COLOR_ATTACHMENT9_EXT                                      0x8CE9
#define GL_COLOR_ATTACHMENT10                                         0x8CEA
#define GL_COLOR_ATTACHMENT10_EXT                                     0x8CEA
#define GL_COLOR_ATTACHMENT11                                         0x8CEB
#define GL_COLOR_ATTACHMENT11_EXT                                     0x8CEB
#define GL_COLOR_ATTACHMENT12                                         0x8CEC
#define GL_COLOR_ATTACHMENT12_EXT                                     0x8CEC
#define GL_COLOR_ATTACHMENT13                                         0x8CED
#define GL_COLOR_ATTACHMENT13_EXT                                     0x8CED
#define GL_COLOR_ATTACHMENT14                                         0x8CEE
#define GL_COLOR_ATTACHMENT14_EXT                                     0x8CEE
#define GL_COLOR_ATTACHMENT15                                         0x8CEF
#define GL_COLOR_ATTACHMENT15_EXT                                     0x8CEF
#define GL_COLOR_ATTACHMENT16                                         0x8CF0
#define GL_COLOR_ATTACHMENT17                                         0x8CF1
#define GL_COLOR_ATTACHMENT18                                         0x8CF2
#define GL_COLOR_ATTACHMENT19                                         0x8CF3
#define GL_COLOR_ATTACHMENT20                                         0x8CF4
#define GL_COLOR_ATTACHMENT21                                         0x8CF5
#define GL_COLOR_ATTACHMENT22                                         0x8CF6
#define GL_COLOR_ATTACHMENT23                                         0x8CF7
#define GL_COLOR_ATTACHMENT24                                         0x8CF8
#define GL_COLOR_ATTACHMENT25                                         0x8CF9
#define GL_COLOR_ATTACHMENT26                                         0x8CFA
#define GL_COLOR_ATTACHMENT27                                         0x8CFB
#define GL_COLOR_ATTACHMENT28                                         0x8CFC
#define GL_COLOR_ATTACHMENT29                                         0x8CFD
#define GL_COLOR_ATTACHMENT30                                         0x8CFE
#define GL_COLOR_ATTACHMENT31                                         0x8CFF
#define GL_DEPTH_ATTACHMENT                                           0x8D00
#define GL_DEPTH_ATTACHMENT_EXT                                       0x8D00
#define GL_STENCIL_ATTACHMENT                                         0x8D20
#define GL_STENCIL_ATTACHMENT_EXT                                     0x8D20
#define GL_FRAMEBUFFER                                                0x8D40
#define GL_FRAMEBUFFER_EXT                                            0x8D40
#define GL_RENDERBUFFER                                               0x8D41
#define GL_RENDERBUFFER_EXT                                           0x8D41
#define GL_RENDERBUFFER_WIDTH                                         0x8D42
#define GL_RENDERBUFFER_WIDTH_EXT                                     0x8D42
#define GL_RENDERBUFFER_HEIGHT                                        0x8D43
#define GL_RENDERBUFFER_HEIGHT_EXT                                    0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT                               0x8D44
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT                           0x8D44
#define GL_STENCIL_INDEX1                                             0x8D46
#define GL_STENCIL_INDEX1_EXT                                         0x8D46
#define GL_STENCIL_INDEX4                                             0x8D47
#define GL_STENCIL_INDEX4_EXT                                         0x8D47
#define GL_STENCIL_INDEX8                                             0x8D48
#define GL_STENCIL_INDEX8_EXT                                         0x8D48
#define GL_STENCIL_INDEX16                                            0x8D49
#define GL_STENCIL_INDEX16_EXT                                        0x8D49
#define GL_RENDERBUFFER_RED_SIZE                                      0x8D50
#define GL_RENDERBUFFER_RED_SIZE_EXT                                  0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE                                    0x8D51
#define GL_RENDERBUFFER_GREEN_SIZE_EXT                                0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE                                     0x8D52
#define GL_RENDERBUFFER_BLUE_SIZE_EXT                                 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE                                    0x8D53
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT                                0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE                                    0x8D54
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT                                0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE                                  0x8D55
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT                              0x8D55
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE                         0x8D56
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT                     0x8D56
#define GL_MAX_SAMPLES                                                0x8D57
#define GL_MAX_SAMPLES_EXT                                            0x8D57
#define GL_RGB565                                                     0x8D62
#define GL_PRIMITIVE_RESTART_FIXED_INDEX                              0x8D69
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE                            0x8D6A
#define GL_MAX_ELEMENT_INDEX                                          0x8D6B
#define GL_RGBA32UI                                                   0x8D70
#define GL_RGBA32UI_EXT                                               0x8D70
#define GL_RGB32UI                                                    0x8D71
#define GL_RGB32UI_EXT                                                0x8D71
#define GL_ALPHA32UI_EXT                                              0x8D72
#define GL_INTENSITY32UI_EXT                                          0x8D73
#define GL_LUMINANCE32UI_EXT                                          0x8D74
#define GL_LUMINANCE_ALPHA32UI_EXT                                    0x8D75
#define GL_RGBA16UI                                                   0x8D76
#define GL_RGBA16UI_EXT                                               0x8D76
#define GL_RGB16UI                                                    0x8D77
#define GL_RGB16UI_EXT                                                0x8D77
#define GL_ALPHA16UI_EXT                                              0x8D78
#define GL_INTENSITY16UI_EXT                                          0x8D79
#define GL_LUMINANCE16UI_EXT                                          0x8D7A
#define GL_LUMINANCE_ALPHA16UI_EXT                                    0x8D7B
#define GL_RGBA8UI                                                    0x8D7C
#define GL_RGBA8UI_EXT                                                0x8D7C
#define GL_RGB8UI                                                     0x8D7D
#define GL_RGB8UI_EXT                                                 0x8D7D
#define GL_ALPHA8UI_EXT                                               0x8D7E
#define GL_INTENSITY8UI_EXT                                           0x8D7F
#define GL_LUMINANCE8UI_EXT                                           0x8D80
#define GL_LUMINANCE_ALPHA8UI_EXT                                     0x8D81
#define GL_RGBA32I                                                    0x8D82
#define GL_RGBA32I_EXT                                                0x8D82
#define GL_RGB32I                                                     0x8D83
#define GL_RGB32I_EXT                                                 0x8D83
#define GL_ALPHA32I_EXT                                               0x8D84
#define GL_INTENSITY32I_EXT                                           0x8D85
#define GL_LUMINANCE32I_EXT                                           0x8D86
#define GL_LUMINANCE_ALPHA32I_EXT                                     0x8D87
#define GL_RGBA16I                                                    0x8D88
#define GL_RGBA16I_EXT                                                0x8D88
#define GL_RGB16I                                                     0x8D89
#define GL_RGB16I_EXT                                                 0x8D89
#define GL_ALPHA16I_EXT                                               0x8D8A
#define GL_INTENSITY16I_EXT                                           0x8D8B
#define GL_LUMINANCE16I_EXT                                           0x8D8C
#define GL_LUMINANCE_ALPHA16I_EXT                                     0x8D8D
#define GL_RGBA8I                                                     0x8D8E
#define GL_RGBA8I_EXT                                                 0x8D8E
#define GL_RGB8I                                                      0x8D8F
#define GL_RGB8I_EXT                                                  0x8D8F
#define GL_ALPHA8I_EXT                                                0x8D90
#define GL_INTENSITY8I_EXT                                            0x8D91
#define GL_LUMINANCE8I_EXT                                            0x8D92
#define GL_LUMINANCE_ALPHA8I_EXT                                      0x8D93
#define GL_RED_INTEGER                                                0x8D94
#define GL_RED_INTEGER_EXT                                            0x8D94
#define GL_GREEN_INTEGER                                              0x8D95
#define GL_GREEN_INTEGER_EXT                                          0x8D95
#define GL_BLUE_INTEGER                                               0x8D96
#define GL_BLUE_INTEGER_EXT                                           0x8D96
#define GL_ALPHA_INTEGER                                              0x8D97
#define GL_ALPHA_INTEGER_EXT                                          0x8D97
#define GL_RGB_INTEGER                                                0x8D98
#define GL_RGB_INTEGER_EXT                                            0x8D98
#define GL_RGBA_INTEGER                                               0x8D99
#define GL_RGBA_INTEGER_EXT                                           0x8D99
#define GL_BGR_INTEGER                                                0x8D9A
#define GL_BGR_INTEGER_EXT                                            0x8D9A
#define GL_BGRA_INTEGER                                               0x8D9B
#define GL_BGRA_INTEGER_EXT                                           0x8D9B
#define GL_LUMINANCE_INTEGER_EXT                                      0x8D9C
#define GL_LUMINANCE_ALPHA_INTEGER_EXT                                0x8D9D
#define GL_RGBA_INTEGER_MODE_EXT                                      0x8D9E
#define GL_INT_2_10_10_10_REV                                         0x8D9F
#define GL_MAX_PROGRAM_PARAMETER_BUFFER_BINDINGS_NV                   0x8DA0
#define GL_MAX_PROGRAM_PARAMETER_BUFFER_SIZE_NV                       0x8DA1
#define GL_VERTEX_PROGRAM_PARAMETER_BUFFER_NV                         0x8DA2
#define GL_GEOMETRY_PROGRAM_PARAMETER_BUFFER_NV                       0x8DA3
#define GL_FRAGMENT_PROGRAM_PARAMETER_BUFFER_NV                       0x8DA4
#define GL_MAX_PROGRAM_GENERIC_ATTRIBS_NV                             0x8DA5
#define GL_MAX_PROGRAM_GENERIC_RESULTS_NV                             0x8DA6
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED                             0x8DA7
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_ARB                         0x8DA7
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT                         0x8DA7
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS                       0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_ARB                   0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT                   0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_ARB                     0x8DA9
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT                     0x8DA9
#define GL_LAYER_NV                                                   0x8DAA
#define GL_DEPTH_COMPONENT32F_NV                                      0x8DAB
#define GL_DEPTH32F_STENCIL8_NV                                       0x8DAC
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV                             0x8DAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV_NV                          0x8DAD
#define GL_SHADER_INCLUDE_ARB                                         0x8DAE
#define GL_DEPTH_BUFFER_FLOAT_MODE_NV                                 0x8DAF
#define GL_FRAMEBUFFER_SRGB                                           0x8DB9
#define GL_FRAMEBUFFER_SRGB_EXT                                       0x8DB9
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT                               0x8DBA
#define GL_COMPRESSED_RED_RGTC1                                       0x8DBB
#define GL_COMPRESSED_RED_RGTC1_EXT                                   0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1                                0x8DBC
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT                            0x8DBC
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT                             0x8DBD
#define GL_COMPRESSED_RG_RGTC2                                        0x8DBD
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT                      0x8DBE
#define GL_COMPRESSED_SIGNED_RG_RGTC2                                 0x8DBE
#define GL_SAMPLER_1D_ARRAY                                           0x8DC0
#define GL_SAMPLER_1D_ARRAY_EXT                                       0x8DC0
#define GL_SAMPLER_2D_ARRAY                                           0x8DC1
#define GL_SAMPLER_2D_ARRAY_EXT                                       0x8DC1
#define GL_SAMPLER_BUFFER                                             0x8DC2
#define GL_SAMPLER_BUFFER_EXT                                         0x8DC2
#define GL_SAMPLER_1D_ARRAY_SHADOW                                    0x8DC3
#define GL_SAMPLER_1D_ARRAY_SHADOW_EXT                                0x8DC3
#define GL_SAMPLER_2D_ARRAY_SHADOW                                    0x8DC4
#define GL_SAMPLER_2D_ARRAY_SHADOW_EXT                                0x8DC4
#define GL_SAMPLER_CUBE_SHADOW                                        0x8DC5
#define GL_SAMPLER_CUBE_SHADOW_EXT                                    0x8DC5
#define GL_UNSIGNED_INT_VEC2                                          0x8DC6
#define GL_UNSIGNED_INT_VEC2_EXT                                      0x8DC6
#define GL_UNSIGNED_INT_VEC3                                          0x8DC7
#define GL_UNSIGNED_INT_VEC3_EXT                                      0x8DC7
#define GL_UNSIGNED_INT_VEC4                                          0x8DC8
#define GL_UNSIGNED_INT_VEC4_EXT                                      0x8DC8
#define GL_INT_SAMPLER_1D                                             0x8DC9
#define GL_INT_SAMPLER_1D_EXT                                         0x8DC9
#define GL_INT_SAMPLER_2D                                             0x8DCA
#define GL_INT_SAMPLER_2D_EXT                                         0x8DCA
#define GL_INT_SAMPLER_3D                                             0x8DCB
#define GL_INT_SAMPLER_3D_EXT                                         0x8DCB
#define GL_INT_SAMPLER_CUBE                                           0x8DCC
#define GL_INT_SAMPLER_CUBE_EXT                                       0x8DCC
#define GL_INT_SAMPLER_2D_RECT                                        0x8DCD
#define GL_INT_SAMPLER_2D_RECT_EXT                                    0x8DCD
#define GL_INT_SAMPLER_1D_ARRAY                                       0x8DCE
#define GL_INT_SAMPLER_1D_ARRAY_EXT                                   0x8DCE
#define GL_INT_SAMPLER_2D_ARRAY                                       0x8DCF
#define GL_INT_SAMPLER_2D_ARRAY_EXT                                   0x8DCF
#define GL_INT_SAMPLER_BUFFER                                         0x8DD0
#define GL_INT_SAMPLER_BUFFER_EXT                                     0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_1D                                    0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_1D_EXT                                0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_2D                                    0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_2D_EXT                                0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D                                    0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_3D_EXT                                0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE                                  0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_CUBE_EXT                              0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT                               0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT                           0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY                              0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT                          0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY                              0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT                          0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_BUFFER                                0x8DD8
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT                            0x8DD8
#define GL_GEOMETRY_SHADER                                            0x8DD9
#define GL_GEOMETRY_SHADER_ARB                                        0x8DD9
#define GL_GEOMETRY_SHADER_EXT                                        0x8DD9
#define GL_GEOMETRY_VERTICES_OUT_ARB                                  0x8DDA
#define GL_GEOMETRY_VERTICES_OUT_EXT                                  0x8DDA
#define GL_GEOMETRY_INPUT_TYPE_ARB                                    0x8DDB
#define GL_GEOMETRY_INPUT_TYPE_EXT                                    0x8DDB
#define GL_GEOMETRY_OUTPUT_TYPE_ARB                                   0x8DDC
#define GL_GEOMETRY_OUTPUT_TYPE_EXT                                   0x8DDC
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS_ARB                        0x8DDD
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT                        0x8DDD
#define GL_MAX_VERTEX_VARYING_COMPONENTS_ARB                          0x8DDE
#define GL_MAX_VERTEX_VARYING_COMPONENTS_EXT                          0x8DDE
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS                            0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB                        0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT                        0x8DDF
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES                               0x8DE0
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB                           0x8DE0
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT                           0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS                       0x8DE1
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB                   0x8DE1
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT                   0x8DE1
#define GL_MAX_VERTEX_BINDABLE_UNIFORMS_EXT                           0x8DE2
#define GL_MAX_FRAGMENT_BINDABLE_UNIFORMS_EXT                         0x8DE3
#define GL_MAX_GEOMETRY_BINDABLE_UNIFORMS_EXT                         0x8DE4
#define GL_ACTIVE_SUBROUTINES                                         0x8DE5
#define GL_ACTIVE_SUBROUTINE_UNIFORMS                                 0x8DE6
#define GL_MAX_SUBROUTINES                                            0x8DE7
#define GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS                           0x8DE8
#define GL_NAMED_STRING_LENGTH_ARB                                    0x8DE9
#define GL_NAMED_STRING_TYPE_ARB                                      0x8DEA
#define GL_MAX_BINDABLE_UNIFORM_SIZE_EXT                              0x8DED
#define GL_UNIFORM_BUFFER_EXT                                         0x8DEE
#define GL_UNIFORM_BUFFER_BINDING_EXT                                 0x8DEF
#define GL_LOW_FLOAT                                                  0x8DF0
#define GL_MEDIUM_FLOAT                                               0x8DF1
#define GL_HIGH_FLOAT                                                 0x8DF2
#define GL_LOW_INT                                                    0x8DF3
#define GL_MEDIUM_INT                                                 0x8DF4
#define GL_HIGH_INT                                                   0x8DF5
#define GL_SHADER_BINARY_FORMATS                                      0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS                                  0x8DF9
#define GL_SHADER_COMPILER                                            0x8DFA
#define GL_MAX_VERTEX_UNIFORM_VECTORS                                 0x8DFB
#define GL_MAX_VARYING_VECTORS                                        0x8DFC
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS                               0x8DFD
#define GL_RENDERBUFFER_COLOR_SAMPLES_NV                              0x8E10
#define GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV                          0x8E11
#define GL_MULTISAMPLE_COVERAGE_MODES_NV                              0x8E12
#define GL_QUERY_WAIT                                                 0x8E13
#define GL_QUERY_WAIT_NV                                              0x8E13
#define GL_QUERY_NO_WAIT                                              0x8E14
#define GL_QUERY_NO_WAIT_NV                                           0x8E14
#define GL_QUERY_BY_REGION_WAIT                                       0x8E15
#define GL_QUERY_BY_REGION_WAIT_NV                                    0x8E15
#define GL_QUERY_BY_REGION_NO_WAIT                                    0x8E16
#define GL_QUERY_BY_REGION_NO_WAIT_NV                                 0x8E16
#define GL_QUERY_WAIT_INVERTED                                        0x8E17
#define GL_QUERY_NO_WAIT_INVERTED                                     0x8E18
#define GL_QUERY_BY_REGION_WAIT_INVERTED                              0x8E19
#define GL_QUERY_BY_REGION_NO_WAIT_INVERTED                           0x8E1A
#define GL_POLYGON_OFFSET_CLAMP                                       0x8E1B
#define GL_POLYGON_OFFSET_CLAMP_EXT                                   0x8E1B
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS               0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS            0x8E1F
#define GL_COLOR_SAMPLES_NV                                           0x8E20
#define GL_TRANSFORM_FEEDBACK                                         0x8E22
#define GL_TRANSFORM_FEEDBACK_NV                                      0x8E22
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED                           0x8E23
#define GL_TRANSFORM_FEEDBACK_PAUSED                                  0x8E23
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED_NV                        0x8E23
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE                           0x8E24
#define GL_TRANSFORM_FEEDBACK_ACTIVE                                  0x8E24
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE_NV                        0x8E24
#define GL_TRANSFORM_FEEDBACK_BINDING                                 0x8E25
#define GL_TRANSFORM_FEEDBACK_BINDING_NV                              0x8E25
#define GL_FRAME_NV                                                   0x8E26
#define GL_FIELDS_NV                                                  0x8E27
#define GL_CURRENT_TIME_NV                                            0x8E28
#define GL_TIMESTAMP                                                  0x8E28
#define GL_NUM_FILL_STREAMS_NV                                        0x8E29
#define GL_PRESENT_TIME_NV                                            0x8E2A
#define GL_PRESENT_DURATION_NV                                        0x8E2B
#define GL_PROGRAM_MATRIX_EXT                                         0x8E2D
#define GL_TRANSPOSE_PROGRAM_MATRIX_EXT                               0x8E2E
#define GL_PROGRAM_MATRIX_STACK_DEPTH_EXT                             0x8E2F
#define GL_TEXTURE_SWIZZLE_R                                          0x8E42
#define GL_TEXTURE_SWIZZLE_R_EXT                                      0x8E42
#define GL_TEXTURE_SWIZZLE_G                                          0x8E43
#define GL_TEXTURE_SWIZZLE_G_EXT                                      0x8E43
#define GL_TEXTURE_SWIZZLE_B                                          0x8E44
#define GL_TEXTURE_SWIZZLE_B_EXT                                      0x8E44
#define GL_TEXTURE_SWIZZLE_A                                          0x8E45
#define GL_TEXTURE_SWIZZLE_A_EXT                                      0x8E45
#define GL_TEXTURE_SWIZZLE_RGBA                                       0x8E46
#define GL_TEXTURE_SWIZZLE_RGBA_EXT                                   0x8E46
#define GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS                        0x8E47
#define GL_ACTIVE_SUBROUTINE_MAX_LENGTH                               0x8E48
#define GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH                       0x8E49
#define GL_NUM_COMPATIBLE_SUBROUTINES                                 0x8E4A
#define GL_COMPATIBLE_SUBROUTINES                                     0x8E4B
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION                   0x8E4C
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT               0x8E4C
#define GL_FIRST_VERTEX_CONVENTION                                    0x8E4D
#define GL_FIRST_VERTEX_CONVENTION_EXT                                0x8E4D
#define GL_LAST_VERTEX_CONVENTION                                     0x8E4E
#define GL_LAST_VERTEX_CONVENTION_EXT                                 0x8E4E
#define GL_PROVOKING_VERTEX                                           0x8E4F
#define GL_PROVOKING_VERTEX_EXT                                       0x8E4F
#define GL_SAMPLE_POSITION                                            0x8E50
#define GL_SAMPLE_POSITION_NV                                         0x8E50
#define GL_SAMPLE_LOCATION_ARB                                        0x8E50
#define GL_SAMPLE_LOCATION_NV                                         0x8E50
#define GL_SAMPLE_MASK                                                0x8E51
#define GL_SAMPLE_MASK_NV                                             0x8E51
#define GL_SAMPLE_MASK_VALUE                                          0x8E52
#define GL_SAMPLE_MASK_VALUE_NV                                       0x8E52
#define GL_TEXTURE_BINDING_RENDERBUFFER_NV                            0x8E53
#define GL_TEXTURE_RENDERBUFFER_DATA_STORE_BINDING_NV                 0x8E54
#define GL_TEXTURE_RENDERBUFFER_NV                                    0x8E55
#define GL_SAMPLER_RENDERBUFFER_NV                                    0x8E56
#define GL_INT_SAMPLER_RENDERBUFFER_NV                                0x8E57
#define GL_UNSIGNED_INT_SAMPLER_RENDERBUFFER_NV                       0x8E58
#define GL_MAX_SAMPLE_MASK_WORDS                                      0x8E59
#define GL_MAX_SAMPLE_MASK_WORDS_NV                                   0x8E59
#define GL_MAX_GEOMETRY_PROGRAM_INVOCATIONS_NV                        0x8E5A
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS                            0x8E5A
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET                          0x8E5B
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET_NV                       0x8E5B
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET                          0x8E5C
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET_NV                       0x8E5C
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS                         0x8E5D
#define GL_FRAGMENT_PROGRAM_INTERPOLATION_OFFSET_BITS_NV              0x8E5D
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET                          0x8E5E
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_ARB                      0x8E5E
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_NV                       0x8E5E
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET                          0x8E5F
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_ARB                      0x8E5F
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_NV                       0x8E5F
#define GL_MAX_MESH_UNIFORM_BLOCKS_NV                                 0x8E60
#define GL_MAX_MESH_TEXTURE_IMAGE_UNITS_NV                            0x8E61
#define GL_MAX_MESH_IMAGE_UNIFORMS_NV                                 0x8E62
#define GL_MAX_MESH_UNIFORM_COMPONENTS_NV                             0x8E63
#define GL_MAX_MESH_ATOMIC_COUNTER_BUFFERS_NV                         0x8E64
#define GL_MAX_MESH_ATOMIC_COUNTERS_NV                                0x8E65
#define GL_MAX_MESH_SHADER_STORAGE_BLOCKS_NV                          0x8E66
#define GL_MAX_COMBINED_MESH_UNIFORM_COMPONENTS_NV                    0x8E67
#define GL_MAX_TASK_UNIFORM_BLOCKS_NV                                 0x8E68
#define GL_MAX_TASK_TEXTURE_IMAGE_UNITS_NV                            0x8E69
#define GL_MAX_TASK_IMAGE_UNIFORMS_NV                                 0x8E6A
#define GL_MAX_TASK_UNIFORM_COMPONENTS_NV                             0x8E6B
#define GL_MAX_TASK_ATOMIC_COUNTER_BUFFERS_NV                         0x8E6C
#define GL_MAX_TASK_ATOMIC_COUNTERS_NV                                0x8E6D
#define GL_MAX_TASK_SHADER_STORAGE_BLOCKS_NV                          0x8E6E
#define GL_MAX_COMBINED_TASK_UNIFORM_COMPONENTS_NV                    0x8E6F
#define GL_MAX_TRANSFORM_FEEDBACK_BUFFERS                             0x8E70
#define GL_MAX_VERTEX_STREAMS                                         0x8E71
#define GL_PATCH_VERTICES                                             0x8E72
#define GL_PATCH_DEFAULT_INNER_LEVEL                                  0x8E73
#define GL_PATCH_DEFAULT_OUTER_LEVEL                                  0x8E74
#define GL_TESS_CONTROL_OUTPUT_VERTICES                               0x8E75
#define GL_TESS_GEN_MODE                                              0x8E76
#define GL_TESS_GEN_SPACING                                           0x8E77
#define GL_TESS_GEN_VERTEX_ORDER                                      0x8E78
#define GL_TESS_GEN_POINT_MODE                                        0x8E79
#define GL_ISOLINES                                                   0x8E7A
#define GL_FRACTIONAL_ODD                                             0x8E7B
#define GL_FRACTIONAL_EVEN                                            0x8E7C
#define GL_MAX_PATCH_VERTICES                                         0x8E7D
#define GL_MAX_TESS_GEN_LEVEL                                         0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS                        0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS                     0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS                       0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS                    0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS                         0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS                                  0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS                   0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS                      0x8E86
#define GL_TESS_EVALUATION_SHADER                                     0x8E87
#define GL_TESS_CONTROL_SHADER                                        0x8E88
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS                            0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS                         0x8E8A
#define GL_COMPRESSED_RGBA_BPTC_UNORM                                 0x8E8C
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB                             0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM                           0x8E8D
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB                       0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT                           0x8E8E
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB                       0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT                         0x8E8F
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB                     0x8E8F
#define GL_INCLUSIVE_EXT                                              0x8F10
#define GL_EXCLUSIVE_EXT                                              0x8F11
#define GL_WINDOW_RECTANGLE_EXT                                       0x8F12
#define GL_WINDOW_RECTANGLE_MODE_EXT                                  0x8F13
#define GL_MAX_WINDOW_RECTANGLES_EXT                                  0x8F14
#define GL_NUM_WINDOW_RECTANGLES_EXT                                  0x8F15
#define GL_BUFFER_GPU_ADDRESS_NV                                      0x8F1D
#define GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV                             0x8F1E
#define GL_ELEMENT_ARRAY_UNIFIED_NV                                   0x8F1F
#define GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV                             0x8F20
#define GL_VERTEX_ARRAY_ADDRESS_NV                                    0x8F21
#define GL_NORMAL_ARRAY_ADDRESS_NV                                    0x8F22
#define GL_COLOR_ARRAY_ADDRESS_NV                                     0x8F23
#define GL_INDEX_ARRAY_ADDRESS_NV                                     0x8F24
#define GL_TEXTURE_COORD_ARRAY_ADDRESS_NV                             0x8F25
#define GL_EDGE_FLAG_ARRAY_ADDRESS_NV                                 0x8F26
#define GL_SECONDARY_COLOR_ARRAY_ADDRESS_NV                           0x8F27
#define GL_FOG_COORD_ARRAY_ADDRESS_NV                                 0x8F28
#define GL_ELEMENT_ARRAY_ADDRESS_NV                                   0x8F29
#define GL_VERTEX_ATTRIB_ARRAY_LENGTH_NV                              0x8F2A
#define GL_VERTEX_ARRAY_LENGTH_NV                                     0x8F2B
#define GL_NORMAL_ARRAY_LENGTH_NV                                     0x8F2C
#define GL_COLOR_ARRAY_LENGTH_NV                                      0x8F2D
#define GL_INDEX_ARRAY_LENGTH_NV                                      0x8F2E
#define GL_TEXTURE_COORD_ARRAY_LENGTH_NV                              0x8F2F
#define GL_EDGE_FLAG_ARRAY_LENGTH_NV                                  0x8F30
#define GL_SECONDARY_COLOR_ARRAY_LENGTH_NV                            0x8F31
#define GL_FOG_COORD_ARRAY_LENGTH_NV                                  0x8F32
#define GL_ELEMENT_ARRAY_LENGTH_NV                                    0x8F33
#define GL_GPU_ADDRESS_NV                                             0x8F34
#define GL_MAX_SHADER_BUFFER_ADDRESS_NV                               0x8F35
#define GL_COPY_READ_BUFFER                                           0x8F36
#define GL_COPY_READ_BUFFER_BINDING                                   0x8F36
#define GL_COPY_WRITE_BUFFER                                          0x8F37
#define GL_COPY_WRITE_BUFFER_BINDING                                  0x8F37
#define GL_MAX_IMAGE_UNITS                                            0x8F38
#define GL_MAX_IMAGE_UNITS_EXT                                        0x8F38
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS              0x8F39
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS_EXT          0x8F39
#define GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES                       0x8F39
#define GL_IMAGE_BINDING_NAME                                         0x8F3A
#define GL_IMAGE_BINDING_NAME_EXT                                     0x8F3A
#define GL_IMAGE_BINDING_LEVEL                                        0x8F3B
#define GL_IMAGE_BINDING_LEVEL_EXT                                    0x8F3B
#define GL_IMAGE_BINDING_LAYERED                                      0x8F3C
#define GL_IMAGE_BINDING_LAYERED_EXT                                  0x8F3C
#define GL_IMAGE_BINDING_LAYER                                        0x8F3D
#define GL_IMAGE_BINDING_LAYER_EXT                                    0x8F3D
#define GL_IMAGE_BINDING_ACCESS                                       0x8F3E
#define GL_IMAGE_BINDING_ACCESS_EXT                                   0x8F3E
#define GL_DRAW_INDIRECT_BUFFER                                       0x8F3F
#define GL_DRAW_INDIRECT_UNIFIED_NV                                   0x8F40
#define GL_DRAW_INDIRECT_ADDRESS_NV                                   0x8F41
#define GL_DRAW_INDIRECT_LENGTH_NV                                    0x8F42
#define GL_DRAW_INDIRECT_BUFFER_BINDING                               0x8F43
#define GL_MAX_PROGRAM_SUBROUTINE_PARAMETERS_NV                       0x8F44
#define GL_MAX_PROGRAM_SUBROUTINE_NUM_NV                              0x8F45
#define GL_DOUBLE_MAT2                                                0x8F46
#define GL_DOUBLE_MAT2_EXT                                            0x8F46
#define GL_DOUBLE_MAT3                                                0x8F47
#define GL_DOUBLE_MAT3_EXT                                            0x8F47
#define GL_DOUBLE_MAT4                                                0x8F48
#define GL_DOUBLE_MAT4_EXT                                            0x8F48
#define GL_DOUBLE_MAT2x3                                              0x8F49
#define GL_DOUBLE_MAT2x3_EXT                                          0x8F49
#define GL_DOUBLE_MAT2x4                                              0x8F4A
#define GL_DOUBLE_MAT2x4_EXT                                          0x8F4A
#define GL_DOUBLE_MAT3x2                                              0x8F4B
#define GL_DOUBLE_MAT3x2_EXT                                          0x8F4B
#define GL_DOUBLE_MAT3x4                                              0x8F4C
#define GL_DOUBLE_MAT3x4_EXT                                          0x8F4C
#define GL_DOUBLE_MAT4x2                                              0x8F4D
#define GL_DOUBLE_MAT4x2_EXT                                          0x8F4D
#define GL_DOUBLE_MAT4x3                                              0x8F4E
#define GL_DOUBLE_MAT4x3_EXT                                          0x8F4E
#define GL_VERTEX_BINDING_BUFFER                                      0x8F4F
#define GL_RED_SNORM                                                  0x8F90
#define GL_RG_SNORM                                                   0x8F91
#define GL_RGB_SNORM                                                  0x8F92
#define GL_RGBA_SNORM                                                 0x8F93
#define GL_R8_SNORM                                                   0x8F94
#define GL_RG8_SNORM                                                  0x8F95
#define GL_RGB8_SNORM                                                 0x8F96
#define GL_RGBA8_SNORM                                                0x8F97
#define GL_R16_SNORM                                                  0x8F98
#define GL_RG16_SNORM                                                 0x8F99
#define GL_RGB16_SNORM                                                0x8F9A
#define GL_RGBA16_SNORM                                               0x8F9B
#define GL_SIGNED_NORMALIZED                                          0x8F9C
#define GL_PRIMITIVE_RESTART                                          0x8F9D
#define GL_PRIMITIVE_RESTART_INDEX                                    0x8F9E
#define GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB                  0x8F9F
#define GL_SR8_EXT                                                    0x8FBD
#define GL_INT8_NV                                                    0x8FE0
#define GL_INT8_VEC2_NV                                               0x8FE1
#define GL_INT8_VEC3_NV                                               0x8FE2
#define GL_INT8_VEC4_NV                                               0x8FE3
#define GL_INT16_NV                                                   0x8FE4
#define GL_INT16_VEC2_NV                                              0x8FE5
#define GL_INT16_VEC3_NV                                              0x8FE6
#define GL_INT16_VEC4_NV                                              0x8FE7
#define GL_INT64_VEC2_ARB                                             0x8FE9
#define GL_INT64_VEC2_NV                                              0x8FE9
#define GL_INT64_VEC3_ARB                                             0x8FEA
#define GL_INT64_VEC3_NV                                              0x8FEA
#define GL_INT64_VEC4_ARB                                             0x8FEB
#define GL_INT64_VEC4_NV                                              0x8FEB
#define GL_UNSIGNED_INT8_NV                                           0x8FEC
#define GL_UNSIGNED_INT8_VEC2_NV                                      0x8FED
#define GL_UNSIGNED_INT8_VEC3_NV                                      0x8FEE
#define GL_UNSIGNED_INT8_VEC4_NV                                      0x8FEF
#define GL_UNSIGNED_INT16_NV                                          0x8FF0
#define GL_UNSIGNED_INT16_VEC2_NV                                     0x8FF1
#define GL_UNSIGNED_INT16_VEC3_NV                                     0x8FF2
#define GL_UNSIGNED_INT16_VEC4_NV                                     0x8FF3
#define GL_UNSIGNED_INT64_VEC2_ARB                                    0x8FF5
#define GL_UNSIGNED_INT64_VEC2_NV                                     0x8FF5
#define GL_UNSIGNED_INT64_VEC3_ARB                                    0x8FF6
#define GL_UNSIGNED_INT64_VEC3_NV                                     0x8FF6
#define GL_UNSIGNED_INT64_VEC4_ARB                                    0x8FF7
#define GL_UNSIGNED_INT64_VEC4_NV                                     0x8FF7
#define GL_FLOAT16_NV                                                 0x8FF8
#define GL_FLOAT16_VEC2_NV                                            0x8FF9
#define GL_FLOAT16_VEC3_NV                                            0x8FFA
#define GL_FLOAT16_VEC4_NV                                            0x8FFB
#define GL_DOUBLE_VEC2                                                0x8FFC
#define GL_DOUBLE_VEC2_EXT                                            0x8FFC
#define GL_DOUBLE_VEC3                                                0x8FFD
#define GL_DOUBLE_VEC3_EXT                                            0x8FFD
#define GL_DOUBLE_VEC4                                                0x8FFE
#define GL_DOUBLE_VEC4_EXT                                            0x8FFE
#define GL_SAMPLER_BUFFER_AMD                                         0x9001
#define GL_INT_SAMPLER_BUFFER_AMD                                     0x9002
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_AMD                            0x9003
#define GL_TESSELLATION_MODE_AMD                                      0x9004
#define GL_TESSELLATION_FACTOR_AMD                                    0x9005
#define GL_DISCRETE_AMD                                               0x9006
#define GL_CONTINUOUS_AMD                                             0x9007
#define GL_TEXTURE_CUBE_MAP_ARRAY                                     0x9009
#define GL_TEXTURE_CUBE_MAP_ARRAY_ARB                                 0x9009
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY                             0x900A
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_ARB                         0x900A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY                               0x900B
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY_ARB                           0x900B
#define GL_SAMPLER_CUBE_MAP_ARRAY                                     0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_ARB                                 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW                              0x900D
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_ARB                          0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY                                 0x900E
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_ARB                             0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY                        0x900F
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_ARB                    0x900F
#define GL_ALPHA_SNORM                                                0x9010
#define GL_LUMINANCE_SNORM                                            0x9011
#define GL_LUMINANCE_ALPHA_SNORM                                      0x9012
#define GL_INTENSITY_SNORM                                            0x9013
#define GL_ALPHA8_SNORM                                               0x9014
#define GL_LUMINANCE8_SNORM                                           0x9015
#define GL_LUMINANCE8_ALPHA8_SNORM                                    0x9016
#define GL_INTENSITY8_SNORM                                           0x9017
#define GL_ALPHA16_SNORM                                              0x9018
#define GL_LUMINANCE16_SNORM                                          0x9019
#define GL_LUMINANCE16_ALPHA16_SNORM                                  0x901A
#define GL_INTENSITY16_SNORM                                          0x901B
#define GL_FACTOR_MIN_AMD                                             0x901C
#define GL_FACTOR_MAX_AMD                                             0x901D
#define GL_DEPTH_CLAMP_NEAR_AMD                                       0x901E
#define GL_DEPTH_CLAMP_FAR_AMD                                        0x901F
#define GL_VIDEO_BUFFER_NV                                            0x9020
#define GL_VIDEO_BUFFER_BINDING_NV                                    0x9021
#define GL_FIELD_UPPER_NV                                             0x9022
#define GL_FIELD_LOWER_NV                                             0x9023
#define GL_NUM_VIDEO_CAPTURE_STREAMS_NV                               0x9024
#define GL_NEXT_VIDEO_CAPTURE_BUFFER_STATUS_NV                        0x9025
#define GL_VIDEO_CAPTURE_TO_422_SUPPORTED_NV                          0x9026
#define GL_LAST_VIDEO_CAPTURE_STATUS_NV                               0x9027
#define GL_VIDEO_BUFFER_PITCH_NV                                      0x9028
#define GL_VIDEO_COLOR_CONVERSION_MATRIX_NV                           0x9029
#define GL_VIDEO_COLOR_CONVERSION_MAX_NV                              0x902A
#define GL_VIDEO_COLOR_CONVERSION_MIN_NV                              0x902B
#define GL_VIDEO_COLOR_CONVERSION_OFFSET_NV                           0x902C
#define GL_VIDEO_BUFFER_INTERNAL_FORMAT_NV                            0x902D
#define GL_PARTIAL_SUCCESS_NV                                         0x902E
#define GL_SUCCESS_NV                                                 0x902F
#define GL_FAILURE_NV                                                 0x9030
#define GL_YCBYCR8_422_NV                                             0x9031
#define GL_YCBAYCR8A_4224_NV                                          0x9032
#define GL_Z6Y10Z6CB10Z6Y10Z6CR10_422_NV                              0x9033
#define GL_Z6Y10Z6CB10Z6A10Z6Y10Z6CR10Z6A10_4224_NV                   0x9034
#define GL_Z4Y12Z4CB12Z4Y12Z4CR12_422_NV                              0x9035
#define GL_Z4Y12Z4CB12Z4A12Z4Y12Z4CR12Z4A12_4224_NV                   0x9036
#define GL_Z4Y12Z4CB12Z4CR12_444_NV                                   0x9037
#define GL_VIDEO_CAPTURE_FRAME_WIDTH_NV                               0x9038
#define GL_VIDEO_CAPTURE_FRAME_HEIGHT_NV                              0x9039
#define GL_VIDEO_CAPTURE_FIELD_UPPER_HEIGHT_NV                        0x903A
#define GL_VIDEO_CAPTURE_FIELD_LOWER_HEIGHT_NV                        0x903B
#define GL_VIDEO_CAPTURE_SURFACE_ORIGIN_NV                            0x903C
#define GL_TEXTURE_COVERAGE_SAMPLES_NV                                0x9045
#define GL_TEXTURE_COLOR_SAMPLES_NV                                   0x9046
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX                       0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX                 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX               0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX                         0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX                         0x904B
#define GL_IMAGE_1D                                                   0x904C
#define GL_IMAGE_1D_EXT                                               0x904C
#define GL_IMAGE_2D                                                   0x904D
#define GL_IMAGE_2D_EXT                                               0x904D
#define GL_IMAGE_3D                                                   0x904E
#define GL_IMAGE_3D_EXT                                               0x904E
#define GL_IMAGE_2D_RECT                                              0x904F
#define GL_IMAGE_2D_RECT_EXT                                          0x904F
#define GL_IMAGE_CUBE                                                 0x9050
#define GL_IMAGE_CUBE_EXT                                             0x9050
#define GL_IMAGE_BUFFER                                               0x9051
#define GL_IMAGE_BUFFER_EXT                                           0x9051
#define GL_IMAGE_1D_ARRAY                                             0x9052
#define GL_IMAGE_1D_ARRAY_EXT                                         0x9052
#define GL_IMAGE_2D_ARRAY                                             0x9053
#define GL_IMAGE_2D_ARRAY_EXT                                         0x9053
#define GL_IMAGE_CUBE_MAP_ARRAY                                       0x9054
#define GL_IMAGE_CUBE_MAP_ARRAY_EXT                                   0x9054
#define GL_IMAGE_2D_MULTISAMPLE                                       0x9055
#define GL_IMAGE_2D_MULTISAMPLE_EXT                                   0x9055
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY                                 0x9056
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY_EXT                             0x9056
#define GL_INT_IMAGE_1D                                               0x9057
#define GL_INT_IMAGE_1D_EXT                                           0x9057
#define GL_INT_IMAGE_2D                                               0x9058
#define GL_INT_IMAGE_2D_EXT                                           0x9058
#define GL_INT_IMAGE_3D                                               0x9059
#define GL_INT_IMAGE_3D_EXT                                           0x9059
#define GL_INT_IMAGE_2D_RECT                                          0x905A
#define GL_INT_IMAGE_2D_RECT_EXT                                      0x905A
#define GL_INT_IMAGE_CUBE                                             0x905B
#define GL_INT_IMAGE_CUBE_EXT                                         0x905B
#define GL_INT_IMAGE_BUFFER                                           0x905C
#define GL_INT_IMAGE_BUFFER_EXT                                       0x905C
#define GL_INT_IMAGE_1D_ARRAY                                         0x905D
#define GL_INT_IMAGE_1D_ARRAY_EXT                                     0x905D
#define GL_INT_IMAGE_2D_ARRAY                                         0x905E
#define GL_INT_IMAGE_2D_ARRAY_EXT                                     0x905E
#define GL_INT_IMAGE_CUBE_MAP_ARRAY                                   0x905F
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT                               0x905F
#define GL_INT_IMAGE_2D_MULTISAMPLE                                   0x9060
#define GL_INT_IMAGE_2D_MULTISAMPLE_EXT                               0x9060
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY                             0x9061
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT                         0x9061
#define GL_UNSIGNED_INT_IMAGE_1D                                      0x9062
#define GL_UNSIGNED_INT_IMAGE_1D_EXT                                  0x9062
#define GL_UNSIGNED_INT_IMAGE_2D                                      0x9063
#define GL_UNSIGNED_INT_IMAGE_2D_EXT                                  0x9063
#define GL_UNSIGNED_INT_IMAGE_3D                                      0x9064
#define GL_UNSIGNED_INT_IMAGE_3D_EXT                                  0x9064
#define GL_UNSIGNED_INT_IMAGE_2D_RECT                                 0x9065
#define GL_UNSIGNED_INT_IMAGE_2D_RECT_EXT                             0x9065
#define GL_UNSIGNED_INT_IMAGE_CUBE                                    0x9066
#define GL_UNSIGNED_INT_IMAGE_CUBE_EXT                                0x9066
#define GL_UNSIGNED_INT_IMAGE_BUFFER                                  0x9067
#define GL_UNSIGNED_INT_IMAGE_BUFFER_EXT                              0x9067
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY                                0x9068
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY_EXT                            0x9068
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY                                0x9069
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY_EXT                            0x9069
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY                          0x906A
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT                      0x906A
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE                          0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_EXT                      0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY                    0x906C
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT                0x906C
#define GL_MAX_IMAGE_SAMPLES                                          0x906D
#define GL_MAX_IMAGE_SAMPLES_EXT                                      0x906D
#define GL_IMAGE_BINDING_FORMAT                                       0x906E
#define GL_IMAGE_BINDING_FORMAT_EXT                                   0x906E
#define GL_RGB10_A2UI                                                 0x906F
#define GL_PATH_FORMAT_SVG_NV                                         0x9070
#define GL_PATH_FORMAT_PS_NV                                          0x9071
#define GL_STANDARD_FONT_NAME_NV                                      0x9072
#define GL_SYSTEM_FONT_NAME_NV                                        0x9073
#define GL_FILE_NAME_NV                                               0x9074
#define GL_PATH_STROKE_WIDTH_NV                                       0x9075
#define GL_PATH_END_CAPS_NV                                           0x9076
#define GL_PATH_INITIAL_END_CAP_NV                                    0x9077
#define GL_PATH_TERMINAL_END_CAP_NV                                   0x9078
#define GL_PATH_JOIN_STYLE_NV                                         0x9079
#define GL_PATH_MITER_LIMIT_NV                                        0x907A
#define GL_PATH_DASH_CAPS_NV                                          0x907B
#define GL_PATH_INITIAL_DASH_CAP_NV                                   0x907C
#define GL_PATH_TERMINAL_DASH_CAP_NV                                  0x907D
#define GL_PATH_DASH_OFFSET_NV                                        0x907E
#define GL_PATH_CLIENT_LENGTH_NV                                      0x907F
#define GL_PATH_FILL_MODE_NV                                          0x9080
#define GL_PATH_FILL_MASK_NV                                          0x9081
#define GL_PATH_FILL_COVER_MODE_NV                                    0x9082
#define GL_PATH_STROKE_COVER_MODE_NV                                  0x9083
#define GL_PATH_STROKE_MASK_NV                                        0x9084
#define GL_COUNT_UP_NV                                                0x9088
#define GL_COUNT_DOWN_NV                                              0x9089
#define GL_PATH_OBJECT_BOUNDING_BOX_NV                                0x908A
#define GL_CONVEX_HULL_NV                                             0x908B
#define GL_BOUNDING_BOX_NV                                            0x908D
#define GL_TRANSLATE_X_NV                                             0x908E
#define GL_TRANSLATE_Y_NV                                             0x908F
#define GL_TRANSLATE_2D_NV                                            0x9090
#define GL_TRANSLATE_3D_NV                                            0x9091
#define GL_AFFINE_2D_NV                                               0x9092
#define GL_AFFINE_3D_NV                                               0x9094
#define GL_TRANSPOSE_AFFINE_2D_NV                                     0x9096
#define GL_TRANSPOSE_AFFINE_3D_NV                                     0x9098
#define GL_UTF8_NV                                                    0x909A
#define GL_UTF16_NV                                                   0x909B
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV                          0x909C
#define GL_PATH_COMMAND_COUNT_NV                                      0x909D
#define GL_PATH_COORD_COUNT_NV                                        0x909E
#define GL_PATH_DASH_ARRAY_COUNT_NV                                   0x909F
#define GL_PATH_COMPUTED_LENGTH_NV                                    0x90A0
#define GL_PATH_FILL_BOUNDING_BOX_NV                                  0x90A1
#define GL_PATH_STROKE_BOUNDING_BOX_NV                                0x90A2
#define GL_SQUARE_NV                                                  0x90A3
#define GL_ROUND_NV                                                   0x90A4
#define GL_TRIANGULAR_NV                                              0x90A5
#define GL_BEVEL_NV                                                   0x90A6
#define GL_MITER_REVERT_NV                                            0x90A7
#define GL_MITER_TRUNCATE_NV                                          0x90A8
#define GL_SKIP_MISSING_GLYPH_NV                                      0x90A9
#define GL_USE_MISSING_GLYPH_NV                                       0x90AA
#define GL_PATH_ERROR_POSITION_NV                                     0x90AB
#define GL_PATH_FOG_GEN_MODE_NV                                       0x90AC
#define GL_ACCUM_ADJACENT_PAIRS_NV                                    0x90AD
#define GL_ADJACENT_PAIRS_NV                                          0x90AE
#define GL_FIRST_TO_REST_NV                                           0x90AF
#define GL_PATH_GEN_MODE_NV                                           0x90B0
#define GL_PATH_GEN_COEFF_NV                                          0x90B1
#define GL_PATH_GEN_COLOR_FORMAT_NV                                   0x90B2
#define GL_PATH_GEN_COMPONENTS_NV                                     0x90B3
#define GL_PATH_DASH_OFFSET_RESET_NV                                  0x90B4
#define GL_MOVE_TO_RESETS_NV                                          0x90B5
#define GL_MOVE_TO_CONTINUES_NV                                       0x90B6
#define GL_PATH_STENCIL_FUNC_NV                                       0x90B7
#define GL_PATH_STENCIL_REF_NV                                        0x90B8
#define GL_PATH_STENCIL_VALUE_MASK_NV                                 0x90B9
#define GL_SCALED_RESOLVE_FASTEST_EXT                                 0x90BA
#define GL_SCALED_RESOLVE_NICEST_EXT                                  0x90BB
#define GL_MIN_MAP_BUFFER_ALIGNMENT                                   0x90BC
#define GL_PATH_STENCIL_DEPTH_OFFSET_FACTOR_NV                        0x90BD
#define GL_PATH_STENCIL_DEPTH_OFFSET_UNITS_NV                         0x90BE
#define GL_PATH_COVER_DEPTH_FUNC_NV                                   0x90BF
#define GL_IMAGE_FORMAT_COMPATIBILITY_TYPE                            0x90C7
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE                         0x90C8
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS                        0x90C9
#define GL_MAX_VERTEX_IMAGE_UNIFORMS                                  0x90CA
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS                            0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS                         0x90CC
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS                                0x90CD
#define GL_MAX_FRAGMENT_IMAGE_UNIFORMS                                0x90CE
#define GL_MAX_COMBINED_IMAGE_UNIFORMS                                0x90CF
#define GL_MAX_DEEP_3D_TEXTURE_WIDTH_HEIGHT_NV                        0x90D0
#define GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV                               0x90D1
#define GL_SHADER_STORAGE_BUFFER                                      0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING                              0x90D3
#define GL_SHADER_STORAGE_BUFFER_START                                0x90D4
#define GL_SHADER_STORAGE_BUFFER_SIZE                                 0x90D5
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS                           0x90D6
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS                         0x90D7
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS                     0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS                  0x90D9
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS                         0x90DA
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS                          0x90DB
#define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS                         0x90DC
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS                         0x90DD
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE                              0x90DE
#define GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT                     0x90DF
#define GL_SYNC_X11_FENCE_EXT                                         0x90E1
#define GL_DEPTH_STENCIL_TEXTURE_MODE                                 0x90EA
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS                         0x90EB
#define GL_MAX_COMPUTE_FIXED_GROUP_INVOCATIONS_ARB                    0x90EB
#define GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER                 0x90EC
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER         0x90ED
#define GL_DISPATCH_INDIRECT_BUFFER                                   0x90EE
#define GL_DISPATCH_INDIRECT_BUFFER_BINDING                           0x90EF
#define GL_CONTEXT_ROBUST_ACCESS                                      0x90F3
#define GL_CONTEXT_ROBUST_ACCESS_KHR                                  0x90F3
#define GL_COMPUTE_PROGRAM_NV                                         0x90FB
#define GL_COMPUTE_PROGRAM_PARAMETER_BUFFER_NV                        0x90FC
#define GL_TEXTURE_2D_MULTISAMPLE                                     0x9100
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE                               0x9101
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY                               0x9102
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY                         0x9103
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE                             0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY                       0x9105
#define GL_TEXTURE_SAMPLES                                            0x9106
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS                             0x9107
#define GL_SAMPLER_2D_MULTISAMPLE                                     0x9108
#define GL_INT_SAMPLER_2D_MULTISAMPLE                                 0x9109
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE                        0x910A
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY                               0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY                           0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY                  0x910D
#define GL_MAX_COLOR_TEXTURE_SAMPLES                                  0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES                                  0x910F
#define GL_MAX_INTEGER_SAMPLES                                        0x9110
#define GL_MAX_SERVER_WAIT_TIMEOUT                                    0x9111
#define GL_OBJECT_TYPE                                                0x9112
#define GL_SYNC_CONDITION                                             0x9113
#define GL_SYNC_STATUS                                                0x9114
#define GL_SYNC_FLAGS                                                 0x9115
#define GL_SYNC_FENCE                                                 0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE                                 0x9117
#define GL_UNSIGNALED                                                 0x9118
#define GL_SIGNALED                                                   0x9119
#define GL_ALREADY_SIGNALED                                           0x911A
#define GL_TIMEOUT_EXPIRED                                            0x911B
#define GL_CONDITION_SATISFIED                                        0x911C
#define GL_WAIT_FAILED                                                0x911D
#define GL_BUFFER_ACCESS_FLAGS                                        0x911F
#define GL_BUFFER_MAP_LENGTH                                          0x9120
#define GL_BUFFER_MAP_OFFSET                                          0x9121
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS                               0x9122
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS                              0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS                             0x9124
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS                              0x9125
#define GL_CONTEXT_PROFILE_MASK                                       0x9126
#define GL_UNPACK_COMPRESSED_BLOCK_WIDTH                              0x9127
#define GL_UNPACK_COMPRESSED_BLOCK_HEIGHT                             0x9128
#define GL_UNPACK_COMPRESSED_BLOCK_DEPTH                              0x9129
#define GL_UNPACK_COMPRESSED_BLOCK_SIZE                               0x912A
#define GL_PACK_COMPRESSED_BLOCK_WIDTH                                0x912B
#define GL_PACK_COMPRESSED_BLOCK_HEIGHT                               0x912C
#define GL_PACK_COMPRESSED_BLOCK_DEPTH                                0x912D
#define GL_PACK_COMPRESSED_BLOCK_SIZE                                 0x912E
#define GL_TEXTURE_IMMUTABLE_FORMAT                                   0x912F
#define GL_MAX_DEBUG_MESSAGE_LENGTH                                   0x9143
#define GL_MAX_DEBUG_MESSAGE_LENGTH_AMD                               0x9143
#define GL_MAX_DEBUG_MESSAGE_LENGTH_ARB                               0x9143
#define GL_MAX_DEBUG_MESSAGE_LENGTH_KHR                               0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES                                  0x9144
#define GL_MAX_DEBUG_LOGGED_MESSAGES_AMD                              0x9144
#define GL_MAX_DEBUG_LOGGED_MESSAGES_ARB                              0x9144
#define GL_MAX_DEBUG_LOGGED_MESSAGES_KHR                              0x9144
#define GL_DEBUG_LOGGED_MESSAGES                                      0x9145
#define GL_DEBUG_LOGGED_MESSAGES_AMD                                  0x9145
#define GL_DEBUG_LOGGED_MESSAGES_ARB                                  0x9145
#define GL_DEBUG_LOGGED_MESSAGES_KHR                                  0x9145
#define GL_DEBUG_SEVERITY_HIGH                                        0x9146
#define GL_DEBUG_SEVERITY_HIGH_AMD                                    0x9146
#define GL_DEBUG_SEVERITY_HIGH_ARB                                    0x9146
#define GL_DEBUG_SEVERITY_HIGH_KHR                                    0x9146
#define GL_DEBUG_SEVERITY_MEDIUM                                      0x9147
#define GL_DEBUG_SEVERITY_MEDIUM_AMD                                  0x9147
#define GL_DEBUG_SEVERITY_MEDIUM_ARB                                  0x9147
#define GL_DEBUG_SEVERITY_MEDIUM_KHR                                  0x9147
#define GL_DEBUG_SEVERITY_LOW                                         0x9148
#define GL_DEBUG_SEVERITY_LOW_AMD                                     0x9148
#define GL_DEBUG_SEVERITY_LOW_ARB                                     0x9148
#define GL_DEBUG_SEVERITY_LOW_KHR                                     0x9148
#define GL_DEBUG_CATEGORY_API_ERROR_AMD                               0x9149
#define GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD                           0x914A
#define GL_DEBUG_CATEGORY_DEPRECATION_AMD                             0x914B
#define GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD                      0x914C
#define GL_DEBUG_CATEGORY_PERFORMANCE_AMD                             0x914D
#define GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD                         0x914E
#define GL_DEBUG_CATEGORY_APPLICATION_AMD                             0x914F
#define GL_DEBUG_CATEGORY_OTHER_AMD                                   0x9150
#define GL_BUFFER_OBJECT_EXT                                          0x9151
#define GL_DATA_BUFFER_AMD                                            0x9151
#define GL_PERFORMANCE_MONITOR_AMD                                    0x9152
#define GL_QUERY_OBJECT_AMD                                           0x9153
#define GL_QUERY_OBJECT_EXT                                           0x9153
#define GL_VERTEX_ARRAY_OBJECT_AMD                                    0x9154
#define GL_VERTEX_ARRAY_OBJECT_EXT                                    0x9154
#define GL_SAMPLER_OBJECT_AMD                                         0x9155
#define GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD                         0x9160
#define GL_QUERY_BUFFER                                               0x9192
#define GL_QUERY_BUFFER_AMD                                           0x9192
#define GL_QUERY_BUFFER_BINDING                                       0x9193
#define GL_QUERY_BUFFER_BINDING_AMD                                   0x9193
#define GL_QUERY_RESULT_NO_WAIT                                       0x9194
#define GL_QUERY_RESULT_NO_WAIT_AMD                                   0x9194
#define GL_VIRTUAL_PAGE_SIZE_X_ARB                                    0x9195
#define GL_VIRTUAL_PAGE_SIZE_X_AMD                                    0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_ARB                                    0x9196
#define GL_VIRTUAL_PAGE_SIZE_Y_AMD                                    0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_ARB                                    0x9197
#define GL_VIRTUAL_PAGE_SIZE_Z_AMD                                    0x9197
#define GL_MAX_SPARSE_TEXTURE_SIZE_ARB                                0x9198
#define GL_MAX_SPARSE_TEXTURE_SIZE_AMD                                0x9198
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB                             0x9199
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_AMD                             0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS                            0x919A
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB                        0x919A
#define GL_MIN_SPARSE_LEVEL_AMD                                       0x919B
#define GL_MIN_LOD_WARNING_AMD                                        0x919C
#define GL_TEXTURE_BUFFER_OFFSET                                      0x919D
#define GL_TEXTURE_BUFFER_SIZE                                        0x919E
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT                            0x919F
#define GL_STREAM_RASTERIZATION_AMD                                   0x91A0
#define GL_VERTEX_ELEMENT_SWIZZLE_AMD                                 0x91A4
#define GL_VERTEX_ID_SWIZZLE_AMD                                      0x91A5
#define GL_TEXTURE_SPARSE_ARB                                         0x91A6
#define GL_VIRTUAL_PAGE_SIZE_INDEX_ARB                                0x91A7
#define GL_NUM_VIRTUAL_PAGE_SIZES_ARB                                 0x91A8
#define GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB                 0x91A9
#define GL_NUM_SPARSE_LEVELS_ARB                                      0x91AA
#define GL_PIXELS_PER_SAMPLE_PATTERN_X_AMD                            0x91AE
#define GL_PIXELS_PER_SAMPLE_PATTERN_Y_AMD                            0x91AF
#define GL_MAX_SHADER_COMPILER_THREADS_KHR                            0x91B0
#define GL_MAX_SHADER_COMPILER_THREADS_ARB                            0x91B0
#define GL_COMPLETION_STATUS_KHR                                      0x91B1
#define GL_COMPLETION_STATUS_ARB                                      0x91B1
#define GL_RENDERBUFFER_STORAGE_SAMPLES_AMD                           0x91B2
#define GL_MAX_COLOR_FRAMEBUFFER_SAMPLES_AMD                          0x91B3
#define GL_MAX_COLOR_FRAMEBUFFER_STORAGE_SAMPLES_AMD                  0x91B4
#define GL_MAX_DEPTH_STENCIL_FRAMEBUFFER_SAMPLES_AMD                  0x91B5
#define GL_NUM_SUPPORTED_MULTISAMPLE_MODES_AMD                        0x91B6
#define GL_SUPPORTED_MULTISAMPLE_MODES_AMD                            0x91B7
#define GL_COMPUTE_SHADER                                             0x91B9
#define GL_MAX_COMPUTE_UNIFORM_BLOCKS                                 0x91BB
#define GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS                            0x91BC
#define GL_MAX_COMPUTE_IMAGE_UNIFORMS                                 0x91BD
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT                               0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE                                0x91BF
#define GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB                           0x91BF
#define GL_FLOAT16_MAT2_AMD                                           0x91C5
#define GL_FLOAT16_MAT3_AMD                                           0x91C6
#define GL_FLOAT16_MAT4_AMD                                           0x91C7
#define GL_FLOAT16_MAT2x3_AMD                                         0x91C8
#define GL_FLOAT16_MAT2x4_AMD                                         0x91C9
#define GL_FLOAT16_MAT3x2_AMD                                         0x91CA
#define GL_FLOAT16_MAT3x4_AMD                                         0x91CB
#define GL_FLOAT16_MAT4x2_AMD                                         0x91CC
#define GL_FLOAT16_MAT4x3_AMD                                         0x91CD
#define GL_COMPRESSED_R11_EAC                                         0x9270
#define GL_COMPRESSED_SIGNED_R11_EAC                                  0x9271
#define GL_COMPRESSED_RG11_EAC                                        0x9272
#define GL_COMPRESSED_SIGNED_RG11_EAC                                 0x9273
#define GL_COMPRESSED_RGB8_ETC2                                       0x9274
#define GL_COMPRESSED_SRGB8_ETC2                                      0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2                   0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2                  0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC                                  0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC                           0x9279
#define GL_BLEND_PREMULTIPLIED_SRC_NV                                 0x9280
#define GL_BLEND_OVERLAP_NV                                           0x9281
#define GL_UNCORRELATED_NV                                            0x9282
#define GL_DISJOINT_NV                                                0x9283
#define GL_CONJOINT_NV                                                0x9284
#define GL_BLEND_ADVANCED_COHERENT_KHR                                0x9285
#define GL_BLEND_ADVANCED_COHERENT_NV                                 0x9285
#define GL_SRC_NV                                                     0x9286
#define GL_DST_NV                                                     0x9287
#define GL_SRC_OVER_NV                                                0x9288
#define GL_DST_OVER_NV                                                0x9289
#define GL_SRC_IN_NV                                                  0x928A
#define GL_DST_IN_NV                                                  0x928B
#define GL_SRC_OUT_NV                                                 0x928C
#define GL_DST_OUT_NV                                                 0x928D
#define GL_SRC_ATOP_NV                                                0x928E
#define GL_DST_ATOP_NV                                                0x928F
#define GL_PLUS_NV                                                    0x9291
#define GL_PLUS_DARKER_NV                                             0x9292
#define GL_MULTIPLY_KHR                                               0x9294
#define GL_MULTIPLY_NV                                                0x9294
#define GL_SCREEN_KHR                                                 0x9295
#define GL_SCREEN_NV                                                  0x9295
#define GL_OVERLAY_KHR                                                0x9296
#define GL_OVERLAY_NV                                                 0x9296
#define GL_DARKEN_KHR                                                 0x9297
#define GL_DARKEN_NV                                                  0x9297
#define GL_LIGHTEN_KHR                                                0x9298
#define GL_LIGHTEN_NV                                                 0x9298
#define GL_COLORDODGE_KHR                                             0x9299
#define GL_COLORDODGE_NV                                              0x9299
#define GL_COLORBURN_KHR                                              0x929A
#define GL_COLORBURN_NV                                               0x929A
#define GL_HARDLIGHT_KHR                                              0x929B
#define GL_HARDLIGHT_NV                                               0x929B
#define GL_SOFTLIGHT_KHR                                              0x929C
#define GL_SOFTLIGHT_NV                                               0x929C
#define GL_DIFFERENCE_KHR                                             0x929E
#define GL_DIFFERENCE_NV                                              0x929E
#define GL_MINUS_NV                                                   0x929F
#define GL_EXCLUSION_KHR                                              0x92A0
#define GL_EXCLUSION_NV                                               0x92A0
#define GL_CONTRAST_NV                                                0x92A1
#define GL_INVERT_RGB_NV                                              0x92A3
#define GL_LINEARDODGE_NV                                             0x92A4
#define GL_LINEARBURN_NV                                              0x92A5
#define GL_VIVIDLIGHT_NV                                              0x92A6
#define GL_LINEARLIGHT_NV                                             0x92A7
#define GL_PINLIGHT_NV                                                0x92A8
#define GL_HARDMIX_NV                                                 0x92A9
#define GL_HSL_HUE_KHR                                                0x92AD
#define GL_HSL_HUE_NV                                                 0x92AD
#define GL_HSL_SATURATION_KHR                                         0x92AE
#define GL_HSL_SATURATION_NV                                          0x92AE
#define GL_HSL_COLOR_KHR                                              0x92AF
#define GL_HSL_COLOR_NV                                               0x92AF
#define GL_HSL_LUMINOSITY_KHR                                         0x92B0
#define GL_HSL_LUMINOSITY_NV                                          0x92B0
#define GL_PLUS_CLAMPED_NV                                            0x92B1
#define GL_PLUS_CLAMPED_ALPHA_NV                                      0x92B2
#define GL_MINUS_CLAMPED_NV                                           0x92B3
#define GL_INVERT_OVG_NV                                              0x92B4
#define GL_MAX_LGPU_GPUS_NVX                                          0x92BA
#define GL_MULTICAST_GPUS_NV                                          0x92BA
#define GL_PURGED_CONTEXT_RESET_NV                                    0x92BB
#define GL_PRIMITIVE_BOUNDING_BOX_ARB                                 0x92BE
#define GL_ALPHA_TO_COVERAGE_DITHER_MODE_NV                           0x92BF
#define GL_ATOMIC_COUNTER_BUFFER                                      0x92C0
#define GL_ATOMIC_COUNTER_BUFFER_BINDING                              0x92C1
#define GL_ATOMIC_COUNTER_BUFFER_START                                0x92C2
#define GL_ATOMIC_COUNTER_BUFFER_SIZE                                 0x92C3
#define GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE                            0x92C4
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS               0x92C5
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES        0x92C6
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_VERTEX_SHADER          0x92C7
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_CONTROL_SHADER    0x92C8
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_EVALUATION_SHADER 0x92C9
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_GEOMETRY_SHADER        0x92CA
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_FRAGMENT_SHADER        0x92CB
#define GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS                          0x92CC
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS                    0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS                 0x92CE
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS                        0x92CF
#define GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS                        0x92D0
#define GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS                        0x92D1
#define GL_MAX_VERTEX_ATOMIC_COUNTERS                                 0x92D2
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS                           0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS                        0x92D4
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS                               0x92D5
#define GL_MAX_FRAGMENT_ATOMIC_COUNTERS                               0x92D6
#define GL_MAX_COMBINED_ATOMIC_COUNTERS                               0x92D7
#define GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE                             0x92D8
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS                              0x92D9
#define GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX                        0x92DA
#define GL_UNSIGNED_INT_ATOMIC_COUNTER                                0x92DB
#define GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS                         0x92DC
#define GL_FRAGMENT_COVERAGE_TO_COLOR_NV                              0x92DD
#define GL_FRAGMENT_COVERAGE_COLOR_NV                                 0x92DE
#define GL_MESH_OUTPUT_PER_VERTEX_GRANULARITY_NV                      0x92DF
#define GL_DEBUG_OUTPUT                                               0x92E0
#define GL_DEBUG_OUTPUT_KHR                                           0x92E0
#define GL_UNIFORM                                                    0x92E1
#define GL_UNIFORM_BLOCK                                              0x92E2
#define GL_PROGRAM_INPUT                                              0x92E3
#define GL_PROGRAM_OUTPUT                                             0x92E4
#define GL_BUFFER_VARIABLE                                            0x92E5
#define GL_SHADER_STORAGE_BLOCK                                       0x92E6
#define GL_IS_PER_PATCH                                               0x92E7
#define GL_VERTEX_SUBROUTINE                                          0x92E8
#define GL_TESS_CONTROL_SUBROUTINE                                    0x92E9
#define GL_TESS_EVALUATION_SUBROUTINE                                 0x92EA
#define GL_GEOMETRY_SUBROUTINE                                        0x92EB
#define GL_FRAGMENT_SUBROUTINE                                        0x92EC
#define GL_COMPUTE_SUBROUTINE                                         0x92ED
#define GL_VERTEX_SUBROUTINE_UNIFORM                                  0x92EE
#define GL_TESS_CONTROL_SUBROUTINE_UNIFORM                            0x92EF
#define GL_TESS_EVALUATION_SUBROUTINE_UNIFORM                         0x92F0
#define GL_GEOMETRY_SUBROUTINE_UNIFORM                                0x92F1
#define GL_FRAGMENT_SUBROUTINE_UNIFORM                                0x92F2
#define GL_COMPUTE_SUBROUTINE_UNIFORM                                 0x92F3
#define GL_TRANSFORM_FEEDBACK_VARYING                                 0x92F4
#define GL_ACTIVE_RESOURCES                                           0x92F5
#define GL_MAX_NAME_LENGTH                                            0x92F6
#define GL_MAX_NUM_ACTIVE_VARIABLES                                   0x92F7
#define GL_MAX_NUM_COMPATIBLE_SUBROUTINES                             0x92F8
#define GL_NAME_LENGTH                                                0x92F9
#define GL_TYPE                                                       0x92FA
#define GL_ARRAY_SIZE                                                 0x92FB
#define GL_OFFSET                                                     0x92FC
#define GL_BLOCK_INDEX                                                0x92FD
#define GL_ARRAY_STRIDE                                               0x92FE
#define GL_MATRIX_STRIDE                                              0x92FF
#define GL_IS_ROW_MAJOR                                               0x9300
#define GL_ATOMIC_COUNTER_BUFFER_INDEX                                0x9301
#define GL_BUFFER_BINDING                                             0x9302
#define GL_BUFFER_DATA_SIZE                                           0x9303
#define GL_NUM_ACTIVE_VARIABLES                                       0x9304
#define GL_ACTIVE_VARIABLES                                           0x9305
#define GL_REFERENCED_BY_VERTEX_SHADER                                0x9306
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER                          0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER                       0x9308
#define GL_REFERENCED_BY_GEOMETRY_SHADER                              0x9309
#define GL_REFERENCED_BY_FRAGMENT_SHADER                              0x930A
#define GL_REFERENCED_BY_COMPUTE_SHADER                               0x930B
#define GL_TOP_LEVEL_ARRAY_SIZE                                       0x930C
#define GL_TOP_LEVEL_ARRAY_STRIDE                                     0x930D
#define GL_LOCATION                                                   0x930E
#define GL_LOCATION_INDEX                                             0x930F
#define GL_FRAMEBUFFER_DEFAULT_WIDTH                                  0x9310
#define GL_FRAMEBUFFER_DEFAULT_HEIGHT                                 0x9311
#define GL_FRAMEBUFFER_DEFAULT_LAYERS                                 0x9312
#define GL_FRAMEBUFFER_DEFAULT_SAMPLES                                0x9313
#define GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS                 0x9314
#define GL_MAX_FRAMEBUFFER_WIDTH                                      0x9315
#define GL_MAX_FRAMEBUFFER_HEIGHT                                     0x9316
#define GL_MAX_FRAMEBUFFER_LAYERS                                     0x9317
#define GL_MAX_FRAMEBUFFER_SAMPLES                                    0x9318
#define GL_RASTER_MULTISAMPLE_EXT                                     0x9327
#define GL_RASTER_SAMPLES_EXT                                         0x9328
#define GL_MAX_RASTER_SAMPLES_EXT                                     0x9329
#define GL_RASTER_FIXED_SAMPLE_LOCATIONS_EXT                          0x932A
#define GL_MULTISAMPLE_RASTERIZATION_ALLOWED_EXT                      0x932B
#define GL_EFFECTIVE_RASTER_SAMPLES_EXT                               0x932C
#define GL_DEPTH_SAMPLES_NV                                           0x932D
#define GL_STENCIL_SAMPLES_NV                                         0x932E
#define GL_MIXED_DEPTH_SAMPLES_SUPPORTED_NV                           0x932F
#define GL_MIXED_STENCIL_SAMPLES_SUPPORTED_NV                         0x9330
#define GL_COVERAGE_MODULATION_TABLE_NV                               0x9331
#define GL_COVERAGE_MODULATION_NV                                     0x9332
#define GL_COVERAGE_MODULATION_TABLE_SIZE_NV                          0x9333
#define GL_WARP_SIZE_NV                                               0x9339
#define GL_WARPS_PER_SM_NV                                            0x933A
#define GL_SM_COUNT_NV                                                0x933B
#define GL_FILL_RECTANGLE_NV                                          0x933C
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_ARB                          0x933D
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_NV                           0x933D
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_ARB                       0x933E
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_NV                        0x933E
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_ARB                      0x933F
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_NV                       0x933F
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_ARB                0x9340
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_NV                 0x9340
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_ARB                           0x9341
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_NV                            0x9341
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_ARB              0x9342
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_NV               0x9342
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_ARB                 0x9343
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_NV                  0x9343
#define GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB                 0x9344
#define GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB                        0x9345
#define GL_CONSERVATIVE_RASTERIZATION_NV                              0x9346
#define GL_SUBPIXEL_PRECISION_BIAS_X_BITS_NV                          0x9347
#define GL_SUBPIXEL_PRECISION_BIAS_Y_BITS_NV                          0x9348
#define GL_MAX_SUBPIXEL_PRECISION_BIAS_BITS_NV                        0x9349
#define GL_LOCATION_COMPONENT                                         0x934A
#define GL_TRANSFORM_FEEDBACK_BUFFER_INDEX                            0x934B
#define GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE                           0x934C
#define GL_ALPHA_TO_COVERAGE_DITHER_DEFAULT_NV                        0x934D
#define GL_ALPHA_TO_COVERAGE_DITHER_ENABLE_NV                         0x934E
#define GL_ALPHA_TO_COVERAGE_DITHER_DISABLE_NV                        0x934F
#define GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV                             0x9350
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV                             0x9351
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV                             0x9352
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV                             0x9353
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV                             0x9354
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV                             0x9355
#define GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV                             0x9356
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_W_NV                             0x9357
#define GL_VIEWPORT_SWIZZLE_X_NV                                      0x9358
#define GL_VIEWPORT_SWIZZLE_Y_NV                                      0x9359
#define GL_VIEWPORT_SWIZZLE_Z_NV                                      0x935A
#define GL_VIEWPORT_SWIZZLE_W_NV                                      0x935B
#define GL_CLIP_ORIGIN                                                0x935C
#define GL_CLIP_DEPTH_MODE                                            0x935D
#define GL_NEGATIVE_ONE_TO_ONE                                        0x935E
#define GL_ZERO_TO_ONE                                                0x935F
#define GL_CLEAR_TEXTURE                                              0x9365
#define GL_TEXTURE_REDUCTION_MODE_ARB                                 0x9366
#define GL_TEXTURE_REDUCTION_MODE_EXT                                 0x9366
#define GL_WEIGHTED_AVERAGE_ARB                                       0x9367
#define GL_WEIGHTED_AVERAGE_EXT                                       0x9367
#define GL_FONT_GLYPHS_AVAILABLE_NV                                   0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV                                 0x9369
#define GL_FONT_UNAVAILABLE_NV                                        0x936A
#define GL_FONT_UNINTELLIGIBLE_NV                                     0x936B
#define GL_STANDARD_FONT_FORMAT_NV                                    0x936C
#define GL_FRAGMENT_INPUT_NV                                          0x936D
#define GL_UNIFORM_BUFFER_UNIFIED_NV                                  0x936E
#define GL_UNIFORM_BUFFER_ADDRESS_NV                                  0x936F
#define GL_UNIFORM_BUFFER_LENGTH_NV                                   0x9370
#define GL_MULTISAMPLES_NV                                            0x9371
#define GL_SUPERSAMPLE_SCALE_X_NV                                     0x9372
#define GL_SUPERSAMPLE_SCALE_Y_NV                                     0x9373
#define GL_CONFORMANT_NV                                              0x9374
#define GL_CONSERVATIVE_RASTER_DILATE_NV                              0x9379
#define GL_CONSERVATIVE_RASTER_DILATE_RANGE_NV                        0x937A
#define GL_CONSERVATIVE_RASTER_DILATE_GRANULARITY_NV                  0x937B
#define GL_VIEWPORT_POSITION_W_SCALE_NV                               0x937C
#define GL_VIEWPORT_POSITION_W_SCALE_X_COEFF_NV                       0x937D
#define GL_VIEWPORT_POSITION_W_SCALE_Y_COEFF_NV                       0x937E
#define GL_REPRESENTATIVE_FRAGMENT_TEST_NV                            0x937F
#define GL_NUM_SAMPLE_COUNTS                                          0x9380
#define GL_MULTISAMPLE_LINE_WIDTH_RANGE_ARB                           0x9381
#define GL_MULTISAMPLE_LINE_WIDTH_GRANULARITY_ARB                     0x9382
#define GL_VIEW_CLASS_EAC_R11                                         0x9383
#define GL_VIEW_CLASS_EAC_RG11                                        0x9384
#define GL_VIEW_CLASS_ETC2_RGB                                        0x9385
#define GL_VIEW_CLASS_ETC2_RGBA                                       0x9386
#define GL_VIEW_CLASS_ETC2_EAC_RGBA                                   0x9387
#define GL_VIEW_CLASS_ASTC_4x4_RGBA                                   0x9388
#define GL_VIEW_CLASS_ASTC_5x4_RGBA                                   0x9389
#define GL_VIEW_CLASS_ASTC_5x5_RGBA                                   0x938A
#define GL_VIEW_CLASS_ASTC_6x5_RGBA                                   0x938B
#define GL_VIEW_CLASS_ASTC_6x6_RGBA                                   0x938C
#define GL_VIEW_CLASS_ASTC_8x5_RGBA                                   0x938D
#define GL_VIEW_CLASS_ASTC_8x6_RGBA                                   0x938E
#define GL_VIEW_CLASS_ASTC_8x8_RGBA                                   0x938F
#define GL_VIEW_CLASS_ASTC_10x5_RGBA                                  0x9390
#define GL_VIEW_CLASS_ASTC_10x6_RGBA                                  0x9391
#define GL_VIEW_CLASS_ASTC_10x8_RGBA                                  0x9392
#define GL_VIEW_CLASS_ASTC_10x10_RGBA                                 0x9393
#define GL_VIEW_CLASS_ASTC_12x10_RGBA                                 0x9394
#define GL_VIEW_CLASS_ASTC_12x12_RGBA                                 0x9395
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR                               0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR                               0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR                               0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR                               0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR                               0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR                               0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR                               0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR                               0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR                              0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR                              0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR                              0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR                             0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR                             0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR                             0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR                       0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR                       0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR                       0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR                       0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR                       0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR                       0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR                       0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR                       0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR                      0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR                      0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR                      0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR                     0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR                     0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR                     0x93DD
#define GL_PERFQUERY_COUNTER_EVENT_INTEL                              0x94F0
#define GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL                      0x94F1
#define GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL                       0x94F2
#define GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL                         0x94F3
#define GL_PERFQUERY_COUNTER_RAW_INTEL                                0x94F4
#define GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL                          0x94F5
#define GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL                        0x94F8
#define GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL                        0x94F9
#define GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL                         0x94FA
#define GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL                        0x94FB
#define GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL                        0x94FC
#define GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL                      0x94FD
#define GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL                    0x94FE
#define GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL                    0x94FF
#define GL_PERFQUERY_GPA_EXTENDED_COUNTERS_INTEL                      0x9500
#define GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT              0x9530
#define GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT              0x9531
#define GL_SUBGROUP_SIZE_KHR                                          0x9532
#define GL_SUBGROUP_SUPPORTED_STAGES_KHR                              0x9533
#define GL_SUBGROUP_SUPPORTED_FEATURES_KHR                            0x9534
#define GL_SUBGROUP_QUAD_ALL_STAGES_KHR                               0x9535
#define GL_MAX_MESH_TOTAL_MEMORY_SIZE_NV                              0x9536
#define GL_MAX_TASK_TOTAL_MEMORY_SIZE_NV                              0x9537
#define GL_MAX_MESH_OUTPUT_VERTICES_NV                                0x9538
#define GL_MAX_MESH_OUTPUT_PRIMITIVES_NV                              0x9539
#define GL_MAX_TASK_OUTPUT_COUNT_NV                                   0x953A
#define GL_MAX_MESH_WORK_GROUP_SIZE_NV                                0x953B
#define GL_MAX_TASK_WORK_GROUP_SIZE_NV                                0x953C
#define GL_MAX_DRAW_MESH_TASKS_COUNT_NV                               0x953D
#define GL_MESH_WORK_GROUP_SIZE_NV                                    0x953E
#define GL_TASK_WORK_GROUP_SIZE_NV                                    0x953F
#define GL_QUERY_RESOURCE_TYPE_VIDMEM_ALLOC_NV                        0x9540
#define GL_QUERY_RESOURCE_MEMTYPE_VIDMEM_NV                           0x9542
#define GL_MESH_OUTPUT_PER_PRIMITIVE_GRANULARITY_NV                   0x9543
#define GL_QUERY_RESOURCE_SYS_RESERVED_NV                             0x9544
#define GL_QUERY_RESOURCE_TEXTURE_NV                                  0x9545
#define GL_QUERY_RESOURCE_RENDERBUFFER_NV                             0x9546
#define GL_QUERY_RESOURCE_BUFFEROBJECT_NV                             0x9547
#define GL_PER_GPU_STORAGE_NV                                         0x9548
#define GL_MULTICAST_PROGRAMMABLE_SAMPLE_LOCATION_NV                  0x9549
#define GL_UPLOAD_GPU_MASK_NVX                                        0x954A
#define GL_CONSERVATIVE_RASTER_MODE_NV                                0x954D
#define GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV                      0x954E
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_TRIANGLES_NV             0x954F
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_NV                       0x9550
#define GL_SHADER_BINARY_FORMAT_SPIR_V                                0x9551
#define GL_SHADER_BINARY_FORMAT_SPIR_V_ARB                            0x9551
#define GL_SPIR_V_BINARY                                              0x9552
#define GL_SPIR_V_BINARY_ARB                                          0x9552
#define GL_SPIR_V_EXTENSIONS                                          0x9553
#define GL_NUM_SPIR_V_EXTENSIONS                                      0x9554
#define GL_SCISSOR_TEST_EXCLUSIVE_NV                                  0x9555
#define GL_SCISSOR_BOX_EXCLUSIVE_NV                                   0x9556
#define GL_MAX_MESH_VIEWS_NV                                          0x9557
#define GL_RENDER_GPU_MASK_NV                                         0x9558
#define GL_MESH_SHADER_NV                                             0x9559
#define GL_TASK_SHADER_NV                                             0x955A
#define GL_SHADING_RATE_IMAGE_BINDING_NV                              0x955B
#define GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV                          0x955C
#define GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV                         0x955D
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV                         0x955E
#define GL_MAX_COARSE_FRAGMENT_SAMPLES_NV                             0x955F
#define GL_SHADING_RATE_IMAGE_NV                                      0x9563
#define GL_SHADING_RATE_NO_INVOCATIONS_NV                             0x9564
#define GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV                     0x9565
#define GL_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV                0x9566
#define GL_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV                0x9567
#define GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV                0x9568
#define GL_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV                0x9569
#define GL_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV                0x956A
#define GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV                0x956B
#define GL_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV                    0x956C
#define GL_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV                    0x956D
#define GL_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV                    0x956E
#define GL_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV                   0x956F
#define GL_MESH_VERTICES_OUT_NV                                       0x9579
#define GL_MESH_PRIMITIVES_OUT_NV                                     0x957A
#define GL_MESH_OUTPUT_TYPE_NV                                        0x957B
#define GL_MESH_SUBROUTINE_NV                                         0x957C
#define GL_TASK_SUBROUTINE_NV                                         0x957D
#define GL_MESH_SUBROUTINE_UNIFORM_NV                                 0x957E
#define GL_TASK_SUBROUTINE_UNIFORM_NV                                 0x957F
#define GL_TEXTURE_TILING_EXT                                         0x9580
#define GL_DEDICATED_MEMORY_OBJECT_EXT                                0x9581
#define GL_NUM_TILING_TYPES_EXT                                       0x9582
#define GL_TILING_TYPES_EXT                                           0x9583
#define GL_OPTIMAL_TILING_EXT                                         0x9584
#define GL_LINEAR_TILING_EXT                                          0x9585
#define GL_HANDLE_TYPE_OPAQUE_FD_EXT                                  0x9586
#define GL_HANDLE_TYPE_OPAQUE_WIN32_EXT                               0x9587
#define GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT                           0x9588
#define GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT                             0x9589
#define GL_HANDLE_TYPE_D3D12_RESOURCE_EXT                             0x958A
#define GL_HANDLE_TYPE_D3D11_IMAGE_EXT                                0x958B
#define GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT                            0x958C
#define GL_LAYOUT_GENERAL_EXT                                         0x958D
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT                                0x958E
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT                        0x958F
#define GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT                         0x9590
#define GL_LAYOUT_SHADER_READ_ONLY_EXT                                0x9591
#define GL_LAYOUT_TRANSFER_SRC_EXT                                    0x9592
#define GL_LAYOUT_TRANSFER_DST_EXT                                    0x9593
#define GL_HANDLE_TYPE_D3D12_FENCE_EXT                                0x9594
#define GL_D3D12_FENCE_VALUE_EXT                                      0x9595
#define GL_TIMELINE_SEMAPHORE_VALUE_NV                                0x9595
#define GL_NUM_DEVICE_UUIDS_EXT                                       0x9596
#define GL_DEVICE_UUID_EXT                                            0x9597
#define GL_DRIVER_UUID_EXT                                            0x9598
#define GL_DEVICE_LUID_EXT                                            0x9599
#define GL_DEVICE_NODE_MASK_EXT                                       0x959A
#define GL_PROTECTED_MEMORY_OBJECT_EXT                                0x959B
#define GL_UNIFORM_BLOCK_REFERENCED_BY_MESH_SHADER_NV                 0x959C
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TASK_SHADER_NV                 0x959D
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_MESH_SHADER_NV         0x959E
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TASK_SHADER_NV         0x959F
#define GL_REFERENCED_BY_MESH_SHADER_NV                               0x95A0
#define GL_REFERENCED_BY_TASK_SHADER_NV                               0x95A1
#define GL_MAX_MESH_WORK_GROUP_INVOCATIONS_NV                         0x95A2
#define GL_MAX_TASK_WORK_GROUP_INVOCATIONS_NV                         0x95A3
#define GL_ATTACHED_MEMORY_OBJECT_NV                                  0x95A4
#define GL_ATTACHED_MEMORY_OFFSET_NV                                  0x95A5
#define GL_MEMORY_ATTACHABLE_ALIGNMENT_NV                             0x95A6
#define GL_MEMORY_ATTACHABLE_SIZE_NV                                  0x95A7
#define GL_MEMORY_ATTACHABLE_NV                                       0x95A8
#define GL_DETACHED_MEMORY_INCARNATION_NV                             0x95A9
#define GL_DETACHED_TEXTURES_NV                                       0x95AA
#define GL_DETACHED_BUFFERS_NV                                        0x95AB
#define GL_MAX_DETACHED_TEXTURES_NV                                   0x95AC
#define GL_MAX_DETACHED_BUFFERS_NV                                    0x95AD
#define GL_SHADING_RATE_SAMPLE_ORDER_DEFAULT_NV                       0x95AE
#define GL_SHADING_RATE_SAMPLE_ORDER_PIXEL_MAJOR_NV                   0x95AF
#define GL_SHADING_RATE_SAMPLE_ORDER_SAMPLE_MAJOR_NV                  0x95B0
#define GL_SEMAPHORE_TYPE_NV                                          0x95B3
#define GL_SEMAPHORE_TYPE_BINARY_NV                                   0x95B4
#define GL_SEMAPHORE_TYPE_TIMELINE_NV                                 0x95B5
#define GL_MAX_TIMELINE_SEMAPHORE_VALUE_DIFFERENCE_NV                 0x95B6


PXR_NAMESPACE_OPEN_SCOPE
namespace internal {
namespace GLApi {


typedef void (*PFNGLACCUMPROC) (GLenum  op, GLfloat  value);
typedef void (*PFNGLACTIVEPROGRAMEXTPROC) (GLuint  program);
typedef void (*PFNGLACTIVESHADERPROGRAMPROC) (GLuint  pipeline, GLuint  program);
typedef void (*PFNGLACTIVESHADERPROGRAMEXTPROC) (GLuint  pipeline, GLuint  program);
typedef void (*PFNGLACTIVESTENCILFACEEXTPROC) (GLenum  face);
typedef void (*PFNGLACTIVETEXTUREPROC) (GLenum  texture);
typedef void (*PFNGLACTIVETEXTUREARBPROC) (GLenum  texture);
typedef void (*PFNGLACTIVEVARYINGNVPROC) (GLuint  program, const GLchar * name);
typedef void (*PFNGLALPHAFUNCPROC) (GLenum  func, GLfloat  ref);
typedef void (*PFNGLALPHATOCOVERAGEDITHERCONTROLNVPROC) (GLenum  mode);
typedef void (*PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC) ();
typedef void (*PFNGLAPPLYTEXTUREEXTPROC) (GLenum  mode);
typedef GLboolean (*PFNGLACQUIREKEYEDMUTEXWIN32EXTPROC) (GLuint  memory, GLuint64  key, GLuint  timeout);
typedef GLboolean (*PFNGLAREPROGRAMSRESIDENTNVPROC) (GLsizei  n, const GLuint * programs, GLboolean * residences);
typedef GLboolean (*PFNGLARETEXTURESRESIDENTPROC) (GLsizei  n, const GLuint * textures, GLboolean * residences);
typedef GLboolean (*PFNGLARETEXTURESRESIDENTEXTPROC) (GLsizei  n, const GLuint * textures, GLboolean * residences);
typedef void (*PFNGLARRAYELEMENTPROC) (GLint  i);
typedef void (*PFNGLARRAYELEMENTEXTPROC) (GLint  i);
typedef GLuint (*PFNGLASYNCCOPYBUFFERSUBDATANVXPROC) (GLsizei  waitSemaphoreCount, const GLuint * waitSemaphoreArray, const GLuint64 * fenceValueArray, GLuint  readGpu, GLbitfield  writeGpuMask, GLuint  readBuffer, GLuint  writeBuffer, GLintptr  readOffset, GLintptr  writeOffset, GLsizeiptr  size, GLsizei  signalSemaphoreCount, const GLuint * signalSemaphoreArray, const GLuint64 * signalValueArray);
typedef GLuint (*PFNGLASYNCCOPYIMAGESUBDATANVXPROC) (GLsizei  waitSemaphoreCount, const GLuint * waitSemaphoreArray, const GLuint64 * waitValueArray, GLuint  srcGpu, GLbitfield  dstGpuMask, GLuint  srcName, GLenum  srcTarget, GLint  srcLevel, GLint  srcX, GLint  srcY, GLint  srcZ, GLuint  dstName, GLenum  dstTarget, GLint  dstLevel, GLint  dstX, GLint  dstY, GLint  dstZ, GLsizei  srcWidth, GLsizei  srcHeight, GLsizei  srcDepth, GLsizei  signalSemaphoreCount, const GLuint * signalSemaphoreArray, const GLuint64 * signalValueArray);
typedef void (*PFNGLATTACHOBJECTARBPROC) (GLhandleARB  containerObj, GLhandleARB  obj);
typedef void (*PFNGLATTACHSHADERPROC) (GLuint  program, GLuint  shader);
typedef void (*PFNGLBEGINPROC) (GLenum  mode);
typedef void (*PFNGLBEGINCONDITIONALRENDERPROC) (GLuint  id, GLenum  mode);
typedef void (*PFNGLBEGINCONDITIONALRENDERNVPROC) (GLuint  id, GLenum  mode);
typedef void (*PFNGLBEGINCONDITIONALRENDERNVXPROC) (GLuint  id);
typedef void (*PFNGLBEGINOCCLUSIONQUERYNVPROC) (GLuint  id);
typedef void (*PFNGLBEGINPERFMONITORAMDPROC) (GLuint  monitor);
typedef void (*PFNGLBEGINPERFQUERYINTELPROC) (GLuint  queryHandle);
typedef void (*PFNGLBEGINQUERYPROC) (GLenum  target, GLuint  id);
typedef void (*PFNGLBEGINQUERYARBPROC) (GLenum  target, GLuint  id);
typedef void (*PFNGLBEGINQUERYINDEXEDPROC) (GLenum  target, GLuint  index, GLuint  id);
typedef void (*PFNGLBEGINTRANSFORMFEEDBACKPROC) (GLenum  primitiveMode);
typedef void (*PFNGLBEGINTRANSFORMFEEDBACKEXTPROC) (GLenum  primitiveMode);
typedef void (*PFNGLBEGINTRANSFORMFEEDBACKNVPROC) (GLenum  primitiveMode);
typedef void (*PFNGLBEGINVERTEXSHADEREXTPROC) ();
typedef void (*PFNGLBEGINVIDEOCAPTURENVPROC) (GLuint  video_capture_slot);
typedef void (*PFNGLBINDATTRIBLOCATIONPROC) (GLuint  program, GLuint  index, const GLchar * name);
typedef void (*PFNGLBINDATTRIBLOCATIONARBPROC) (GLhandleARB  programObj, GLuint  index, const GLcharARB * name);
typedef void (*PFNGLBINDBUFFERPROC) (GLenum  target, GLuint  buffer);
typedef void (*PFNGLBINDBUFFERARBPROC) (GLenum  target, GLuint  buffer);
typedef void (*PFNGLBINDBUFFERBASEPROC) (GLenum  target, GLuint  index, GLuint  buffer);
typedef void (*PFNGLBINDBUFFERBASEEXTPROC) (GLenum  target, GLuint  index, GLuint  buffer);
typedef void (*PFNGLBINDBUFFERBASENVPROC) (GLenum  target, GLuint  index, GLuint  buffer);
typedef void (*PFNGLBINDBUFFEROFFSETEXTPROC) (GLenum  target, GLuint  index, GLuint  buffer, GLintptr  offset);
typedef void (*PFNGLBINDBUFFEROFFSETNVPROC) (GLenum  target, GLuint  index, GLuint  buffer, GLintptr  offset);
typedef void (*PFNGLBINDBUFFERRANGEPROC) (GLenum  target, GLuint  index, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLBINDBUFFERRANGEEXTPROC) (GLenum  target, GLuint  index, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLBINDBUFFERRANGENVPROC) (GLenum  target, GLuint  index, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLBINDBUFFERSBASEPROC) (GLenum  target, GLuint  first, GLsizei  count, const GLuint * buffers);
typedef void (*PFNGLBINDBUFFERSRANGEPROC) (GLenum  target, GLuint  first, GLsizei  count, const GLuint * buffers, const GLintptr * offsets, const GLsizeiptr * sizes);
typedef void (*PFNGLBINDFRAGDATALOCATIONPROC) (GLuint  program, GLuint  color, const GLchar * name);
typedef void (*PFNGLBINDFRAGDATALOCATIONEXTPROC) (GLuint  program, GLuint  color, const GLchar * name);
typedef void (*PFNGLBINDFRAGDATALOCATIONINDEXEDPROC) (GLuint  program, GLuint  colorNumber, GLuint  index, const GLchar * name);
typedef void (*PFNGLBINDFRAMEBUFFERPROC) (GLenum  target, GLuint  framebuffer);
typedef void (*PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum  target, GLuint  framebuffer);
typedef void (*PFNGLBINDIMAGETEXTUREPROC) (GLuint  unit, GLuint  texture, GLint  level, GLboolean  layered, GLint  layer, GLenum  access, GLenum  format);
typedef void (*PFNGLBINDIMAGETEXTUREEXTPROC) (GLuint  index, GLuint  texture, GLint  level, GLboolean  layered, GLint  layer, GLenum  access, GLint  format);
typedef void (*PFNGLBINDIMAGETEXTURESPROC) (GLuint  first, GLsizei  count, const GLuint * textures);
typedef GLuint (*PFNGLBINDLIGHTPARAMETEREXTPROC) (GLenum  light, GLenum  value);
typedef GLuint (*PFNGLBINDMATERIALPARAMETEREXTPROC) (GLenum  face, GLenum  value);
typedef void (*PFNGLBINDMULTITEXTUREEXTPROC) (GLenum  texunit, GLenum  target, GLuint  texture);
typedef GLuint (*PFNGLBINDPARAMETEREXTPROC) (GLenum  value);
typedef void (*PFNGLBINDPROGRAMARBPROC) (GLenum  target, GLuint  program);
typedef void (*PFNGLBINDPROGRAMNVPROC) (GLenum  target, GLuint  id);
typedef void (*PFNGLBINDPROGRAMPIPELINEPROC) (GLuint  pipeline);
typedef void (*PFNGLBINDPROGRAMPIPELINEEXTPROC) (GLuint  pipeline);
typedef void (*PFNGLBINDRENDERBUFFERPROC) (GLenum  target, GLuint  renderbuffer);
typedef void (*PFNGLBINDRENDERBUFFEREXTPROC) (GLenum  target, GLuint  renderbuffer);
typedef void (*PFNGLBINDSAMPLERPROC) (GLuint  unit, GLuint  sampler);
typedef void (*PFNGLBINDSAMPLERSPROC) (GLuint  first, GLsizei  count, const GLuint * samplers);
typedef void (*PFNGLBINDSHADINGRATEIMAGENVPROC) (GLuint  texture);
typedef GLuint (*PFNGLBINDTEXGENPARAMETEREXTPROC) (GLenum  unit, GLenum  coord, GLenum  value);
typedef void (*PFNGLBINDTEXTUREPROC) (GLenum  target, GLuint  texture);
typedef void (*PFNGLBINDTEXTUREEXTPROC) (GLenum  target, GLuint  texture);
typedef void (*PFNGLBINDTEXTUREUNITPROC) (GLuint  unit, GLuint  texture);
typedef GLuint (*PFNGLBINDTEXTUREUNITPARAMETEREXTPROC) (GLenum  unit, GLenum  value);
typedef void (*PFNGLBINDTEXTURESPROC) (GLuint  first, GLsizei  count, const GLuint * textures);
typedef void (*PFNGLBINDTRANSFORMFEEDBACKPROC) (GLenum  target, GLuint  id);
typedef void (*PFNGLBINDTRANSFORMFEEDBACKNVPROC) (GLenum  target, GLuint  id);
typedef void (*PFNGLBINDVERTEXARRAYPROC) (GLuint  array);
typedef void (*PFNGLBINDVERTEXARRAYAPPLEPROC) (GLuint  array);
typedef void (*PFNGLBINDVERTEXBUFFERPROC) (GLuint  bindingindex, GLuint  buffer, GLintptr  offset, GLsizei  stride);
typedef void (*PFNGLBINDVERTEXBUFFERSPROC) (GLuint  first, GLsizei  count, const GLuint * buffers, const GLintptr * offsets, const GLsizei * strides);
typedef void (*PFNGLBINDVERTEXSHADEREXTPROC) (GLuint  id);
typedef void (*PFNGLBINDVIDEOCAPTURESTREAMBUFFERNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  frame_region, GLintptrARB  offset);
typedef void (*PFNGLBINDVIDEOCAPTURESTREAMTEXTURENVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  frame_region, GLenum  target, GLuint  texture);
typedef void (*PFNGLBINORMAL3BEXTPROC) (GLbyte  bx, GLbyte  by, GLbyte  bz);
typedef void (*PFNGLBINORMAL3BVEXTPROC) (const GLbyte * v);
typedef void (*PFNGLBINORMAL3DEXTPROC) (GLdouble  bx, GLdouble  by, GLdouble  bz);
typedef void (*PFNGLBINORMAL3DVEXTPROC) (const GLdouble * v);
typedef void (*PFNGLBINORMAL3FEXTPROC) (GLfloat  bx, GLfloat  by, GLfloat  bz);
typedef void (*PFNGLBINORMAL3FVEXTPROC) (const GLfloat * v);
typedef void (*PFNGLBINORMAL3IEXTPROC) (GLint  bx, GLint  by, GLint  bz);
typedef void (*PFNGLBINORMAL3IVEXTPROC) (const GLint * v);
typedef void (*PFNGLBINORMAL3SEXTPROC) (GLshort  bx, GLshort  by, GLshort  bz);
typedef void (*PFNGLBINORMAL3SVEXTPROC) (const GLshort * v);
typedef void (*PFNGLBINORMALPOINTEREXTPROC) (GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLBITMAPPROC) (GLsizei  width, GLsizei  height, GLfloat  xorig, GLfloat  yorig, GLfloat  xmove, GLfloat  ymove, const GLubyte * bitmap);
typedef void (*PFNGLBLENDBARRIERKHRPROC) ();
typedef void (*PFNGLBLENDBARRIERNVPROC) ();
typedef void (*PFNGLBLENDCOLORPROC) (GLfloat  red, GLfloat  green, GLfloat  blue, GLfloat  alpha);
typedef void (*PFNGLBLENDCOLOREXTPROC) (GLfloat  red, GLfloat  green, GLfloat  blue, GLfloat  alpha);
typedef void (*PFNGLBLENDEQUATIONPROC) (GLenum  mode);
typedef void (*PFNGLBLENDEQUATIONEXTPROC) (GLenum  mode);
typedef void (*PFNGLBLENDEQUATIONINDEXEDAMDPROC) (GLuint  buf, GLenum  mode);
typedef void (*PFNGLBLENDEQUATIONSEPARATEPROC) (GLenum  modeRGB, GLenum  modeAlpha);
typedef void (*PFNGLBLENDEQUATIONSEPARATEEXTPROC) (GLenum  modeRGB, GLenum  modeAlpha);
typedef void (*PFNGLBLENDEQUATIONSEPARATEINDEXEDAMDPROC) (GLuint  buf, GLenum  modeRGB, GLenum  modeAlpha);
typedef void (*PFNGLBLENDEQUATIONSEPARATEIPROC) (GLuint  buf, GLenum  modeRGB, GLenum  modeAlpha);
typedef void (*PFNGLBLENDEQUATIONSEPARATEIARBPROC) (GLuint  buf, GLenum  modeRGB, GLenum  modeAlpha);
typedef void (*PFNGLBLENDEQUATIONIPROC) (GLuint  buf, GLenum  mode);
typedef void (*PFNGLBLENDEQUATIONIARBPROC) (GLuint  buf, GLenum  mode);
typedef void (*PFNGLBLENDFUNCPROC) (GLenum  sfactor, GLenum  dfactor);
typedef void (*PFNGLBLENDFUNCINDEXEDAMDPROC) (GLuint  buf, GLenum  src, GLenum  dst);
typedef void (*PFNGLBLENDFUNCSEPARATEPROC) (GLenum  sfactorRGB, GLenum  dfactorRGB, GLenum  sfactorAlpha, GLenum  dfactorAlpha);
typedef void (*PFNGLBLENDFUNCSEPARATEEXTPROC) (GLenum  sfactorRGB, GLenum  dfactorRGB, GLenum  sfactorAlpha, GLenum  dfactorAlpha);
typedef void (*PFNGLBLENDFUNCSEPARATEINDEXEDAMDPROC) (GLuint  buf, GLenum  srcRGB, GLenum  dstRGB, GLenum  srcAlpha, GLenum  dstAlpha);
typedef void (*PFNGLBLENDFUNCSEPARATEIPROC) (GLuint  buf, GLenum  srcRGB, GLenum  dstRGB, GLenum  srcAlpha, GLenum  dstAlpha);
typedef void (*PFNGLBLENDFUNCSEPARATEIARBPROC) (GLuint  buf, GLenum  srcRGB, GLenum  dstRGB, GLenum  srcAlpha, GLenum  dstAlpha);
typedef void (*PFNGLBLENDFUNCIPROC) (GLuint  buf, GLenum  src, GLenum  dst);
typedef void (*PFNGLBLENDFUNCIARBPROC) (GLuint  buf, GLenum  src, GLenum  dst);
typedef void (*PFNGLBLENDPARAMETERINVPROC) (GLenum  pname, GLint  value);
typedef void (*PFNGLBLITFRAMEBUFFERPROC) (GLint  srcX0, GLint  srcY0, GLint  srcX1, GLint  srcY1, GLint  dstX0, GLint  dstY0, GLint  dstX1, GLint  dstY1, GLbitfield  mask, GLenum  filter);
typedef void (*PFNGLBLITFRAMEBUFFEREXTPROC) (GLint  srcX0, GLint  srcY0, GLint  srcX1, GLint  srcY1, GLint  dstX0, GLint  dstY0, GLint  dstX1, GLint  dstY1, GLbitfield  mask, GLenum  filter);
typedef void (*PFNGLBLITNAMEDFRAMEBUFFERPROC) (GLuint  readFramebuffer, GLuint  drawFramebuffer, GLint  srcX0, GLint  srcY0, GLint  srcX1, GLint  srcY1, GLint  dstX0, GLint  dstY0, GLint  dstX1, GLint  dstY1, GLbitfield  mask, GLenum  filter);
typedef void (*PFNGLBUFFERADDRESSRANGENVPROC) (GLenum  pname, GLuint  index, GLuint64EXT  address, GLsizeiptr  length);
typedef void (*PFNGLBUFFERATTACHMEMORYNVPROC) (GLenum  target, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLBUFFERDATAPROC) (GLenum  target, GLsizeiptr  size, const void * data, GLenum  usage);
typedef void (*PFNGLBUFFERDATAARBPROC) (GLenum  target, GLsizeiptrARB  size, const void * data, GLenum  usage);
typedef void (*PFNGLBUFFERPAGECOMMITMENTARBPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  size, GLboolean  commit);
typedef void (*PFNGLBUFFERPAGECOMMITMENTMEMNVPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  size, GLuint  memory, GLuint64  memOffset, GLboolean  commit);
typedef void (*PFNGLBUFFERPARAMETERIAPPLEPROC) (GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLBUFFERSTORAGEPROC) (GLenum  target, GLsizeiptr  size, const void * data, GLbitfield  flags);
typedef void (*PFNGLBUFFERSTORAGEEXTERNALEXTPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  size, GLeglClientBufferEXT  clientBuffer, GLbitfield  flags);
typedef void (*PFNGLBUFFERSTORAGEMEMEXTPROC) (GLenum  target, GLsizeiptr  size, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLBUFFERSUBDATAPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  size, const void * data);
typedef void (*PFNGLBUFFERSUBDATAARBPROC) (GLenum  target, GLintptrARB  offset, GLsizeiptrARB  size, const void * data);
typedef void (*PFNGLCALLCOMMANDLISTNVPROC) (GLuint  list);
typedef void (*PFNGLCALLLISTPROC) (GLuint  list);
typedef void (*PFNGLCALLLISTSPROC) (GLsizei  n, GLenum  type, const void * lists);
typedef GLenum (*PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum  target);
typedef GLenum (*PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum  target);
typedef GLenum (*PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC) (GLuint  framebuffer, GLenum  target);
typedef GLenum (*PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC) (GLuint  framebuffer, GLenum  target);
typedef void (*PFNGLCLAMPCOLORPROC) (GLenum  target, GLenum  clamp);
typedef void (*PFNGLCLAMPCOLORARBPROC) (GLenum  target, GLenum  clamp);
typedef void (*PFNGLCLEARPROC) (GLbitfield  mask);
typedef void (*PFNGLCLEARACCUMPROC) (GLfloat  red, GLfloat  green, GLfloat  blue, GLfloat  alpha);
typedef void (*PFNGLCLEARBUFFERDATAPROC) (GLenum  target, GLenum  internalformat, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARBUFFERSUBDATAPROC) (GLenum  target, GLenum  internalformat, GLintptr  offset, GLsizeiptr  size, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARBUFFERFIPROC) (GLenum  buffer, GLint  drawbuffer, GLfloat  depth, GLint  stencil);
typedef void (*PFNGLCLEARBUFFERFVPROC) (GLenum  buffer, GLint  drawbuffer, const GLfloat * value);
typedef void (*PFNGLCLEARBUFFERIVPROC) (GLenum  buffer, GLint  drawbuffer, const GLint * value);
typedef void (*PFNGLCLEARBUFFERUIVPROC) (GLenum  buffer, GLint  drawbuffer, const GLuint * value);
typedef void (*PFNGLCLEARCOLORPROC) (GLfloat  red, GLfloat  green, GLfloat  blue, GLfloat  alpha);
typedef void (*PFNGLCLEARCOLORIIEXTPROC) (GLint  red, GLint  green, GLint  blue, GLint  alpha);
typedef void (*PFNGLCLEARCOLORIUIEXTPROC) (GLuint  red, GLuint  green, GLuint  blue, GLuint  alpha);
typedef void (*PFNGLCLEARDEPTHPROC) (GLdouble  depth);
typedef void (*PFNGLCLEARDEPTHDNVPROC) (GLdouble  depth);
typedef void (*PFNGLCLEARDEPTHFPROC) (GLfloat  d);
typedef void (*PFNGLCLEARINDEXPROC) (GLfloat  c);
typedef void (*PFNGLCLEARNAMEDBUFFERDATAPROC) (GLuint  buffer, GLenum  internalformat, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARNAMEDBUFFERDATAEXTPROC) (GLuint  buffer, GLenum  internalformat, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARNAMEDBUFFERSUBDATAPROC) (GLuint  buffer, GLenum  internalformat, GLintptr  offset, GLsizeiptr  size, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC) (GLuint  buffer, GLenum  internalformat, GLsizeiptr  offset, GLsizeiptr  size, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARNAMEDFRAMEBUFFERFIPROC) (GLuint  framebuffer, GLenum  buffer, GLint  drawbuffer, GLfloat  depth, GLint  stencil);
typedef void (*PFNGLCLEARNAMEDFRAMEBUFFERFVPROC) (GLuint  framebuffer, GLenum  buffer, GLint  drawbuffer, const GLfloat * value);
typedef void (*PFNGLCLEARNAMEDFRAMEBUFFERIVPROC) (GLuint  framebuffer, GLenum  buffer, GLint  drawbuffer, const GLint * value);
typedef void (*PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC) (GLuint  framebuffer, GLenum  buffer, GLint  drawbuffer, const GLuint * value);
typedef void (*PFNGLCLEARSTENCILPROC) (GLint  s);
typedef void (*PFNGLCLEARTEXIMAGEPROC) (GLuint  texture, GLint  level, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLEARTEXSUBIMAGEPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCLIENTACTIVETEXTUREPROC) (GLenum  texture);
typedef void (*PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum  texture);
typedef void (*PFNGLCLIENTATTRIBDEFAULTEXTPROC) (GLbitfield  mask);
typedef void (*PFNGLCLIENTWAITSEMAPHOREUI64NVXPROC) (GLsizei  fenceObjectCount, const GLuint * semaphoreArray, const GLuint64 * fenceValueArray);
typedef GLenum (*PFNGLCLIENTWAITSYNCPROC) (GLsync  sync, GLbitfield  flags, GLuint64  timeout);
typedef void (*PFNGLCLIPCONTROLPROC) (GLenum  origin, GLenum  depth);
typedef void (*PFNGLCLIPPLANEPROC) (GLenum  plane, const GLdouble * equation);
typedef void (*PFNGLCOLOR3BPROC) (GLbyte  red, GLbyte  green, GLbyte  blue);
typedef void (*PFNGLCOLOR3BVPROC) (const GLbyte * v);
typedef void (*PFNGLCOLOR3DPROC) (GLdouble  red, GLdouble  green, GLdouble  blue);
typedef void (*PFNGLCOLOR3DVPROC) (const GLdouble * v);
typedef void (*PFNGLCOLOR3FPROC) (GLfloat  red, GLfloat  green, GLfloat  blue);
typedef void (*PFNGLCOLOR3FVPROC) (const GLfloat * v);
typedef void (*PFNGLCOLOR3HNVPROC) (GLhalfNV  red, GLhalfNV  green, GLhalfNV  blue);
typedef void (*PFNGLCOLOR3HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLCOLOR3IPROC) (GLint  red, GLint  green, GLint  blue);
typedef void (*PFNGLCOLOR3IVPROC) (const GLint * v);
typedef void (*PFNGLCOLOR3SPROC) (GLshort  red, GLshort  green, GLshort  blue);
typedef void (*PFNGLCOLOR3SVPROC) (const GLshort * v);
typedef void (*PFNGLCOLOR3UBPROC) (GLubyte  red, GLubyte  green, GLubyte  blue);
typedef void (*PFNGLCOLOR3UBVPROC) (const GLubyte * v);
typedef void (*PFNGLCOLOR3UIPROC) (GLuint  red, GLuint  green, GLuint  blue);
typedef void (*PFNGLCOLOR3UIVPROC) (const GLuint * v);
typedef void (*PFNGLCOLOR3USPROC) (GLushort  red, GLushort  green, GLushort  blue);
typedef void (*PFNGLCOLOR3USVPROC) (const GLushort * v);
typedef void (*PFNGLCOLOR4BPROC) (GLbyte  red, GLbyte  green, GLbyte  blue, GLbyte  alpha);
typedef void (*PFNGLCOLOR4BVPROC) (const GLbyte * v);
typedef void (*PFNGLCOLOR4DPROC) (GLdouble  red, GLdouble  green, GLdouble  blue, GLdouble  alpha);
typedef void (*PFNGLCOLOR4DVPROC) (const GLdouble * v);
typedef void (*PFNGLCOLOR4FPROC) (GLfloat  red, GLfloat  green, GLfloat  blue, GLfloat  alpha);
typedef void (*PFNGLCOLOR4FVPROC) (const GLfloat * v);
typedef void (*PFNGLCOLOR4HNVPROC) (GLhalfNV  red, GLhalfNV  green, GLhalfNV  blue, GLhalfNV  alpha);
typedef void (*PFNGLCOLOR4HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLCOLOR4IPROC) (GLint  red, GLint  green, GLint  blue, GLint  alpha);
typedef void (*PFNGLCOLOR4IVPROC) (const GLint * v);
typedef void (*PFNGLCOLOR4SPROC) (GLshort  red, GLshort  green, GLshort  blue, GLshort  alpha);
typedef void (*PFNGLCOLOR4SVPROC) (const GLshort * v);
typedef void (*PFNGLCOLOR4UBPROC) (GLubyte  red, GLubyte  green, GLubyte  blue, GLubyte  alpha);
typedef void (*PFNGLCOLOR4UBVPROC) (const GLubyte * v);
typedef void (*PFNGLCOLOR4UIPROC) (GLuint  red, GLuint  green, GLuint  blue, GLuint  alpha);
typedef void (*PFNGLCOLOR4UIVPROC) (const GLuint * v);
typedef void (*PFNGLCOLOR4USPROC) (GLushort  red, GLushort  green, GLushort  blue, GLushort  alpha);
typedef void (*PFNGLCOLOR4USVPROC) (const GLushort * v);
typedef void (*PFNGLCOLORFORMATNVPROC) (GLint  size, GLenum  type, GLsizei  stride);
typedef void (*PFNGLCOLORMASKPROC) (GLboolean  red, GLboolean  green, GLboolean  blue, GLboolean  alpha);
typedef void (*PFNGLCOLORMASKINDEXEDEXTPROC) (GLuint  index, GLboolean  r, GLboolean  g, GLboolean  b, GLboolean  a);
typedef void (*PFNGLCOLORMASKIPROC) (GLuint  index, GLboolean  r, GLboolean  g, GLboolean  b, GLboolean  a);
typedef void (*PFNGLCOLORMATERIALPROC) (GLenum  face, GLenum  mode);
typedef void (*PFNGLCOLORP3UIPROC) (GLenum  type, GLuint  color);
typedef void (*PFNGLCOLORP3UIVPROC) (GLenum  type, const GLuint * color);
typedef void (*PFNGLCOLORP4UIPROC) (GLenum  type, GLuint  color);
typedef void (*PFNGLCOLORP4UIVPROC) (GLenum  type, const GLuint * color);
typedef void (*PFNGLCOLORPOINTERPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLCOLORPOINTEREXTPROC) (GLint  size, GLenum  type, GLsizei  stride, GLsizei  count, const void * pointer);
typedef void (*PFNGLCOLORPOINTERVINTELPROC) (GLint  size, GLenum  type, const void ** pointer);
typedef void (*PFNGLCOLORSUBTABLEPROC) (GLenum  target, GLsizei  start, GLsizei  count, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCOLORSUBTABLEEXTPROC) (GLenum  target, GLsizei  start, GLsizei  count, GLenum  format, GLenum  type, const void * data);
typedef void (*PFNGLCOLORTABLEPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLenum  format, GLenum  type, const void * table);
typedef void (*PFNGLCOLORTABLEEXTPROC) (GLenum  target, GLenum  internalFormat, GLsizei  width, GLenum  format, GLenum  type, const void * table);
typedef void (*PFNGLCOLORTABLEPARAMETERFVPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLCOLORTABLEPARAMETERIVPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLCOMBINERINPUTNVPROC) (GLenum  stage, GLenum  portion, GLenum  variable, GLenum  input, GLenum  mapping, GLenum  componentUsage);
typedef void (*PFNGLCOMBINEROUTPUTNVPROC) (GLenum  stage, GLenum  portion, GLenum  abOutput, GLenum  cdOutput, GLenum  sumOutput, GLenum  scale, GLenum  bias, GLboolean  abDotProduct, GLboolean  cdDotProduct, GLboolean  muxSum);
typedef void (*PFNGLCOMBINERPARAMETERFNVPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLCOMBINERPARAMETERFVNVPROC) (GLenum  pname, const GLfloat * params);
typedef void (*PFNGLCOMBINERPARAMETERINVPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLCOMBINERPARAMETERIVNVPROC) (GLenum  pname, const GLint * params);
typedef void (*PFNGLCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum  stage, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLCOMMANDLISTSEGMENTSNVPROC) (GLuint  list, GLuint  segments);
typedef void (*PFNGLCOMPILECOMMANDLISTNVPROC) (GLuint  list);
typedef void (*PFNGLCOMPILESHADERPROC) (GLuint  shader);
typedef void (*PFNGLCOMPILESHADERARBPROC) (GLhandleARB  shaderObj);
typedef void (*PFNGLCOMPILESHADERINCLUDEARBPROC) (GLuint  shader, GLsizei  count, const GLchar *const* path, const GLint * length);
typedef void (*PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLint  border, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDTEXIMAGE1DPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLint  border, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXIMAGE1DARBPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLint  border, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXIMAGE2DPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXIMAGE3DPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXIMAGE3DARBPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC) (GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLint  border, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLsizei  imageSize, const void * data);
typedef void (*PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLsizei  imageSize, const void * bits);
typedef void (*PFNGLCONSERVATIVERASTERPARAMETERFNVPROC) (GLenum  pname, GLfloat  value);
typedef void (*PFNGLCONSERVATIVERASTERPARAMETERINVPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLCONVOLUTIONFILTER1DPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLenum  format, GLenum  type, const void * image);
typedef void (*PFNGLCONVOLUTIONFILTER1DEXTPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLenum  format, GLenum  type, const void * image);
typedef void (*PFNGLCONVOLUTIONFILTER2DPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * image);
typedef void (*PFNGLCONVOLUTIONFILTER2DEXTPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * image);
typedef void (*PFNGLCONVOLUTIONPARAMETERFPROC) (GLenum  target, GLenum  pname, GLfloat  params);
typedef void (*PFNGLCONVOLUTIONPARAMETERFEXTPROC) (GLenum  target, GLenum  pname, GLfloat  params);
typedef void (*PFNGLCONVOLUTIONPARAMETERFVPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLCONVOLUTIONPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLCONVOLUTIONPARAMETERIPROC) (GLenum  target, GLenum  pname, GLint  params);
typedef void (*PFNGLCONVOLUTIONPARAMETERIEXTPROC) (GLenum  target, GLenum  pname, GLint  params);
typedef void (*PFNGLCONVOLUTIONPARAMETERIVPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLCONVOLUTIONPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLCOPYBUFFERSUBDATAPROC) (GLenum  readTarget, GLenum  writeTarget, GLintptr  readOffset, GLintptr  writeOffset, GLsizeiptr  size);
typedef void (*PFNGLCOPYCOLORSUBTABLEPROC) (GLenum  target, GLsizei  start, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYCOLORSUBTABLEEXTPROC) (GLenum  target, GLsizei  start, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYCOLORTABLEPROC) (GLenum  target, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYCONVOLUTIONFILTER1DPROC) (GLenum  target, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYCONVOLUTIONFILTER1DEXTPROC) (GLenum  target, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYCONVOLUTIONFILTER2DPROC) (GLenum  target, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYCONVOLUTIONFILTER2DEXTPROC) (GLenum  target, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYIMAGESUBDATAPROC) (GLuint  srcName, GLenum  srcTarget, GLint  srcLevel, GLint  srcX, GLint  srcY, GLint  srcZ, GLuint  dstName, GLenum  dstTarget, GLint  dstLevel, GLint  dstX, GLint  dstY, GLint  dstZ, GLsizei  srcWidth, GLsizei  srcHeight, GLsizei  srcDepth);
typedef void (*PFNGLCOPYIMAGESUBDATANVPROC) (GLuint  srcName, GLenum  srcTarget, GLint  srcLevel, GLint  srcX, GLint  srcY, GLint  srcZ, GLuint  dstName, GLenum  dstTarget, GLint  dstLevel, GLint  dstX, GLint  dstY, GLint  dstZ, GLsizei  width, GLsizei  height, GLsizei  depth);
typedef void (*PFNGLCOPYMULTITEXIMAGE1DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLint  border);
typedef void (*PFNGLCOPYMULTITEXIMAGE2DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLint  border);
typedef void (*PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYNAMEDBUFFERSUBDATAPROC) (GLuint  readBuffer, GLuint  writeBuffer, GLintptr  readOffset, GLintptr  writeOffset, GLsizeiptr  size);
typedef void (*PFNGLCOPYPATHNVPROC) (GLuint  resultPath, GLuint  srcPath);
typedef void (*PFNGLCOPYPIXELSPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLenum  type);
typedef void (*PFNGLCOPYTEXIMAGE1DPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLint  border);
typedef void (*PFNGLCOPYTEXIMAGE1DEXTPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLint  border);
typedef void (*PFNGLCOPYTEXIMAGE2DPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLint  border);
typedef void (*PFNGLCOPYTEXIMAGE2DEXTPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLint  border);
typedef void (*PFNGLCOPYTEXSUBIMAGE1DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYTEXSUBIMAGE1DEXTPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYTEXSUBIMAGE2DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXSUBIMAGE2DEXTPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXSUBIMAGE3DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXSUBIMAGE3DEXTPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXTUREIMAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLint  border);
typedef void (*PFNGLCOPYTEXTUREIMAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  internalformat, GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLint  border);
typedef void (*PFNGLCOPYTEXTURESUBIMAGE1DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  x, GLint  y, GLsizei  width);
typedef void (*PFNGLCOPYTEXTURESUBIMAGE2DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXTURESUBIMAGE3DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLCOVERFILLPATHINSTANCEDNVPROC) (GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLenum  coverMode, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLCOVERFILLPATHNVPROC) (GLuint  path, GLenum  coverMode);
typedef void (*PFNGLCOVERSTROKEPATHINSTANCEDNVPROC) (GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLenum  coverMode, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLCOVERSTROKEPATHNVPROC) (GLuint  path, GLenum  coverMode);
typedef void (*PFNGLCOVERAGEMODULATIONNVPROC) (GLenum  components);
typedef void (*PFNGLCOVERAGEMODULATIONTABLENVPROC) (GLsizei  n, const GLfloat * v);
typedef void (*PFNGLCREATEBUFFERSPROC) (GLsizei  n, GLuint * buffers);
typedef void (*PFNGLCREATECOMMANDLISTSNVPROC) (GLsizei  n, GLuint * lists);
typedef void (*PFNGLCREATEFRAMEBUFFERSPROC) (GLsizei  n, GLuint * framebuffers);
typedef void (*PFNGLCREATEMEMORYOBJECTSEXTPROC) (GLsizei  n, GLuint * memoryObjects);
typedef void (*PFNGLCREATEPERFQUERYINTELPROC) (GLuint  queryId, GLuint * queryHandle);
typedef GLuint (*PFNGLCREATEPROGRAMPROC) ();
typedef GLhandleARB (*PFNGLCREATEPROGRAMOBJECTARBPROC) ();
typedef void (*PFNGLCREATEPROGRAMPIPELINESPROC) (GLsizei  n, GLuint * pipelines);
typedef GLuint (*PFNGLCREATEPROGRESSFENCENVXPROC) ();
typedef void (*PFNGLCREATEQUERIESPROC) (GLenum  target, GLsizei  n, GLuint * ids);
typedef void (*PFNGLCREATERENDERBUFFERSPROC) (GLsizei  n, GLuint * renderbuffers);
typedef void (*PFNGLCREATESAMPLERSPROC) (GLsizei  n, GLuint * samplers);
typedef void (*PFNGLCREATESEMAPHORESNVPROC) (GLsizei  n, GLuint * semaphores);
typedef GLuint (*PFNGLCREATESHADERPROC) (GLenum  type);
typedef GLhandleARB (*PFNGLCREATESHADEROBJECTARBPROC) (GLenum  shaderType);
typedef GLuint (*PFNGLCREATESHADERPROGRAMEXTPROC) (GLenum  type, const GLchar * string);
typedef GLuint (*PFNGLCREATESHADERPROGRAMVPROC) (GLenum  type, GLsizei  count, const GLchar *const* strings);
typedef GLuint (*PFNGLCREATESHADERPROGRAMVEXTPROC) (GLenum  type, GLsizei  count, const GLchar ** strings);
typedef void (*PFNGLCREATESTATESNVPROC) (GLsizei  n, GLuint * states);
typedef GLsync (*PFNGLCREATESYNCFROMCLEVENTARBPROC) (struct _cl_context * context, struct _cl_event * event, GLbitfield  flags);
typedef void (*PFNGLCREATETEXTURESPROC) (GLenum  target, GLsizei  n, GLuint * textures);
typedef void (*PFNGLCREATETRANSFORMFEEDBACKSPROC) (GLsizei  n, GLuint * ids);
typedef void (*PFNGLCREATEVERTEXARRAYSPROC) (GLsizei  n, GLuint * arrays);
typedef void (*PFNGLCULLFACEPROC) (GLenum  mode);
typedef void (*PFNGLCULLPARAMETERDVEXTPROC) (GLenum  pname, GLdouble * params);
typedef void (*PFNGLCULLPARAMETERFVEXTPROC) (GLenum  pname, GLfloat * params);
typedef void (*PFNGLCURRENTPALETTEMATRIXARBPROC) (GLint  index);
typedef void (*PFNGLDEBUGMESSAGECALLBACKPROC) (GLDEBUGPROC  callback, const void * userParam);
typedef void (*PFNGLDEBUGMESSAGECALLBACKAMDPROC) (GLDEBUGPROCAMD  callback, void * userParam);
typedef void (*PFNGLDEBUGMESSAGECALLBACKARBPROC) (GLDEBUGPROCARB  callback, const void * userParam);
typedef void (*PFNGLDEBUGMESSAGECALLBACKKHRPROC) (GLDEBUGPROCKHR  callback, const void * userParam);
typedef void (*PFNGLDEBUGMESSAGECONTROLPROC) (GLenum  source, GLenum  type, GLenum  severity, GLsizei  count, const GLuint * ids, GLboolean  enabled);
typedef void (*PFNGLDEBUGMESSAGECONTROLARBPROC) (GLenum  source, GLenum  type, GLenum  severity, GLsizei  count, const GLuint * ids, GLboolean  enabled);
typedef void (*PFNGLDEBUGMESSAGECONTROLKHRPROC) (GLenum  source, GLenum  type, GLenum  severity, GLsizei  count, const GLuint * ids, GLboolean  enabled);
typedef void (*PFNGLDEBUGMESSAGEENABLEAMDPROC) (GLenum  category, GLenum  severity, GLsizei  count, const GLuint * ids, GLboolean  enabled);
typedef void (*PFNGLDEBUGMESSAGEINSERTPROC) (GLenum  source, GLenum  type, GLuint  id, GLenum  severity, GLsizei  length, const GLchar * buf);
typedef void (*PFNGLDEBUGMESSAGEINSERTAMDPROC) (GLenum  category, GLenum  severity, GLuint  id, GLsizei  length, const GLchar * buf);
typedef void (*PFNGLDEBUGMESSAGEINSERTARBPROC) (GLenum  source, GLenum  type, GLuint  id, GLenum  severity, GLsizei  length, const GLchar * buf);
typedef void (*PFNGLDEBUGMESSAGEINSERTKHRPROC) (GLenum  source, GLenum  type, GLuint  id, GLenum  severity, GLsizei  length, const GLchar * buf);
typedef void (*PFNGLDELETEBUFFERSPROC) (GLsizei  n, const GLuint * buffers);
typedef void (*PFNGLDELETEBUFFERSARBPROC) (GLsizei  n, const GLuint * buffers);
typedef void (*PFNGLDELETECOMMANDLISTSNVPROC) (GLsizei  n, const GLuint * lists);
typedef void (*PFNGLDELETEFENCESAPPLEPROC) (GLsizei  n, const GLuint * fences);
typedef void (*PFNGLDELETEFENCESNVPROC) (GLsizei  n, const GLuint * fences);
typedef void (*PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei  n, const GLuint * framebuffers);
typedef void (*PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei  n, const GLuint * framebuffers);
typedef void (*PFNGLDELETELISTSPROC) (GLuint  list, GLsizei  range);
typedef void (*PFNGLDELETEMEMORYOBJECTSEXTPROC) (GLsizei  n, const GLuint * memoryObjects);
typedef void (*PFNGLDELETENAMEDSTRINGARBPROC) (GLint  namelen, const GLchar * name);
typedef void (*PFNGLDELETENAMESAMDPROC) (GLenum  identifier, GLuint  num, const GLuint * names);
typedef void (*PFNGLDELETEOBJECTARBPROC) (GLhandleARB  obj);
typedef void (*PFNGLDELETEOCCLUSIONQUERIESNVPROC) (GLsizei  n, const GLuint * ids);
typedef void (*PFNGLDELETEPATHSNVPROC) (GLuint  path, GLsizei  range);
typedef void (*PFNGLDELETEPERFMONITORSAMDPROC) (GLsizei  n, GLuint * monitors);
typedef void (*PFNGLDELETEPERFQUERYINTELPROC) (GLuint  queryHandle);
typedef void (*PFNGLDELETEPROGRAMPROC) (GLuint  program);
typedef void (*PFNGLDELETEPROGRAMPIPELINESPROC) (GLsizei  n, const GLuint * pipelines);
typedef void (*PFNGLDELETEPROGRAMPIPELINESEXTPROC) (GLsizei  n, const GLuint * pipelines);
typedef void (*PFNGLDELETEPROGRAMSARBPROC) (GLsizei  n, const GLuint * programs);
typedef void (*PFNGLDELETEPROGRAMSNVPROC) (GLsizei  n, const GLuint * programs);
typedef void (*PFNGLDELETEQUERIESPROC) (GLsizei  n, const GLuint * ids);
typedef void (*PFNGLDELETEQUERIESARBPROC) (GLsizei  n, const GLuint * ids);
typedef void (*PFNGLDELETEQUERYRESOURCETAGNVPROC) (GLsizei  n, const GLint * tagIds);
typedef void (*PFNGLDELETERENDERBUFFERSPROC) (GLsizei  n, const GLuint * renderbuffers);
typedef void (*PFNGLDELETERENDERBUFFERSEXTPROC) (GLsizei  n, const GLuint * renderbuffers);
typedef void (*PFNGLDELETESAMPLERSPROC) (GLsizei  count, const GLuint * samplers);
typedef void (*PFNGLDELETESEMAPHORESEXTPROC) (GLsizei  n, const GLuint * semaphores);
typedef void (*PFNGLDELETESHADERPROC) (GLuint  shader);
typedef void (*PFNGLDELETESTATESNVPROC) (GLsizei  n, const GLuint * states);
typedef void (*PFNGLDELETESYNCPROC) (GLsync  sync);
typedef void (*PFNGLDELETETEXTURESPROC) (GLsizei  n, const GLuint * textures);
typedef void (*PFNGLDELETETEXTURESEXTPROC) (GLsizei  n, const GLuint * textures);
typedef void (*PFNGLDELETETRANSFORMFEEDBACKSPROC) (GLsizei  n, const GLuint * ids);
typedef void (*PFNGLDELETETRANSFORMFEEDBACKSNVPROC) (GLsizei  n, const GLuint * ids);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC) (GLsizei  n, const GLuint * arrays);
typedef void (*PFNGLDELETEVERTEXARRAYSAPPLEPROC) (GLsizei  n, const GLuint * arrays);
typedef void (*PFNGLDELETEVERTEXSHADEREXTPROC) (GLuint  id);
typedef void (*PFNGLDEPTHBOUNDSEXTPROC) (GLclampd  zmin, GLclampd  zmax);
typedef void (*PFNGLDEPTHBOUNDSDNVPROC) (GLdouble  zmin, GLdouble  zmax);
typedef void (*PFNGLDEPTHFUNCPROC) (GLenum  func);
typedef void (*PFNGLDEPTHMASKPROC) (GLboolean  flag);
typedef void (*PFNGLDEPTHRANGEPROC) (GLdouble  n, GLdouble  f);
typedef void (*PFNGLDEPTHRANGEARRAYDVNVPROC) (GLuint  first, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLDEPTHRANGEARRAYVPROC) (GLuint  first, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLDEPTHRANGEINDEXEDPROC) (GLuint  index, GLdouble  n, GLdouble  f);
typedef void (*PFNGLDEPTHRANGEINDEXEDDNVPROC) (GLuint  index, GLdouble  n, GLdouble  f);
typedef void (*PFNGLDEPTHRANGEDNVPROC) (GLdouble  zNear, GLdouble  zFar);
typedef void (*PFNGLDEPTHRANGEFPROC) (GLfloat  n, GLfloat  f);
typedef void (*PFNGLDETACHOBJECTARBPROC) (GLhandleARB  containerObj, GLhandleARB  attachedObj);
typedef void (*PFNGLDETACHSHADERPROC) (GLuint  program, GLuint  shader);
typedef void (*PFNGLDISABLEPROC) (GLenum  cap);
typedef void (*PFNGLDISABLECLIENTSTATEPROC) (GLenum  array);
typedef void (*PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC) (GLenum  array, GLuint  index);
typedef void (*PFNGLDISABLECLIENTSTATEIEXTPROC) (GLenum  array, GLuint  index);
typedef void (*PFNGLDISABLEINDEXEDEXTPROC) (GLenum  target, GLuint  index);
typedef void (*PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC) (GLuint  id);
typedef void (*PFNGLDISABLEVERTEXARRAYATTRIBPROC) (GLuint  vaobj, GLuint  index);
typedef void (*PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC) (GLuint  vaobj, GLuint  index);
typedef void (*PFNGLDISABLEVERTEXARRAYEXTPROC) (GLuint  vaobj, GLenum  array);
typedef void (*PFNGLDISABLEVERTEXATTRIBAPPLEPROC) (GLuint  index, GLenum  pname);
typedef void (*PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint  index);
typedef void (*PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint  index);
typedef void (*PFNGLDISABLEIPROC) (GLenum  target, GLuint  index);
typedef void (*PFNGLDISPATCHCOMPUTEPROC) (GLuint  num_groups_x, GLuint  num_groups_y, GLuint  num_groups_z);
typedef void (*PFNGLDISPATCHCOMPUTEGROUPSIZEARBPROC) (GLuint  num_groups_x, GLuint  num_groups_y, GLuint  num_groups_z, GLuint  group_size_x, GLuint  group_size_y, GLuint  group_size_z);
typedef void (*PFNGLDISPATCHCOMPUTEINDIRECTPROC) (GLintptr  indirect);
typedef void (*PFNGLDRAWARRAYSPROC) (GLenum  mode, GLint  first, GLsizei  count);
typedef void (*PFNGLDRAWARRAYSEXTPROC) (GLenum  mode, GLint  first, GLsizei  count);
typedef void (*PFNGLDRAWARRAYSINDIRECTPROC) (GLenum  mode, const void * indirect);
typedef void (*PFNGLDRAWARRAYSINSTANCEDPROC) (GLenum  mode, GLint  first, GLsizei  count, GLsizei  instancecount);
typedef void (*PFNGLDRAWARRAYSINSTANCEDARBPROC) (GLenum  mode, GLint  first, GLsizei  count, GLsizei  primcount);
typedef void (*PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC) (GLenum  mode, GLint  first, GLsizei  count, GLsizei  instancecount, GLuint  baseinstance);
typedef void (*PFNGLDRAWARRAYSINSTANCEDEXTPROC) (GLenum  mode, GLint  start, GLsizei  count, GLsizei  primcount);
typedef void (*PFNGLDRAWBUFFERPROC) (GLenum  buf);
typedef void (*PFNGLDRAWBUFFERSPROC) (GLsizei  n, const GLenum * bufs);
typedef void (*PFNGLDRAWBUFFERSARBPROC) (GLsizei  n, const GLenum * bufs);
typedef void (*PFNGLDRAWCOMMANDSADDRESSNVPROC) (GLenum  primitiveMode, const GLuint64 * indirects, const GLsizei * sizes, GLuint  count);
typedef void (*PFNGLDRAWCOMMANDSNVPROC) (GLenum  primitiveMode, GLuint  buffer, const GLintptr * indirects, const GLsizei * sizes, GLuint  count);
typedef void (*PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC) (const GLuint64 * indirects, const GLsizei * sizes, const GLuint * states, const GLuint * fbos, GLuint  count);
typedef void (*PFNGLDRAWCOMMANDSSTATESNVPROC) (GLuint  buffer, const GLintptr * indirects, const GLsizei * sizes, const GLuint * states, const GLuint * fbos, GLuint  count);
typedef void (*PFNGLDRAWELEMENTARRAYAPPLEPROC) (GLenum  mode, GLint  first, GLsizei  count);
typedef void (*PFNGLDRAWELEMENTSPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices);
typedef void (*PFNGLDRAWELEMENTSBASEVERTEXPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLint  basevertex);
typedef void (*PFNGLDRAWELEMENTSINDIRECTPROC) (GLenum  mode, GLenum  type, const void * indirect);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLsizei  instancecount);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDARBPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLsizei  primcount);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLsizei  instancecount, GLuint  baseinstance);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLsizei  instancecount, GLint  basevertex);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLsizei  instancecount, GLint  basevertex, GLuint  baseinstance);
typedef void (*PFNGLDRAWELEMENTSINSTANCEDEXTPROC) (GLenum  mode, GLsizei  count, GLenum  type, const void * indices, GLsizei  primcount);
typedef void (*PFNGLDRAWMESHTASKSNVPROC) (GLuint  first, GLuint  count);
typedef void (*PFNGLDRAWMESHTASKSINDIRECTNVPROC) (GLintptr  indirect);
typedef void (*PFNGLDRAWPIXELSPROC) (GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLDRAWRANGEELEMENTARRAYAPPLEPROC) (GLenum  mode, GLuint  start, GLuint  end, GLint  first, GLsizei  count);
typedef void (*PFNGLDRAWRANGEELEMENTSPROC) (GLenum  mode, GLuint  start, GLuint  end, GLsizei  count, GLenum  type, const void * indices);
typedef void (*PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) (GLenum  mode, GLuint  start, GLuint  end, GLsizei  count, GLenum  type, const void * indices, GLint  basevertex);
typedef void (*PFNGLDRAWRANGEELEMENTSEXTPROC) (GLenum  mode, GLuint  start, GLuint  end, GLsizei  count, GLenum  type, const void * indices);
typedef void (*PFNGLDRAWTEXTURENVPROC) (GLuint  texture, GLuint  sampler, GLfloat  x0, GLfloat  y0, GLfloat  x1, GLfloat  y1, GLfloat  z, GLfloat  s0, GLfloat  t0, GLfloat  s1, GLfloat  t1);
typedef void (*PFNGLDRAWTRANSFORMFEEDBACKPROC) (GLenum  mode, GLuint  id);
typedef void (*PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC) (GLenum  mode, GLuint  id, GLsizei  instancecount);
typedef void (*PFNGLDRAWTRANSFORMFEEDBACKNVPROC) (GLenum  mode, GLuint  id);
typedef void (*PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC) (GLenum  mode, GLuint  id, GLuint  stream);
typedef void (*PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC) (GLenum  mode, GLuint  id, GLuint  stream, GLsizei  instancecount);
typedef void (*PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC) (GLenum  target, GLeglImageOES  image, const GLint*  attrib_list);
typedef void (*PFNGLEGLIMAGETARGETTEXTURESTORAGEEXTPROC) (GLuint  texture, GLeglImageOES  image, const GLint*  attrib_list);
typedef void (*PFNGLEDGEFLAGPROC) (GLboolean  flag);
typedef void (*PFNGLEDGEFLAGFORMATNVPROC) (GLsizei  stride);
typedef void (*PFNGLEDGEFLAGPOINTERPROC) (GLsizei  stride, const void * pointer);
typedef void (*PFNGLEDGEFLAGPOINTEREXTPROC) (GLsizei  stride, GLsizei  count, const GLboolean * pointer);
typedef void (*PFNGLEDGEFLAGVPROC) (const GLboolean * flag);
typedef void (*PFNGLELEMENTPOINTERAPPLEPROC) (GLenum  type, const void * pointer);
typedef void (*PFNGLENABLEPROC) (GLenum  cap);
typedef void (*PFNGLENABLECLIENTSTATEPROC) (GLenum  array);
typedef void (*PFNGLENABLECLIENTSTATEINDEXEDEXTPROC) (GLenum  array, GLuint  index);
typedef void (*PFNGLENABLECLIENTSTATEIEXTPROC) (GLenum  array, GLuint  index);
typedef void (*PFNGLENABLEINDEXEDEXTPROC) (GLenum  target, GLuint  index);
typedef void (*PFNGLENABLEVARIANTCLIENTSTATEEXTPROC) (GLuint  id);
typedef void (*PFNGLENABLEVERTEXARRAYATTRIBPROC) (GLuint  vaobj, GLuint  index);
typedef void (*PFNGLENABLEVERTEXARRAYATTRIBEXTPROC) (GLuint  vaobj, GLuint  index);
typedef void (*PFNGLENABLEVERTEXARRAYEXTPROC) (GLuint  vaobj, GLenum  array);
typedef void (*PFNGLENABLEVERTEXATTRIBAPPLEPROC) (GLuint  index, GLenum  pname);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint  index);
typedef void (*PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint  index);
typedef void (*PFNGLENABLEIPROC) (GLenum  target, GLuint  index);
typedef void (*PFNGLENDPROC) ();
typedef void (*PFNGLENDCONDITIONALRENDERPROC) ();
typedef void (*PFNGLENDCONDITIONALRENDERNVPROC) ();
typedef void (*PFNGLENDCONDITIONALRENDERNVXPROC) ();
typedef void (*PFNGLENDLISTPROC) ();
typedef void (*PFNGLENDOCCLUSIONQUERYNVPROC) ();
typedef void (*PFNGLENDPERFMONITORAMDPROC) (GLuint  monitor);
typedef void (*PFNGLENDPERFQUERYINTELPROC) (GLuint  queryHandle);
typedef void (*PFNGLENDQUERYPROC) (GLenum  target);
typedef void (*PFNGLENDQUERYARBPROC) (GLenum  target);
typedef void (*PFNGLENDQUERYINDEXEDPROC) (GLenum  target, GLuint  index);
typedef void (*PFNGLENDTRANSFORMFEEDBACKPROC) ();
typedef void (*PFNGLENDTRANSFORMFEEDBACKEXTPROC) ();
typedef void (*PFNGLENDTRANSFORMFEEDBACKNVPROC) ();
typedef void (*PFNGLENDVERTEXSHADEREXTPROC) ();
typedef void (*PFNGLENDVIDEOCAPTURENVPROC) (GLuint  video_capture_slot);
typedef void (*PFNGLEVALCOORD1DPROC) (GLdouble  u);
typedef void (*PFNGLEVALCOORD1DVPROC) (const GLdouble * u);
typedef void (*PFNGLEVALCOORD1FPROC) (GLfloat  u);
typedef void (*PFNGLEVALCOORD1FVPROC) (const GLfloat * u);
typedef void (*PFNGLEVALCOORD2DPROC) (GLdouble  u, GLdouble  v);
typedef void (*PFNGLEVALCOORD2DVPROC) (const GLdouble * u);
typedef void (*PFNGLEVALCOORD2FPROC) (GLfloat  u, GLfloat  v);
typedef void (*PFNGLEVALCOORD2FVPROC) (const GLfloat * u);
typedef void (*PFNGLEVALMAPSNVPROC) (GLenum  target, GLenum  mode);
typedef void (*PFNGLEVALMESH1PROC) (GLenum  mode, GLint  i1, GLint  i2);
typedef void (*PFNGLEVALMESH2PROC) (GLenum  mode, GLint  i1, GLint  i2, GLint  j1, GLint  j2);
typedef void (*PFNGLEVALPOINT1PROC) (GLint  i);
typedef void (*PFNGLEVALPOINT2PROC) (GLint  i, GLint  j);
typedef void (*PFNGLEVALUATEDEPTHVALUESARBPROC) ();
typedef void (*PFNGLEXECUTEPROGRAMNVPROC) (GLenum  target, GLuint  id, const GLfloat * params);
typedef void (*PFNGLEXTRACTCOMPONENTEXTPROC) (GLuint  res, GLuint  src, GLuint  num);
typedef void (*PFNGLFEEDBACKBUFFERPROC) (GLsizei  size, GLenum  type, GLfloat * buffer);
typedef GLsync (*PFNGLFENCESYNCPROC) (GLenum  condition, GLbitfield  flags);
typedef void (*PFNGLFINALCOMBINERINPUTNVPROC) (GLenum  variable, GLenum  input, GLenum  mapping, GLenum  componentUsage);
typedef void (*PFNGLFINISHPROC) ();
typedef void (*PFNGLFINISHFENCEAPPLEPROC) (GLuint  fence);
typedef void (*PFNGLFINISHFENCENVPROC) (GLuint  fence);
typedef void (*PFNGLFINISHOBJECTAPPLEPROC) (GLenum  object, GLint  name);
typedef void (*PFNGLFLUSHPROC) ();
typedef void (*PFNGLFLUSHMAPPEDBUFFERRANGEPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  length);
typedef void (*PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  length);
typedef void (*PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  length);
typedef void (*PFNGLFLUSHPIXELDATARANGENVPROC) (GLenum  target);
typedef void (*PFNGLFLUSHVERTEXARRAYRANGEAPPLEPROC) (GLsizei  length, void * pointer);
typedef void (*PFNGLFLUSHVERTEXARRAYRANGENVPROC) ();
typedef void (*PFNGLFOGCOORDFORMATNVPROC) (GLenum  type, GLsizei  stride);
typedef void (*PFNGLFOGCOORDPOINTERPROC) (GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLFOGCOORDPOINTEREXTPROC) (GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLFOGCOORDDPROC) (GLdouble  coord);
typedef void (*PFNGLFOGCOORDDEXTPROC) (GLdouble  coord);
typedef void (*PFNGLFOGCOORDDVPROC) (const GLdouble * coord);
typedef void (*PFNGLFOGCOORDDVEXTPROC) (const GLdouble * coord);
typedef void (*PFNGLFOGCOORDFPROC) (GLfloat  coord);
typedef void (*PFNGLFOGCOORDFEXTPROC) (GLfloat  coord);
typedef void (*PFNGLFOGCOORDFVPROC) (const GLfloat * coord);
typedef void (*PFNGLFOGCOORDFVEXTPROC) (const GLfloat * coord);
typedef void (*PFNGLFOGCOORDHNVPROC) (GLhalfNV  fog);
typedef void (*PFNGLFOGCOORDHVNVPROC) (const GLhalfNV * fog);
typedef void (*PFNGLFOGFPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLFOGFVPROC) (GLenum  pname, const GLfloat * params);
typedef void (*PFNGLFOGIPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLFOGIVPROC) (GLenum  pname, const GLint * params);
typedef void (*PFNGLFRAGMENTCOVERAGECOLORNVPROC) (GLuint  color);
typedef void (*PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC) (GLuint  framebuffer, GLenum  mode);
typedef void (*PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC) (GLuint  framebuffer, GLsizei  n, const GLenum * bufs);
typedef void (*PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC) ();
typedef void (*PFNGLFRAMEBUFFERPARAMETERIPROC) (GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLFRAMEBUFFERREADBUFFEREXTPROC) (GLuint  framebuffer, GLenum  mode);
typedef void (*PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum  target, GLenum  attachment, GLenum  renderbuffertarget, GLuint  renderbuffer);
typedef void (*PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum  target, GLenum  attachment, GLenum  renderbuffertarget, GLuint  renderbuffer);
typedef void (*PFNGLFRAMEBUFFERSAMPLELOCATIONSFVARBPROC) (GLenum  target, GLuint  start, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC) (GLenum  target, GLuint  start, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC) (GLenum  target, GLuint  numsamples, GLuint  pixelindex, const GLfloat * values);
typedef void (*PFNGLFRAMEBUFFERTEXTUREPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTURE1DPROC) (GLenum  target, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) (GLenum  target, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum  target, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum  target, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTURE3DPROC) (GLenum  target, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level, GLint  zoffset);
typedef void (*PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) (GLenum  target, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level, GLint  zoffset);
typedef void (*PFNGLFRAMEBUFFERTEXTUREARBPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTUREEXTPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level);
typedef void (*PFNGLFRAMEBUFFERTEXTUREFACEARBPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level, GLenum  face);
typedef void (*PFNGLFRAMEBUFFERTEXTUREFACEEXTPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level, GLenum  face);
typedef void (*PFNGLFRAMEBUFFERTEXTURELAYERPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level, GLint  layer);
typedef void (*PFNGLFRAMEBUFFERTEXTURELAYERARBPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level, GLint  layer);
typedef void (*PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC) (GLenum  target, GLenum  attachment, GLuint  texture, GLint  level, GLint  layer);
typedef void (*PFNGLFRONTFACEPROC) (GLenum  mode);
typedef void (*PFNGLFRUSTUMPROC) (GLdouble  left, GLdouble  right, GLdouble  bottom, GLdouble  top, GLdouble  zNear, GLdouble  zFar);
typedef void (*PFNGLGENBUFFERSPROC) (GLsizei  n, GLuint * buffers);
typedef void (*PFNGLGENBUFFERSARBPROC) (GLsizei  n, GLuint * buffers);
typedef void (*PFNGLGENFENCESAPPLEPROC) (GLsizei  n, GLuint * fences);
typedef void (*PFNGLGENFENCESNVPROC) (GLsizei  n, GLuint * fences);
typedef void (*PFNGLGENFRAMEBUFFERSPROC) (GLsizei  n, GLuint * framebuffers);
typedef void (*PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei  n, GLuint * framebuffers);
typedef GLuint (*PFNGLGENLISTSPROC) (GLsizei  range);
typedef void (*PFNGLGENNAMESAMDPROC) (GLenum  identifier, GLuint  num, GLuint * names);
typedef void (*PFNGLGENOCCLUSIONQUERIESNVPROC) (GLsizei  n, GLuint * ids);
typedef GLuint (*PFNGLGENPATHSNVPROC) (GLsizei  range);
typedef void (*PFNGLGENPERFMONITORSAMDPROC) (GLsizei  n, GLuint * monitors);
typedef void (*PFNGLGENPROGRAMPIPELINESPROC) (GLsizei  n, GLuint * pipelines);
typedef void (*PFNGLGENPROGRAMPIPELINESEXTPROC) (GLsizei  n, GLuint * pipelines);
typedef void (*PFNGLGENPROGRAMSARBPROC) (GLsizei  n, GLuint * programs);
typedef void (*PFNGLGENPROGRAMSNVPROC) (GLsizei  n, GLuint * programs);
typedef void (*PFNGLGENQUERIESPROC) (GLsizei  n, GLuint * ids);
typedef void (*PFNGLGENQUERIESARBPROC) (GLsizei  n, GLuint * ids);
typedef void (*PFNGLGENQUERYRESOURCETAGNVPROC) (GLsizei  n, GLint * tagIds);
typedef void (*PFNGLGENRENDERBUFFERSPROC) (GLsizei  n, GLuint * renderbuffers);
typedef void (*PFNGLGENRENDERBUFFERSEXTPROC) (GLsizei  n, GLuint * renderbuffers);
typedef void (*PFNGLGENSAMPLERSPROC) (GLsizei  count, GLuint * samplers);
typedef void (*PFNGLGENSEMAPHORESEXTPROC) (GLsizei  n, GLuint * semaphores);
typedef GLuint (*PFNGLGENSYMBOLSEXTPROC) (GLenum  datatype, GLenum  storagetype, GLenum  range, GLuint  components);
typedef void (*PFNGLGENTEXTURESPROC) (GLsizei  n, GLuint * textures);
typedef void (*PFNGLGENTEXTURESEXTPROC) (GLsizei  n, GLuint * textures);
typedef void (*PFNGLGENTRANSFORMFEEDBACKSPROC) (GLsizei  n, GLuint * ids);
typedef void (*PFNGLGENTRANSFORMFEEDBACKSNVPROC) (GLsizei  n, GLuint * ids);
typedef void (*PFNGLGENVERTEXARRAYSPROC) (GLsizei  n, GLuint * arrays);
typedef void (*PFNGLGENVERTEXARRAYSAPPLEPROC) (GLsizei  n, GLuint * arrays);
typedef GLuint (*PFNGLGENVERTEXSHADERSEXTPROC) (GLuint  range);
typedef void (*PFNGLGENERATEMIPMAPPROC) (GLenum  target);
typedef void (*PFNGLGENERATEMIPMAPEXTPROC) (GLenum  target);
typedef void (*PFNGLGENERATEMULTITEXMIPMAPEXTPROC) (GLenum  texunit, GLenum  target);
typedef void (*PFNGLGENERATETEXTUREMIPMAPPROC) (GLuint  texture);
typedef void (*PFNGLGENERATETEXTUREMIPMAPEXTPROC) (GLuint  texture, GLenum  target);
typedef void (*PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC) (GLuint  program, GLuint  bufferIndex, GLenum  pname, GLint * params);
typedef void (*PFNGLGETACTIVEATTRIBPROC) (GLuint  program, GLuint  index, GLsizei  bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name);
typedef void (*PFNGLGETACTIVEATTRIBARBPROC) (GLhandleARB  programObj, GLuint  index, GLsizei  maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name);
typedef void (*PFNGLGETACTIVESUBROUTINENAMEPROC) (GLuint  program, GLenum  shadertype, GLuint  index, GLsizei  bufSize, GLsizei * length, GLchar * name);
typedef void (*PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC) (GLuint  program, GLenum  shadertype, GLuint  index, GLsizei  bufSize, GLsizei * length, GLchar * name);
typedef void (*PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC) (GLuint  program, GLenum  shadertype, GLuint  index, GLenum  pname, GLint * values);
typedef void (*PFNGLGETACTIVEUNIFORMPROC) (GLuint  program, GLuint  index, GLsizei  bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name);
typedef void (*PFNGLGETACTIVEUNIFORMARBPROC) (GLhandleARB  programObj, GLuint  index, GLsizei  maxLength, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name);
typedef void (*PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) (GLuint  program, GLuint  uniformBlockIndex, GLsizei  bufSize, GLsizei * length, GLchar * uniformBlockName);
typedef void (*PFNGLGETACTIVEUNIFORMBLOCKIVPROC) (GLuint  program, GLuint  uniformBlockIndex, GLenum  pname, GLint * params);
typedef void (*PFNGLGETACTIVEUNIFORMNAMEPROC) (GLuint  program, GLuint  uniformIndex, GLsizei  bufSize, GLsizei * length, GLchar * uniformName);
typedef void (*PFNGLGETACTIVEUNIFORMSIVPROC) (GLuint  program, GLsizei  uniformCount, const GLuint * uniformIndices, GLenum  pname, GLint * params);
typedef void (*PFNGLGETACTIVEVARYINGNVPROC) (GLuint  program, GLuint  index, GLsizei  bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name);
typedef void (*PFNGLGETATTACHEDOBJECTSARBPROC) (GLhandleARB  containerObj, GLsizei  maxCount, GLsizei * count, GLhandleARB * obj);
typedef void (*PFNGLGETATTACHEDSHADERSPROC) (GLuint  program, GLsizei  maxCount, GLsizei * count, GLuint * shaders);
typedef GLint (*PFNGLGETATTRIBLOCATIONPROC) (GLuint  program, const GLchar * name);
typedef GLint (*PFNGLGETATTRIBLOCATIONARBPROC) (GLhandleARB  programObj, const GLcharARB * name);
typedef void (*PFNGLGETBOOLEANINDEXEDVEXTPROC) (GLenum  target, GLuint  index, GLboolean * data);
typedef void (*PFNGLGETBOOLEANI_VPROC) (GLenum  target, GLuint  index, GLboolean * data);
typedef void (*PFNGLGETBOOLEANVPROC) (GLenum  pname, GLboolean * data);
typedef void (*PFNGLGETBUFFERPARAMETERI64VPROC) (GLenum  target, GLenum  pname, GLint64 * params);
typedef void (*PFNGLGETBUFFERPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETBUFFERPARAMETERIVARBPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETBUFFERPARAMETERUI64VNVPROC) (GLenum  target, GLenum  pname, GLuint64EXT * params);
typedef void (*PFNGLGETBUFFERPOINTERVPROC) (GLenum  target, GLenum  pname, void ** params);
typedef void (*PFNGLGETBUFFERPOINTERVARBPROC) (GLenum  target, GLenum  pname, void ** params);
typedef void (*PFNGLGETBUFFERSUBDATAPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  size, void * data);
typedef void (*PFNGLGETBUFFERSUBDATAARBPROC) (GLenum  target, GLintptrARB  offset, GLsizeiptrARB  size, void * data);
typedef void (*PFNGLGETCLIPPLANEPROC) (GLenum  plane, GLdouble * equation);
typedef void (*PFNGLGETCOLORTABLEPROC) (GLenum  target, GLenum  format, GLenum  type, void * table);
typedef void (*PFNGLGETCOLORTABLEEXTPROC) (GLenum  target, GLenum  format, GLenum  type, void * data);
typedef void (*PFNGLGETCOLORTABLEPARAMETERFVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETCOLORTABLEPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETCOLORTABLEPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETCOLORTABLEPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) (GLenum  stage, GLenum  portion, GLenum  variable, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) (GLenum  stage, GLenum  portion, GLenum  variable, GLenum  pname, GLint * params);
typedef void (*PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) (GLenum  stage, GLenum  portion, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) (GLenum  stage, GLenum  portion, GLenum  pname, GLint * params);
typedef void (*PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC) (GLenum  stage, GLenum  pname, GLfloat * params);
typedef GLuint (*PFNGLGETCOMMANDHEADERNVPROC) (GLenum  tokenID, GLuint  size);
typedef void (*PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC) (GLenum  texunit, GLenum  target, GLint  lod, void * img);
typedef void (*PFNGLGETCOMPRESSEDTEXIMAGEPROC) (GLenum  target, GLint  level, void * img);
typedef void (*PFNGLGETCOMPRESSEDTEXIMAGEARBPROC) (GLenum  target, GLint  level, void * img);
typedef void (*PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC) (GLuint  texture, GLint  level, GLsizei  bufSize, void * pixels);
typedef void (*PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC) (GLuint  texture, GLenum  target, GLint  lod, void * img);
typedef void (*PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLsizei  bufSize, void * pixels);
typedef void (*PFNGLGETCONVOLUTIONFILTERPROC) (GLenum  target, GLenum  format, GLenum  type, void * image);
typedef void (*PFNGLGETCONVOLUTIONFILTEREXTPROC) (GLenum  target, GLenum  format, GLenum  type, void * image);
typedef void (*PFNGLGETCONVOLUTIONPARAMETERFVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETCONVOLUTIONPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETCOVERAGEMODULATIONTABLENVPROC) (GLsizei  bufSize, GLfloat * v);
typedef GLuint (*PFNGLGETDEBUGMESSAGELOGPROC) (GLuint  count, GLsizei  bufSize, GLenum * sources, GLenum * types, GLuint * ids, GLenum * severities, GLsizei * lengths, GLchar * messageLog);
typedef GLuint (*PFNGLGETDEBUGMESSAGELOGAMDPROC) (GLuint  count, GLsizei  bufSize, GLenum * categories, GLuint * severities, GLuint * ids, GLsizei * lengths, GLchar * message);
typedef GLuint (*PFNGLGETDEBUGMESSAGELOGARBPROC) (GLuint  count, GLsizei  bufSize, GLenum * sources, GLenum * types, GLuint * ids, GLenum * severities, GLsizei * lengths, GLchar * messageLog);
typedef GLuint (*PFNGLGETDEBUGMESSAGELOGKHRPROC) (GLuint  count, GLsizei  bufSize, GLenum * sources, GLenum * types, GLuint * ids, GLenum * severities, GLsizei * lengths, GLchar * messageLog);
typedef void (*PFNGLGETDOUBLEINDEXEDVEXTPROC) (GLenum  target, GLuint  index, GLdouble * data);
typedef void (*PFNGLGETDOUBLEI_VPROC) (GLenum  target, GLuint  index, GLdouble * data);
typedef void (*PFNGLGETDOUBLEI_VEXTPROC) (GLenum  pname, GLuint  index, GLdouble * params);
typedef void (*PFNGLGETDOUBLEVPROC) (GLenum  pname, GLdouble * data);
typedef GLenum (*PFNGLGETERRORPROC) ();
typedef void (*PFNGLGETFENCEIVNVPROC) (GLuint  fence, GLenum  pname, GLint * params);
typedef void (*PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) (GLenum  variable, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) (GLenum  variable, GLenum  pname, GLint * params);
typedef void (*PFNGLGETFIRSTPERFQUERYIDINTELPROC) (GLuint * queryId);
typedef void (*PFNGLGETFLOATINDEXEDVEXTPROC) (GLenum  target, GLuint  index, GLfloat * data);
typedef void (*PFNGLGETFLOATI_VPROC) (GLenum  target, GLuint  index, GLfloat * data);
typedef void (*PFNGLGETFLOATI_VEXTPROC) (GLenum  pname, GLuint  index, GLfloat * params);
typedef void (*PFNGLGETFLOATVPROC) (GLenum  pname, GLfloat * data);
typedef GLint (*PFNGLGETFRAGDATAINDEXPROC) (GLuint  program, const GLchar * name);
typedef GLint (*PFNGLGETFRAGDATALOCATIONPROC) (GLuint  program, const GLchar * name);
typedef GLint (*PFNGLGETFRAGDATALOCATIONEXTPROC) (GLuint  program, const GLchar * name);
typedef void (*PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) (GLenum  target, GLenum  attachment, GLenum  pname, GLint * params);
typedef void (*PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLenum  target, GLenum  attachment, GLenum  pname, GLint * params);
typedef void (*PFNGLGETFRAMEBUFFERPARAMETERFVAMDPROC) (GLenum  target, GLenum  pname, GLuint  numsamples, GLuint  pixelindex, GLsizei  size, GLfloat * values);
typedef void (*PFNGLGETFRAMEBUFFERPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC) (GLuint  framebuffer, GLenum  pname, GLint * params);
typedef GLenum (*PFNGLGETGRAPHICSRESETSTATUSPROC) ();
typedef GLenum (*PFNGLGETGRAPHICSRESETSTATUSARBPROC) ();
typedef GLenum (*PFNGLGETGRAPHICSRESETSTATUSKHRPROC) ();
typedef GLhandleARB (*PFNGLGETHANDLEARBPROC) (GLenum  pname);
typedef void (*PFNGLGETHISTOGRAMPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, void * values);
typedef void (*PFNGLGETHISTOGRAMEXTPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, void * values);
typedef void (*PFNGLGETHISTOGRAMPARAMETERFVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETHISTOGRAMPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETHISTOGRAMPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETHISTOGRAMPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef GLuint64 (*PFNGLGETIMAGEHANDLEARBPROC) (GLuint  texture, GLint  level, GLboolean  layered, GLint  layer, GLenum  format);
typedef GLuint64 (*PFNGLGETIMAGEHANDLENVPROC) (GLuint  texture, GLint  level, GLboolean  layered, GLint  layer, GLenum  format);
typedef void (*PFNGLGETINFOLOGARBPROC) (GLhandleARB  obj, GLsizei  maxLength, GLsizei * length, GLcharARB * infoLog);
typedef void (*PFNGLGETINTEGER64I_VPROC) (GLenum  target, GLuint  index, GLint64 * data);
typedef void (*PFNGLGETINTEGER64VPROC) (GLenum  pname, GLint64 * data);
typedef void (*PFNGLGETINTEGERINDEXEDVEXTPROC) (GLenum  target, GLuint  index, GLint * data);
typedef void (*PFNGLGETINTEGERI_VPROC) (GLenum  target, GLuint  index, GLint * data);
typedef void (*PFNGLGETINTEGERUI64I_VNVPROC) (GLenum  value, GLuint  index, GLuint64EXT * result);
typedef void (*PFNGLGETINTEGERUI64VNVPROC) (GLenum  value, GLuint64EXT * result);
typedef void (*PFNGLGETINTEGERVPROC) (GLenum  pname, GLint * data);
typedef void (*PFNGLGETINTERNALFORMATSAMPLEIVNVPROC) (GLenum  target, GLenum  internalformat, GLsizei  samples, GLenum  pname, GLsizei  count, GLint * params);
typedef void (*PFNGLGETINTERNALFORMATI64VPROC) (GLenum  target, GLenum  internalformat, GLenum  pname, GLsizei  count, GLint64 * params);
typedef void (*PFNGLGETINTERNALFORMATIVPROC) (GLenum  target, GLenum  internalformat, GLenum  pname, GLsizei  count, GLint * params);
typedef void (*PFNGLGETINVARIANTBOOLEANVEXTPROC) (GLuint  id, GLenum  value, GLboolean * data);
typedef void (*PFNGLGETINVARIANTFLOATVEXTPROC) (GLuint  id, GLenum  value, GLfloat * data);
typedef void (*PFNGLGETINVARIANTINTEGERVEXTPROC) (GLuint  id, GLenum  value, GLint * data);
typedef void (*PFNGLGETLIGHTFVPROC) (GLenum  light, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETLIGHTIVPROC) (GLenum  light, GLenum  pname, GLint * params);
typedef void (*PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC) (GLuint  id, GLenum  value, GLboolean * data);
typedef void (*PFNGLGETLOCALCONSTANTFLOATVEXTPROC) (GLuint  id, GLenum  value, GLfloat * data);
typedef void (*PFNGLGETLOCALCONSTANTINTEGERVEXTPROC) (GLuint  id, GLenum  value, GLint * data);
typedef void (*PFNGLGETMAPATTRIBPARAMETERFVNVPROC) (GLenum  target, GLuint  index, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMAPATTRIBPARAMETERIVNVPROC) (GLenum  target, GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMAPCONTROLPOINTSNVPROC) (GLenum  target, GLuint  index, GLenum  type, GLsizei  ustride, GLsizei  vstride, GLboolean  packed, void * points);
typedef void (*PFNGLGETMAPPARAMETERFVNVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMAPPARAMETERIVNVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMAPDVPROC) (GLenum  target, GLenum  query, GLdouble * v);
typedef void (*PFNGLGETMAPFVPROC) (GLenum  target, GLenum  query, GLfloat * v);
typedef void (*PFNGLGETMAPIVPROC) (GLenum  target, GLenum  query, GLint * v);
typedef void (*PFNGLGETMATERIALFVPROC) (GLenum  face, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMATERIALIVPROC) (GLenum  face, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMEMORYOBJECTDETACHEDRESOURCESUIVNVPROC) (GLuint  memory, GLenum  pname, GLint  first, GLsizei  count, GLuint * params);
typedef void (*PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC) (GLuint  memoryObject, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMINMAXPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, void * values);
typedef void (*PFNGLGETMINMAXEXTPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, void * values);
typedef void (*PFNGLGETMINMAXPARAMETERFVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMINMAXPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMINMAXPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMINMAXPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMULTITEXENVFVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMULTITEXENVIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMULTITEXGENDVEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETMULTITEXGENFVEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMULTITEXGENIVEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMULTITEXIMAGEEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  format, GLenum  type, void * pixels);
typedef void (*PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMULTITEXPARAMETERIIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMULTITEXPARAMETERIUIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETMULTITEXPARAMETERFVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETMULTITEXPARAMETERIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETMULTISAMPLEFVPROC) (GLenum  pname, GLuint  index, GLfloat * val);
typedef void (*PFNGLGETMULTISAMPLEFVNVPROC) (GLenum  pname, GLuint  index, GLfloat * val);
typedef void (*PFNGLGETNAMEDBUFFERPARAMETERI64VPROC) (GLuint  buffer, GLenum  pname, GLint64 * params);
typedef void (*PFNGLGETNAMEDBUFFERPARAMETERIVPROC) (GLuint  buffer, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC) (GLuint  buffer, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDBUFFERPARAMETERUI64VNVPROC) (GLuint  buffer, GLenum  pname, GLuint64EXT * params);
typedef void (*PFNGLGETNAMEDBUFFERPOINTERVPROC) (GLuint  buffer, GLenum  pname, void ** params);
typedef void (*PFNGLGETNAMEDBUFFERPOINTERVEXTPROC) (GLuint  buffer, GLenum  pname, void ** params);
typedef void (*PFNGLGETNAMEDBUFFERSUBDATAPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, void * data);
typedef void (*PFNGLGETNAMEDBUFFERSUBDATAEXTPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, void * data);
typedef void (*PFNGLGETNAMEDFRAMEBUFFERPARAMETERFVAMDPROC) (GLuint  framebuffer, GLenum  pname, GLuint  numsamples, GLuint  pixelindex, GLsizei  size, GLfloat * values);
typedef void (*PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC) (GLuint  framebuffer, GLenum  pname, GLint * param);
typedef void (*PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC) (GLuint  framebuffer, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLint * params);
typedef void (*PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLuint * params);
typedef void (*PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLdouble * params);
typedef void (*PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLfloat * params);
typedef void (*PFNGLGETNAMEDPROGRAMSTRINGEXTPROC) (GLuint  program, GLenum  target, GLenum  pname, void * string);
typedef void (*PFNGLGETNAMEDPROGRAMIVEXTPROC) (GLuint  program, GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC) (GLuint  renderbuffer, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC) (GLuint  renderbuffer, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNAMEDSTRINGARBPROC) (GLint  namelen, const GLchar * name, GLsizei  bufSize, GLint * stringlen, GLchar * string);
typedef void (*PFNGLGETNAMEDSTRINGIVARBPROC) (GLint  namelen, const GLchar * name, GLenum  pname, GLint * params);
typedef void (*PFNGLGETNEXTPERFQUERYIDINTELPROC) (GLuint  queryId, GLuint * nextQueryId);
typedef void (*PFNGLGETOBJECTLABELPROC) (GLenum  identifier, GLuint  name, GLsizei  bufSize, GLsizei * length, GLchar * label);
typedef void (*PFNGLGETOBJECTLABELEXTPROC) (GLenum  type, GLuint  object, GLsizei  bufSize, GLsizei * length, GLchar * label);
typedef void (*PFNGLGETOBJECTLABELKHRPROC) (GLenum  identifier, GLuint  name, GLsizei  bufSize, GLsizei * length, GLchar * label);
typedef void (*PFNGLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB  obj, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETOBJECTPARAMETERIVAPPLEPROC) (GLenum  objectType, GLuint  name, GLenum  pname, GLint * params);
typedef void (*PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB  obj, GLenum  pname, GLint * params);
typedef void (*PFNGLGETOBJECTPTRLABELPROC) (const void * ptr, GLsizei  bufSize, GLsizei * length, GLchar * label);
typedef void (*PFNGLGETOBJECTPTRLABELKHRPROC) (const void * ptr, GLsizei  bufSize, GLsizei * length, GLchar * label);
typedef void (*PFNGLGETOCCLUSIONQUERYIVNVPROC) (GLuint  id, GLenum  pname, GLint * params);
typedef void (*PFNGLGETOCCLUSIONQUERYUIVNVPROC) (GLuint  id, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETPATHCOLORGENFVNVPROC) (GLenum  color, GLenum  pname, GLfloat * value);
typedef void (*PFNGLGETPATHCOLORGENIVNVPROC) (GLenum  color, GLenum  pname, GLint * value);
typedef void (*PFNGLGETPATHCOMMANDSNVPROC) (GLuint  path, GLubyte * commands);
typedef void (*PFNGLGETPATHCOORDSNVPROC) (GLuint  path, GLfloat * coords);
typedef void (*PFNGLGETPATHDASHARRAYNVPROC) (GLuint  path, GLfloat * dashArray);
typedef GLfloat (*PFNGLGETPATHLENGTHNVPROC) (GLuint  path, GLsizei  startSegment, GLsizei  numSegments);
typedef void (*PFNGLGETPATHMETRICRANGENVPROC) (GLbitfield  metricQueryMask, GLuint  firstPathName, GLsizei  numPaths, GLsizei  stride, GLfloat * metrics);
typedef void (*PFNGLGETPATHMETRICSNVPROC) (GLbitfield  metricQueryMask, GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLsizei  stride, GLfloat * metrics);
typedef void (*PFNGLGETPATHPARAMETERFVNVPROC) (GLuint  path, GLenum  pname, GLfloat * value);
typedef void (*PFNGLGETPATHPARAMETERIVNVPROC) (GLuint  path, GLenum  pname, GLint * value);
typedef void (*PFNGLGETPATHSPACINGNVPROC) (GLenum  pathListMode, GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLfloat  advanceScale, GLfloat  kerningScale, GLenum  transformType, GLfloat * returnedSpacing);
typedef void (*PFNGLGETPATHTEXGENFVNVPROC) (GLenum  texCoordSet, GLenum  pname, GLfloat * value);
typedef void (*PFNGLGETPATHTEXGENIVNVPROC) (GLenum  texCoordSet, GLenum  pname, GLint * value);
typedef void (*PFNGLGETPERFCOUNTERINFOINTELPROC) (GLuint  queryId, GLuint  counterId, GLuint  counterNameLength, GLchar * counterName, GLuint  counterDescLength, GLchar * counterDesc, GLuint * counterOffset, GLuint * counterDataSize, GLuint * counterTypeEnum, GLuint * counterDataTypeEnum, GLuint64 * rawCounterMaxValue);
typedef void (*PFNGLGETPERFMONITORCOUNTERDATAAMDPROC) (GLuint  monitor, GLenum  pname, GLsizei  dataSize, GLuint * data, GLint * bytesWritten);
typedef void (*PFNGLGETPERFMONITORCOUNTERINFOAMDPROC) (GLuint  group, GLuint  counter, GLenum  pname, void * data);
typedef void (*PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC) (GLuint  group, GLuint  counter, GLsizei  bufSize, GLsizei * length, GLchar * counterString);
typedef void (*PFNGLGETPERFMONITORCOUNTERSAMDPROC) (GLuint  group, GLint * numCounters, GLint * maxActiveCounters, GLsizei  counterSize, GLuint * counters);
typedef void (*PFNGLGETPERFMONITORGROUPSTRINGAMDPROC) (GLuint  group, GLsizei  bufSize, GLsizei * length, GLchar * groupString);
typedef void (*PFNGLGETPERFMONITORGROUPSAMDPROC) (GLint * numGroups, GLsizei  groupsSize, GLuint * groups);
typedef void (*PFNGLGETPERFQUERYDATAINTELPROC) (GLuint  queryHandle, GLuint  flags, GLsizei  dataSize, void * data, GLuint * bytesWritten);
typedef void (*PFNGLGETPERFQUERYIDBYNAMEINTELPROC) (GLchar * queryName, GLuint * queryId);
typedef void (*PFNGLGETPERFQUERYINFOINTELPROC) (GLuint  queryId, GLuint  queryNameLength, GLchar * queryName, GLuint * dataSize, GLuint * noCounters, GLuint * noInstances, GLuint * capsMask);
typedef void (*PFNGLGETPIXELMAPFVPROC) (GLenum  map, GLfloat * values);
typedef void (*PFNGLGETPIXELMAPUIVPROC) (GLenum  map, GLuint * values);
typedef void (*PFNGLGETPIXELMAPUSVPROC) (GLenum  map, GLushort * values);
typedef void (*PFNGLGETPIXELTRANSFORMPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETPIXELTRANSFORMPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETPOINTERINDEXEDVEXTPROC) (GLenum  target, GLuint  index, void ** data);
typedef void (*PFNGLGETPOINTERI_VEXTPROC) (GLenum  pname, GLuint  index, void ** params);
typedef void (*PFNGLGETPOINTERVPROC) (GLenum  pname, void ** params);
typedef void (*PFNGLGETPOINTERVEXTPROC) (GLenum  pname, void ** params);
typedef void (*PFNGLGETPOINTERVKHRPROC) (GLenum  pname, void ** params);
typedef void (*PFNGLGETPOLYGONSTIPPLEPROC) (GLubyte * mask);
typedef void (*PFNGLGETPROGRAMBINARYPROC) (GLuint  program, GLsizei  bufSize, GLsizei * length, GLenum * binaryFormat, void * binary);
typedef void (*PFNGLGETPROGRAMENVPARAMETERIIVNVPROC) (GLenum  target, GLuint  index, GLint * params);
typedef void (*PFNGLGETPROGRAMENVPARAMETERIUIVNVPROC) (GLenum  target, GLuint  index, GLuint * params);
typedef void (*PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum  target, GLuint  index, GLdouble * params);
typedef void (*PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum  target, GLuint  index, GLfloat * params);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC) (GLuint  program, GLsizei  bufSize, GLsizei * length, GLchar * infoLog);
typedef void (*PFNGLGETPROGRAMINTERFACEIVPROC) (GLuint  program, GLenum  programInterface, GLenum  pname, GLint * params);
typedef void (*PFNGLGETPROGRAMLOCALPARAMETERIIVNVPROC) (GLenum  target, GLuint  index, GLint * params);
typedef void (*PFNGLGETPROGRAMLOCALPARAMETERIUIVNVPROC) (GLenum  target, GLuint  index, GLuint * params);
typedef void (*PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum  target, GLuint  index, GLdouble * params);
typedef void (*PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum  target, GLuint  index, GLfloat * params);
typedef void (*PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC) (GLuint  id, GLsizei  len, const GLubyte * name, GLdouble * params);
typedef void (*PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC) (GLuint  id, GLsizei  len, const GLubyte * name, GLfloat * params);
typedef void (*PFNGLGETPROGRAMPARAMETERDVNVPROC) (GLenum  target, GLuint  index, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETPROGRAMPARAMETERFVNVPROC) (GLenum  target, GLuint  index, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETPROGRAMPIPELINEINFOLOGPROC) (GLuint  pipeline, GLsizei  bufSize, GLsizei * length, GLchar * infoLog);
typedef void (*PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC) (GLuint  pipeline, GLsizei  bufSize, GLsizei * length, GLchar * infoLog);
typedef void (*PFNGLGETPROGRAMPIPELINEIVPROC) (GLuint  pipeline, GLenum  pname, GLint * params);
typedef void (*PFNGLGETPROGRAMPIPELINEIVEXTPROC) (GLuint  pipeline, GLenum  pname, GLint * params);
typedef GLuint (*PFNGLGETPROGRAMRESOURCEINDEXPROC) (GLuint  program, GLenum  programInterface, const GLchar * name);
typedef GLint (*PFNGLGETPROGRAMRESOURCELOCATIONPROC) (GLuint  program, GLenum  programInterface, const GLchar * name);
typedef GLint (*PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC) (GLuint  program, GLenum  programInterface, const GLchar * name);
typedef void (*PFNGLGETPROGRAMRESOURCENAMEPROC) (GLuint  program, GLenum  programInterface, GLuint  index, GLsizei  bufSize, GLsizei * length, GLchar * name);
typedef void (*PFNGLGETPROGRAMRESOURCEFVNVPROC) (GLuint  program, GLenum  programInterface, GLuint  index, GLsizei  propCount, const GLenum * props, GLsizei  count, GLsizei * length, GLfloat * params);
typedef void (*PFNGLGETPROGRAMRESOURCEIVPROC) (GLuint  program, GLenum  programInterface, GLuint  index, GLsizei  propCount, const GLenum * props, GLsizei  count, GLsizei * length, GLint * params);
typedef void (*PFNGLGETPROGRAMSTAGEIVPROC) (GLuint  program, GLenum  shadertype, GLenum  pname, GLint * values);
typedef void (*PFNGLGETPROGRAMSTRINGARBPROC) (GLenum  target, GLenum  pname, void * string);
typedef void (*PFNGLGETPROGRAMSTRINGNVPROC) (GLuint  id, GLenum  pname, GLubyte * program);
typedef void (*PFNGLGETPROGRAMSUBROUTINEPARAMETERUIVNVPROC) (GLenum  target, GLuint  index, GLuint * param);
typedef void (*PFNGLGETPROGRAMIVPROC) (GLuint  program, GLenum  pname, GLint * params);
typedef void (*PFNGLGETPROGRAMIVARBPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETPROGRAMIVNVPROC) (GLuint  id, GLenum  pname, GLint * params);
typedef void (*PFNGLGETQUERYBUFFEROBJECTI64VPROC) (GLuint  id, GLuint  buffer, GLenum  pname, GLintptr  offset);
typedef void (*PFNGLGETQUERYBUFFEROBJECTIVPROC) (GLuint  id, GLuint  buffer, GLenum  pname, GLintptr  offset);
typedef void (*PFNGLGETQUERYBUFFEROBJECTUI64VPROC) (GLuint  id, GLuint  buffer, GLenum  pname, GLintptr  offset);
typedef void (*PFNGLGETQUERYBUFFEROBJECTUIVPROC) (GLuint  id, GLuint  buffer, GLenum  pname, GLintptr  offset);
typedef void (*PFNGLGETQUERYINDEXEDIVPROC) (GLenum  target, GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETQUERYOBJECTI64VPROC) (GLuint  id, GLenum  pname, GLint64 * params);
typedef void (*PFNGLGETQUERYOBJECTI64VEXTPROC) (GLuint  id, GLenum  pname, GLint64 * params);
typedef void (*PFNGLGETQUERYOBJECTIVPROC) (GLuint  id, GLenum  pname, GLint * params);
typedef void (*PFNGLGETQUERYOBJECTIVARBPROC) (GLuint  id, GLenum  pname, GLint * params);
typedef void (*PFNGLGETQUERYOBJECTUI64VPROC) (GLuint  id, GLenum  pname, GLuint64 * params);
typedef void (*PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint  id, GLenum  pname, GLuint64 * params);
typedef void (*PFNGLGETQUERYOBJECTUIVPROC) (GLuint  id, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETQUERYOBJECTUIVARBPROC) (GLuint  id, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETQUERYIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETQUERYIVARBPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETRENDERBUFFERPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETSAMPLERPARAMETERIIVPROC) (GLuint  sampler, GLenum  pname, GLint * params);
typedef void (*PFNGLGETSAMPLERPARAMETERIUIVPROC) (GLuint  sampler, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETSAMPLERPARAMETERFVPROC) (GLuint  sampler, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETSAMPLERPARAMETERIVPROC) (GLuint  sampler, GLenum  pname, GLint * params);
typedef void (*PFNGLGETSEMAPHOREPARAMETERIVNVPROC) (GLuint  semaphore, GLenum  pname, GLint * params);
typedef void (*PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC) (GLuint  semaphore, GLenum  pname, GLuint64 * params);
typedef void (*PFNGLGETSEPARABLEFILTERPROC) (GLenum  target, GLenum  format, GLenum  type, void * row, void * column, void * span);
typedef void (*PFNGLGETSEPARABLEFILTEREXTPROC) (GLenum  target, GLenum  format, GLenum  type, void * row, void * column, void * span);
typedef void (*PFNGLGETSHADERINFOLOGPROC) (GLuint  shader, GLsizei  bufSize, GLsizei * length, GLchar * infoLog);
typedef void (*PFNGLGETSHADERPRECISIONFORMATPROC) (GLenum  shadertype, GLenum  precisiontype, GLint * range, GLint * precision);
typedef void (*PFNGLGETSHADERSOURCEPROC) (GLuint  shader, GLsizei  bufSize, GLsizei * length, GLchar * source);
typedef void (*PFNGLGETSHADERSOURCEARBPROC) (GLhandleARB  obj, GLsizei  maxLength, GLsizei * length, GLcharARB * source);
typedef void (*PFNGLGETSHADERIVPROC) (GLuint  shader, GLenum  pname, GLint * params);
typedef void (*PFNGLGETSHADINGRATEIMAGEPALETTENVPROC) (GLuint  viewport, GLuint  entry, GLenum * rate);
typedef void (*PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC) (GLenum  rate, GLuint  samples, GLuint  index, GLint * location);
typedef GLushort (*PFNGLGETSTAGEINDEXNVPROC) (GLenum  shadertype);
typedef const GLubyte *(*PFNGLGETSTRINGPROC) (GLenum  name);
typedef const GLubyte *(*PFNGLGETSTRINGIPROC) (GLenum  name, GLuint  index);
typedef GLuint (*PFNGLGETSUBROUTINEINDEXPROC) (GLuint  program, GLenum  shadertype, const GLchar * name);
typedef GLint (*PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC) (GLuint  program, GLenum  shadertype, const GLchar * name);
typedef void (*PFNGLGETSYNCIVPROC) (GLsync  sync, GLenum  pname, GLsizei  count, GLsizei * length, GLint * values);
typedef void (*PFNGLGETTEXENVFVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXENVIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXGENDVPROC) (GLenum  coord, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETTEXGENFVPROC) (GLenum  coord, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXGENIVPROC) (GLenum  coord, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXIMAGEPROC) (GLenum  target, GLint  level, GLenum  format, GLenum  type, void * pixels);
typedef void (*PFNGLGETTEXLEVELPARAMETERFVPROC) (GLenum  target, GLint  level, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXLEVELPARAMETERIVPROC) (GLenum  target, GLint  level, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXPARAMETERIIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXPARAMETERIIVEXTPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXPARAMETERIUIVPROC) (GLenum  target, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETTEXPARAMETERIUIVEXTPROC) (GLenum  target, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETTEXPARAMETERPOINTERVAPPLEPROC) (GLenum  target, GLenum  pname, void ** params);
typedef void (*PFNGLGETTEXPARAMETERFVPROC) (GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXPARAMETERIVPROC) (GLenum  target, GLenum  pname, GLint * params);
typedef GLuint64 (*PFNGLGETTEXTUREHANDLEARBPROC) (GLuint  texture);
typedef GLuint64 (*PFNGLGETTEXTUREHANDLENVPROC) (GLuint  texture);
typedef void (*PFNGLGETTEXTUREIMAGEPROC) (GLuint  texture, GLint  level, GLenum  format, GLenum  type, GLsizei  bufSize, void * pixels);
typedef void (*PFNGLGETTEXTUREIMAGEEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  format, GLenum  type, void * pixels);
typedef void (*PFNGLGETTEXTURELEVELPARAMETERFVPROC) (GLuint  texture, GLint  level, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXTURELEVELPARAMETERIVPROC) (GLuint  texture, GLint  level, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXTUREPARAMETERIIVPROC) (GLuint  texture, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXTUREPARAMETERIIVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXTUREPARAMETERIUIVPROC) (GLuint  texture, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETTEXTUREPARAMETERIUIVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETTEXTUREPARAMETERFVPROC) (GLuint  texture, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXTUREPARAMETERFVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETTEXTUREPARAMETERIVPROC) (GLuint  texture, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTEXTUREPARAMETERIVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, GLint * params);
typedef GLuint64 (*PFNGLGETTEXTURESAMPLERHANDLEARBPROC) (GLuint  texture, GLuint  sampler);
typedef GLuint64 (*PFNGLGETTEXTURESAMPLERHANDLENVPROC) (GLuint  texture, GLuint  sampler);
typedef void (*PFNGLGETTEXTURESUBIMAGEPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, GLsizei  bufSize, void * pixels);
typedef void (*PFNGLGETTRACKMATRIXIVNVPROC) (GLenum  target, GLuint  address, GLenum  pname, GLint * params);
typedef void (*PFNGLGETTRANSFORMFEEDBACKVARYINGPROC) (GLuint  program, GLuint  index, GLsizei  bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name);
typedef void (*PFNGLGETTRANSFORMFEEDBACKVARYINGEXTPROC) (GLuint  program, GLuint  index, GLsizei  bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name);
typedef void (*PFNGLGETTRANSFORMFEEDBACKVARYINGNVPROC) (GLuint  program, GLuint  index, GLint * location);
typedef void (*PFNGLGETTRANSFORMFEEDBACKI64_VPROC) (GLuint  xfb, GLenum  pname, GLuint  index, GLint64 * param);
typedef void (*PFNGLGETTRANSFORMFEEDBACKI_VPROC) (GLuint  xfb, GLenum  pname, GLuint  index, GLint * param);
typedef void (*PFNGLGETTRANSFORMFEEDBACKIVPROC) (GLuint  xfb, GLenum  pname, GLint * param);
typedef GLuint (*PFNGLGETUNIFORMBLOCKINDEXPROC) (GLuint  program, const GLchar * uniformBlockName);
typedef GLint (*PFNGLGETUNIFORMBUFFERSIZEEXTPROC) (GLuint  program, GLint  location);
typedef void (*PFNGLGETUNIFORMINDICESPROC) (GLuint  program, GLsizei  uniformCount, const GLchar *const* uniformNames, GLuint * uniformIndices);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC) (GLuint  program, const GLchar * name);
typedef GLint (*PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB  programObj, const GLcharARB * name);
typedef GLintptr (*PFNGLGETUNIFORMOFFSETEXTPROC) (GLuint  program, GLint  location);
typedef void (*PFNGLGETUNIFORMSUBROUTINEUIVPROC) (GLenum  shadertype, GLint  location, GLuint * params);
typedef void (*PFNGLGETUNIFORMDVPROC) (GLuint  program, GLint  location, GLdouble * params);
typedef void (*PFNGLGETUNIFORMFVPROC) (GLuint  program, GLint  location, GLfloat * params);
typedef void (*PFNGLGETUNIFORMFVARBPROC) (GLhandleARB  programObj, GLint  location, GLfloat * params);
typedef void (*PFNGLGETUNIFORMI64VARBPROC) (GLuint  program, GLint  location, GLint64 * params);
typedef void (*PFNGLGETUNIFORMI64VNVPROC) (GLuint  program, GLint  location, GLint64EXT * params);
typedef void (*PFNGLGETUNIFORMIVPROC) (GLuint  program, GLint  location, GLint * params);
typedef void (*PFNGLGETUNIFORMIVARBPROC) (GLhandleARB  programObj, GLint  location, GLint * params);
typedef void (*PFNGLGETUNIFORMUI64VARBPROC) (GLuint  program, GLint  location, GLuint64 * params);
typedef void (*PFNGLGETUNIFORMUI64VNVPROC) (GLuint  program, GLint  location, GLuint64EXT * params);
typedef void (*PFNGLGETUNIFORMUIVPROC) (GLuint  program, GLint  location, GLuint * params);
typedef void (*PFNGLGETUNIFORMUIVEXTPROC) (GLuint  program, GLint  location, GLuint * params);
typedef void (*PFNGLGETUNSIGNEDBYTEVEXTPROC) (GLenum  pname, GLubyte * data);
typedef void (*PFNGLGETUNSIGNEDBYTEI_VEXTPROC) (GLenum  target, GLuint  index, GLubyte * data);
typedef void (*PFNGLGETVARIANTBOOLEANVEXTPROC) (GLuint  id, GLenum  value, GLboolean * data);
typedef void (*PFNGLGETVARIANTFLOATVEXTPROC) (GLuint  id, GLenum  value, GLfloat * data);
typedef void (*PFNGLGETVARIANTINTEGERVEXTPROC) (GLuint  id, GLenum  value, GLint * data);
typedef void (*PFNGLGETVARIANTPOINTERVEXTPROC) (GLuint  id, GLenum  value, void ** data);
typedef GLint (*PFNGLGETVARYINGLOCATIONNVPROC) (GLuint  program, const GLchar * name);
typedef void (*PFNGLGETVERTEXARRAYINDEXED64IVPROC) (GLuint  vaobj, GLuint  index, GLenum  pname, GLint64 * param);
typedef void (*PFNGLGETVERTEXARRAYINDEXEDIVPROC) (GLuint  vaobj, GLuint  index, GLenum  pname, GLint * param);
typedef void (*PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC) (GLuint  vaobj, GLuint  index, GLenum  pname, GLint * param);
typedef void (*PFNGLGETVERTEXARRAYINTEGERVEXTPROC) (GLuint  vaobj, GLenum  pname, GLint * param);
typedef void (*PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC) (GLuint  vaobj, GLuint  index, GLenum  pname, void ** param);
typedef void (*PFNGLGETVERTEXARRAYPOINTERVEXTPROC) (GLuint  vaobj, GLenum  pname, void ** param);
typedef void (*PFNGLGETVERTEXARRAYIVPROC) (GLuint  vaobj, GLenum  pname, GLint * param);
typedef void (*PFNGLGETVERTEXATTRIBIIVPROC) (GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVERTEXATTRIBIIVEXTPROC) (GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVERTEXATTRIBIUIVPROC) (GLuint  index, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETVERTEXATTRIBIUIVEXTPROC) (GLuint  index, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETVERTEXATTRIBLDVPROC) (GLuint  index, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETVERTEXATTRIBLDVEXTPROC) (GLuint  index, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETVERTEXATTRIBLI64VNVPROC) (GLuint  index, GLenum  pname, GLint64EXT * params);
typedef void (*PFNGLGETVERTEXATTRIBLUI64VARBPROC) (GLuint  index, GLenum  pname, GLuint64EXT * params);
typedef void (*PFNGLGETVERTEXATTRIBLUI64VNVPROC) (GLuint  index, GLenum  pname, GLuint64EXT * params);
typedef void (*PFNGLGETVERTEXATTRIBPOINTERVPROC) (GLuint  index, GLenum  pname, void ** pointer);
typedef void (*PFNGLGETVERTEXATTRIBPOINTERVARBPROC) (GLuint  index, GLenum  pname, void ** pointer);
typedef void (*PFNGLGETVERTEXATTRIBPOINTERVNVPROC) (GLuint  index, GLenum  pname, void ** pointer);
typedef void (*PFNGLGETVERTEXATTRIBDVPROC) (GLuint  index, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETVERTEXATTRIBDVARBPROC) (GLuint  index, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETVERTEXATTRIBDVNVPROC) (GLuint  index, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETVERTEXATTRIBFVPROC) (GLuint  index, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETVERTEXATTRIBFVARBPROC) (GLuint  index, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETVERTEXATTRIBFVNVPROC) (GLuint  index, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETVERTEXATTRIBIVPROC) (GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVERTEXATTRIBIVARBPROC) (GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVERTEXATTRIBIVNVPROC) (GLuint  index, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVIDEOCAPTURESTREAMDVNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  pname, GLdouble * params);
typedef void (*PFNGLGETVIDEOCAPTURESTREAMFVNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  pname, GLfloat * params);
typedef void (*PFNGLGETVIDEOCAPTURESTREAMIVNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVIDEOCAPTUREIVNVPROC) (GLuint  video_capture_slot, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVIDEOI64VNVPROC) (GLuint  video_slot, GLenum  pname, GLint64EXT * params);
typedef void (*PFNGLGETVIDEOIVNVPROC) (GLuint  video_slot, GLenum  pname, GLint * params);
typedef void (*PFNGLGETVIDEOUI64VNVPROC) (GLuint  video_slot, GLenum  pname, GLuint64EXT * params);
typedef void (*PFNGLGETVIDEOUIVNVPROC) (GLuint  video_slot, GLenum  pname, GLuint * params);
typedef void (*PFNGLGETNCOLORTABLEPROC) (GLenum  target, GLenum  format, GLenum  type, GLsizei  bufSize, void * table);
typedef void (*PFNGLGETNCOLORTABLEARBPROC) (GLenum  target, GLenum  format, GLenum  type, GLsizei  bufSize, void * table);
typedef void (*PFNGLGETNCOMPRESSEDTEXIMAGEPROC) (GLenum  target, GLint  lod, GLsizei  bufSize, void * pixels);
typedef void (*PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC) (GLenum  target, GLint  lod, GLsizei  bufSize, void * img);
typedef void (*PFNGLGETNCONVOLUTIONFILTERPROC) (GLenum  target, GLenum  format, GLenum  type, GLsizei  bufSize, void * image);
typedef void (*PFNGLGETNCONVOLUTIONFILTERARBPROC) (GLenum  target, GLenum  format, GLenum  type, GLsizei  bufSize, void * image);
typedef void (*PFNGLGETNHISTOGRAMPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, GLsizei  bufSize, void * values);
typedef void (*PFNGLGETNHISTOGRAMARBPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, GLsizei  bufSize, void * values);
typedef void (*PFNGLGETNMAPDVPROC) (GLenum  target, GLenum  query, GLsizei  bufSize, GLdouble * v);
typedef void (*PFNGLGETNMAPDVARBPROC) (GLenum  target, GLenum  query, GLsizei  bufSize, GLdouble * v);
typedef void (*PFNGLGETNMAPFVPROC) (GLenum  target, GLenum  query, GLsizei  bufSize, GLfloat * v);
typedef void (*PFNGLGETNMAPFVARBPROC) (GLenum  target, GLenum  query, GLsizei  bufSize, GLfloat * v);
typedef void (*PFNGLGETNMAPIVPROC) (GLenum  target, GLenum  query, GLsizei  bufSize, GLint * v);
typedef void (*PFNGLGETNMAPIVARBPROC) (GLenum  target, GLenum  query, GLsizei  bufSize, GLint * v);
typedef void (*PFNGLGETNMINMAXPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, GLsizei  bufSize, void * values);
typedef void (*PFNGLGETNMINMAXARBPROC) (GLenum  target, GLboolean  reset, GLenum  format, GLenum  type, GLsizei  bufSize, void * values);
typedef void (*PFNGLGETNPIXELMAPFVPROC) (GLenum  map, GLsizei  bufSize, GLfloat * values);
typedef void (*PFNGLGETNPIXELMAPFVARBPROC) (GLenum  map, GLsizei  bufSize, GLfloat * values);
typedef void (*PFNGLGETNPIXELMAPUIVPROC) (GLenum  map, GLsizei  bufSize, GLuint * values);
typedef void (*PFNGLGETNPIXELMAPUIVARBPROC) (GLenum  map, GLsizei  bufSize, GLuint * values);
typedef void (*PFNGLGETNPIXELMAPUSVPROC) (GLenum  map, GLsizei  bufSize, GLushort * values);
typedef void (*PFNGLGETNPIXELMAPUSVARBPROC) (GLenum  map, GLsizei  bufSize, GLushort * values);
typedef void (*PFNGLGETNPOLYGONSTIPPLEPROC) (GLsizei  bufSize, GLubyte * pattern);
typedef void (*PFNGLGETNPOLYGONSTIPPLEARBPROC) (GLsizei  bufSize, GLubyte * pattern);
typedef void (*PFNGLGETNSEPARABLEFILTERPROC) (GLenum  target, GLenum  format, GLenum  type, GLsizei  rowBufSize, void * row, GLsizei  columnBufSize, void * column, void * span);
typedef void (*PFNGLGETNSEPARABLEFILTERARBPROC) (GLenum  target, GLenum  format, GLenum  type, GLsizei  rowBufSize, void * row, GLsizei  columnBufSize, void * column, void * span);
typedef void (*PFNGLGETNTEXIMAGEPROC) (GLenum  target, GLint  level, GLenum  format, GLenum  type, GLsizei  bufSize, void * pixels);
typedef void (*PFNGLGETNTEXIMAGEARBPROC) (GLenum  target, GLint  level, GLenum  format, GLenum  type, GLsizei  bufSize, void * img);
typedef void (*PFNGLGETNUNIFORMDVPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLdouble * params);
typedef void (*PFNGLGETNUNIFORMDVARBPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLdouble * params);
typedef void (*PFNGLGETNUNIFORMFVPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLfloat * params);
typedef void (*PFNGLGETNUNIFORMFVARBPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLfloat * params);
typedef void (*PFNGLGETNUNIFORMFVKHRPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLfloat * params);
typedef void (*PFNGLGETNUNIFORMI64VARBPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLint64 * params);
typedef void (*PFNGLGETNUNIFORMIVPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLint * params);
typedef void (*PFNGLGETNUNIFORMIVARBPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLint * params);
typedef void (*PFNGLGETNUNIFORMIVKHRPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLint * params);
typedef void (*PFNGLGETNUNIFORMUI64VARBPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLuint64 * params);
typedef void (*PFNGLGETNUNIFORMUIVPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLuint * params);
typedef void (*PFNGLGETNUNIFORMUIVARBPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLuint * params);
typedef void (*PFNGLGETNUNIFORMUIVKHRPROC) (GLuint  program, GLint  location, GLsizei  bufSize, GLuint * params);
typedef void (*PFNGLHINTPROC) (GLenum  target, GLenum  mode);
typedef void (*PFNGLHISTOGRAMPROC) (GLenum  target, GLsizei  width, GLenum  internalformat, GLboolean  sink);
typedef void (*PFNGLHISTOGRAMEXTPROC) (GLenum  target, GLsizei  width, GLenum  internalformat, GLboolean  sink);
typedef void (*PFNGLIMPORTMEMORYFDEXTPROC) (GLuint  memory, GLuint64  size, GLenum  handleType, GLint  fd);
typedef void (*PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC) (GLuint  memory, GLuint64  size, GLenum  handleType, void * handle);
typedef void (*PFNGLIMPORTMEMORYWIN32NAMEEXTPROC) (GLuint  memory, GLuint64  size, GLenum  handleType, const void * name);
typedef void (*PFNGLIMPORTSEMAPHOREFDEXTPROC) (GLuint  semaphore, GLenum  handleType, GLint  fd);
typedef void (*PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC) (GLuint  semaphore, GLenum  handleType, void * handle);
typedef void (*PFNGLIMPORTSEMAPHOREWIN32NAMEEXTPROC) (GLuint  semaphore, GLenum  handleType, const void * name);
typedef GLsync (*PFNGLIMPORTSYNCEXTPROC) (GLenum  external_sync_type, GLintptr  external_sync, GLbitfield  flags);
typedef void (*PFNGLINDEXFORMATNVPROC) (GLenum  type, GLsizei  stride);
typedef void (*PFNGLINDEXFUNCEXTPROC) (GLenum  func, GLclampf  ref);
typedef void (*PFNGLINDEXMASKPROC) (GLuint  mask);
typedef void (*PFNGLINDEXMATERIALEXTPROC) (GLenum  face, GLenum  mode);
typedef void (*PFNGLINDEXPOINTERPROC) (GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLINDEXPOINTEREXTPROC) (GLenum  type, GLsizei  stride, GLsizei  count, const void * pointer);
typedef void (*PFNGLINDEXDPROC) (GLdouble  c);
typedef void (*PFNGLINDEXDVPROC) (const GLdouble * c);
typedef void (*PFNGLINDEXFPROC) (GLfloat  c);
typedef void (*PFNGLINDEXFVPROC) (const GLfloat * c);
typedef void (*PFNGLINDEXIPROC) (GLint  c);
typedef void (*PFNGLINDEXIVPROC) (const GLint * c);
typedef void (*PFNGLINDEXSPROC) (GLshort  c);
typedef void (*PFNGLINDEXSVPROC) (const GLshort * c);
typedef void (*PFNGLINDEXUBPROC) (GLubyte  c);
typedef void (*PFNGLINDEXUBVPROC) (const GLubyte * c);
typedef void (*PFNGLINITNAMESPROC) ();
typedef void (*PFNGLINSERTCOMPONENTEXTPROC) (GLuint  res, GLuint  src, GLuint  num);
typedef void (*PFNGLINSERTEVENTMARKEREXTPROC) (GLsizei  length, const GLchar * marker);
typedef void (*PFNGLINTERLEAVEDARRAYSPROC) (GLenum  format, GLsizei  stride, const void * pointer);
typedef void (*PFNGLINTERPOLATEPATHSNVPROC) (GLuint  resultPath, GLuint  pathA, GLuint  pathB, GLfloat  weight);
typedef void (*PFNGLINVALIDATEBUFFERDATAPROC) (GLuint  buffer);
typedef void (*PFNGLINVALIDATEBUFFERSUBDATAPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  length);
typedef void (*PFNGLINVALIDATEFRAMEBUFFERPROC) (GLenum  target, GLsizei  numAttachments, const GLenum * attachments);
typedef void (*PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC) (GLuint  framebuffer, GLsizei  numAttachments, const GLenum * attachments);
typedef void (*PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC) (GLuint  framebuffer, GLsizei  numAttachments, const GLenum * attachments, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLINVALIDATESUBFRAMEBUFFERPROC) (GLenum  target, GLsizei  numAttachments, const GLenum * attachments, GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLINVALIDATETEXIMAGEPROC) (GLuint  texture, GLint  level);
typedef void (*PFNGLINVALIDATETEXSUBIMAGEPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth);
typedef GLboolean (*PFNGLISBUFFERPROC) (GLuint  buffer);
typedef GLboolean (*PFNGLISBUFFERARBPROC) (GLuint  buffer);
typedef GLboolean (*PFNGLISBUFFERRESIDENTNVPROC) (GLenum  target);
typedef GLboolean (*PFNGLISCOMMANDLISTNVPROC) (GLuint  list);
typedef GLboolean (*PFNGLISENABLEDPROC) (GLenum  cap);
typedef GLboolean (*PFNGLISENABLEDINDEXEDEXTPROC) (GLenum  target, GLuint  index);
typedef GLboolean (*PFNGLISENABLEDIPROC) (GLenum  target, GLuint  index);
typedef GLboolean (*PFNGLISFENCEAPPLEPROC) (GLuint  fence);
typedef GLboolean (*PFNGLISFENCENVPROC) (GLuint  fence);
typedef GLboolean (*PFNGLISFRAMEBUFFERPROC) (GLuint  framebuffer);
typedef GLboolean (*PFNGLISFRAMEBUFFEREXTPROC) (GLuint  framebuffer);
typedef GLboolean (*PFNGLISIMAGEHANDLERESIDENTARBPROC) (GLuint64  handle);
typedef GLboolean (*PFNGLISIMAGEHANDLERESIDENTNVPROC) (GLuint64  handle);
typedef GLboolean (*PFNGLISLISTPROC) (GLuint  list);
typedef GLboolean (*PFNGLISMEMORYOBJECTEXTPROC) (GLuint  memoryObject);
typedef GLboolean (*PFNGLISNAMEAMDPROC) (GLenum  identifier, GLuint  name);
typedef GLboolean (*PFNGLISNAMEDBUFFERRESIDENTNVPROC) (GLuint  buffer);
typedef GLboolean (*PFNGLISNAMEDSTRINGARBPROC) (GLint  namelen, const GLchar * name);
typedef GLboolean (*PFNGLISOCCLUSIONQUERYNVPROC) (GLuint  id);
typedef GLboolean (*PFNGLISPATHNVPROC) (GLuint  path);
typedef GLboolean (*PFNGLISPOINTINFILLPATHNVPROC) (GLuint  path, GLuint  mask, GLfloat  x, GLfloat  y);
typedef GLboolean (*PFNGLISPOINTINSTROKEPATHNVPROC) (GLuint  path, GLfloat  x, GLfloat  y);
typedef GLboolean (*PFNGLISPROGRAMPROC) (GLuint  program);
typedef GLboolean (*PFNGLISPROGRAMARBPROC) (GLuint  program);
typedef GLboolean (*PFNGLISPROGRAMNVPROC) (GLuint  id);
typedef GLboolean (*PFNGLISPROGRAMPIPELINEPROC) (GLuint  pipeline);
typedef GLboolean (*PFNGLISPROGRAMPIPELINEEXTPROC) (GLuint  pipeline);
typedef GLboolean (*PFNGLISQUERYPROC) (GLuint  id);
typedef GLboolean (*PFNGLISQUERYARBPROC) (GLuint  id);
typedef GLboolean (*PFNGLISRENDERBUFFERPROC) (GLuint  renderbuffer);
typedef GLboolean (*PFNGLISRENDERBUFFEREXTPROC) (GLuint  renderbuffer);
typedef GLboolean (*PFNGLISSEMAPHOREEXTPROC) (GLuint  semaphore);
typedef GLboolean (*PFNGLISSAMPLERPROC) (GLuint  sampler);
typedef GLboolean (*PFNGLISSHADERPROC) (GLuint  shader);
typedef GLboolean (*PFNGLISSTATENVPROC) (GLuint  state);
typedef GLboolean (*PFNGLISSYNCPROC) (GLsync  sync);
typedef GLboolean (*PFNGLISTEXTUREPROC) (GLuint  texture);
typedef GLboolean (*PFNGLISTEXTUREEXTPROC) (GLuint  texture);
typedef GLboolean (*PFNGLISTEXTUREHANDLERESIDENTARBPROC) (GLuint64  handle);
typedef GLboolean (*PFNGLISTEXTUREHANDLERESIDENTNVPROC) (GLuint64  handle);
typedef GLboolean (*PFNGLISTRANSFORMFEEDBACKPROC) (GLuint  id);
typedef GLboolean (*PFNGLISTRANSFORMFEEDBACKNVPROC) (GLuint  id);
typedef GLboolean (*PFNGLISVARIANTENABLEDEXTPROC) (GLuint  id, GLenum  cap);
typedef GLboolean (*PFNGLISVERTEXARRAYPROC) (GLuint  array);
typedef GLboolean (*PFNGLISVERTEXARRAYAPPLEPROC) (GLuint  array);
typedef GLboolean (*PFNGLISVERTEXATTRIBENABLEDAPPLEPROC) (GLuint  index, GLenum  pname);
typedef void (*PFNGLLGPUCOPYIMAGESUBDATANVXPROC) (GLuint  sourceGpu, GLbitfield  destinationGpuMask, GLuint  srcName, GLenum  srcTarget, GLint  srcLevel, GLint  srcX, GLint  srxY, GLint  srcZ, GLuint  dstName, GLenum  dstTarget, GLint  dstLevel, GLint  dstX, GLint  dstY, GLint  dstZ, GLsizei  width, GLsizei  height, GLsizei  depth);
typedef void (*PFNGLLGPUINTERLOCKNVXPROC) ();
typedef void (*PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC) (GLbitfield  gpuMask, GLuint  buffer, GLintptr  offset, GLsizeiptr  size, const void * data);
typedef void (*PFNGLLABELOBJECTEXTPROC) (GLenum  type, GLuint  object, GLsizei  length, const GLchar * label);
typedef void (*PFNGLLIGHTMODELFPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLLIGHTMODELFVPROC) (GLenum  pname, const GLfloat * params);
typedef void (*PFNGLLIGHTMODELIPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLLIGHTMODELIVPROC) (GLenum  pname, const GLint * params);
typedef void (*PFNGLLIGHTFPROC) (GLenum  light, GLenum  pname, GLfloat  param);
typedef void (*PFNGLLIGHTFVPROC) (GLenum  light, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLLIGHTIPROC) (GLenum  light, GLenum  pname, GLint  param);
typedef void (*PFNGLLIGHTIVPROC) (GLenum  light, GLenum  pname, const GLint * params);
typedef void (*PFNGLLINESTIPPLEPROC) (GLint  factor, GLushort  pattern);
typedef void (*PFNGLLINEWIDTHPROC) (GLfloat  width);
typedef void (*PFNGLLINKPROGRAMPROC) (GLuint  program);
typedef void (*PFNGLLINKPROGRAMARBPROC) (GLhandleARB  programObj);
typedef void (*PFNGLLISTBASEPROC) (GLuint  base);
typedef void (*PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC) (GLuint  list, GLuint  segment, const void ** indirects, const GLsizei * sizes, const GLuint * states, const GLuint * fbos, GLuint  count);
typedef void (*PFNGLLOADIDENTITYPROC) ();
typedef void (*PFNGLLOADMATRIXDPROC) (const GLdouble * m);
typedef void (*PFNGLLOADMATRIXFPROC) (const GLfloat * m);
typedef void (*PFNGLLOADNAMEPROC) (GLuint  name);
typedef void (*PFNGLLOADPROGRAMNVPROC) (GLenum  target, GLuint  id, GLsizei  len, const GLubyte * program);
typedef void (*PFNGLLOADTRANSPOSEMATRIXDPROC) (const GLdouble * m);
typedef void (*PFNGLLOADTRANSPOSEMATRIXDARBPROC) (const GLdouble * m);
typedef void (*PFNGLLOADTRANSPOSEMATRIXFPROC) (const GLfloat * m);
typedef void (*PFNGLLOADTRANSPOSEMATRIXFARBPROC) (const GLfloat * m);
typedef void (*PFNGLLOCKARRAYSEXTPROC) (GLint  first, GLsizei  count);
typedef void (*PFNGLLOGICOPPROC) (GLenum  opcode);
typedef void (*PFNGLMAKEBUFFERNONRESIDENTNVPROC) (GLenum  target);
typedef void (*PFNGLMAKEBUFFERRESIDENTNVPROC) (GLenum  target, GLenum  access);
typedef void (*PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC) (GLuint64  handle);
typedef void (*PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC) (GLuint64  handle);
typedef void (*PFNGLMAKEIMAGEHANDLERESIDENTARBPROC) (GLuint64  handle, GLenum  access);
typedef void (*PFNGLMAKEIMAGEHANDLERESIDENTNVPROC) (GLuint64  handle, GLenum  access);
typedef void (*PFNGLMAKENAMEDBUFFERNONRESIDENTNVPROC) (GLuint  buffer);
typedef void (*PFNGLMAKENAMEDBUFFERRESIDENTNVPROC) (GLuint  buffer, GLenum  access);
typedef void (*PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC) (GLuint64  handle);
typedef void (*PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC) (GLuint64  handle);
typedef void (*PFNGLMAKETEXTUREHANDLERESIDENTARBPROC) (GLuint64  handle);
typedef void (*PFNGLMAKETEXTUREHANDLERESIDENTNVPROC) (GLuint64  handle);
typedef void (*PFNGLMAP1DPROC) (GLenum  target, GLdouble  u1, GLdouble  u2, GLint  stride, GLint  order, const GLdouble * points);
typedef void (*PFNGLMAP1FPROC) (GLenum  target, GLfloat  u1, GLfloat  u2, GLint  stride, GLint  order, const GLfloat * points);
typedef void (*PFNGLMAP2DPROC) (GLenum  target, GLdouble  u1, GLdouble  u2, GLint  ustride, GLint  uorder, GLdouble  v1, GLdouble  v2, GLint  vstride, GLint  vorder, const GLdouble * points);
typedef void (*PFNGLMAP2FPROC) (GLenum  target, GLfloat  u1, GLfloat  u2, GLint  ustride, GLint  uorder, GLfloat  v1, GLfloat  v2, GLint  vstride, GLint  vorder, const GLfloat * points);
typedef void *(*PFNGLMAPBUFFERPROC) (GLenum  target, GLenum  access);
typedef void *(*PFNGLMAPBUFFERARBPROC) (GLenum  target, GLenum  access);
typedef void *(*PFNGLMAPBUFFERRANGEPROC) (GLenum  target, GLintptr  offset, GLsizeiptr  length, GLbitfield  access);
typedef void (*PFNGLMAPCONTROLPOINTSNVPROC) (GLenum  target, GLuint  index, GLenum  type, GLsizei  ustride, GLsizei  vstride, GLint  uorder, GLint  vorder, GLboolean  packed, const void * points);
typedef void (*PFNGLMAPGRID1DPROC) (GLint  un, GLdouble  u1, GLdouble  u2);
typedef void (*PFNGLMAPGRID1FPROC) (GLint  un, GLfloat  u1, GLfloat  u2);
typedef void (*PFNGLMAPGRID2DPROC) (GLint  un, GLdouble  u1, GLdouble  u2, GLint  vn, GLdouble  v1, GLdouble  v2);
typedef void (*PFNGLMAPGRID2FPROC) (GLint  un, GLfloat  u1, GLfloat  u2, GLint  vn, GLfloat  v1, GLfloat  v2);
typedef void *(*PFNGLMAPNAMEDBUFFERPROC) (GLuint  buffer, GLenum  access);
typedef void *(*PFNGLMAPNAMEDBUFFEREXTPROC) (GLuint  buffer, GLenum  access);
typedef void *(*PFNGLMAPNAMEDBUFFERRANGEPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  length, GLbitfield  access);
typedef void *(*PFNGLMAPNAMEDBUFFERRANGEEXTPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  length, GLbitfield  access);
typedef void (*PFNGLMAPPARAMETERFVNVPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLMAPPARAMETERIVNVPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void *(*PFNGLMAPTEXTURE2DINTELPROC) (GLuint  texture, GLint  level, GLbitfield  access, GLint * stride, GLenum * layout);
typedef void (*PFNGLMAPVERTEXATTRIB1DAPPLEPROC) (GLuint  index, GLuint  size, GLdouble  u1, GLdouble  u2, GLint  stride, GLint  order, const GLdouble * points);
typedef void (*PFNGLMAPVERTEXATTRIB1FAPPLEPROC) (GLuint  index, GLuint  size, GLfloat  u1, GLfloat  u2, GLint  stride, GLint  order, const GLfloat * points);
typedef void (*PFNGLMAPVERTEXATTRIB2DAPPLEPROC) (GLuint  index, GLuint  size, GLdouble  u1, GLdouble  u2, GLint  ustride, GLint  uorder, GLdouble  v1, GLdouble  v2, GLint  vstride, GLint  vorder, const GLdouble * points);
typedef void (*PFNGLMAPVERTEXATTRIB2FAPPLEPROC) (GLuint  index, GLuint  size, GLfloat  u1, GLfloat  u2, GLint  ustride, GLint  uorder, GLfloat  v1, GLfloat  v2, GLint  vstride, GLint  vorder, const GLfloat * points);
typedef void (*PFNGLMATERIALFPROC) (GLenum  face, GLenum  pname, GLfloat  param);
typedef void (*PFNGLMATERIALFVPROC) (GLenum  face, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLMATERIALIPROC) (GLenum  face, GLenum  pname, GLint  param);
typedef void (*PFNGLMATERIALIVPROC) (GLenum  face, GLenum  pname, const GLint * params);
typedef void (*PFNGLMATRIXFRUSTUMEXTPROC) (GLenum  mode, GLdouble  left, GLdouble  right, GLdouble  bottom, GLdouble  top, GLdouble  zNear, GLdouble  zFar);
typedef void (*PFNGLMATRIXINDEXPOINTERARBPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLMATRIXINDEXUBVARBPROC) (GLint  size, const GLubyte * indices);
typedef void (*PFNGLMATRIXINDEXUIVARBPROC) (GLint  size, const GLuint * indices);
typedef void (*PFNGLMATRIXINDEXUSVARBPROC) (GLint  size, const GLushort * indices);
typedef void (*PFNGLMATRIXLOAD3X2FNVPROC) (GLenum  matrixMode, const GLfloat * m);
typedef void (*PFNGLMATRIXLOAD3X3FNVPROC) (GLenum  matrixMode, const GLfloat * m);
typedef void (*PFNGLMATRIXLOADIDENTITYEXTPROC) (GLenum  mode);
typedef void (*PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC) (GLenum  matrixMode, const GLfloat * m);
typedef void (*PFNGLMATRIXLOADTRANSPOSEDEXTPROC) (GLenum  mode, const GLdouble * m);
typedef void (*PFNGLMATRIXLOADTRANSPOSEFEXTPROC) (GLenum  mode, const GLfloat * m);
typedef void (*PFNGLMATRIXLOADDEXTPROC) (GLenum  mode, const GLdouble * m);
typedef void (*PFNGLMATRIXLOADFEXTPROC) (GLenum  mode, const GLfloat * m);
typedef void (*PFNGLMATRIXMODEPROC) (GLenum  mode);
typedef void (*PFNGLMATRIXMULT3X2FNVPROC) (GLenum  matrixMode, const GLfloat * m);
typedef void (*PFNGLMATRIXMULT3X3FNVPROC) (GLenum  matrixMode, const GLfloat * m);
typedef void (*PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC) (GLenum  matrixMode, const GLfloat * m);
typedef void (*PFNGLMATRIXMULTTRANSPOSEDEXTPROC) (GLenum  mode, const GLdouble * m);
typedef void (*PFNGLMATRIXMULTTRANSPOSEFEXTPROC) (GLenum  mode, const GLfloat * m);
typedef void (*PFNGLMATRIXMULTDEXTPROC) (GLenum  mode, const GLdouble * m);
typedef void (*PFNGLMATRIXMULTFEXTPROC) (GLenum  mode, const GLfloat * m);
typedef void (*PFNGLMATRIXORTHOEXTPROC) (GLenum  mode, GLdouble  left, GLdouble  right, GLdouble  bottom, GLdouble  top, GLdouble  zNear, GLdouble  zFar);
typedef void (*PFNGLMATRIXPOPEXTPROC) (GLenum  mode);
typedef void (*PFNGLMATRIXPUSHEXTPROC) (GLenum  mode);
typedef void (*PFNGLMATRIXROTATEDEXTPROC) (GLenum  mode, GLdouble  angle, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLMATRIXROTATEFEXTPROC) (GLenum  mode, GLfloat  angle, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLMATRIXSCALEDEXTPROC) (GLenum  mode, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLMATRIXSCALEFEXTPROC) (GLenum  mode, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLMATRIXTRANSLATEDEXTPROC) (GLenum  mode, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLMATRIXTRANSLATEFEXTPROC) (GLenum  mode, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLMAXSHADERCOMPILERTHREADSKHRPROC) (GLuint  count);
typedef void (*PFNGLMAXSHADERCOMPILERTHREADSARBPROC) (GLuint  count);
typedef void (*PFNGLMEMORYBARRIERPROC) (GLbitfield  barriers);
typedef void (*PFNGLMEMORYBARRIERBYREGIONPROC) (GLbitfield  barriers);
typedef void (*PFNGLMEMORYBARRIEREXTPROC) (GLbitfield  barriers);
typedef void (*PFNGLMEMORYOBJECTPARAMETERIVEXTPROC) (GLuint  memoryObject, GLenum  pname, const GLint * params);
typedef void (*PFNGLMINSAMPLESHADINGPROC) (GLfloat  value);
typedef void (*PFNGLMINSAMPLESHADINGARBPROC) (GLfloat  value);
typedef void (*PFNGLMINMAXPROC) (GLenum  target, GLenum  internalformat, GLboolean  sink);
typedef void (*PFNGLMINMAXEXTPROC) (GLenum  target, GLenum  internalformat, GLboolean  sink);
typedef void (*PFNGLMULTMATRIXDPROC) (const GLdouble * m);
typedef void (*PFNGLMULTMATRIXFPROC) (const GLfloat * m);
typedef void (*PFNGLMULTTRANSPOSEMATRIXDPROC) (const GLdouble * m);
typedef void (*PFNGLMULTTRANSPOSEMATRIXDARBPROC) (const GLdouble * m);
typedef void (*PFNGLMULTTRANSPOSEMATRIXFPROC) (const GLfloat * m);
typedef void (*PFNGLMULTTRANSPOSEMATRIXFARBPROC) (const GLfloat * m);
typedef void (*PFNGLMULTIDRAWARRAYSPROC) (GLenum  mode, const GLint * first, const GLsizei * count, GLsizei  drawcount);
typedef void (*PFNGLMULTIDRAWARRAYSEXTPROC) (GLenum  mode, const GLint * first, const GLsizei * count, GLsizei  primcount);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTPROC) (GLenum  mode, const void * indirect, GLsizei  drawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTAMDPROC) (GLenum  mode, const void * indirect, GLsizei  primcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNVPROC) (GLenum  mode, const void * indirect, GLsizei  drawCount, GLsizei  maxDrawCount, GLsizei  stride, GLint  vertexBufferCount);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSNVPROC) (GLenum  mode, const void * indirect, GLsizei  drawCount, GLsizei  stride, GLint  vertexBufferCount);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC) (GLenum  mode, const void * indirect, GLintptr  drawcount, GLsizei  maxdrawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC) (GLenum  mode, const void * indirect, GLintptr  drawcount, GLsizei  maxdrawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWELEMENTARRAYAPPLEPROC) (GLenum  mode, const GLint * first, const GLsizei * count, GLsizei  primcount);
typedef void (*PFNGLMULTIDRAWELEMENTSPROC) (GLenum  mode, const GLsizei * count, GLenum  type, const void *const* indices, GLsizei  drawcount);
typedef void (*PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) (GLenum  mode, const GLsizei * count, GLenum  type, const void *const* indices, GLsizei  drawcount, const GLint * basevertex);
typedef void (*PFNGLMULTIDRAWELEMENTSEXTPROC) (GLenum  mode, const GLsizei * count, GLenum  type, const void *const* indices, GLsizei  primcount);
typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTPROC) (GLenum  mode, GLenum  type, const void * indirect, GLsizei  drawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTAMDPROC) (GLenum  mode, GLenum  type, const void * indirect, GLsizei  primcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNVPROC) (GLenum  mode, GLenum  type, const void * indirect, GLsizei  drawCount, GLsizei  maxDrawCount, GLsizei  stride, GLint  vertexBufferCount);
typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSNVPROC) (GLenum  mode, GLenum  type, const void * indirect, GLsizei  drawCount, GLsizei  stride, GLint  vertexBufferCount);
typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC) (GLenum  mode, GLenum  type, const void * indirect, GLintptr  drawcount, GLsizei  maxdrawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC) (GLenum  mode, GLenum  type, const void * indirect, GLintptr  drawcount, GLsizei  maxdrawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC) (GLintptr  indirect, GLsizei  drawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC) (GLintptr  indirect, GLintptr  drawcount, GLsizei  maxdrawcount, GLsizei  stride);
typedef void (*PFNGLMULTIDRAWRANGEELEMENTARRAYAPPLEPROC) (GLenum  mode, GLuint  start, GLuint  end, const GLint * first, const GLsizei * count, GLsizei  primcount);
typedef void (*PFNGLMULTITEXBUFFEREXTPROC) (GLenum  texunit, GLenum  target, GLenum  internalformat, GLuint  buffer);
typedef void (*PFNGLMULTITEXCOORD1DPROC) (GLenum  target, GLdouble  s);
typedef void (*PFNGLMULTITEXCOORD1DARBPROC) (GLenum  target, GLdouble  s);
typedef void (*PFNGLMULTITEXCOORD1DVPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD1DVARBPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD1FPROC) (GLenum  target, GLfloat  s);
typedef void (*PFNGLMULTITEXCOORD1FARBPROC) (GLenum  target, GLfloat  s);
typedef void (*PFNGLMULTITEXCOORD1FVPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD1FVARBPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD1HNVPROC) (GLenum  target, GLhalfNV  s);
typedef void (*PFNGLMULTITEXCOORD1HVNVPROC) (GLenum  target, const GLhalfNV * v);
typedef void (*PFNGLMULTITEXCOORD1IPROC) (GLenum  target, GLint  s);
typedef void (*PFNGLMULTITEXCOORD1IARBPROC) (GLenum  target, GLint  s);
typedef void (*PFNGLMULTITEXCOORD1IVPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD1IVARBPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD1SPROC) (GLenum  target, GLshort  s);
typedef void (*PFNGLMULTITEXCOORD1SARBPROC) (GLenum  target, GLshort  s);
typedef void (*PFNGLMULTITEXCOORD1SVPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD1SVARBPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD2DPROC) (GLenum  target, GLdouble  s, GLdouble  t);
typedef void (*PFNGLMULTITEXCOORD2DARBPROC) (GLenum  target, GLdouble  s, GLdouble  t);
typedef void (*PFNGLMULTITEXCOORD2DVPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD2DVARBPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD2FPROC) (GLenum  target, GLfloat  s, GLfloat  t);
typedef void (*PFNGLMULTITEXCOORD2FARBPROC) (GLenum  target, GLfloat  s, GLfloat  t);
typedef void (*PFNGLMULTITEXCOORD2FVPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD2FVARBPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD2HNVPROC) (GLenum  target, GLhalfNV  s, GLhalfNV  t);
typedef void (*PFNGLMULTITEXCOORD2HVNVPROC) (GLenum  target, const GLhalfNV * v);
typedef void (*PFNGLMULTITEXCOORD2IPROC) (GLenum  target, GLint  s, GLint  t);
typedef void (*PFNGLMULTITEXCOORD2IARBPROC) (GLenum  target, GLint  s, GLint  t);
typedef void (*PFNGLMULTITEXCOORD2IVPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD2IVARBPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD2SPROC) (GLenum  target, GLshort  s, GLshort  t);
typedef void (*PFNGLMULTITEXCOORD2SARBPROC) (GLenum  target, GLshort  s, GLshort  t);
typedef void (*PFNGLMULTITEXCOORD2SVPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD2SVARBPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD3DPROC) (GLenum  target, GLdouble  s, GLdouble  t, GLdouble  r);
typedef void (*PFNGLMULTITEXCOORD3DARBPROC) (GLenum  target, GLdouble  s, GLdouble  t, GLdouble  r);
typedef void (*PFNGLMULTITEXCOORD3DVPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD3DVARBPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD3FPROC) (GLenum  target, GLfloat  s, GLfloat  t, GLfloat  r);
typedef void (*PFNGLMULTITEXCOORD3FARBPROC) (GLenum  target, GLfloat  s, GLfloat  t, GLfloat  r);
typedef void (*PFNGLMULTITEXCOORD3FVPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD3FVARBPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD3HNVPROC) (GLenum  target, GLhalfNV  s, GLhalfNV  t, GLhalfNV  r);
typedef void (*PFNGLMULTITEXCOORD3HVNVPROC) (GLenum  target, const GLhalfNV * v);
typedef void (*PFNGLMULTITEXCOORD3IPROC) (GLenum  target, GLint  s, GLint  t, GLint  r);
typedef void (*PFNGLMULTITEXCOORD3IARBPROC) (GLenum  target, GLint  s, GLint  t, GLint  r);
typedef void (*PFNGLMULTITEXCOORD3IVPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD3IVARBPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD3SPROC) (GLenum  target, GLshort  s, GLshort  t, GLshort  r);
typedef void (*PFNGLMULTITEXCOORD3SARBPROC) (GLenum  target, GLshort  s, GLshort  t, GLshort  r);
typedef void (*PFNGLMULTITEXCOORD3SVPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD3SVARBPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD4DPROC) (GLenum  target, GLdouble  s, GLdouble  t, GLdouble  r, GLdouble  q);
typedef void (*PFNGLMULTITEXCOORD4DARBPROC) (GLenum  target, GLdouble  s, GLdouble  t, GLdouble  r, GLdouble  q);
typedef void (*PFNGLMULTITEXCOORD4DVPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD4DVARBPROC) (GLenum  target, const GLdouble * v);
typedef void (*PFNGLMULTITEXCOORD4FPROC) (GLenum  target, GLfloat  s, GLfloat  t, GLfloat  r, GLfloat  q);
typedef void (*PFNGLMULTITEXCOORD4FARBPROC) (GLenum  target, GLfloat  s, GLfloat  t, GLfloat  r, GLfloat  q);
typedef void (*PFNGLMULTITEXCOORD4FVPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD4FVARBPROC) (GLenum  target, const GLfloat * v);
typedef void (*PFNGLMULTITEXCOORD4HNVPROC) (GLenum  target, GLhalfNV  s, GLhalfNV  t, GLhalfNV  r, GLhalfNV  q);
typedef void (*PFNGLMULTITEXCOORD4HVNVPROC) (GLenum  target, const GLhalfNV * v);
typedef void (*PFNGLMULTITEXCOORD4IPROC) (GLenum  target, GLint  s, GLint  t, GLint  r, GLint  q);
typedef void (*PFNGLMULTITEXCOORD4IARBPROC) (GLenum  target, GLint  s, GLint  t, GLint  r, GLint  q);
typedef void (*PFNGLMULTITEXCOORD4IVPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD4IVARBPROC) (GLenum  target, const GLint * v);
typedef void (*PFNGLMULTITEXCOORD4SPROC) (GLenum  target, GLshort  s, GLshort  t, GLshort  r, GLshort  q);
typedef void (*PFNGLMULTITEXCOORD4SARBPROC) (GLenum  target, GLshort  s, GLshort  t, GLshort  r, GLshort  q);
typedef void (*PFNGLMULTITEXCOORD4SVPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORD4SVARBPROC) (GLenum  target, const GLshort * v);
typedef void (*PFNGLMULTITEXCOORDP1UIPROC) (GLenum  texture, GLenum  type, GLuint  coords);
typedef void (*PFNGLMULTITEXCOORDP1UIVPROC) (GLenum  texture, GLenum  type, const GLuint * coords);
typedef void (*PFNGLMULTITEXCOORDP2UIPROC) (GLenum  texture, GLenum  type, GLuint  coords);
typedef void (*PFNGLMULTITEXCOORDP2UIVPROC) (GLenum  texture, GLenum  type, const GLuint * coords);
typedef void (*PFNGLMULTITEXCOORDP3UIPROC) (GLenum  texture, GLenum  type, GLuint  coords);
typedef void (*PFNGLMULTITEXCOORDP3UIVPROC) (GLenum  texture, GLenum  type, const GLuint * coords);
typedef void (*PFNGLMULTITEXCOORDP4UIPROC) (GLenum  texture, GLenum  type, GLuint  coords);
typedef void (*PFNGLMULTITEXCOORDP4UIVPROC) (GLenum  texture, GLenum  type, const GLuint * coords);
typedef void (*PFNGLMULTITEXCOORDPOINTEREXTPROC) (GLenum  texunit, GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLMULTITEXENVFEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLfloat  param);
typedef void (*PFNGLMULTITEXENVFVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLMULTITEXENVIEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLMULTITEXENVIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLMULTITEXGENDEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, GLdouble  param);
typedef void (*PFNGLMULTITEXGENDVEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, const GLdouble * params);
typedef void (*PFNGLMULTITEXGENFEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, GLfloat  param);
typedef void (*PFNGLMULTITEXGENFVEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLMULTITEXGENIEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, GLint  param);
typedef void (*PFNGLMULTITEXGENIVEXTPROC) (GLenum  texunit, GLenum  coord, GLenum  pname, const GLint * params);
typedef void (*PFNGLMULTITEXIMAGE1DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLMULTITEXIMAGE2DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLMULTITEXIMAGE3DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLMULTITEXPARAMETERIIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLMULTITEXPARAMETERIUIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, const GLuint * params);
typedef void (*PFNGLMULTITEXPARAMETERFEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLfloat  param);
typedef void (*PFNGLMULTITEXPARAMETERFVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLMULTITEXPARAMETERIEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLMULTITEXPARAMETERIVEXTPROC) (GLenum  texunit, GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLMULTITEXRENDERBUFFEREXTPROC) (GLenum  texunit, GLenum  target, GLuint  renderbuffer);
typedef void (*PFNGLMULTITEXSUBIMAGE1DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLMULTITEXSUBIMAGE2DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLMULTITEXSUBIMAGE3DEXTPROC) (GLenum  texunit, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLMULTICASTBARRIERNVPROC) ();
typedef void (*PFNGLMULTICASTBLITFRAMEBUFFERNVPROC) (GLuint  srcGpu, GLuint  dstGpu, GLint  srcX0, GLint  srcY0, GLint  srcX1, GLint  srcY1, GLint  dstX0, GLint  dstY0, GLint  dstX1, GLint  dstY1, GLbitfield  mask, GLenum  filter);
typedef void (*PFNGLMULTICASTBUFFERSUBDATANVPROC) (GLbitfield  gpuMask, GLuint  buffer, GLintptr  offset, GLsizeiptr  size, const void * data);
typedef void (*PFNGLMULTICASTCOPYBUFFERSUBDATANVPROC) (GLuint  readGpu, GLbitfield  writeGpuMask, GLuint  readBuffer, GLuint  writeBuffer, GLintptr  readOffset, GLintptr  writeOffset, GLsizeiptr  size);
typedef void (*PFNGLMULTICASTCOPYIMAGESUBDATANVPROC) (GLuint  srcGpu, GLbitfield  dstGpuMask, GLuint  srcName, GLenum  srcTarget, GLint  srcLevel, GLint  srcX, GLint  srcY, GLint  srcZ, GLuint  dstName, GLenum  dstTarget, GLint  dstLevel, GLint  dstX, GLint  dstY, GLint  dstZ, GLsizei  srcWidth, GLsizei  srcHeight, GLsizei  srcDepth);
typedef void (*PFNGLMULTICASTFRAMEBUFFERSAMPLELOCATIONSFVNVPROC) (GLuint  gpu, GLuint  framebuffer, GLuint  start, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLMULTICASTGETQUERYOBJECTI64VNVPROC) (GLuint  gpu, GLuint  id, GLenum  pname, GLint64 * params);
typedef void (*PFNGLMULTICASTGETQUERYOBJECTIVNVPROC) (GLuint  gpu, GLuint  id, GLenum  pname, GLint * params);
typedef void (*PFNGLMULTICASTGETQUERYOBJECTUI64VNVPROC) (GLuint  gpu, GLuint  id, GLenum  pname, GLuint64 * params);
typedef void (*PFNGLMULTICASTGETQUERYOBJECTUIVNVPROC) (GLuint  gpu, GLuint  id, GLenum  pname, GLuint * params);
typedef void (*PFNGLMULTICASTSCISSORARRAYVNVXPROC) (GLuint  gpu, GLuint  first, GLsizei  count, const GLint * v);
typedef void (*PFNGLMULTICASTVIEWPORTARRAYVNVXPROC) (GLuint  gpu, GLuint  first, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLMULTICASTVIEWPORTPOSITIONWSCALENVXPROC) (GLuint  gpu, GLuint  index, GLfloat  xcoeff, GLfloat  ycoeff);
typedef void (*PFNGLMULTICASTWAITSYNCNVPROC) (GLuint  signalGpu, GLbitfield  waitGpuMask);
typedef void (*PFNGLNAMEDBUFFERATTACHMEMORYNVPROC) (GLuint  buffer, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLNAMEDBUFFERDATAPROC) (GLuint  buffer, GLsizeiptr  size, const void * data, GLenum  usage);
typedef void (*PFNGLNAMEDBUFFERDATAEXTPROC) (GLuint  buffer, GLsizeiptr  size, const void * data, GLenum  usage);
typedef void (*PFNGLNAMEDBUFFERPAGECOMMITMENTARBPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, GLboolean  commit);
typedef void (*PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, GLboolean  commit);
typedef void (*PFNGLNAMEDBUFFERPAGECOMMITMENTMEMNVPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, GLuint  memory, GLuint64  memOffset, GLboolean  commit);
typedef void (*PFNGLNAMEDBUFFERSTORAGEPROC) (GLuint  buffer, GLsizeiptr  size, const void * data, GLbitfield  flags);
typedef void (*PFNGLNAMEDBUFFERSTORAGEEXTERNALEXTPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, GLeglClientBufferEXT  clientBuffer, GLbitfield  flags);
typedef void (*PFNGLNAMEDBUFFERSTORAGEEXTPROC) (GLuint  buffer, GLsizeiptr  size, const void * data, GLbitfield  flags);
typedef void (*PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC) (GLuint  buffer, GLsizeiptr  size, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLNAMEDBUFFERSUBDATAPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, const void * data);
typedef void (*PFNGLNAMEDBUFFERSUBDATAEXTPROC) (GLuint  buffer, GLintptr  offset, GLsizeiptr  size, const void * data);
typedef void (*PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC) (GLuint  readBuffer, GLuint  writeBuffer, GLintptr  readOffset, GLintptr  writeOffset, GLsizeiptr  size);
typedef void (*PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC) (GLuint  framebuffer, GLenum  buf);
typedef void (*PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC) (GLuint  framebuffer, GLsizei  n, const GLenum * bufs);
typedef void (*PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC) (GLuint  framebuffer, GLenum  pname, GLint  param);
typedef void (*PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC) (GLuint  framebuffer, GLenum  pname, GLint  param);
typedef void (*PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC) (GLuint  framebuffer, GLenum  src);
typedef void (*PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  renderbuffertarget, GLuint  renderbuffer);
typedef void (*PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  renderbuffertarget, GLuint  renderbuffer);
typedef void (*PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC) (GLuint  framebuffer, GLuint  start, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC) (GLuint  framebuffer, GLuint  start, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTUREPROC) (GLuint  framebuffer, GLenum  attachment, GLuint  texture, GLint  level);
typedef void (*PFNGLNAMEDFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC) (GLuint  framebuffer, GLuint  numsamples, GLuint  pixelindex, const GLfloat * values);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC) (GLuint  framebuffer, GLenum  attachment, GLenum  textarget, GLuint  texture, GLint  level, GLint  zoffset);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC) (GLuint  framebuffer, GLenum  attachment, GLuint  texture, GLint  level);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC) (GLuint  framebuffer, GLenum  attachment, GLuint  texture, GLint  level, GLenum  face);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC) (GLuint  framebuffer, GLenum  attachment, GLuint  texture, GLint  level, GLint  layer);
typedef void (*PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC) (GLuint  framebuffer, GLenum  attachment, GLuint  texture, GLint  level, GLint  layer);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, const GLdouble * params);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, const GLfloat * params);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, const GLint * params);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLuint  x, GLuint  y, GLuint  z, GLuint  w);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, const GLuint * params);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLsizei  count, const GLfloat * params);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLsizei  count, const GLint * params);
typedef void (*PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC) (GLuint  program, GLenum  target, GLuint  index, GLsizei  count, const GLuint * params);
typedef void (*PFNGLNAMEDPROGRAMSTRINGEXTPROC) (GLuint  program, GLenum  target, GLenum  format, GLsizei  len, const void * string);
typedef void (*PFNGLNAMEDRENDERBUFFERSTORAGEPROC) (GLuint  renderbuffer, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC) (GLuint  renderbuffer, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC) (GLuint  renderbuffer, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC) (GLuint  renderbuffer, GLsizei  samples, GLsizei  storageSamples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC) (GLuint  renderbuffer, GLsizei  coverageSamples, GLsizei  colorSamples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLuint  renderbuffer, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLNAMEDSTRINGARBPROC) (GLenum  type, GLint  namelen, const GLchar * name, GLint  stringlen, const GLchar * string);
typedef void (*PFNGLNEWLISTPROC) (GLuint  list, GLenum  mode);
typedef void (*PFNGLNORMAL3BPROC) (GLbyte  nx, GLbyte  ny, GLbyte  nz);
typedef void (*PFNGLNORMAL3BVPROC) (const GLbyte * v);
typedef void (*PFNGLNORMAL3DPROC) (GLdouble  nx, GLdouble  ny, GLdouble  nz);
typedef void (*PFNGLNORMAL3DVPROC) (const GLdouble * v);
typedef void (*PFNGLNORMAL3FPROC) (GLfloat  nx, GLfloat  ny, GLfloat  nz);
typedef void (*PFNGLNORMAL3FVPROC) (const GLfloat * v);
typedef void (*PFNGLNORMAL3HNVPROC) (GLhalfNV  nx, GLhalfNV  ny, GLhalfNV  nz);
typedef void (*PFNGLNORMAL3HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLNORMAL3IPROC) (GLint  nx, GLint  ny, GLint  nz);
typedef void (*PFNGLNORMAL3IVPROC) (const GLint * v);
typedef void (*PFNGLNORMAL3SPROC) (GLshort  nx, GLshort  ny, GLshort  nz);
typedef void (*PFNGLNORMAL3SVPROC) (const GLshort * v);
typedef void (*PFNGLNORMALFORMATNVPROC) (GLenum  type, GLsizei  stride);
typedef void (*PFNGLNORMALP3UIPROC) (GLenum  type, GLuint  coords);
typedef void (*PFNGLNORMALP3UIVPROC) (GLenum  type, const GLuint * coords);
typedef void (*PFNGLNORMALPOINTERPROC) (GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLNORMALPOINTEREXTPROC) (GLenum  type, GLsizei  stride, GLsizei  count, const void * pointer);
typedef void (*PFNGLNORMALPOINTERVINTELPROC) (GLenum  type, const void ** pointer);
typedef void (*PFNGLOBJECTLABELPROC) (GLenum  identifier, GLuint  name, GLsizei  length, const GLchar * label);
typedef void (*PFNGLOBJECTLABELKHRPROC) (GLenum  identifier, GLuint  name, GLsizei  length, const GLchar * label);
typedef void (*PFNGLOBJECTPTRLABELPROC) (const void * ptr, GLsizei  length, const GLchar * label);
typedef void (*PFNGLOBJECTPTRLABELKHRPROC) (const void * ptr, GLsizei  length, const GLchar * label);
typedef GLenum (*PFNGLOBJECTPURGEABLEAPPLEPROC) (GLenum  objectType, GLuint  name, GLenum  option);
typedef GLenum (*PFNGLOBJECTUNPURGEABLEAPPLEPROC) (GLenum  objectType, GLuint  name, GLenum  option);
typedef void (*PFNGLORTHOPROC) (GLdouble  left, GLdouble  right, GLdouble  bottom, GLdouble  top, GLdouble  zNear, GLdouble  zFar);
typedef void (*PFNGLPASSTHROUGHPROC) (GLfloat  token);
typedef void (*PFNGLPATCHPARAMETERFVPROC) (GLenum  pname, const GLfloat * values);
typedef void (*PFNGLPATCHPARAMETERIPROC) (GLenum  pname, GLint  value);
typedef void (*PFNGLPATHCOLORGENNVPROC) (GLenum  color, GLenum  genMode, GLenum  colorFormat, const GLfloat * coeffs);
typedef void (*PFNGLPATHCOMMANDSNVPROC) (GLuint  path, GLsizei  numCommands, const GLubyte * commands, GLsizei  numCoords, GLenum  coordType, const void * coords);
typedef void (*PFNGLPATHCOORDSNVPROC) (GLuint  path, GLsizei  numCoords, GLenum  coordType, const void * coords);
typedef void (*PFNGLPATHCOVERDEPTHFUNCNVPROC) (GLenum  func);
typedef void (*PFNGLPATHDASHARRAYNVPROC) (GLuint  path, GLsizei  dashCount, const GLfloat * dashArray);
typedef void (*PFNGLPATHFOGGENNVPROC) (GLenum  genMode);
typedef GLenum (*PFNGLPATHGLYPHINDEXARRAYNVPROC) (GLuint  firstPathName, GLenum  fontTarget, const void * fontName, GLbitfield  fontStyle, GLuint  firstGlyphIndex, GLsizei  numGlyphs, GLuint  pathParameterTemplate, GLfloat  emScale);
typedef GLenum (*PFNGLPATHGLYPHINDEXRANGENVPROC) (GLenum  fontTarget, const void * fontName, GLbitfield  fontStyle, GLuint  pathParameterTemplate, GLfloat  emScale, GLuint  baseAndCount);
typedef void (*PFNGLPATHGLYPHRANGENVPROC) (GLuint  firstPathName, GLenum  fontTarget, const void * fontName, GLbitfield  fontStyle, GLuint  firstGlyph, GLsizei  numGlyphs, GLenum  handleMissingGlyphs, GLuint  pathParameterTemplate, GLfloat  emScale);
typedef void (*PFNGLPATHGLYPHSNVPROC) (GLuint  firstPathName, GLenum  fontTarget, const void * fontName, GLbitfield  fontStyle, GLsizei  numGlyphs, GLenum  type, const void * charcodes, GLenum  handleMissingGlyphs, GLuint  pathParameterTemplate, GLfloat  emScale);
typedef GLenum (*PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC) (GLuint  firstPathName, GLenum  fontTarget, GLsizeiptr  fontSize, const void * fontData, GLsizei  faceIndex, GLuint  firstGlyphIndex, GLsizei  numGlyphs, GLuint  pathParameterTemplate, GLfloat  emScale);
typedef void (*PFNGLPATHPARAMETERFNVPROC) (GLuint  path, GLenum  pname, GLfloat  value);
typedef void (*PFNGLPATHPARAMETERFVNVPROC) (GLuint  path, GLenum  pname, const GLfloat * value);
typedef void (*PFNGLPATHPARAMETERINVPROC) (GLuint  path, GLenum  pname, GLint  value);
typedef void (*PFNGLPATHPARAMETERIVNVPROC) (GLuint  path, GLenum  pname, const GLint * value);
typedef void (*PFNGLPATHSTENCILDEPTHOFFSETNVPROC) (GLfloat  factor, GLfloat  units);
typedef void (*PFNGLPATHSTENCILFUNCNVPROC) (GLenum  func, GLint  ref, GLuint  mask);
typedef void (*PFNGLPATHSTRINGNVPROC) (GLuint  path, GLenum  format, GLsizei  length, const void * pathString);
typedef void (*PFNGLPATHSUBCOMMANDSNVPROC) (GLuint  path, GLsizei  commandStart, GLsizei  commandsToDelete, GLsizei  numCommands, const GLubyte * commands, GLsizei  numCoords, GLenum  coordType, const void * coords);
typedef void (*PFNGLPATHSUBCOORDSNVPROC) (GLuint  path, GLsizei  coordStart, GLsizei  numCoords, GLenum  coordType, const void * coords);
typedef void (*PFNGLPATHTEXGENNVPROC) (GLenum  texCoordSet, GLenum  genMode, GLint  components, const GLfloat * coeffs);
typedef void (*PFNGLPAUSETRANSFORMFEEDBACKPROC) ();
typedef void (*PFNGLPAUSETRANSFORMFEEDBACKNVPROC) ();
typedef void (*PFNGLPIXELDATARANGENVPROC) (GLenum  target, GLsizei  length, const void * pointer);
typedef void (*PFNGLPIXELMAPFVPROC) (GLenum  map, GLsizei  mapsize, const GLfloat * values);
typedef void (*PFNGLPIXELMAPUIVPROC) (GLenum  map, GLsizei  mapsize, const GLuint * values);
typedef void (*PFNGLPIXELMAPUSVPROC) (GLenum  map, GLsizei  mapsize, const GLushort * values);
typedef void (*PFNGLPIXELSTOREFPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLPIXELSTOREIPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLPIXELTRANSFERFPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLPIXELTRANSFERIPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLPIXELTRANSFORMPARAMETERFEXTPROC) (GLenum  target, GLenum  pname, GLfloat  param);
typedef void (*PFNGLPIXELTRANSFORMPARAMETERFVEXTPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLPIXELTRANSFORMPARAMETERIEXTPROC) (GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLPIXELTRANSFORMPARAMETERIVEXTPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLPIXELZOOMPROC) (GLfloat  xfactor, GLfloat  yfactor);
typedef GLboolean (*PFNGLPOINTALONGPATHNVPROC) (GLuint  path, GLsizei  startSegment, GLsizei  numSegments, GLfloat  distance, GLfloat * x, GLfloat * y, GLfloat * tangentX, GLfloat * tangentY);
typedef void (*PFNGLPOINTPARAMETERFPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLPOINTPARAMETERFARBPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLPOINTPARAMETERFEXTPROC) (GLenum  pname, GLfloat  param);
typedef void (*PFNGLPOINTPARAMETERFVPROC) (GLenum  pname, const GLfloat * params);
typedef void (*PFNGLPOINTPARAMETERFVARBPROC) (GLenum  pname, const GLfloat * params);
typedef void (*PFNGLPOINTPARAMETERFVEXTPROC) (GLenum  pname, const GLfloat * params);
typedef void (*PFNGLPOINTPARAMETERIPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLPOINTPARAMETERINVPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLPOINTPARAMETERIVPROC) (GLenum  pname, const GLint * params);
typedef void (*PFNGLPOINTPARAMETERIVNVPROC) (GLenum  pname, const GLint * params);
typedef void (*PFNGLPOINTSIZEPROC) (GLfloat  size);
typedef void (*PFNGLPOLYGONMODEPROC) (GLenum  face, GLenum  mode);
typedef void (*PFNGLPOLYGONOFFSETPROC) (GLfloat  factor, GLfloat  units);
typedef void (*PFNGLPOLYGONOFFSETCLAMPPROC) (GLfloat  factor, GLfloat  units, GLfloat  clamp);
typedef void (*PFNGLPOLYGONOFFSETCLAMPEXTPROC) (GLfloat  factor, GLfloat  units, GLfloat  clamp);
typedef void (*PFNGLPOLYGONOFFSETEXTPROC) (GLfloat  factor, GLfloat  bias);
typedef void (*PFNGLPOLYGONSTIPPLEPROC) (const GLubyte * mask);
typedef void (*PFNGLPOPATTRIBPROC) ();
typedef void (*PFNGLPOPCLIENTATTRIBPROC) ();
typedef void (*PFNGLPOPDEBUGGROUPPROC) ();
typedef void (*PFNGLPOPDEBUGGROUPKHRPROC) ();
typedef void (*PFNGLPOPGROUPMARKEREXTPROC) ();
typedef void (*PFNGLPOPMATRIXPROC) ();
typedef void (*PFNGLPOPNAMEPROC) ();
typedef void (*PFNGLPRESENTFRAMEDUALFILLNVPROC) (GLuint  video_slot, GLuint64EXT  minPresentTime, GLuint  beginPresentTimeId, GLuint  presentDurationId, GLenum  type, GLenum  target0, GLuint  fill0, GLenum  target1, GLuint  fill1, GLenum  target2, GLuint  fill2, GLenum  target3, GLuint  fill3);
typedef void (*PFNGLPRESENTFRAMEKEYEDNVPROC) (GLuint  video_slot, GLuint64EXT  minPresentTime, GLuint  beginPresentTimeId, GLuint  presentDurationId, GLenum  type, GLenum  target0, GLuint  fill0, GLuint  key0, GLenum  target1, GLuint  fill1, GLuint  key1);
typedef void (*PFNGLPRIMITIVEBOUNDINGBOXARBPROC) (GLfloat  minX, GLfloat  minY, GLfloat  minZ, GLfloat  minW, GLfloat  maxX, GLfloat  maxY, GLfloat  maxZ, GLfloat  maxW);
typedef void (*PFNGLPRIMITIVERESTARTINDEXPROC) (GLuint  index);
typedef void (*PFNGLPRIMITIVERESTARTINDEXNVPROC) (GLuint  index);
typedef void (*PFNGLPRIMITIVERESTARTNVPROC) ();
typedef void (*PFNGLPRIORITIZETEXTURESPROC) (GLsizei  n, const GLuint * textures, const GLfloat * priorities);
typedef void (*PFNGLPRIORITIZETEXTURESEXTPROC) (GLsizei  n, const GLuint * textures, const GLclampf * priorities);
typedef void (*PFNGLPROGRAMBINARYPROC) (GLuint  program, GLenum  binaryFormat, const void * binary, GLsizei  length);
typedef void (*PFNGLPROGRAMBUFFERPARAMETERSIIVNVPROC) (GLenum  target, GLuint  bindingIndex, GLuint  wordIndex, GLsizei  count, const GLint * params);
typedef void (*PFNGLPROGRAMBUFFERPARAMETERSIUIVNVPROC) (GLenum  target, GLuint  bindingIndex, GLuint  wordIndex, GLsizei  count, const GLuint * params);
typedef void (*PFNGLPROGRAMBUFFERPARAMETERSFVNVPROC) (GLenum  target, GLuint  bindingIndex, GLuint  wordIndex, GLsizei  count, const GLfloat * params);
typedef void (*PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum  target, GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum  target, GLuint  index, const GLdouble * params);
typedef void (*PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum  target, GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum  target, GLuint  index, const GLfloat * params);
typedef void (*PFNGLPROGRAMENVPARAMETERI4INVPROC) (GLenum  target, GLuint  index, GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLPROGRAMENVPARAMETERI4IVNVPROC) (GLenum  target, GLuint  index, const GLint * params);
typedef void (*PFNGLPROGRAMENVPARAMETERI4UINVPROC) (GLenum  target, GLuint  index, GLuint  x, GLuint  y, GLuint  z, GLuint  w);
typedef void (*PFNGLPROGRAMENVPARAMETERI4UIVNVPROC) (GLenum  target, GLuint  index, const GLuint * params);
typedef void (*PFNGLPROGRAMENVPARAMETERS4FVEXTPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLfloat * params);
typedef void (*PFNGLPROGRAMENVPARAMETERSI4IVNVPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLint * params);
typedef void (*PFNGLPROGRAMENVPARAMETERSI4UIVNVPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLuint * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum  target, GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum  target, GLuint  index, const GLdouble * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum  target, GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum  target, GLuint  index, const GLfloat * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETERI4INVPROC) (GLenum  target, GLuint  index, GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLPROGRAMLOCALPARAMETERI4IVNVPROC) (GLenum  target, GLuint  index, const GLint * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETERI4UINVPROC) (GLenum  target, GLuint  index, GLuint  x, GLuint  y, GLuint  z, GLuint  w);
typedef void (*PFNGLPROGRAMLOCALPARAMETERI4UIVNVPROC) (GLenum  target, GLuint  index, const GLuint * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLfloat * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETERSI4IVNVPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLint * params);
typedef void (*PFNGLPROGRAMLOCALPARAMETERSI4UIVNVPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLuint * params);
typedef void (*PFNGLPROGRAMNAMEDPARAMETER4DNVPROC) (GLuint  id, GLsizei  len, const GLubyte * name, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC) (GLuint  id, GLsizei  len, const GLubyte * name, const GLdouble * v);
typedef void (*PFNGLPROGRAMNAMEDPARAMETER4FNVPROC) (GLuint  id, GLsizei  len, const GLubyte * name, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC) (GLuint  id, GLsizei  len, const GLubyte * name, const GLfloat * v);
typedef void (*PFNGLPROGRAMPARAMETER4DNVPROC) (GLenum  target, GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLPROGRAMPARAMETER4DVNVPROC) (GLenum  target, GLuint  index, const GLdouble * v);
typedef void (*PFNGLPROGRAMPARAMETER4FNVPROC) (GLenum  target, GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLPROGRAMPARAMETER4FVNVPROC) (GLenum  target, GLuint  index, const GLfloat * v);
typedef void (*PFNGLPROGRAMPARAMETERIPROC) (GLuint  program, GLenum  pname, GLint  value);
typedef void (*PFNGLPROGRAMPARAMETERIARBPROC) (GLuint  program, GLenum  pname, GLint  value);
typedef void (*PFNGLPROGRAMPARAMETERIEXTPROC) (GLuint  program, GLenum  pname, GLint  value);
typedef void (*PFNGLPROGRAMPARAMETERS4DVNVPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLPROGRAMPARAMETERS4FVNVPROC) (GLenum  target, GLuint  index, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC) (GLuint  program, GLint  location, GLenum  genMode, GLint  components, const GLfloat * coeffs);
typedef void (*PFNGLPROGRAMSTRINGARBPROC) (GLenum  target, GLenum  format, GLsizei  len, const void * string);
typedef void (*PFNGLPROGRAMSUBROUTINEPARAMETERSUIVNVPROC) (GLenum  target, GLsizei  count, const GLuint * params);
typedef void (*PFNGLPROGRAMUNIFORM1DPROC) (GLuint  program, GLint  location, GLdouble  v0);
typedef void (*PFNGLPROGRAMUNIFORM1DEXTPROC) (GLuint  program, GLint  location, GLdouble  x);
typedef void (*PFNGLPROGRAMUNIFORM1DVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM1DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM1FPROC) (GLuint  program, GLint  location, GLfloat  v0);
typedef void (*PFNGLPROGRAMUNIFORM1FEXTPROC) (GLuint  program, GLint  location, GLfloat  v0);
typedef void (*PFNGLPROGRAMUNIFORM1FVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM1FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM1IPROC) (GLuint  program, GLint  location, GLint  v0);
typedef void (*PFNGLPROGRAMUNIFORM1I64ARBPROC) (GLuint  program, GLint  location, GLint64  x);
typedef void (*PFNGLPROGRAMUNIFORM1I64NVPROC) (GLuint  program, GLint  location, GLint64EXT  x);
typedef void (*PFNGLPROGRAMUNIFORM1I64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM1I64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM1IEXTPROC) (GLuint  program, GLint  location, GLint  v0);
typedef void (*PFNGLPROGRAMUNIFORM1IVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM1IVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM1UIPROC) (GLuint  program, GLint  location, GLuint  v0);
typedef void (*PFNGLPROGRAMUNIFORM1UI64ARBPROC) (GLuint  program, GLint  location, GLuint64  x);
typedef void (*PFNGLPROGRAMUNIFORM1UI64NVPROC) (GLuint  program, GLint  location, GLuint64EXT  x);
typedef void (*PFNGLPROGRAMUNIFORM1UI64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM1UI64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM1UIEXTPROC) (GLuint  program, GLint  location, GLuint  v0);
typedef void (*PFNGLPROGRAMUNIFORM1UIVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM1UIVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM2DPROC) (GLuint  program, GLint  location, GLdouble  v0, GLdouble  v1);
typedef void (*PFNGLPROGRAMUNIFORM2DEXTPROC) (GLuint  program, GLint  location, GLdouble  x, GLdouble  y);
typedef void (*PFNGLPROGRAMUNIFORM2DVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM2DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM2FPROC) (GLuint  program, GLint  location, GLfloat  v0, GLfloat  v1);
typedef void (*PFNGLPROGRAMUNIFORM2FEXTPROC) (GLuint  program, GLint  location, GLfloat  v0, GLfloat  v1);
typedef void (*PFNGLPROGRAMUNIFORM2FVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM2FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM2IPROC) (GLuint  program, GLint  location, GLint  v0, GLint  v1);
typedef void (*PFNGLPROGRAMUNIFORM2I64ARBPROC) (GLuint  program, GLint  location, GLint64  x, GLint64  y);
typedef void (*PFNGLPROGRAMUNIFORM2I64NVPROC) (GLuint  program, GLint  location, GLint64EXT  x, GLint64EXT  y);
typedef void (*PFNGLPROGRAMUNIFORM2I64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM2I64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM2IEXTPROC) (GLuint  program, GLint  location, GLint  v0, GLint  v1);
typedef void (*PFNGLPROGRAMUNIFORM2IVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM2IVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM2UIPROC) (GLuint  program, GLint  location, GLuint  v0, GLuint  v1);
typedef void (*PFNGLPROGRAMUNIFORM2UI64ARBPROC) (GLuint  program, GLint  location, GLuint64  x, GLuint64  y);
typedef void (*PFNGLPROGRAMUNIFORM2UI64NVPROC) (GLuint  program, GLint  location, GLuint64EXT  x, GLuint64EXT  y);
typedef void (*PFNGLPROGRAMUNIFORM2UI64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM2UI64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM2UIEXTPROC) (GLuint  program, GLint  location, GLuint  v0, GLuint  v1);
typedef void (*PFNGLPROGRAMUNIFORM2UIVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM2UIVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM3DPROC) (GLuint  program, GLint  location, GLdouble  v0, GLdouble  v1, GLdouble  v2);
typedef void (*PFNGLPROGRAMUNIFORM3DEXTPROC) (GLuint  program, GLint  location, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLPROGRAMUNIFORM3DVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM3DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM3FPROC) (GLuint  program, GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2);
typedef void (*PFNGLPROGRAMUNIFORM3FEXTPROC) (GLuint  program, GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2);
typedef void (*PFNGLPROGRAMUNIFORM3FVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM3FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM3IPROC) (GLuint  program, GLint  location, GLint  v0, GLint  v1, GLint  v2);
typedef void (*PFNGLPROGRAMUNIFORM3I64ARBPROC) (GLuint  program, GLint  location, GLint64  x, GLint64  y, GLint64  z);
typedef void (*PFNGLPROGRAMUNIFORM3I64NVPROC) (GLuint  program, GLint  location, GLint64EXT  x, GLint64EXT  y, GLint64EXT  z);
typedef void (*PFNGLPROGRAMUNIFORM3I64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM3I64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM3IEXTPROC) (GLuint  program, GLint  location, GLint  v0, GLint  v1, GLint  v2);
typedef void (*PFNGLPROGRAMUNIFORM3IVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM3IVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM3UIPROC) (GLuint  program, GLint  location, GLuint  v0, GLuint  v1, GLuint  v2);
typedef void (*PFNGLPROGRAMUNIFORM3UI64ARBPROC) (GLuint  program, GLint  location, GLuint64  x, GLuint64  y, GLuint64  z);
typedef void (*PFNGLPROGRAMUNIFORM3UI64NVPROC) (GLuint  program, GLint  location, GLuint64EXT  x, GLuint64EXT  y, GLuint64EXT  z);
typedef void (*PFNGLPROGRAMUNIFORM3UI64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM3UI64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM3UIEXTPROC) (GLuint  program, GLint  location, GLuint  v0, GLuint  v1, GLuint  v2);
typedef void (*PFNGLPROGRAMUNIFORM3UIVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM3UIVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM4DPROC) (GLuint  program, GLint  location, GLdouble  v0, GLdouble  v1, GLdouble  v2, GLdouble  v3);
typedef void (*PFNGLPROGRAMUNIFORM4DEXTPROC) (GLuint  program, GLint  location, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLPROGRAMUNIFORM4DVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM4DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORM4FPROC) (GLuint  program, GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3);
typedef void (*PFNGLPROGRAMUNIFORM4FEXTPROC) (GLuint  program, GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3);
typedef void (*PFNGLPROGRAMUNIFORM4FVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM4FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORM4IPROC) (GLuint  program, GLint  location, GLint  v0, GLint  v1, GLint  v2, GLint  v3);
typedef void (*PFNGLPROGRAMUNIFORM4I64ARBPROC) (GLuint  program, GLint  location, GLint64  x, GLint64  y, GLint64  z, GLint64  w);
typedef void (*PFNGLPROGRAMUNIFORM4I64NVPROC) (GLuint  program, GLint  location, GLint64EXT  x, GLint64EXT  y, GLint64EXT  z, GLint64EXT  w);
typedef void (*PFNGLPROGRAMUNIFORM4I64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM4I64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM4IEXTPROC) (GLuint  program, GLint  location, GLint  v0, GLint  v1, GLint  v2, GLint  v3);
typedef void (*PFNGLPROGRAMUNIFORM4IVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM4IVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLPROGRAMUNIFORM4UIPROC) (GLuint  program, GLint  location, GLuint  v0, GLuint  v1, GLuint  v2, GLuint  v3);
typedef void (*PFNGLPROGRAMUNIFORM4UI64ARBPROC) (GLuint  program, GLint  location, GLuint64  x, GLuint64  y, GLuint64  z, GLuint64  w);
typedef void (*PFNGLPROGRAMUNIFORM4UI64NVPROC) (GLuint  program, GLint  location, GLuint64EXT  x, GLuint64EXT  y, GLuint64EXT  z, GLuint64EXT  w);
typedef void (*PFNGLPROGRAMUNIFORM4UI64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLPROGRAMUNIFORM4UI64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLPROGRAMUNIFORM4UIEXTPROC) (GLuint  program, GLint  location, GLuint  v0, GLuint  v1, GLuint  v2, GLuint  v3);
typedef void (*PFNGLPROGRAMUNIFORM4UIVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORM4UIVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC) (GLuint  program, GLint  location, GLuint64  value);
typedef void (*PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC) (GLuint  program, GLint  location, GLuint64  value);
typedef void (*PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64 * values);
typedef void (*PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64 * values);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC) (GLuint  program, GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLPROGRAMUNIFORMUI64NVPROC) (GLuint  program, GLint  location, GLuint64EXT  value);
typedef void (*PFNGLPROGRAMUNIFORMUI64VNVPROC) (GLuint  program, GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLPROGRAMVERTEXLIMITNVPROC) (GLenum  target, GLint  limit);
typedef void (*PFNGLPROVOKINGVERTEXPROC) (GLenum  mode);
typedef void (*PFNGLPROVOKINGVERTEXEXTPROC) (GLenum  mode);
typedef void (*PFNGLPUSHATTRIBPROC) (GLbitfield  mask);
typedef void (*PFNGLPUSHCLIENTATTRIBPROC) (GLbitfield  mask);
typedef void (*PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC) (GLbitfield  mask);
typedef void (*PFNGLPUSHDEBUGGROUPPROC) (GLenum  source, GLuint  id, GLsizei  length, const GLchar * message);
typedef void (*PFNGLPUSHDEBUGGROUPKHRPROC) (GLenum  source, GLuint  id, GLsizei  length, const GLchar * message);
typedef void (*PFNGLPUSHGROUPMARKEREXTPROC) (GLsizei  length, const GLchar * marker);
typedef void (*PFNGLPUSHMATRIXPROC) ();
typedef void (*PFNGLPUSHNAMEPROC) (GLuint  name);
typedef void (*PFNGLQUERYCOUNTERPROC) (GLuint  id, GLenum  target);
typedef void (*PFNGLQUERYOBJECTPARAMETERUIAMDPROC) (GLenum  target, GLuint  id, GLenum  pname, GLuint  param);
typedef GLint (*PFNGLQUERYRESOURCENVPROC) (GLenum  queryType, GLint  tagId, GLuint  count, GLint * buffer);
typedef void (*PFNGLQUERYRESOURCETAGNVPROC) (GLint  tagId, const GLchar * tagString);
typedef void (*PFNGLRASTERPOS2DPROC) (GLdouble  x, GLdouble  y);
typedef void (*PFNGLRASTERPOS2DVPROC) (const GLdouble * v);
typedef void (*PFNGLRASTERPOS2FPROC) (GLfloat  x, GLfloat  y);
typedef void (*PFNGLRASTERPOS2FVPROC) (const GLfloat * v);
typedef void (*PFNGLRASTERPOS2IPROC) (GLint  x, GLint  y);
typedef void (*PFNGLRASTERPOS2IVPROC) (const GLint * v);
typedef void (*PFNGLRASTERPOS2SPROC) (GLshort  x, GLshort  y);
typedef void (*PFNGLRASTERPOS2SVPROC) (const GLshort * v);
typedef void (*PFNGLRASTERPOS3DPROC) (GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLRASTERPOS3DVPROC) (const GLdouble * v);
typedef void (*PFNGLRASTERPOS3FPROC) (GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLRASTERPOS3FVPROC) (const GLfloat * v);
typedef void (*PFNGLRASTERPOS3IPROC) (GLint  x, GLint  y, GLint  z);
typedef void (*PFNGLRASTERPOS3IVPROC) (const GLint * v);
typedef void (*PFNGLRASTERPOS3SPROC) (GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLRASTERPOS3SVPROC) (const GLshort * v);
typedef void (*PFNGLRASTERPOS4DPROC) (GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLRASTERPOS4DVPROC) (const GLdouble * v);
typedef void (*PFNGLRASTERPOS4FPROC) (GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLRASTERPOS4FVPROC) (const GLfloat * v);
typedef void (*PFNGLRASTERPOS4IPROC) (GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLRASTERPOS4IVPROC) (const GLint * v);
typedef void (*PFNGLRASTERPOS4SPROC) (GLshort  x, GLshort  y, GLshort  z, GLshort  w);
typedef void (*PFNGLRASTERPOS4SVPROC) (const GLshort * v);
typedef void (*PFNGLRASTERSAMPLESEXTPROC) (GLuint  samples, GLboolean  fixedsamplelocations);
typedef void (*PFNGLREADBUFFERPROC) (GLenum  src);
typedef void (*PFNGLREADPIXELSPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, void * pixels);
typedef void (*PFNGLREADNPIXELSPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, GLsizei  bufSize, void * data);
typedef void (*PFNGLREADNPIXELSARBPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, GLsizei  bufSize, void * data);
typedef void (*PFNGLREADNPIXELSKHRPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, GLsizei  bufSize, void * data);
typedef GLboolean (*PFNGLRELEASEKEYEDMUTEXWIN32EXTPROC) (GLuint  memory, GLuint64  key);
typedef void (*PFNGLRECTDPROC) (GLdouble  x1, GLdouble  y1, GLdouble  x2, GLdouble  y2);
typedef void (*PFNGLRECTDVPROC) (const GLdouble * v1, const GLdouble * v2);
typedef void (*PFNGLRECTFPROC) (GLfloat  x1, GLfloat  y1, GLfloat  x2, GLfloat  y2);
typedef void (*PFNGLRECTFVPROC) (const GLfloat * v1, const GLfloat * v2);
typedef void (*PFNGLRECTIPROC) (GLint  x1, GLint  y1, GLint  x2, GLint  y2);
typedef void (*PFNGLRECTIVPROC) (const GLint * v1, const GLint * v2);
typedef void (*PFNGLRECTSPROC) (GLshort  x1, GLshort  y1, GLshort  x2, GLshort  y2);
typedef void (*PFNGLRECTSVPROC) (const GLshort * v1, const GLshort * v2);
typedef void (*PFNGLRELEASESHADERCOMPILERPROC) ();
typedef void (*PFNGLRENDERGPUMASKNVPROC) (GLbitfield  mask);
typedef GLint (*PFNGLRENDERMODEPROC) (GLenum  mode);
typedef void (*PFNGLRENDERBUFFERSTORAGEPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLRENDERBUFFERSTORAGEEXTPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) (GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC) (GLenum  target, GLsizei  samples, GLsizei  storageSamples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC) (GLenum  target, GLsizei  coverageSamples, GLsizei  colorSamples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLREQUESTRESIDENTPROGRAMSNVPROC) (GLsizei  n, const GLuint * programs);
typedef void (*PFNGLRESETHISTOGRAMPROC) (GLenum  target);
typedef void (*PFNGLRESETHISTOGRAMEXTPROC) (GLenum  target);
typedef void (*PFNGLRESETMEMORYOBJECTPARAMETERNVPROC) (GLuint  memory, GLenum  pname);
typedef void (*PFNGLRESETMINMAXPROC) (GLenum  target);
typedef void (*PFNGLRESETMINMAXEXTPROC) (GLenum  target);
typedef void (*PFNGLRESOLVEDEPTHVALUESNVPROC) ();
typedef void (*PFNGLRESUMETRANSFORMFEEDBACKPROC) ();
typedef void (*PFNGLRESUMETRANSFORMFEEDBACKNVPROC) ();
typedef void (*PFNGLROTATEDPROC) (GLdouble  angle, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLROTATEFPROC) (GLfloat  angle, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLSAMPLECOVERAGEPROC) (GLfloat  value, GLboolean  invert);
typedef void (*PFNGLSAMPLECOVERAGEARBPROC) (GLfloat  value, GLboolean  invert);
typedef void (*PFNGLSAMPLEMASKEXTPROC) (GLclampf  value, GLboolean  invert);
typedef void (*PFNGLSAMPLEMASKINDEXEDNVPROC) (GLuint  index, GLbitfield  mask);
typedef void (*PFNGLSAMPLEMASKIPROC) (GLuint  maskNumber, GLbitfield  mask);
typedef void (*PFNGLSAMPLEPATTERNEXTPROC) (GLenum  pattern);
typedef void (*PFNGLSAMPLERPARAMETERIIVPROC) (GLuint  sampler, GLenum  pname, const GLint * param);
typedef void (*PFNGLSAMPLERPARAMETERIUIVPROC) (GLuint  sampler, GLenum  pname, const GLuint * param);
typedef void (*PFNGLSAMPLERPARAMETERFPROC) (GLuint  sampler, GLenum  pname, GLfloat  param);
typedef void (*PFNGLSAMPLERPARAMETERFVPROC) (GLuint  sampler, GLenum  pname, const GLfloat * param);
typedef void (*PFNGLSAMPLERPARAMETERIPROC) (GLuint  sampler, GLenum  pname, GLint  param);
typedef void (*PFNGLSAMPLERPARAMETERIVPROC) (GLuint  sampler, GLenum  pname, const GLint * param);
typedef void (*PFNGLSCALEDPROC) (GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLSCALEFPROC) (GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLSCISSORPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLSCISSORARRAYVPROC) (GLuint  first, GLsizei  count, const GLint * v);
typedef void (*PFNGLSCISSOREXCLUSIVEARRAYVNVPROC) (GLuint  first, GLsizei  count, const GLint * v);
typedef void (*PFNGLSCISSOREXCLUSIVENVPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLSCISSORINDEXEDPROC) (GLuint  index, GLint  left, GLint  bottom, GLsizei  width, GLsizei  height);
typedef void (*PFNGLSCISSORINDEXEDVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLSECONDARYCOLOR3BPROC) (GLbyte  red, GLbyte  green, GLbyte  blue);
typedef void (*PFNGLSECONDARYCOLOR3BEXTPROC) (GLbyte  red, GLbyte  green, GLbyte  blue);
typedef void (*PFNGLSECONDARYCOLOR3BVPROC) (const GLbyte * v);
typedef void (*PFNGLSECONDARYCOLOR3BVEXTPROC) (const GLbyte * v);
typedef void (*PFNGLSECONDARYCOLOR3DPROC) (GLdouble  red, GLdouble  green, GLdouble  blue);
typedef void (*PFNGLSECONDARYCOLOR3DEXTPROC) (GLdouble  red, GLdouble  green, GLdouble  blue);
typedef void (*PFNGLSECONDARYCOLOR3DVPROC) (const GLdouble * v);
typedef void (*PFNGLSECONDARYCOLOR3DVEXTPROC) (const GLdouble * v);
typedef void (*PFNGLSECONDARYCOLOR3FPROC) (GLfloat  red, GLfloat  green, GLfloat  blue);
typedef void (*PFNGLSECONDARYCOLOR3FEXTPROC) (GLfloat  red, GLfloat  green, GLfloat  blue);
typedef void (*PFNGLSECONDARYCOLOR3FVPROC) (const GLfloat * v);
typedef void (*PFNGLSECONDARYCOLOR3FVEXTPROC) (const GLfloat * v);
typedef void (*PFNGLSECONDARYCOLOR3HNVPROC) (GLhalfNV  red, GLhalfNV  green, GLhalfNV  blue);
typedef void (*PFNGLSECONDARYCOLOR3HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLSECONDARYCOLOR3IPROC) (GLint  red, GLint  green, GLint  blue);
typedef void (*PFNGLSECONDARYCOLOR3IEXTPROC) (GLint  red, GLint  green, GLint  blue);
typedef void (*PFNGLSECONDARYCOLOR3IVPROC) (const GLint * v);
typedef void (*PFNGLSECONDARYCOLOR3IVEXTPROC) (const GLint * v);
typedef void (*PFNGLSECONDARYCOLOR3SPROC) (GLshort  red, GLshort  green, GLshort  blue);
typedef void (*PFNGLSECONDARYCOLOR3SEXTPROC) (GLshort  red, GLshort  green, GLshort  blue);
typedef void (*PFNGLSECONDARYCOLOR3SVPROC) (const GLshort * v);
typedef void (*PFNGLSECONDARYCOLOR3SVEXTPROC) (const GLshort * v);
typedef void (*PFNGLSECONDARYCOLOR3UBPROC) (GLubyte  red, GLubyte  green, GLubyte  blue);
typedef void (*PFNGLSECONDARYCOLOR3UBEXTPROC) (GLubyte  red, GLubyte  green, GLubyte  blue);
typedef void (*PFNGLSECONDARYCOLOR3UBVPROC) (const GLubyte * v);
typedef void (*PFNGLSECONDARYCOLOR3UBVEXTPROC) (const GLubyte * v);
typedef void (*PFNGLSECONDARYCOLOR3UIPROC) (GLuint  red, GLuint  green, GLuint  blue);
typedef void (*PFNGLSECONDARYCOLOR3UIEXTPROC) (GLuint  red, GLuint  green, GLuint  blue);
typedef void (*PFNGLSECONDARYCOLOR3UIVPROC) (const GLuint * v);
typedef void (*PFNGLSECONDARYCOLOR3UIVEXTPROC) (const GLuint * v);
typedef void (*PFNGLSECONDARYCOLOR3USPROC) (GLushort  red, GLushort  green, GLushort  blue);
typedef void (*PFNGLSECONDARYCOLOR3USEXTPROC) (GLushort  red, GLushort  green, GLushort  blue);
typedef void (*PFNGLSECONDARYCOLOR3USVPROC) (const GLushort * v);
typedef void (*PFNGLSECONDARYCOLOR3USVEXTPROC) (const GLushort * v);
typedef void (*PFNGLSECONDARYCOLORFORMATNVPROC) (GLint  size, GLenum  type, GLsizei  stride);
typedef void (*PFNGLSECONDARYCOLORP3UIPROC) (GLenum  type, GLuint  color);
typedef void (*PFNGLSECONDARYCOLORP3UIVPROC) (GLenum  type, const GLuint * color);
typedef void (*PFNGLSECONDARYCOLORPOINTERPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLSECONDARYCOLORPOINTEREXTPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLSELECTBUFFERPROC) (GLsizei  size, GLuint * buffer);
typedef void (*PFNGLSELECTPERFMONITORCOUNTERSAMDPROC) (GLuint  monitor, GLboolean  enable, GLuint  group, GLint  numCounters, GLuint * counterList);
typedef void (*PFNGLSEMAPHOREPARAMETERIVNVPROC) (GLuint  semaphore, GLenum  pname, const GLint * params);
typedef void (*PFNGLSEMAPHOREPARAMETERUI64VEXTPROC) (GLuint  semaphore, GLenum  pname, const GLuint64 * params);
typedef void (*PFNGLSEPARABLEFILTER2DPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * row, const void * column);
typedef void (*PFNGLSEPARABLEFILTER2DEXTPROC) (GLenum  target, GLenum  internalformat, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * row, const void * column);
typedef void (*PFNGLSETFENCEAPPLEPROC) (GLuint  fence);
typedef void (*PFNGLSETFENCENVPROC) (GLuint  fence, GLenum  condition);
typedef void (*PFNGLSETINVARIANTEXTPROC) (GLuint  id, GLenum  type, const void * addr);
typedef void (*PFNGLSETLOCALCONSTANTEXTPROC) (GLuint  id, GLenum  type, const void * addr);
typedef void (*PFNGLSETMULTISAMPLEFVAMDPROC) (GLenum  pname, GLuint  index, const GLfloat * val);
typedef void (*PFNGLSHADEMODELPROC) (GLenum  mode);
typedef void (*PFNGLSHADERBINARYPROC) (GLsizei  count, const GLuint * shaders, GLenum  binaryFormat, const void * binary, GLsizei  length);
typedef void (*PFNGLSHADEROP1EXTPROC) (GLenum  op, GLuint  res, GLuint  arg1);
typedef void (*PFNGLSHADEROP2EXTPROC) (GLenum  op, GLuint  res, GLuint  arg1, GLuint  arg2);
typedef void (*PFNGLSHADEROP3EXTPROC) (GLenum  op, GLuint  res, GLuint  arg1, GLuint  arg2, GLuint  arg3);
typedef void (*PFNGLSHADERSOURCEPROC) (GLuint  shader, GLsizei  count, const GLchar *const* string, const GLint * length);
typedef void (*PFNGLSHADERSOURCEARBPROC) (GLhandleARB  shaderObj, GLsizei  count, const GLcharARB ** string, const GLint * length);
typedef void (*PFNGLSHADERSTORAGEBLOCKBINDINGPROC) (GLuint  program, GLuint  storageBlockIndex, GLuint  storageBlockBinding);
typedef void (*PFNGLSHADINGRATEIMAGEBARRIERNVPROC) (GLboolean  synchronize);
typedef void (*PFNGLSHADINGRATEIMAGEPALETTENVPROC) (GLuint  viewport, GLuint  first, GLsizei  count, const GLenum * rates);
typedef void (*PFNGLSHADINGRATESAMPLEORDERNVPROC) (GLenum  order);
typedef void (*PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC) (GLenum  rate, GLuint  samples, const GLint * locations);
typedef void (*PFNGLSIGNALSEMAPHOREEXTPROC) (GLuint  semaphore, GLuint  numBufferBarriers, const GLuint * buffers, GLuint  numTextureBarriers, const GLuint * textures, const GLenum * dstLayouts);
typedef void (*PFNGLSIGNALSEMAPHOREUI64NVXPROC) (GLuint  signalGpu, GLsizei  fenceObjectCount, const GLuint * semaphoreArray, const GLuint64 * fenceValueArray);
typedef void (*PFNGLSPECIALIZESHADERPROC) (GLuint  shader, const GLchar * pEntryPoint, GLuint  numSpecializationConstants, const GLuint * pConstantIndex, const GLuint * pConstantValue);
typedef void (*PFNGLSPECIALIZESHADERARBPROC) (GLuint  shader, const GLchar * pEntryPoint, GLuint  numSpecializationConstants, const GLuint * pConstantIndex, const GLuint * pConstantValue);
typedef void (*PFNGLSTATECAPTURENVPROC) (GLuint  state, GLenum  mode);
typedef void (*PFNGLSTENCILCLEARTAGEXTPROC) (GLsizei  stencilTagBits, GLuint  stencilClearTag);
typedef void (*PFNGLSTENCILFILLPATHINSTANCEDNVPROC) (GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLenum  fillMode, GLuint  mask, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLSTENCILFILLPATHNVPROC) (GLuint  path, GLenum  fillMode, GLuint  mask);
typedef void (*PFNGLSTENCILFUNCPROC) (GLenum  func, GLint  ref, GLuint  mask);
typedef void (*PFNGLSTENCILFUNCSEPARATEPROC) (GLenum  face, GLenum  func, GLint  ref, GLuint  mask);
typedef void (*PFNGLSTENCILMASKPROC) (GLuint  mask);
typedef void (*PFNGLSTENCILMASKSEPARATEPROC) (GLenum  face, GLuint  mask);
typedef void (*PFNGLSTENCILOPPROC) (GLenum  fail, GLenum  zfail, GLenum  zpass);
typedef void (*PFNGLSTENCILOPSEPARATEPROC) (GLenum  face, GLenum  sfail, GLenum  dpfail, GLenum  dppass);
typedef void (*PFNGLSTENCILOPVALUEAMDPROC) (GLenum  face, GLuint  value);
typedef void (*PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC) (GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLint  reference, GLuint  mask, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLSTENCILSTROKEPATHNVPROC) (GLuint  path, GLint  reference, GLuint  mask);
typedef void (*PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC) (GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLenum  fillMode, GLuint  mask, GLenum  coverMode, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLSTENCILTHENCOVERFILLPATHNVPROC) (GLuint  path, GLenum  fillMode, GLuint  mask, GLenum  coverMode);
typedef void (*PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC) (GLsizei  numPaths, GLenum  pathNameType, const void * paths, GLuint  pathBase, GLint  reference, GLuint  mask, GLenum  coverMode, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC) (GLuint  path, GLint  reference, GLuint  mask, GLenum  coverMode);
typedef void (*PFNGLSUBPIXELPRECISIONBIASNVPROC) (GLuint  xbits, GLuint  ybits);
typedef void (*PFNGLSWIZZLEEXTPROC) (GLuint  res, GLuint  in, GLenum  outX, GLenum  outY, GLenum  outZ, GLenum  outW);
typedef void (*PFNGLSYNCTEXTUREINTELPROC) (GLuint  texture);
typedef void (*PFNGLTANGENT3BEXTPROC) (GLbyte  tx, GLbyte  ty, GLbyte  tz);
typedef void (*PFNGLTANGENT3BVEXTPROC) (const GLbyte * v);
typedef void (*PFNGLTANGENT3DEXTPROC) (GLdouble  tx, GLdouble  ty, GLdouble  tz);
typedef void (*PFNGLTANGENT3DVEXTPROC) (const GLdouble * v);
typedef void (*PFNGLTANGENT3FEXTPROC) (GLfloat  tx, GLfloat  ty, GLfloat  tz);
typedef void (*PFNGLTANGENT3FVEXTPROC) (const GLfloat * v);
typedef void (*PFNGLTANGENT3IEXTPROC) (GLint  tx, GLint  ty, GLint  tz);
typedef void (*PFNGLTANGENT3IVEXTPROC) (const GLint * v);
typedef void (*PFNGLTANGENT3SEXTPROC) (GLshort  tx, GLshort  ty, GLshort  tz);
typedef void (*PFNGLTANGENT3SVEXTPROC) (const GLshort * v);
typedef void (*PFNGLTANGENTPOINTEREXTPROC) (GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLTESSELLATIONFACTORAMDPROC) (GLfloat  factor);
typedef void (*PFNGLTESSELLATIONMODEAMDPROC) (GLenum  mode);
typedef GLboolean (*PFNGLTESTFENCEAPPLEPROC) (GLuint  fence);
typedef GLboolean (*PFNGLTESTFENCENVPROC) (GLuint  fence);
typedef GLboolean (*PFNGLTESTOBJECTAPPLEPROC) (GLenum  object, GLuint  name);
typedef void (*PFNGLTEXATTACHMEMORYNVPROC) (GLenum  target, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXBUFFERPROC) (GLenum  target, GLenum  internalformat, GLuint  buffer);
typedef void (*PFNGLTEXBUFFERARBPROC) (GLenum  target, GLenum  internalformat, GLuint  buffer);
typedef void (*PFNGLTEXBUFFEREXTPROC) (GLenum  target, GLenum  internalformat, GLuint  buffer);
typedef void (*PFNGLTEXBUFFERRANGEPROC) (GLenum  target, GLenum  internalformat, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLTEXCOORD1DPROC) (GLdouble  s);
typedef void (*PFNGLTEXCOORD1DVPROC) (const GLdouble * v);
typedef void (*PFNGLTEXCOORD1FPROC) (GLfloat  s);
typedef void (*PFNGLTEXCOORD1FVPROC) (const GLfloat * v);
typedef void (*PFNGLTEXCOORD1HNVPROC) (GLhalfNV  s);
typedef void (*PFNGLTEXCOORD1HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLTEXCOORD1IPROC) (GLint  s);
typedef void (*PFNGLTEXCOORD1IVPROC) (const GLint * v);
typedef void (*PFNGLTEXCOORD1SPROC) (GLshort  s);
typedef void (*PFNGLTEXCOORD1SVPROC) (const GLshort * v);
typedef void (*PFNGLTEXCOORD2DPROC) (GLdouble  s, GLdouble  t);
typedef void (*PFNGLTEXCOORD2DVPROC) (const GLdouble * v);
typedef void (*PFNGLTEXCOORD2FPROC) (GLfloat  s, GLfloat  t);
typedef void (*PFNGLTEXCOORD2FVPROC) (const GLfloat * v);
typedef void (*PFNGLTEXCOORD2HNVPROC) (GLhalfNV  s, GLhalfNV  t);
typedef void (*PFNGLTEXCOORD2HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLTEXCOORD2IPROC) (GLint  s, GLint  t);
typedef void (*PFNGLTEXCOORD2IVPROC) (const GLint * v);
typedef void (*PFNGLTEXCOORD2SPROC) (GLshort  s, GLshort  t);
typedef void (*PFNGLTEXCOORD2SVPROC) (const GLshort * v);
typedef void (*PFNGLTEXCOORD3DPROC) (GLdouble  s, GLdouble  t, GLdouble  r);
typedef void (*PFNGLTEXCOORD3DVPROC) (const GLdouble * v);
typedef void (*PFNGLTEXCOORD3FPROC) (GLfloat  s, GLfloat  t, GLfloat  r);
typedef void (*PFNGLTEXCOORD3FVPROC) (const GLfloat * v);
typedef void (*PFNGLTEXCOORD3HNVPROC) (GLhalfNV  s, GLhalfNV  t, GLhalfNV  r);
typedef void (*PFNGLTEXCOORD3HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLTEXCOORD3IPROC) (GLint  s, GLint  t, GLint  r);
typedef void (*PFNGLTEXCOORD3IVPROC) (const GLint * v);
typedef void (*PFNGLTEXCOORD3SPROC) (GLshort  s, GLshort  t, GLshort  r);
typedef void (*PFNGLTEXCOORD3SVPROC) (const GLshort * v);
typedef void (*PFNGLTEXCOORD4DPROC) (GLdouble  s, GLdouble  t, GLdouble  r, GLdouble  q);
typedef void (*PFNGLTEXCOORD4DVPROC) (const GLdouble * v);
typedef void (*PFNGLTEXCOORD4FPROC) (GLfloat  s, GLfloat  t, GLfloat  r, GLfloat  q);
typedef void (*PFNGLTEXCOORD4FVPROC) (const GLfloat * v);
typedef void (*PFNGLTEXCOORD4HNVPROC) (GLhalfNV  s, GLhalfNV  t, GLhalfNV  r, GLhalfNV  q);
typedef void (*PFNGLTEXCOORD4HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLTEXCOORD4IPROC) (GLint  s, GLint  t, GLint  r, GLint  q);
typedef void (*PFNGLTEXCOORD4IVPROC) (const GLint * v);
typedef void (*PFNGLTEXCOORD4SPROC) (GLshort  s, GLshort  t, GLshort  r, GLshort  q);
typedef void (*PFNGLTEXCOORD4SVPROC) (const GLshort * v);
typedef void (*PFNGLTEXCOORDFORMATNVPROC) (GLint  size, GLenum  type, GLsizei  stride);
typedef void (*PFNGLTEXCOORDP1UIPROC) (GLenum  type, GLuint  coords);
typedef void (*PFNGLTEXCOORDP1UIVPROC) (GLenum  type, const GLuint * coords);
typedef void (*PFNGLTEXCOORDP2UIPROC) (GLenum  type, GLuint  coords);
typedef void (*PFNGLTEXCOORDP2UIVPROC) (GLenum  type, const GLuint * coords);
typedef void (*PFNGLTEXCOORDP3UIPROC) (GLenum  type, GLuint  coords);
typedef void (*PFNGLTEXCOORDP3UIVPROC) (GLenum  type, const GLuint * coords);
typedef void (*PFNGLTEXCOORDP4UIPROC) (GLenum  type, GLuint  coords);
typedef void (*PFNGLTEXCOORDP4UIVPROC) (GLenum  type, const GLuint * coords);
typedef void (*PFNGLTEXCOORDPOINTERPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLTEXCOORDPOINTEREXTPROC) (GLint  size, GLenum  type, GLsizei  stride, GLsizei  count, const void * pointer);
typedef void (*PFNGLTEXCOORDPOINTERVINTELPROC) (GLint  size, GLenum  type, const void ** pointer);
typedef void (*PFNGLTEXENVFPROC) (GLenum  target, GLenum  pname, GLfloat  param);
typedef void (*PFNGLTEXENVFVPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLTEXENVIPROC) (GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLTEXENVIVPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXGENDPROC) (GLenum  coord, GLenum  pname, GLdouble  param);
typedef void (*PFNGLTEXGENDVPROC) (GLenum  coord, GLenum  pname, const GLdouble * params);
typedef void (*PFNGLTEXGENFPROC) (GLenum  coord, GLenum  pname, GLfloat  param);
typedef void (*PFNGLTEXGENFVPROC) (GLenum  coord, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLTEXGENIPROC) (GLenum  coord, GLenum  pname, GLint  param);
typedef void (*PFNGLTEXGENIVPROC) (GLenum  coord, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXIMAGE1DPROC) (GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXIMAGE2DPROC) (GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXIMAGE2DMULTISAMPLEPROC) (GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXIMAGE2DMULTISAMPLECOVERAGENVPROC) (GLenum  target, GLsizei  coverageSamples, GLsizei  colorSamples, GLint  internalFormat, GLsizei  width, GLsizei  height, GLboolean  fixedSampleLocations);
typedef void (*PFNGLTEXIMAGE3DPROC) (GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXIMAGE3DEXTPROC) (GLenum  target, GLint  level, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXIMAGE3DMULTISAMPLEPROC) (GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXIMAGE3DMULTISAMPLECOVERAGENVPROC) (GLenum  target, GLsizei  coverageSamples, GLsizei  colorSamples, GLint  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedSampleLocations);
typedef void (*PFNGLTEXPAGECOMMITMENTARBPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  commit);
typedef void (*PFNGLTEXPAGECOMMITMENTMEMNVPROC) (GLenum  target, GLint  layer, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLuint  memory, GLuint64  offset, GLboolean  commit);
typedef void (*PFNGLTEXPARAMETERIIVPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXPARAMETERIIVEXTPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXPARAMETERIUIVPROC) (GLenum  target, GLenum  pname, const GLuint * params);
typedef void (*PFNGLTEXPARAMETERIUIVEXTPROC) (GLenum  target, GLenum  pname, const GLuint * params);
typedef void (*PFNGLTEXPARAMETERFPROC) (GLenum  target, GLenum  pname, GLfloat  param);
typedef void (*PFNGLTEXPARAMETERFVPROC) (GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLTEXPARAMETERIPROC) (GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLTEXPARAMETERIVPROC) (GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXRENDERBUFFERNVPROC) (GLenum  target, GLuint  renderbuffer);
typedef void (*PFNGLTEXSTORAGE1DPROC) (GLenum  target, GLsizei  levels, GLenum  internalformat, GLsizei  width);
typedef void (*PFNGLTEXSTORAGE2DPROC) (GLenum  target, GLsizei  levels, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLTEXSTORAGE2DMULTISAMPLEPROC) (GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXSTORAGE3DPROC) (GLenum  target, GLsizei  levels, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth);
typedef void (*PFNGLTEXSTORAGE3DMULTISAMPLEPROC) (GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXSTORAGEMEM1DEXTPROC) (GLenum  target, GLsizei  levels, GLenum  internalFormat, GLsizei  width, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXSTORAGEMEM2DEXTPROC) (GLenum  target, GLsizei  levels, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC) (GLenum  target, GLsizei  samples, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLboolean  fixedSampleLocations, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXSTORAGEMEM3DEXTPROC) (GLenum  target, GLsizei  levels, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC) (GLenum  target, GLsizei  samples, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedSampleLocations, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXSTORAGESPARSEAMDPROC) (GLenum  target, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLsizei  layers, GLbitfield  flags);
typedef void (*PFNGLTEXSUBIMAGE1DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXSUBIMAGE1DEXTPROC) (GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXSUBIMAGE2DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXSUBIMAGE2DEXTPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXSUBIMAGE3DPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXSUBIMAGE3DEXTPROC) (GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTUREATTACHMEMORYNVPROC) (GLuint  texture, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXTUREBARRIERPROC) ();
typedef void (*PFNGLTEXTUREBARRIERNVPROC) ();
typedef void (*PFNGLTEXTUREBUFFERPROC) (GLuint  texture, GLenum  internalformat, GLuint  buffer);
typedef void (*PFNGLTEXTUREBUFFEREXTPROC) (GLuint  texture, GLenum  target, GLenum  internalformat, GLuint  buffer);
typedef void (*PFNGLTEXTUREBUFFERRANGEPROC) (GLuint  texture, GLenum  internalformat, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLTEXTUREBUFFERRANGEEXTPROC) (GLuint  texture, GLenum  target, GLenum  internalformat, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLTEXTUREIMAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTUREIMAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLsizei  height, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTUREIMAGE2DMULTISAMPLECOVERAGENVPROC) (GLuint  texture, GLenum  target, GLsizei  coverageSamples, GLsizei  colorSamples, GLint  internalFormat, GLsizei  width, GLsizei  height, GLboolean  fixedSampleLocations);
typedef void (*PFNGLTEXTUREIMAGE2DMULTISAMPLENVPROC) (GLuint  texture, GLenum  target, GLsizei  samples, GLint  internalFormat, GLsizei  width, GLsizei  height, GLboolean  fixedSampleLocations);
typedef void (*PFNGLTEXTUREIMAGE3DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLint  border, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTUREIMAGE3DMULTISAMPLECOVERAGENVPROC) (GLuint  texture, GLenum  target, GLsizei  coverageSamples, GLsizei  colorSamples, GLint  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedSampleLocations);
typedef void (*PFNGLTEXTUREIMAGE3DMULTISAMPLENVPROC) (GLuint  texture, GLenum  target, GLsizei  samples, GLint  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedSampleLocations);
typedef void (*PFNGLTEXTURELIGHTEXTPROC) (GLenum  pname);
typedef void (*PFNGLTEXTUREMATERIALEXTPROC) (GLenum  face, GLenum  mode);
typedef void (*PFNGLTEXTURENORMALEXTPROC) (GLenum  mode);
typedef void (*PFNGLTEXTUREPAGECOMMITMENTEXTPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  commit);
typedef void (*PFNGLTEXTUREPAGECOMMITMENTMEMNVPROC) (GLuint  texture, GLint  layer, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLuint  memory, GLuint64  offset, GLboolean  commit);
typedef void (*PFNGLTEXTUREPARAMETERIIVPROC) (GLuint  texture, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXTUREPARAMETERIIVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXTUREPARAMETERIUIVPROC) (GLuint  texture, GLenum  pname, const GLuint * params);
typedef void (*PFNGLTEXTUREPARAMETERIUIVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, const GLuint * params);
typedef void (*PFNGLTEXTUREPARAMETERFPROC) (GLuint  texture, GLenum  pname, GLfloat  param);
typedef void (*PFNGLTEXTUREPARAMETERFEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, GLfloat  param);
typedef void (*PFNGLTEXTUREPARAMETERFVPROC) (GLuint  texture, GLenum  pname, const GLfloat * param);
typedef void (*PFNGLTEXTUREPARAMETERFVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLTEXTUREPARAMETERIPROC) (GLuint  texture, GLenum  pname, GLint  param);
typedef void (*PFNGLTEXTUREPARAMETERIEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, GLint  param);
typedef void (*PFNGLTEXTUREPARAMETERIVPROC) (GLuint  texture, GLenum  pname, const GLint * param);
typedef void (*PFNGLTEXTUREPARAMETERIVEXTPROC) (GLuint  texture, GLenum  target, GLenum  pname, const GLint * params);
typedef void (*PFNGLTEXTURERANGEAPPLEPROC) (GLenum  target, GLsizei  length, const void * pointer);
typedef void (*PFNGLTEXTURERENDERBUFFEREXTPROC) (GLuint  texture, GLenum  target, GLuint  renderbuffer);
typedef void (*PFNGLTEXTURESTORAGE1DPROC) (GLuint  texture, GLsizei  levels, GLenum  internalformat, GLsizei  width);
typedef void (*PFNGLTEXTURESTORAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLsizei  levels, GLenum  internalformat, GLsizei  width);
typedef void (*PFNGLTEXTURESTORAGE2DPROC) (GLuint  texture, GLsizei  levels, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLTEXTURESTORAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLsizei  levels, GLenum  internalformat, GLsizei  width, GLsizei  height);
typedef void (*PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC) (GLuint  texture, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC) (GLuint  texture, GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXTURESTORAGE3DPROC) (GLuint  texture, GLsizei  levels, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth);
typedef void (*PFNGLTEXTURESTORAGE3DEXTPROC) (GLuint  texture, GLenum  target, GLsizei  levels, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth);
typedef void (*PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC) (GLuint  texture, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC) (GLuint  texture, GLenum  target, GLsizei  samples, GLenum  internalformat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedsamplelocations);
typedef void (*PFNGLTEXTURESTORAGEMEM1DEXTPROC) (GLuint  texture, GLsizei  levels, GLenum  internalFormat, GLsizei  width, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXTURESTORAGEMEM2DEXTPROC) (GLuint  texture, GLsizei  levels, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC) (GLuint  texture, GLsizei  samples, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLboolean  fixedSampleLocations, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXTURESTORAGEMEM3DEXTPROC) (GLuint  texture, GLsizei  levels, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC) (GLuint  texture, GLsizei  samples, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLboolean  fixedSampleLocations, GLuint  memory, GLuint64  offset);
typedef void (*PFNGLTEXTURESTORAGESPARSEAMDPROC) (GLuint  texture, GLenum  target, GLenum  internalFormat, GLsizei  width, GLsizei  height, GLsizei  depth, GLsizei  layers, GLbitfield  flags);
typedef void (*PFNGLTEXTURESUBIMAGE1DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTURESUBIMAGE1DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLsizei  width, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTURESUBIMAGE2DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTURESUBIMAGE2DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLsizei  width, GLsizei  height, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTURESUBIMAGE3DPROC) (GLuint  texture, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTURESUBIMAGE3DEXTPROC) (GLuint  texture, GLenum  target, GLint  level, GLint  xoffset, GLint  yoffset, GLint  zoffset, GLsizei  width, GLsizei  height, GLsizei  depth, GLenum  format, GLenum  type, const void * pixels);
typedef void (*PFNGLTEXTUREVIEWPROC) (GLuint  texture, GLenum  target, GLuint  origtexture, GLenum  internalformat, GLuint  minlevel, GLuint  numlevels, GLuint  minlayer, GLuint  numlayers);
typedef void (*PFNGLTRACKMATRIXNVPROC) (GLenum  target, GLuint  address, GLenum  matrix, GLenum  transform);
typedef void (*PFNGLTRANSFORMFEEDBACKATTRIBSNVPROC) (GLsizei  count, const GLint * attribs, GLenum  bufferMode);
typedef void (*PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC) (GLuint  xfb, GLuint  index, GLuint  buffer);
typedef void (*PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC) (GLuint  xfb, GLuint  index, GLuint  buffer, GLintptr  offset, GLsizeiptr  size);
typedef void (*PFNGLTRANSFORMFEEDBACKSTREAMATTRIBSNVPROC) (GLsizei  count, const GLint * attribs, GLsizei  nbuffers, const GLint * bufstreams, GLenum  bufferMode);
typedef void (*PFNGLTRANSFORMFEEDBACKVARYINGSPROC) (GLuint  program, GLsizei  count, const GLchar *const* varyings, GLenum  bufferMode);
typedef void (*PFNGLTRANSFORMFEEDBACKVARYINGSEXTPROC) (GLuint  program, GLsizei  count, const GLchar *const* varyings, GLenum  bufferMode);
typedef void (*PFNGLTRANSFORMFEEDBACKVARYINGSNVPROC) (GLuint  program, GLsizei  count, const GLint * locations, GLenum  bufferMode);
typedef void (*PFNGLTRANSFORMPATHNVPROC) (GLuint  resultPath, GLuint  srcPath, GLenum  transformType, const GLfloat * transformValues);
typedef void (*PFNGLTRANSLATEDPROC) (GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLTRANSLATEFPROC) (GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLUNIFORM1DPROC) (GLint  location, GLdouble  x);
typedef void (*PFNGLUNIFORM1DVPROC) (GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLUNIFORM1FPROC) (GLint  location, GLfloat  v0);
typedef void (*PFNGLUNIFORM1FARBPROC) (GLint  location, GLfloat  v0);
typedef void (*PFNGLUNIFORM1FVPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM1FVARBPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM1IPROC) (GLint  location, GLint  v0);
typedef void (*PFNGLUNIFORM1I64ARBPROC) (GLint  location, GLint64  x);
typedef void (*PFNGLUNIFORM1I64NVPROC) (GLint  location, GLint64EXT  x);
typedef void (*PFNGLUNIFORM1I64VARBPROC) (GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLUNIFORM1I64VNVPROC) (GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLUNIFORM1IARBPROC) (GLint  location, GLint  v0);
typedef void (*PFNGLUNIFORM1IVPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM1IVARBPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM1UIPROC) (GLint  location, GLuint  v0);
typedef void (*PFNGLUNIFORM1UI64ARBPROC) (GLint  location, GLuint64  x);
typedef void (*PFNGLUNIFORM1UI64NVPROC) (GLint  location, GLuint64EXT  x);
typedef void (*PFNGLUNIFORM1UI64VARBPROC) (GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLUNIFORM1UI64VNVPROC) (GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLUNIFORM1UIEXTPROC) (GLint  location, GLuint  v0);
typedef void (*PFNGLUNIFORM1UIVPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM1UIVEXTPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM2DPROC) (GLint  location, GLdouble  x, GLdouble  y);
typedef void (*PFNGLUNIFORM2DVPROC) (GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLUNIFORM2FPROC) (GLint  location, GLfloat  v0, GLfloat  v1);
typedef void (*PFNGLUNIFORM2FARBPROC) (GLint  location, GLfloat  v0, GLfloat  v1);
typedef void (*PFNGLUNIFORM2FVPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM2FVARBPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM2IPROC) (GLint  location, GLint  v0, GLint  v1);
typedef void (*PFNGLUNIFORM2I64ARBPROC) (GLint  location, GLint64  x, GLint64  y);
typedef void (*PFNGLUNIFORM2I64NVPROC) (GLint  location, GLint64EXT  x, GLint64EXT  y);
typedef void (*PFNGLUNIFORM2I64VARBPROC) (GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLUNIFORM2I64VNVPROC) (GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLUNIFORM2IARBPROC) (GLint  location, GLint  v0, GLint  v1);
typedef void (*PFNGLUNIFORM2IVPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM2IVARBPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM2UIPROC) (GLint  location, GLuint  v0, GLuint  v1);
typedef void (*PFNGLUNIFORM2UI64ARBPROC) (GLint  location, GLuint64  x, GLuint64  y);
typedef void (*PFNGLUNIFORM2UI64NVPROC) (GLint  location, GLuint64EXT  x, GLuint64EXT  y);
typedef void (*PFNGLUNIFORM2UI64VARBPROC) (GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLUNIFORM2UI64VNVPROC) (GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLUNIFORM2UIEXTPROC) (GLint  location, GLuint  v0, GLuint  v1);
typedef void (*PFNGLUNIFORM2UIVPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM2UIVEXTPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM3DPROC) (GLint  location, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLUNIFORM3DVPROC) (GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLUNIFORM3FPROC) (GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2);
typedef void (*PFNGLUNIFORM3FARBPROC) (GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2);
typedef void (*PFNGLUNIFORM3FVPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM3FVARBPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM3IPROC) (GLint  location, GLint  v0, GLint  v1, GLint  v2);
typedef void (*PFNGLUNIFORM3I64ARBPROC) (GLint  location, GLint64  x, GLint64  y, GLint64  z);
typedef void (*PFNGLUNIFORM3I64NVPROC) (GLint  location, GLint64EXT  x, GLint64EXT  y, GLint64EXT  z);
typedef void (*PFNGLUNIFORM3I64VARBPROC) (GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLUNIFORM3I64VNVPROC) (GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLUNIFORM3IARBPROC) (GLint  location, GLint  v0, GLint  v1, GLint  v2);
typedef void (*PFNGLUNIFORM3IVPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM3IVARBPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM3UIPROC) (GLint  location, GLuint  v0, GLuint  v1, GLuint  v2);
typedef void (*PFNGLUNIFORM3UI64ARBPROC) (GLint  location, GLuint64  x, GLuint64  y, GLuint64  z);
typedef void (*PFNGLUNIFORM3UI64NVPROC) (GLint  location, GLuint64EXT  x, GLuint64EXT  y, GLuint64EXT  z);
typedef void (*PFNGLUNIFORM3UI64VARBPROC) (GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLUNIFORM3UI64VNVPROC) (GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLUNIFORM3UIEXTPROC) (GLint  location, GLuint  v0, GLuint  v1, GLuint  v2);
typedef void (*PFNGLUNIFORM3UIVPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM3UIVEXTPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM4DPROC) (GLint  location, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLUNIFORM4DVPROC) (GLint  location, GLsizei  count, const GLdouble * value);
typedef void (*PFNGLUNIFORM4FPROC) (GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3);
typedef void (*PFNGLUNIFORM4FARBPROC) (GLint  location, GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3);
typedef void (*PFNGLUNIFORM4FVPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM4FVARBPROC) (GLint  location, GLsizei  count, const GLfloat * value);
typedef void (*PFNGLUNIFORM4IPROC) (GLint  location, GLint  v0, GLint  v1, GLint  v2, GLint  v3);
typedef void (*PFNGLUNIFORM4I64ARBPROC) (GLint  location, GLint64  x, GLint64  y, GLint64  z, GLint64  w);
typedef void (*PFNGLUNIFORM4I64NVPROC) (GLint  location, GLint64EXT  x, GLint64EXT  y, GLint64EXT  z, GLint64EXT  w);
typedef void (*PFNGLUNIFORM4I64VARBPROC) (GLint  location, GLsizei  count, const GLint64 * value);
typedef void (*PFNGLUNIFORM4I64VNVPROC) (GLint  location, GLsizei  count, const GLint64EXT * value);
typedef void (*PFNGLUNIFORM4IARBPROC) (GLint  location, GLint  v0, GLint  v1, GLint  v2, GLint  v3);
typedef void (*PFNGLUNIFORM4IVPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM4IVARBPROC) (GLint  location, GLsizei  count, const GLint * value);
typedef void (*PFNGLUNIFORM4UIPROC) (GLint  location, GLuint  v0, GLuint  v1, GLuint  v2, GLuint  v3);
typedef void (*PFNGLUNIFORM4UI64ARBPROC) (GLint  location, GLuint64  x, GLuint64  y, GLuint64  z, GLuint64  w);
typedef void (*PFNGLUNIFORM4UI64NVPROC) (GLint  location, GLuint64EXT  x, GLuint64EXT  y, GLuint64EXT  z, GLuint64EXT  w);
typedef void (*PFNGLUNIFORM4UI64VARBPROC) (GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLUNIFORM4UI64VNVPROC) (GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLUNIFORM4UIEXTPROC) (GLint  location, GLuint  v0, GLuint  v1, GLuint  v2, GLuint  v3);
typedef void (*PFNGLUNIFORM4UIVPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORM4UIVEXTPROC) (GLint  location, GLsizei  count, const GLuint * value);
typedef void (*PFNGLUNIFORMBLOCKBINDINGPROC) (GLuint  program, GLuint  uniformBlockIndex, GLuint  uniformBlockBinding);
typedef void (*PFNGLUNIFORMBUFFEREXTPROC) (GLuint  program, GLint  location, GLuint  buffer);
typedef void (*PFNGLUNIFORMHANDLEUI64ARBPROC) (GLint  location, GLuint64  value);
typedef void (*PFNGLUNIFORMHANDLEUI64NVPROC) (GLint  location, GLuint64  value);
typedef void (*PFNGLUNIFORMHANDLEUI64VARBPROC) (GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLUNIFORMHANDLEUI64VNVPROC) (GLint  location, GLsizei  count, const GLuint64 * value);
typedef void (*PFNGLUNIFORMMATRIX2DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX2FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX2FVARBPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX2X3DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX2X3FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX2X4DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX2X4FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX3DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX3FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX3FVARBPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX3X2DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX3X2FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX3X4DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX3X4FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX4DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX4FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX4FVARBPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX4X2DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX4X2FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMMATRIX4X3DVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLdouble * value);
typedef void (*PFNGLUNIFORMMATRIX4X3FVPROC) (GLint  location, GLsizei  count, GLboolean  transpose, const GLfloat * value);
typedef void (*PFNGLUNIFORMSUBROUTINESUIVPROC) (GLenum  shadertype, GLsizei  count, const GLuint * indices);
typedef void (*PFNGLUNIFORMUI64NVPROC) (GLint  location, GLuint64EXT  value);
typedef void (*PFNGLUNIFORMUI64VNVPROC) (GLint  location, GLsizei  count, const GLuint64EXT * value);
typedef void (*PFNGLUNLOCKARRAYSEXTPROC) ();
typedef GLboolean (*PFNGLUNMAPBUFFERPROC) (GLenum  target);
typedef GLboolean (*PFNGLUNMAPBUFFERARBPROC) (GLenum  target);
typedef GLboolean (*PFNGLUNMAPNAMEDBUFFERPROC) (GLuint  buffer);
typedef GLboolean (*PFNGLUNMAPNAMEDBUFFEREXTPROC) (GLuint  buffer);
typedef void (*PFNGLUNMAPTEXTURE2DINTELPROC) (GLuint  texture, GLint  level);
typedef void (*PFNGLUPLOADGPUMASKNVXPROC) (GLbitfield  mask);
typedef void (*PFNGLUSEPROGRAMPROC) (GLuint  program);
typedef void (*PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB  programObj);
typedef void (*PFNGLUSEPROGRAMSTAGESPROC) (GLuint  pipeline, GLbitfield  stages, GLuint  program);
typedef void (*PFNGLUSEPROGRAMSTAGESEXTPROC) (GLuint  pipeline, GLbitfield  stages, GLuint  program);
typedef void (*PFNGLUSESHADERPROGRAMEXTPROC) (GLenum  type, GLuint  program);
typedef void (*PFNGLVDPAUFININVPROC) ();
typedef void (*PFNGLVDPAUGETSURFACEIVNVPROC) (GLvdpauSurfaceNV  surface, GLenum  pname, GLsizei  count, GLsizei * length, GLint * values);
typedef void (*PFNGLVDPAUINITNVPROC) (const void * vdpDevice, const void * getProcAddress);
typedef GLboolean (*PFNGLVDPAUISSURFACENVPROC) (GLvdpauSurfaceNV  surface);
typedef void (*PFNGLVDPAUMAPSURFACESNVPROC) (GLsizei  numSurfaces, const GLvdpauSurfaceNV * surfaces);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC) (const void * vdpSurface, GLenum  target, GLsizei  numTextureNames, const GLuint * textureNames);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTERVIDEOSURFACENVPROC) (const void * vdpSurface, GLenum  target, GLsizei  numTextureNames, const GLuint * textureNames);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTERVIDEOSURFACEWITHPICTURESTRUCTURENVPROC) (const void * vdpSurface, GLenum  target, GLsizei  numTextureNames, const GLuint * textureNames, GLboolean  isFrameStructure);
typedef void (*PFNGLVDPAUSURFACEACCESSNVPROC) (GLvdpauSurfaceNV  surface, GLenum  access);
typedef void (*PFNGLVDPAUUNMAPSURFACESNVPROC) (GLsizei  numSurface, const GLvdpauSurfaceNV * surfaces);
typedef void (*PFNGLVDPAUUNREGISTERSURFACENVPROC) (GLvdpauSurfaceNV  surface);
typedef void (*PFNGLVALIDATEPROGRAMPROC) (GLuint  program);
typedef void (*PFNGLVALIDATEPROGRAMARBPROC) (GLhandleARB  programObj);
typedef void (*PFNGLVALIDATEPROGRAMPIPELINEPROC) (GLuint  pipeline);
typedef void (*PFNGLVALIDATEPROGRAMPIPELINEEXTPROC) (GLuint  pipeline);
typedef void (*PFNGLVARIANTPOINTEREXTPROC) (GLuint  id, GLenum  type, GLuint  stride, const void * addr);
typedef void (*PFNGLVARIANTBVEXTPROC) (GLuint  id, const GLbyte * addr);
typedef void (*PFNGLVARIANTDVEXTPROC) (GLuint  id, const GLdouble * addr);
typedef void (*PFNGLVARIANTFVEXTPROC) (GLuint  id, const GLfloat * addr);
typedef void (*PFNGLVARIANTIVEXTPROC) (GLuint  id, const GLint * addr);
typedef void (*PFNGLVARIANTSVEXTPROC) (GLuint  id, const GLshort * addr);
typedef void (*PFNGLVARIANTUBVEXTPROC) (GLuint  id, const GLubyte * addr);
typedef void (*PFNGLVARIANTUIVEXTPROC) (GLuint  id, const GLuint * addr);
typedef void (*PFNGLVARIANTUSVEXTPROC) (GLuint  id, const GLushort * addr);
typedef void (*PFNGLVERTEX2DPROC) (GLdouble  x, GLdouble  y);
typedef void (*PFNGLVERTEX2DVPROC) (const GLdouble * v);
typedef void (*PFNGLVERTEX2FPROC) (GLfloat  x, GLfloat  y);
typedef void (*PFNGLVERTEX2FVPROC) (const GLfloat * v);
typedef void (*PFNGLVERTEX2HNVPROC) (GLhalfNV  x, GLhalfNV  y);
typedef void (*PFNGLVERTEX2HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLVERTEX2IPROC) (GLint  x, GLint  y);
typedef void (*PFNGLVERTEX2IVPROC) (const GLint * v);
typedef void (*PFNGLVERTEX2SPROC) (GLshort  x, GLshort  y);
typedef void (*PFNGLVERTEX2SVPROC) (const GLshort * v);
typedef void (*PFNGLVERTEX3DPROC) (GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLVERTEX3DVPROC) (const GLdouble * v);
typedef void (*PFNGLVERTEX3FPROC) (GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLVERTEX3FVPROC) (const GLfloat * v);
typedef void (*PFNGLVERTEX3HNVPROC) (GLhalfNV  x, GLhalfNV  y, GLhalfNV  z);
typedef void (*PFNGLVERTEX3HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLVERTEX3IPROC) (GLint  x, GLint  y, GLint  z);
typedef void (*PFNGLVERTEX3IVPROC) (const GLint * v);
typedef void (*PFNGLVERTEX3SPROC) (GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLVERTEX3SVPROC) (const GLshort * v);
typedef void (*PFNGLVERTEX4DPROC) (GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLVERTEX4DVPROC) (const GLdouble * v);
typedef void (*PFNGLVERTEX4FPROC) (GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLVERTEX4FVPROC) (const GLfloat * v);
typedef void (*PFNGLVERTEX4HNVPROC) (GLhalfNV  x, GLhalfNV  y, GLhalfNV  z, GLhalfNV  w);
typedef void (*PFNGLVERTEX4HVNVPROC) (const GLhalfNV * v);
typedef void (*PFNGLVERTEX4IPROC) (GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLVERTEX4IVPROC) (const GLint * v);
typedef void (*PFNGLVERTEX4SPROC) (GLshort  x, GLshort  y, GLshort  z, GLshort  w);
typedef void (*PFNGLVERTEX4SVPROC) (const GLshort * v);
typedef void (*PFNGLVERTEXARRAYATTRIBBINDINGPROC) (GLuint  vaobj, GLuint  attribindex, GLuint  bindingindex);
typedef void (*PFNGLVERTEXARRAYATTRIBFORMATPROC) (GLuint  vaobj, GLuint  attribindex, GLint  size, GLenum  type, GLboolean  normalized, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXARRAYATTRIBIFORMATPROC) (GLuint  vaobj, GLuint  attribindex, GLint  size, GLenum  type, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXARRAYATTRIBLFORMATPROC) (GLuint  vaobj, GLuint  attribindex, GLint  size, GLenum  type, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC) (GLuint  vaobj, GLuint  bindingindex, GLuint  buffer, GLintptr  offset, GLsizei  stride);
typedef void (*PFNGLVERTEXARRAYBINDINGDIVISORPROC) (GLuint  vaobj, GLuint  bindingindex, GLuint  divisor);
typedef void (*PFNGLVERTEXARRAYCOLOROFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYELEMENTBUFFERPROC) (GLuint  vaobj, GLuint  buffer);
typedef void (*PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYINDEXOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLenum  texunit, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYNORMALOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYPARAMETERIAPPLEPROC) (GLenum  pname, GLint  param);
typedef void (*PFNGLVERTEXARRAYRANGEAPPLEPROC) (GLsizei  length, void * pointer);
typedef void (*PFNGLVERTEXARRAYRANGENVPROC) (GLsizei  length, const void * pointer);
typedef void (*PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC) (GLuint  vaobj, GLuint  attribindex, GLuint  bindingindex);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC) (GLuint  vaobj, GLuint  index, GLuint  divisor);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC) (GLuint  vaobj, GLuint  attribindex, GLint  size, GLenum  type, GLboolean  normalized, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC) (GLuint  vaobj, GLuint  attribindex, GLint  size, GLenum  type, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLuint  index, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC) (GLuint  vaobj, GLuint  attribindex, GLint  size, GLenum  type, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLuint  index, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLuint  index, GLint  size, GLenum  type, GLboolean  normalized, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC) (GLuint  vaobj, GLuint  bindingindex, GLuint  divisor);
typedef void (*PFNGLVERTEXARRAYVERTEXBUFFERPROC) (GLuint  vaobj, GLuint  bindingindex, GLuint  buffer, GLintptr  offset, GLsizei  stride);
typedef void (*PFNGLVERTEXARRAYVERTEXBUFFERSPROC) (GLuint  vaobj, GLuint  first, GLsizei  count, const GLuint * buffers, const GLintptr * offsets, const GLsizei * strides);
typedef void (*PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC) (GLuint  vaobj, GLuint  buffer, GLint  size, GLenum  type, GLsizei  stride, GLintptr  offset);
typedef void (*PFNGLVERTEXATTRIB1DPROC) (GLuint  index, GLdouble  x);
typedef void (*PFNGLVERTEXATTRIB1DARBPROC) (GLuint  index, GLdouble  x);
typedef void (*PFNGLVERTEXATTRIB1DNVPROC) (GLuint  index, GLdouble  x);
typedef void (*PFNGLVERTEXATTRIB1DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB1DVARBPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB1DVNVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB1FPROC) (GLuint  index, GLfloat  x);
typedef void (*PFNGLVERTEXATTRIB1FARBPROC) (GLuint  index, GLfloat  x);
typedef void (*PFNGLVERTEXATTRIB1FNVPROC) (GLuint  index, GLfloat  x);
typedef void (*PFNGLVERTEXATTRIB1FVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB1FVARBPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB1FVNVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB1HNVPROC) (GLuint  index, GLhalfNV  x);
typedef void (*PFNGLVERTEXATTRIB1HVNVPROC) (GLuint  index, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIB1SPROC) (GLuint  index, GLshort  x);
typedef void (*PFNGLVERTEXATTRIB1SARBPROC) (GLuint  index, GLshort  x);
typedef void (*PFNGLVERTEXATTRIB1SNVPROC) (GLuint  index, GLshort  x);
typedef void (*PFNGLVERTEXATTRIB1SVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB1SVARBPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB1SVNVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB2DPROC) (GLuint  index, GLdouble  x, GLdouble  y);
typedef void (*PFNGLVERTEXATTRIB2DARBPROC) (GLuint  index, GLdouble  x, GLdouble  y);
typedef void (*PFNGLVERTEXATTRIB2DNVPROC) (GLuint  index, GLdouble  x, GLdouble  y);
typedef void (*PFNGLVERTEXATTRIB2DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB2DVARBPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB2DVNVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB2FPROC) (GLuint  index, GLfloat  x, GLfloat  y);
typedef void (*PFNGLVERTEXATTRIB2FARBPROC) (GLuint  index, GLfloat  x, GLfloat  y);
typedef void (*PFNGLVERTEXATTRIB2FNVPROC) (GLuint  index, GLfloat  x, GLfloat  y);
typedef void (*PFNGLVERTEXATTRIB2FVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB2FVARBPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB2FVNVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB2HNVPROC) (GLuint  index, GLhalfNV  x, GLhalfNV  y);
typedef void (*PFNGLVERTEXATTRIB2HVNVPROC) (GLuint  index, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIB2SPROC) (GLuint  index, GLshort  x, GLshort  y);
typedef void (*PFNGLVERTEXATTRIB2SARBPROC) (GLuint  index, GLshort  x, GLshort  y);
typedef void (*PFNGLVERTEXATTRIB2SNVPROC) (GLuint  index, GLshort  x, GLshort  y);
typedef void (*PFNGLVERTEXATTRIB2SVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB2SVARBPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB2SVNVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB3DPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLVERTEXATTRIB3DARBPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLVERTEXATTRIB3DNVPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLVERTEXATTRIB3DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB3DVARBPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB3DVNVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB3FPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLVERTEXATTRIB3FARBPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLVERTEXATTRIB3FNVPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLVERTEXATTRIB3FVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB3FVARBPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB3FVNVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB3HNVPROC) (GLuint  index, GLhalfNV  x, GLhalfNV  y, GLhalfNV  z);
typedef void (*PFNGLVERTEXATTRIB3HVNVPROC) (GLuint  index, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIB3SPROC) (GLuint  index, GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLVERTEXATTRIB3SARBPROC) (GLuint  index, GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLVERTEXATTRIB3SNVPROC) (GLuint  index, GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLVERTEXATTRIB3SVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB3SVARBPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB3SVNVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB4NBVPROC) (GLuint  index, const GLbyte * v);
typedef void (*PFNGLVERTEXATTRIB4NBVARBPROC) (GLuint  index, const GLbyte * v);
typedef void (*PFNGLVERTEXATTRIB4NIVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIB4NIVARBPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIB4NSVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB4NSVARBPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB4NUBPROC) (GLuint  index, GLubyte  x, GLubyte  y, GLubyte  z, GLubyte  w);
typedef void (*PFNGLVERTEXATTRIB4NUBARBPROC) (GLuint  index, GLubyte  x, GLubyte  y, GLubyte  z, GLubyte  w);
typedef void (*PFNGLVERTEXATTRIB4NUBVPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIB4NUBVARBPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIB4NUIVPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIB4NUIVARBPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIB4NUSVPROC) (GLuint  index, const GLushort * v);
typedef void (*PFNGLVERTEXATTRIB4NUSVARBPROC) (GLuint  index, const GLushort * v);
typedef void (*PFNGLVERTEXATTRIB4BVPROC) (GLuint  index, const GLbyte * v);
typedef void (*PFNGLVERTEXATTRIB4BVARBPROC) (GLuint  index, const GLbyte * v);
typedef void (*PFNGLVERTEXATTRIB4DPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLVERTEXATTRIB4DARBPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLVERTEXATTRIB4DNVPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLVERTEXATTRIB4DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB4DVARBPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB4DVNVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIB4FPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLVERTEXATTRIB4FARBPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLVERTEXATTRIB4FNVPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w);
typedef void (*PFNGLVERTEXATTRIB4FVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB4FVARBPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB4FVNVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIB4HNVPROC) (GLuint  index, GLhalfNV  x, GLhalfNV  y, GLhalfNV  z, GLhalfNV  w);
typedef void (*PFNGLVERTEXATTRIB4HVNVPROC) (GLuint  index, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIB4IVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIB4IVARBPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIB4SPROC) (GLuint  index, GLshort  x, GLshort  y, GLshort  z, GLshort  w);
typedef void (*PFNGLVERTEXATTRIB4SARBPROC) (GLuint  index, GLshort  x, GLshort  y, GLshort  z, GLshort  w);
typedef void (*PFNGLVERTEXATTRIB4SNVPROC) (GLuint  index, GLshort  x, GLshort  y, GLshort  z, GLshort  w);
typedef void (*PFNGLVERTEXATTRIB4SVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB4SVARBPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB4SVNVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIB4UBNVPROC) (GLuint  index, GLubyte  x, GLubyte  y, GLubyte  z, GLubyte  w);
typedef void (*PFNGLVERTEXATTRIB4UBVPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIB4UBVARBPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIB4UBVNVPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIB4UIVPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIB4UIVARBPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIB4USVPROC) (GLuint  index, const GLushort * v);
typedef void (*PFNGLVERTEXATTRIB4USVARBPROC) (GLuint  index, const GLushort * v);
typedef void (*PFNGLVERTEXATTRIBBINDINGPROC) (GLuint  attribindex, GLuint  bindingindex);
typedef void (*PFNGLVERTEXATTRIBDIVISORPROC) (GLuint  index, GLuint  divisor);
typedef void (*PFNGLVERTEXATTRIBDIVISORARBPROC) (GLuint  index, GLuint  divisor);
typedef void (*PFNGLVERTEXATTRIBFORMATPROC) (GLuint  attribindex, GLint  size, GLenum  type, GLboolean  normalized, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXATTRIBFORMATNVPROC) (GLuint  index, GLint  size, GLenum  type, GLboolean  normalized, GLsizei  stride);
typedef void (*PFNGLVERTEXATTRIBI1IPROC) (GLuint  index, GLint  x);
typedef void (*PFNGLVERTEXATTRIBI1IEXTPROC) (GLuint  index, GLint  x);
typedef void (*PFNGLVERTEXATTRIBI1IVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI1IVEXTPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI1UIPROC) (GLuint  index, GLuint  x);
typedef void (*PFNGLVERTEXATTRIBI1UIEXTPROC) (GLuint  index, GLuint  x);
typedef void (*PFNGLVERTEXATTRIBI1UIVPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI1UIVEXTPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI2IPROC) (GLuint  index, GLint  x, GLint  y);
typedef void (*PFNGLVERTEXATTRIBI2IEXTPROC) (GLuint  index, GLint  x, GLint  y);
typedef void (*PFNGLVERTEXATTRIBI2IVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI2IVEXTPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI2UIPROC) (GLuint  index, GLuint  x, GLuint  y);
typedef void (*PFNGLVERTEXATTRIBI2UIEXTPROC) (GLuint  index, GLuint  x, GLuint  y);
typedef void (*PFNGLVERTEXATTRIBI2UIVPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI2UIVEXTPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI3IPROC) (GLuint  index, GLint  x, GLint  y, GLint  z);
typedef void (*PFNGLVERTEXATTRIBI3IEXTPROC) (GLuint  index, GLint  x, GLint  y, GLint  z);
typedef void (*PFNGLVERTEXATTRIBI3IVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI3IVEXTPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI3UIPROC) (GLuint  index, GLuint  x, GLuint  y, GLuint  z);
typedef void (*PFNGLVERTEXATTRIBI3UIEXTPROC) (GLuint  index, GLuint  x, GLuint  y, GLuint  z);
typedef void (*PFNGLVERTEXATTRIBI3UIVPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI3UIVEXTPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI4BVPROC) (GLuint  index, const GLbyte * v);
typedef void (*PFNGLVERTEXATTRIBI4BVEXTPROC) (GLuint  index, const GLbyte * v);
typedef void (*PFNGLVERTEXATTRIBI4IPROC) (GLuint  index, GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLVERTEXATTRIBI4IEXTPROC) (GLuint  index, GLint  x, GLint  y, GLint  z, GLint  w);
typedef void (*PFNGLVERTEXATTRIBI4IVPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI4IVEXTPROC) (GLuint  index, const GLint * v);
typedef void (*PFNGLVERTEXATTRIBI4SVPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIBI4SVEXTPROC) (GLuint  index, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIBI4UBVPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIBI4UBVEXTPROC) (GLuint  index, const GLubyte * v);
typedef void (*PFNGLVERTEXATTRIBI4UIPROC) (GLuint  index, GLuint  x, GLuint  y, GLuint  z, GLuint  w);
typedef void (*PFNGLVERTEXATTRIBI4UIEXTPROC) (GLuint  index, GLuint  x, GLuint  y, GLuint  z, GLuint  w);
typedef void (*PFNGLVERTEXATTRIBI4UIVPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI4UIVEXTPROC) (GLuint  index, const GLuint * v);
typedef void (*PFNGLVERTEXATTRIBI4USVPROC) (GLuint  index, const GLushort * v);
typedef void (*PFNGLVERTEXATTRIBI4USVEXTPROC) (GLuint  index, const GLushort * v);
typedef void (*PFNGLVERTEXATTRIBIFORMATPROC) (GLuint  attribindex, GLint  size, GLenum  type, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXATTRIBIFORMATNVPROC) (GLuint  index, GLint  size, GLenum  type, GLsizei  stride);
typedef void (*PFNGLVERTEXATTRIBIPOINTERPROC) (GLuint  index, GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBIPOINTEREXTPROC) (GLuint  index, GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBL1DPROC) (GLuint  index, GLdouble  x);
typedef void (*PFNGLVERTEXATTRIBL1DEXTPROC) (GLuint  index, GLdouble  x);
typedef void (*PFNGLVERTEXATTRIBL1DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL1DVEXTPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL1I64NVPROC) (GLuint  index, GLint64EXT  x);
typedef void (*PFNGLVERTEXATTRIBL1I64VNVPROC) (GLuint  index, const GLint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL1UI64ARBPROC) (GLuint  index, GLuint64EXT  x);
typedef void (*PFNGLVERTEXATTRIBL1UI64NVPROC) (GLuint  index, GLuint64EXT  x);
typedef void (*PFNGLVERTEXATTRIBL1UI64VARBPROC) (GLuint  index, const GLuint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL1UI64VNVPROC) (GLuint  index, const GLuint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL2DPROC) (GLuint  index, GLdouble  x, GLdouble  y);
typedef void (*PFNGLVERTEXATTRIBL2DEXTPROC) (GLuint  index, GLdouble  x, GLdouble  y);
typedef void (*PFNGLVERTEXATTRIBL2DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL2DVEXTPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL2I64NVPROC) (GLuint  index, GLint64EXT  x, GLint64EXT  y);
typedef void (*PFNGLVERTEXATTRIBL2I64VNVPROC) (GLuint  index, const GLint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL2UI64NVPROC) (GLuint  index, GLuint64EXT  x, GLuint64EXT  y);
typedef void (*PFNGLVERTEXATTRIBL2UI64VNVPROC) (GLuint  index, const GLuint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL3DPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLVERTEXATTRIBL3DEXTPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLVERTEXATTRIBL3DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL3DVEXTPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL3I64NVPROC) (GLuint  index, GLint64EXT  x, GLint64EXT  y, GLint64EXT  z);
typedef void (*PFNGLVERTEXATTRIBL3I64VNVPROC) (GLuint  index, const GLint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL3UI64NVPROC) (GLuint  index, GLuint64EXT  x, GLuint64EXT  y, GLuint64EXT  z);
typedef void (*PFNGLVERTEXATTRIBL3UI64VNVPROC) (GLuint  index, const GLuint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL4DPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLVERTEXATTRIBL4DEXTPROC) (GLuint  index, GLdouble  x, GLdouble  y, GLdouble  z, GLdouble  w);
typedef void (*PFNGLVERTEXATTRIBL4DVPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL4DVEXTPROC) (GLuint  index, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBL4I64NVPROC) (GLuint  index, GLint64EXT  x, GLint64EXT  y, GLint64EXT  z, GLint64EXT  w);
typedef void (*PFNGLVERTEXATTRIBL4I64VNVPROC) (GLuint  index, const GLint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBL4UI64NVPROC) (GLuint  index, GLuint64EXT  x, GLuint64EXT  y, GLuint64EXT  z, GLuint64EXT  w);
typedef void (*PFNGLVERTEXATTRIBL4UI64VNVPROC) (GLuint  index, const GLuint64EXT * v);
typedef void (*PFNGLVERTEXATTRIBLFORMATPROC) (GLuint  attribindex, GLint  size, GLenum  type, GLuint  relativeoffset);
typedef void (*PFNGLVERTEXATTRIBLFORMATNVPROC) (GLuint  index, GLint  size, GLenum  type, GLsizei  stride);
typedef void (*PFNGLVERTEXATTRIBLPOINTERPROC) (GLuint  index, GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBLPOINTEREXTPROC) (GLuint  index, GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBP1UIPROC) (GLuint  index, GLenum  type, GLboolean  normalized, GLuint  value);
typedef void (*PFNGLVERTEXATTRIBP1UIVPROC) (GLuint  index, GLenum  type, GLboolean  normalized, const GLuint * value);
typedef void (*PFNGLVERTEXATTRIBP2UIPROC) (GLuint  index, GLenum  type, GLboolean  normalized, GLuint  value);
typedef void (*PFNGLVERTEXATTRIBP2UIVPROC) (GLuint  index, GLenum  type, GLboolean  normalized, const GLuint * value);
typedef void (*PFNGLVERTEXATTRIBP3UIPROC) (GLuint  index, GLenum  type, GLboolean  normalized, GLuint  value);
typedef void (*PFNGLVERTEXATTRIBP3UIVPROC) (GLuint  index, GLenum  type, GLboolean  normalized, const GLuint * value);
typedef void (*PFNGLVERTEXATTRIBP4UIPROC) (GLuint  index, GLenum  type, GLboolean  normalized, GLuint  value);
typedef void (*PFNGLVERTEXATTRIBP4UIVPROC) (GLuint  index, GLenum  type, GLboolean  normalized, const GLuint * value);
typedef void (*PFNGLVERTEXATTRIBPARAMETERIAMDPROC) (GLuint  index, GLenum  pname, GLint  param);
typedef void (*PFNGLVERTEXATTRIBPOINTERPROC) (GLuint  index, GLint  size, GLenum  type, GLboolean  normalized, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint  index, GLint  size, GLenum  type, GLboolean  normalized, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBPOINTERNVPROC) (GLuint  index, GLint  fsize, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXATTRIBS1DVNVPROC) (GLuint  index, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBS1FVNVPROC) (GLuint  index, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIBS1HVNVPROC) (GLuint  index, GLsizei  n, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIBS1SVNVPROC) (GLuint  index, GLsizei  count, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIBS2DVNVPROC) (GLuint  index, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBS2FVNVPROC) (GLuint  index, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIBS2HVNVPROC) (GLuint  index, GLsizei  n, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIBS2SVNVPROC) (GLuint  index, GLsizei  count, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIBS3DVNVPROC) (GLuint  index, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBS3FVNVPROC) (GLuint  index, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIBS3HVNVPROC) (GLuint  index, GLsizei  n, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIBS3SVNVPROC) (GLuint  index, GLsizei  count, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIBS4DVNVPROC) (GLuint  index, GLsizei  count, const GLdouble * v);
typedef void (*PFNGLVERTEXATTRIBS4FVNVPROC) (GLuint  index, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLVERTEXATTRIBS4HVNVPROC) (GLuint  index, GLsizei  n, const GLhalfNV * v);
typedef void (*PFNGLVERTEXATTRIBS4SVNVPROC) (GLuint  index, GLsizei  count, const GLshort * v);
typedef void (*PFNGLVERTEXATTRIBS4UBVNVPROC) (GLuint  index, GLsizei  count, const GLubyte * v);
typedef void (*PFNGLVERTEXBINDINGDIVISORPROC) (GLuint  bindingindex, GLuint  divisor);
typedef void (*PFNGLVERTEXBLENDARBPROC) (GLint  count);
typedef void (*PFNGLVERTEXFORMATNVPROC) (GLint  size, GLenum  type, GLsizei  stride);
typedef void (*PFNGLVERTEXP2UIPROC) (GLenum  type, GLuint  value);
typedef void (*PFNGLVERTEXP2UIVPROC) (GLenum  type, const GLuint * value);
typedef void (*PFNGLVERTEXP3UIPROC) (GLenum  type, GLuint  value);
typedef void (*PFNGLVERTEXP3UIVPROC) (GLenum  type, const GLuint * value);
typedef void (*PFNGLVERTEXP4UIPROC) (GLenum  type, GLuint  value);
typedef void (*PFNGLVERTEXP4UIVPROC) (GLenum  type, const GLuint * value);
typedef void (*PFNGLVERTEXPOINTERPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXPOINTEREXTPROC) (GLint  size, GLenum  type, GLsizei  stride, GLsizei  count, const void * pointer);
typedef void (*PFNGLVERTEXPOINTERVINTELPROC) (GLint  size, GLenum  type, const void ** pointer);
typedef void (*PFNGLVERTEXWEIGHTPOINTEREXTPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLVERTEXWEIGHTFEXTPROC) (GLfloat  weight);
typedef void (*PFNGLVERTEXWEIGHTFVEXTPROC) (const GLfloat * weight);
typedef void (*PFNGLVERTEXWEIGHTHNVPROC) (GLhalfNV  weight);
typedef void (*PFNGLVERTEXWEIGHTHVNVPROC) (const GLhalfNV * weight);
typedef GLenum (*PFNGLVIDEOCAPTURENVPROC) (GLuint  video_capture_slot, GLuint * sequence_num, GLuint64EXT * capture_time);
typedef void (*PFNGLVIDEOCAPTURESTREAMPARAMETERDVNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  pname, const GLdouble * params);
typedef void (*PFNGLVIDEOCAPTURESTREAMPARAMETERFVNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  pname, const GLfloat * params);
typedef void (*PFNGLVIDEOCAPTURESTREAMPARAMETERIVNVPROC) (GLuint  video_capture_slot, GLuint  stream, GLenum  pname, const GLint * params);
typedef void (*PFNGLVIEWPORTPROC) (GLint  x, GLint  y, GLsizei  width, GLsizei  height);
typedef void (*PFNGLVIEWPORTARRAYVPROC) (GLuint  first, GLsizei  count, const GLfloat * v);
typedef void (*PFNGLVIEWPORTINDEXEDFPROC) (GLuint  index, GLfloat  x, GLfloat  y, GLfloat  w, GLfloat  h);
typedef void (*PFNGLVIEWPORTINDEXEDFVPROC) (GLuint  index, const GLfloat * v);
typedef void (*PFNGLVIEWPORTPOSITIONWSCALENVPROC) (GLuint  index, GLfloat  xcoeff, GLfloat  ycoeff);
typedef void (*PFNGLVIEWPORTSWIZZLENVPROC) (GLuint  index, GLenum  swizzlex, GLenum  swizzley, GLenum  swizzlez, GLenum  swizzlew);
typedef void (*PFNGLWAITSEMAPHOREEXTPROC) (GLuint  semaphore, GLuint  numBufferBarriers, const GLuint * buffers, GLuint  numTextureBarriers, const GLuint * textures, const GLenum * srcLayouts);
typedef void (*PFNGLWAITSEMAPHOREUI64NVXPROC) (GLuint  waitGpu, GLsizei  fenceObjectCount, const GLuint * semaphoreArray, const GLuint64 * fenceValueArray);
typedef void (*PFNGLWAITSYNCPROC) (GLsync  sync, GLbitfield  flags, GLuint64  timeout);
typedef void (*PFNGLWEIGHTPATHSNVPROC) (GLuint  resultPath, GLsizei  numPaths, const GLuint * paths, const GLfloat * weights);
typedef void (*PFNGLWEIGHTPOINTERARBPROC) (GLint  size, GLenum  type, GLsizei  stride, const void * pointer);
typedef void (*PFNGLWEIGHTBVARBPROC) (GLint  size, const GLbyte * weights);
typedef void (*PFNGLWEIGHTDVARBPROC) (GLint  size, const GLdouble * weights);
typedef void (*PFNGLWEIGHTFVARBPROC) (GLint  size, const GLfloat * weights);
typedef void (*PFNGLWEIGHTIVARBPROC) (GLint  size, const GLint * weights);
typedef void (*PFNGLWEIGHTSVARBPROC) (GLint  size, const GLshort * weights);
typedef void (*PFNGLWEIGHTUBVARBPROC) (GLint  size, const GLubyte * weights);
typedef void (*PFNGLWEIGHTUIVARBPROC) (GLint  size, const GLuint * weights);
typedef void (*PFNGLWEIGHTUSVARBPROC) (GLint  size, const GLushort * weights);
typedef void (*PFNGLWINDOWPOS2DPROC) (GLdouble  x, GLdouble  y);
typedef void (*PFNGLWINDOWPOS2DARBPROC) (GLdouble  x, GLdouble  y);
typedef void (*PFNGLWINDOWPOS2DVPROC) (const GLdouble * v);
typedef void (*PFNGLWINDOWPOS2DVARBPROC) (const GLdouble * v);
typedef void (*PFNGLWINDOWPOS2FPROC) (GLfloat  x, GLfloat  y);
typedef void (*PFNGLWINDOWPOS2FARBPROC) (GLfloat  x, GLfloat  y);
typedef void (*PFNGLWINDOWPOS2FVPROC) (const GLfloat * v);
typedef void (*PFNGLWINDOWPOS2FVARBPROC) (const GLfloat * v);
typedef void (*PFNGLWINDOWPOS2IPROC) (GLint  x, GLint  y);
typedef void (*PFNGLWINDOWPOS2IARBPROC) (GLint  x, GLint  y);
typedef void (*PFNGLWINDOWPOS2IVPROC) (const GLint * v);
typedef void (*PFNGLWINDOWPOS2IVARBPROC) (const GLint * v);
typedef void (*PFNGLWINDOWPOS2SPROC) (GLshort  x, GLshort  y);
typedef void (*PFNGLWINDOWPOS2SARBPROC) (GLshort  x, GLshort  y);
typedef void (*PFNGLWINDOWPOS2SVPROC) (const GLshort * v);
typedef void (*PFNGLWINDOWPOS2SVARBPROC) (const GLshort * v);
typedef void (*PFNGLWINDOWPOS3DPROC) (GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLWINDOWPOS3DARBPROC) (GLdouble  x, GLdouble  y, GLdouble  z);
typedef void (*PFNGLWINDOWPOS3DVPROC) (const GLdouble * v);
typedef void (*PFNGLWINDOWPOS3DVARBPROC) (const GLdouble * v);
typedef void (*PFNGLWINDOWPOS3FPROC) (GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLWINDOWPOS3FARBPROC) (GLfloat  x, GLfloat  y, GLfloat  z);
typedef void (*PFNGLWINDOWPOS3FVPROC) (const GLfloat * v);
typedef void (*PFNGLWINDOWPOS3FVARBPROC) (const GLfloat * v);
typedef void (*PFNGLWINDOWPOS3IPROC) (GLint  x, GLint  y, GLint  z);
typedef void (*PFNGLWINDOWPOS3IARBPROC) (GLint  x, GLint  y, GLint  z);
typedef void (*PFNGLWINDOWPOS3IVPROC) (const GLint * v);
typedef void (*PFNGLWINDOWPOS3IVARBPROC) (const GLint * v);
typedef void (*PFNGLWINDOWPOS3SPROC) (GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLWINDOWPOS3SARBPROC) (GLshort  x, GLshort  y, GLshort  z);
typedef void (*PFNGLWINDOWPOS3SVPROC) (const GLshort * v);
typedef void (*PFNGLWINDOWPOS3SVARBPROC) (const GLshort * v);
typedef void (*PFNGLWINDOWRECTANGLESEXTPROC) (GLenum  mode, GLsizei  count, const GLint * box);
typedef void (*PFNGLWRITEMASKEXTPROC) (GLuint  res, GLuint  in, GLenum  outX, GLenum  outY, GLenum  outZ, GLenum  outW);
typedef void (*PFNGLDRAWVKIMAGENVPROC) (GLuint64  vkImage, GLuint  sampler, GLfloat  x0, GLfloat  y0, GLfloat  x1, GLfloat  y1, GLfloat  z, GLfloat  s0, GLfloat  t0, GLfloat  s1, GLfloat  t1);
typedef GLVULKANPROCNV (*PFNGLGETVKPROCADDRNVPROC) (const GLchar * name);
typedef void (*PFNGLWAITVKSEMAPHORENVPROC) (GLuint64  vkSemaphore);
typedef void (*PFNGLSIGNALVKSEMAPHORENVPROC) (GLuint64  vkSemaphore);
typedef void (*PFNGLSIGNALVKFENCENVPROC) (GLuint64  vkFence);


GARCH_API extern bool GARCH_GL_VERSION_1_0;
GARCH_API extern bool GARCH_GL_VERSION_1_1;
GARCH_API extern bool GARCH_GL_VERSION_1_2;
GARCH_API extern bool GARCH_GL_VERSION_1_3;
GARCH_API extern bool GARCH_GL_VERSION_1_4;
GARCH_API extern bool GARCH_GL_VERSION_1_5;
GARCH_API extern bool GARCH_GL_VERSION_2_0;
GARCH_API extern bool GARCH_GL_VERSION_2_1;
GARCH_API extern bool GARCH_GL_VERSION_3_0;
GARCH_API extern bool GARCH_GL_VERSION_3_1;
GARCH_API extern bool GARCH_GL_VERSION_3_2;
GARCH_API extern bool GARCH_GL_VERSION_3_3;
GARCH_API extern bool GARCH_GL_VERSION_4_0;
GARCH_API extern bool GARCH_GL_VERSION_4_1;
GARCH_API extern bool GARCH_GL_VERSION_4_2;
GARCH_API extern bool GARCH_GL_VERSION_4_3;
GARCH_API extern bool GARCH_GL_VERSION_4_4;
GARCH_API extern bool GARCH_GL_VERSION_4_5;
GARCH_API extern bool GARCH_GL_VERSION_4_6;


GARCH_API extern bool GARCH_GL_AMD_blend_minmax_factor;
GARCH_API extern bool GARCH_GL_AMD_conservative_depth;
GARCH_API extern bool GARCH_GL_AMD_debug_output;
GARCH_API extern bool GARCH_GL_AMD_depth_clamp_separate;
GARCH_API extern bool GARCH_GL_AMD_draw_buffers_blend;
GARCH_API extern bool GARCH_GL_AMD_framebuffer_multisample_advanced;
GARCH_API extern bool GARCH_GL_AMD_framebuffer_sample_positions;
GARCH_API extern bool GARCH_GL_AMD_gcn_shader;
GARCH_API extern bool GARCH_GL_AMD_gpu_shader_half_float;
GARCH_API extern bool GARCH_GL_AMD_gpu_shader_int16;
GARCH_API extern bool GARCH_GL_AMD_gpu_shader_int64;
GARCH_API extern bool GARCH_GL_AMD_interleaved_elements;
GARCH_API extern bool GARCH_GL_AMD_multi_draw_indirect;
GARCH_API extern bool GARCH_GL_AMD_name_gen_delete;
GARCH_API extern bool GARCH_GL_AMD_occlusion_query_event;
GARCH_API extern bool GARCH_GL_AMD_performance_monitor;
GARCH_API extern bool GARCH_GL_AMD_pinned_memory;
GARCH_API extern bool GARCH_GL_AMD_query_buffer_object;
GARCH_API extern bool GARCH_GL_AMD_sample_positions;
GARCH_API extern bool GARCH_GL_AMD_seamless_cubemap_per_texture;
GARCH_API extern bool GARCH_GL_AMD_shader_atomic_counter_ops;
GARCH_API extern bool GARCH_GL_AMD_shader_ballot;
GARCH_API extern bool GARCH_GL_AMD_shader_gpu_shader_half_float_fetch;
GARCH_API extern bool GARCH_GL_AMD_shader_image_load_store_lod;
GARCH_API extern bool GARCH_GL_AMD_shader_stencil_export;
GARCH_API extern bool GARCH_GL_AMD_shader_trinary_minmax;
GARCH_API extern bool GARCH_GL_AMD_shader_explicit_vertex_parameter;
GARCH_API extern bool GARCH_GL_AMD_sparse_texture;
GARCH_API extern bool GARCH_GL_AMD_stencil_operation_extended;
GARCH_API extern bool GARCH_GL_AMD_texture_gather_bias_lod;
GARCH_API extern bool GARCH_GL_AMD_texture_texture4;
GARCH_API extern bool GARCH_GL_AMD_transform_feedback3_lines_triangles;
GARCH_API extern bool GARCH_GL_AMD_transform_feedback4;
GARCH_API extern bool GARCH_GL_AMD_vertex_shader_layer;
GARCH_API extern bool GARCH_GL_AMD_vertex_shader_tessellator;
GARCH_API extern bool GARCH_GL_AMD_vertex_shader_viewport_index;
GARCH_API extern bool GARCH_GL_APPLE_aux_depth_stencil;
GARCH_API extern bool GARCH_GL_APPLE_client_storage;
GARCH_API extern bool GARCH_GL_APPLE_element_array;
GARCH_API extern bool GARCH_GL_APPLE_fence;
GARCH_API extern bool GARCH_GL_APPLE_float_pixels;
GARCH_API extern bool GARCH_GL_APPLE_flush_buffer_range;
GARCH_API extern bool GARCH_GL_APPLE_object_purgeable;
GARCH_API extern bool GARCH_GL_APPLE_rgb_422;
GARCH_API extern bool GARCH_GL_APPLE_row_bytes;
GARCH_API extern bool GARCH_GL_APPLE_specular_vector;
GARCH_API extern bool GARCH_GL_APPLE_texture_range;
GARCH_API extern bool GARCH_GL_APPLE_transform_hint;
GARCH_API extern bool GARCH_GL_APPLE_vertex_array_object;
GARCH_API extern bool GARCH_GL_APPLE_vertex_array_range;
GARCH_API extern bool GARCH_GL_APPLE_vertex_program_evaluators;
GARCH_API extern bool GARCH_GL_APPLE_ycbcr_422;
GARCH_API extern bool GARCH_GL_ARB_ES2_compatibility;
GARCH_API extern bool GARCH_GL_ARB_ES3_1_compatibility;
GARCH_API extern bool GARCH_GL_ARB_ES3_2_compatibility;
GARCH_API extern bool GARCH_GL_ARB_ES3_compatibility;
GARCH_API extern bool GARCH_GL_ARB_arrays_of_arrays;
GARCH_API extern bool GARCH_GL_ARB_base_instance;
GARCH_API extern bool GARCH_GL_ARB_bindless_texture;
GARCH_API extern bool GARCH_GL_ARB_blend_func_extended;
GARCH_API extern bool GARCH_GL_ARB_buffer_storage;
GARCH_API extern bool GARCH_GL_ARB_cl_event;
GARCH_API extern bool GARCH_GL_ARB_clear_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_clear_texture;
GARCH_API extern bool GARCH_GL_ARB_clip_control;
GARCH_API extern bool GARCH_GL_ARB_color_buffer_float;
GARCH_API extern bool GARCH_GL_ARB_compatibility;
GARCH_API extern bool GARCH_GL_ARB_compressed_texture_pixel_storage;
GARCH_API extern bool GARCH_GL_ARB_compute_shader;
GARCH_API extern bool GARCH_GL_ARB_compute_variable_group_size;
GARCH_API extern bool GARCH_GL_ARB_conditional_render_inverted;
GARCH_API extern bool GARCH_GL_ARB_conservative_depth;
GARCH_API extern bool GARCH_GL_ARB_copy_buffer;
GARCH_API extern bool GARCH_GL_ARB_copy_image;
GARCH_API extern bool GARCH_GL_ARB_cull_distance;
GARCH_API extern bool GARCH_GL_ARB_debug_output;
GARCH_API extern bool GARCH_GL_ARB_depth_buffer_float;
GARCH_API extern bool GARCH_GL_ARB_depth_clamp;
GARCH_API extern bool GARCH_GL_ARB_depth_texture;
GARCH_API extern bool GARCH_GL_ARB_derivative_control;
GARCH_API extern bool GARCH_GL_ARB_direct_state_access;
GARCH_API extern bool GARCH_GL_ARB_draw_buffers;
GARCH_API extern bool GARCH_GL_ARB_draw_buffers_blend;
GARCH_API extern bool GARCH_GL_ARB_draw_elements_base_vertex;
GARCH_API extern bool GARCH_GL_ARB_draw_indirect;
GARCH_API extern bool GARCH_GL_ARB_draw_instanced;
GARCH_API extern bool GARCH_GL_ARB_enhanced_layouts;
GARCH_API extern bool GARCH_GL_ARB_explicit_attrib_location;
GARCH_API extern bool GARCH_GL_ARB_explicit_uniform_location;
GARCH_API extern bool GARCH_GL_ARB_fragment_coord_conventions;
GARCH_API extern bool GARCH_GL_ARB_fragment_layer_viewport;
GARCH_API extern bool GARCH_GL_ARB_fragment_program;
GARCH_API extern bool GARCH_GL_ARB_fragment_program_shadow;
GARCH_API extern bool GARCH_GL_ARB_fragment_shader;
GARCH_API extern bool GARCH_GL_ARB_fragment_shader_interlock;
GARCH_API extern bool GARCH_GL_ARB_framebuffer_no_attachments;
GARCH_API extern bool GARCH_GL_ARB_framebuffer_object;
GARCH_API extern bool GARCH_GL_ARB_framebuffer_sRGB;
GARCH_API extern bool GARCH_GL_ARB_geometry_shader4;
GARCH_API extern bool GARCH_GL_ARB_get_program_binary;
GARCH_API extern bool GARCH_GL_ARB_get_texture_sub_image;
GARCH_API extern bool GARCH_GL_ARB_gl_spirv;
GARCH_API extern bool GARCH_GL_ARB_gpu_shader5;
GARCH_API extern bool GARCH_GL_ARB_gpu_shader_fp64;
GARCH_API extern bool GARCH_GL_ARB_gpu_shader_int64;
GARCH_API extern bool GARCH_GL_ARB_half_float_pixel;
GARCH_API extern bool GARCH_GL_ARB_half_float_vertex;
GARCH_API extern bool GARCH_GL_ARB_imaging;
GARCH_API extern bool GARCH_GL_ARB_indirect_parameters;
GARCH_API extern bool GARCH_GL_ARB_instanced_arrays;
GARCH_API extern bool GARCH_GL_ARB_internalformat_query;
GARCH_API extern bool GARCH_GL_ARB_internalformat_query2;
GARCH_API extern bool GARCH_GL_ARB_invalidate_subdata;
GARCH_API extern bool GARCH_GL_ARB_map_buffer_alignment;
GARCH_API extern bool GARCH_GL_ARB_map_buffer_range;
GARCH_API extern bool GARCH_GL_ARB_matrix_palette;
GARCH_API extern bool GARCH_GL_ARB_multi_bind;
GARCH_API extern bool GARCH_GL_ARB_multi_draw_indirect;
GARCH_API extern bool GARCH_GL_ARB_multisample;
GARCH_API extern bool GARCH_GL_ARB_multitexture;
GARCH_API extern bool GARCH_GL_ARB_occlusion_query;
GARCH_API extern bool GARCH_GL_ARB_occlusion_query2;
GARCH_API extern bool GARCH_GL_ARB_parallel_shader_compile;
GARCH_API extern bool GARCH_GL_ARB_pipeline_statistics_query;
GARCH_API extern bool GARCH_GL_ARB_pixel_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_point_parameters;
GARCH_API extern bool GARCH_GL_ARB_point_sprite;
GARCH_API extern bool GARCH_GL_ARB_polygon_offset_clamp;
GARCH_API extern bool GARCH_GL_ARB_post_depth_coverage;
GARCH_API extern bool GARCH_GL_ARB_program_interface_query;
GARCH_API extern bool GARCH_GL_ARB_provoking_vertex;
GARCH_API extern bool GARCH_GL_ARB_query_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_robust_buffer_access_behavior;
GARCH_API extern bool GARCH_GL_ARB_robustness;
GARCH_API extern bool GARCH_GL_ARB_robustness_isolation;
GARCH_API extern bool GARCH_GL_ARB_sample_locations;
GARCH_API extern bool GARCH_GL_ARB_sample_shading;
GARCH_API extern bool GARCH_GL_ARB_sampler_objects;
GARCH_API extern bool GARCH_GL_ARB_seamless_cube_map;
GARCH_API extern bool GARCH_GL_ARB_seamless_cubemap_per_texture;
GARCH_API extern bool GARCH_GL_ARB_separate_shader_objects;
GARCH_API extern bool GARCH_GL_ARB_shader_atomic_counter_ops;
GARCH_API extern bool GARCH_GL_ARB_shader_atomic_counters;
GARCH_API extern bool GARCH_GL_ARB_shader_ballot;
GARCH_API extern bool GARCH_GL_ARB_shader_bit_encoding;
GARCH_API extern bool GARCH_GL_ARB_shader_clock;
GARCH_API extern bool GARCH_GL_ARB_shader_draw_parameters;
GARCH_API extern bool GARCH_GL_ARB_shader_group_vote;
GARCH_API extern bool GARCH_GL_ARB_shader_image_load_store;
GARCH_API extern bool GARCH_GL_ARB_shader_image_size;
GARCH_API extern bool GARCH_GL_ARB_shader_objects;
GARCH_API extern bool GARCH_GL_ARB_shader_precision;
GARCH_API extern bool GARCH_GL_ARB_shader_stencil_export;
GARCH_API extern bool GARCH_GL_ARB_shader_storage_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_shader_subroutine;
GARCH_API extern bool GARCH_GL_ARB_shader_texture_image_samples;
GARCH_API extern bool GARCH_GL_ARB_shader_texture_lod;
GARCH_API extern bool GARCH_GL_ARB_shader_viewport_layer_array;
GARCH_API extern bool GARCH_GL_ARB_shading_language_100;
GARCH_API extern bool GARCH_GL_ARB_shading_language_420pack;
GARCH_API extern bool GARCH_GL_ARB_shading_language_include;
GARCH_API extern bool GARCH_GL_ARB_shading_language_packing;
GARCH_API extern bool GARCH_GL_ARB_shadow;
GARCH_API extern bool GARCH_GL_ARB_shadow_ambient;
GARCH_API extern bool GARCH_GL_ARB_sparse_buffer;
GARCH_API extern bool GARCH_GL_ARB_sparse_texture;
GARCH_API extern bool GARCH_GL_ARB_sparse_texture2;
GARCH_API extern bool GARCH_GL_ARB_sparse_texture_clamp;
GARCH_API extern bool GARCH_GL_ARB_spirv_extensions;
GARCH_API extern bool GARCH_GL_ARB_stencil_texturing;
GARCH_API extern bool GARCH_GL_ARB_sync;
GARCH_API extern bool GARCH_GL_ARB_tessellation_shader;
GARCH_API extern bool GARCH_GL_ARB_texture_barrier;
GARCH_API extern bool GARCH_GL_ARB_texture_border_clamp;
GARCH_API extern bool GARCH_GL_ARB_texture_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_texture_buffer_object_rgb32;
GARCH_API extern bool GARCH_GL_ARB_texture_buffer_range;
GARCH_API extern bool GARCH_GL_ARB_texture_compression;
GARCH_API extern bool GARCH_GL_ARB_texture_compression_bptc;
GARCH_API extern bool GARCH_GL_ARB_texture_compression_rgtc;
GARCH_API extern bool GARCH_GL_ARB_texture_cube_map;
GARCH_API extern bool GARCH_GL_ARB_texture_cube_map_array;
GARCH_API extern bool GARCH_GL_ARB_texture_env_add;
GARCH_API extern bool GARCH_GL_ARB_texture_env_combine;
GARCH_API extern bool GARCH_GL_ARB_texture_env_crossbar;
GARCH_API extern bool GARCH_GL_ARB_texture_env_dot3;
GARCH_API extern bool GARCH_GL_ARB_texture_filter_anisotropic;
GARCH_API extern bool GARCH_GL_ARB_texture_filter_minmax;
GARCH_API extern bool GARCH_GL_ARB_texture_float;
GARCH_API extern bool GARCH_GL_ARB_texture_gather;
GARCH_API extern bool GARCH_GL_ARB_texture_mirror_clamp_to_edge;
GARCH_API extern bool GARCH_GL_ARB_texture_mirrored_repeat;
GARCH_API extern bool GARCH_GL_ARB_texture_multisample;
GARCH_API extern bool GARCH_GL_ARB_texture_non_power_of_two;
GARCH_API extern bool GARCH_GL_ARB_texture_query_levels;
GARCH_API extern bool GARCH_GL_ARB_texture_query_lod;
GARCH_API extern bool GARCH_GL_ARB_texture_rectangle;
GARCH_API extern bool GARCH_GL_ARB_texture_rg;
GARCH_API extern bool GARCH_GL_ARB_texture_rgb10_a2ui;
GARCH_API extern bool GARCH_GL_ARB_texture_stencil8;
GARCH_API extern bool GARCH_GL_ARB_texture_storage;
GARCH_API extern bool GARCH_GL_ARB_texture_storage_multisample;
GARCH_API extern bool GARCH_GL_ARB_texture_swizzle;
GARCH_API extern bool GARCH_GL_ARB_texture_view;
GARCH_API extern bool GARCH_GL_ARB_timer_query;
GARCH_API extern bool GARCH_GL_ARB_transform_feedback2;
GARCH_API extern bool GARCH_GL_ARB_transform_feedback3;
GARCH_API extern bool GARCH_GL_ARB_transform_feedback_instanced;
GARCH_API extern bool GARCH_GL_ARB_transform_feedback_overflow_query;
GARCH_API extern bool GARCH_GL_ARB_transpose_matrix;
GARCH_API extern bool GARCH_GL_ARB_uniform_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_vertex_array_bgra;
GARCH_API extern bool GARCH_GL_ARB_vertex_array_object;
GARCH_API extern bool GARCH_GL_ARB_vertex_attrib_64bit;
GARCH_API extern bool GARCH_GL_ARB_vertex_attrib_binding;
GARCH_API extern bool GARCH_GL_ARB_vertex_blend;
GARCH_API extern bool GARCH_GL_ARB_vertex_buffer_object;
GARCH_API extern bool GARCH_GL_ARB_vertex_program;
GARCH_API extern bool GARCH_GL_ARB_vertex_shader;
GARCH_API extern bool GARCH_GL_ARB_vertex_type_10f_11f_11f_rev;
GARCH_API extern bool GARCH_GL_ARB_vertex_type_2_10_10_10_rev;
GARCH_API extern bool GARCH_GL_ARB_viewport_array;
GARCH_API extern bool GARCH_GL_ARB_window_pos;
GARCH_API extern bool GARCH_GL_EXT_422_pixels;
GARCH_API extern bool GARCH_GL_EXT_EGL_image_storage;
GARCH_API extern bool GARCH_GL_EXT_EGL_sync;
GARCH_API extern bool GARCH_GL_EXT_abgr;
GARCH_API extern bool GARCH_GL_EXT_bgra;
GARCH_API extern bool GARCH_GL_EXT_bindable_uniform;
GARCH_API extern bool GARCH_GL_EXT_blend_color;
GARCH_API extern bool GARCH_GL_EXT_blend_equation_separate;
GARCH_API extern bool GARCH_GL_EXT_blend_func_separate;
GARCH_API extern bool GARCH_GL_EXT_blend_logic_op;
GARCH_API extern bool GARCH_GL_EXT_blend_minmax;
GARCH_API extern bool GARCH_GL_EXT_blend_subtract;
GARCH_API extern bool GARCH_GL_EXT_clip_volume_hint;
GARCH_API extern bool GARCH_GL_EXT_cmyka;
GARCH_API extern bool GARCH_GL_EXT_color_subtable;
GARCH_API extern bool GARCH_GL_EXT_compiled_vertex_array;
GARCH_API extern bool GARCH_GL_EXT_convolution;
GARCH_API extern bool GARCH_GL_EXT_coordinate_frame;
GARCH_API extern bool GARCH_GL_EXT_copy_texture;
GARCH_API extern bool GARCH_GL_EXT_cull_vertex;
GARCH_API extern bool GARCH_GL_EXT_debug_label;
GARCH_API extern bool GARCH_GL_EXT_debug_marker;
GARCH_API extern bool GARCH_GL_EXT_depth_bounds_test;
GARCH_API extern bool GARCH_GL_EXT_direct_state_access;
GARCH_API extern bool GARCH_GL_EXT_draw_buffers2;
GARCH_API extern bool GARCH_GL_EXT_draw_instanced;
GARCH_API extern bool GARCH_GL_EXT_draw_range_elements;
GARCH_API extern bool GARCH_GL_EXT_external_buffer;
GARCH_API extern bool GARCH_GL_EXT_fog_coord;
GARCH_API extern bool GARCH_GL_EXT_framebuffer_blit;
GARCH_API extern bool GARCH_GL_EXT_framebuffer_multisample;
GARCH_API extern bool GARCH_GL_EXT_framebuffer_multisample_blit_scaled;
GARCH_API extern bool GARCH_GL_EXT_framebuffer_object;
GARCH_API extern bool GARCH_GL_EXT_framebuffer_sRGB;
GARCH_API extern bool GARCH_GL_EXT_geometry_shader4;
GARCH_API extern bool GARCH_GL_EXT_gpu_program_parameters;
GARCH_API extern bool GARCH_GL_EXT_gpu_shader4;
GARCH_API extern bool GARCH_GL_EXT_histogram;
GARCH_API extern bool GARCH_GL_EXT_index_array_formats;
GARCH_API extern bool GARCH_GL_EXT_index_func;
GARCH_API extern bool GARCH_GL_EXT_index_material;
GARCH_API extern bool GARCH_GL_EXT_index_texture;
GARCH_API extern bool GARCH_GL_EXT_light_texture;
GARCH_API extern bool GARCH_GL_EXT_memory_object;
GARCH_API extern bool GARCH_GL_EXT_memory_object_fd;
GARCH_API extern bool GARCH_GL_EXT_memory_object_win32;
GARCH_API extern bool GARCH_GL_EXT_misc_attribute;
GARCH_API extern bool GARCH_GL_EXT_multi_draw_arrays;
GARCH_API extern bool GARCH_GL_EXT_multisample;
GARCH_API extern bool GARCH_GL_EXT_multiview_tessellation_geometry_shader;
GARCH_API extern bool GARCH_GL_EXT_multiview_texture_multisample;
GARCH_API extern bool GARCH_GL_EXT_multiview_timer_query;
GARCH_API extern bool GARCH_GL_EXT_packed_depth_stencil;
GARCH_API extern bool GARCH_GL_EXT_packed_float;
GARCH_API extern bool GARCH_GL_EXT_packed_pixels;
GARCH_API extern bool GARCH_GL_EXT_paletted_texture;
GARCH_API extern bool GARCH_GL_EXT_pixel_buffer_object;
GARCH_API extern bool GARCH_GL_EXT_pixel_transform;
GARCH_API extern bool GARCH_GL_EXT_pixel_transform_color_table;
GARCH_API extern bool GARCH_GL_EXT_point_parameters;
GARCH_API extern bool GARCH_GL_EXT_polygon_offset;
GARCH_API extern bool GARCH_GL_EXT_polygon_offset_clamp;
GARCH_API extern bool GARCH_GL_EXT_post_depth_coverage;
GARCH_API extern bool GARCH_GL_EXT_provoking_vertex;
GARCH_API extern bool GARCH_GL_EXT_raster_multisample;
GARCH_API extern bool GARCH_GL_EXT_rescale_normal;
GARCH_API extern bool GARCH_GL_EXT_semaphore;
GARCH_API extern bool GARCH_GL_EXT_semaphore_fd;
GARCH_API extern bool GARCH_GL_EXT_semaphore_win32;
GARCH_API extern bool GARCH_GL_EXT_secondary_color;
GARCH_API extern bool GARCH_GL_EXT_separate_shader_objects;
GARCH_API extern bool GARCH_GL_EXT_separate_specular_color;
GARCH_API extern bool GARCH_GL_EXT_shader_framebuffer_fetch;
GARCH_API extern bool GARCH_GL_EXT_shader_framebuffer_fetch_non_coherent;
GARCH_API extern bool GARCH_GL_EXT_shader_image_load_formatted;
GARCH_API extern bool GARCH_GL_EXT_shader_image_load_store;
GARCH_API extern bool GARCH_GL_EXT_shader_integer_mix;
GARCH_API extern bool GARCH_GL_EXT_shadow_funcs;
GARCH_API extern bool GARCH_GL_EXT_shared_texture_palette;
GARCH_API extern bool GARCH_GL_EXT_sparse_texture2;
GARCH_API extern bool GARCH_GL_EXT_stencil_clear_tag;
GARCH_API extern bool GARCH_GL_EXT_stencil_two_side;
GARCH_API extern bool GARCH_GL_EXT_stencil_wrap;
GARCH_API extern bool GARCH_GL_EXT_subtexture;
GARCH_API extern bool GARCH_GL_EXT_texture;
GARCH_API extern bool GARCH_GL_EXT_texture3D;
GARCH_API extern bool GARCH_GL_EXT_texture_array;
GARCH_API extern bool GARCH_GL_EXT_texture_buffer_object;
GARCH_API extern bool GARCH_GL_EXT_texture_compression_latc;
GARCH_API extern bool GARCH_GL_EXT_texture_compression_rgtc;
GARCH_API extern bool GARCH_GL_EXT_texture_compression_s3tc;
GARCH_API extern bool GARCH_GL_EXT_texture_cube_map;
GARCH_API extern bool GARCH_GL_EXT_texture_env_add;
GARCH_API extern bool GARCH_GL_EXT_texture_env_combine;
GARCH_API extern bool GARCH_GL_EXT_texture_env_dot3;
GARCH_API extern bool GARCH_GL_EXT_texture_filter_anisotropic;
GARCH_API extern bool GARCH_GL_EXT_texture_filter_minmax;
GARCH_API extern bool GARCH_GL_EXT_texture_integer;
GARCH_API extern bool GARCH_GL_EXT_texture_lod_bias;
GARCH_API extern bool GARCH_GL_EXT_texture_mirror_clamp;
GARCH_API extern bool GARCH_GL_EXT_texture_object;
GARCH_API extern bool GARCH_GL_EXT_texture_perturb_normal;
GARCH_API extern bool GARCH_GL_EXT_texture_sRGB;
GARCH_API extern bool GARCH_GL_EXT_texture_sRGB_R8;
GARCH_API extern bool GARCH_GL_EXT_texture_sRGB_decode;
GARCH_API extern bool GARCH_GL_EXT_texture_shared_exponent;
GARCH_API extern bool GARCH_GL_EXT_texture_snorm;
GARCH_API extern bool GARCH_GL_EXT_texture_swizzle;
GARCH_API extern bool GARCH_GL_NV_timeline_semaphore;
GARCH_API extern bool GARCH_GL_EXT_timer_query;
GARCH_API extern bool GARCH_GL_EXT_transform_feedback;
GARCH_API extern bool GARCH_GL_EXT_vertex_array;
GARCH_API extern bool GARCH_GL_EXT_vertex_array_bgra;
GARCH_API extern bool GARCH_GL_EXT_vertex_attrib_64bit;
GARCH_API extern bool GARCH_GL_EXT_vertex_shader;
GARCH_API extern bool GARCH_GL_EXT_vertex_weighting;
GARCH_API extern bool GARCH_GL_EXT_win32_keyed_mutex;
GARCH_API extern bool GARCH_GL_EXT_window_rectangles;
GARCH_API extern bool GARCH_GL_EXT_x11_sync_object;
GARCH_API extern bool GARCH_GL_INTEL_conservative_rasterization;
GARCH_API extern bool GARCH_GL_INTEL_fragment_shader_ordering;
GARCH_API extern bool GARCH_GL_INTEL_framebuffer_CMAA;
GARCH_API extern bool GARCH_GL_INTEL_map_texture;
GARCH_API extern bool GARCH_GL_INTEL_blackhole_render;
GARCH_API extern bool GARCH_GL_INTEL_parallel_arrays;
GARCH_API extern bool GARCH_GL_INTEL_performance_query;
GARCH_API extern bool GARCH_GL_KHR_blend_equation_advanced;
GARCH_API extern bool GARCH_GL_KHR_blend_equation_advanced_coherent;
GARCH_API extern bool GARCH_GL_KHR_context_flush_control;
GARCH_API extern bool GARCH_GL_KHR_debug;
GARCH_API extern bool GARCH_GL_KHR_no_error;
GARCH_API extern bool GARCH_GL_KHR_robust_buffer_access_behavior;
GARCH_API extern bool GARCH_GL_KHR_robustness;
GARCH_API extern bool GARCH_GL_KHR_shader_subgroup;
GARCH_API extern bool GARCH_GL_KHR_texture_compression_astc_hdr;
GARCH_API extern bool GARCH_GL_KHR_texture_compression_astc_ldr;
GARCH_API extern bool GARCH_GL_KHR_texture_compression_astc_sliced_3d;
GARCH_API extern bool GARCH_GL_KHR_parallel_shader_compile;
GARCH_API extern bool GARCH_GL_NVX_blend_equation_advanced_multi_draw_buffers;
GARCH_API extern bool GARCH_GL_NVX_conditional_render;
GARCH_API extern bool GARCH_GL_NVX_gpu_memory_info;
GARCH_API extern bool GARCH_GL_NVX_linked_gpu_multicast;
GARCH_API extern bool GARCH_GL_NV_alpha_to_coverage_dither_control;
GARCH_API extern bool GARCH_GL_NV_bindless_multi_draw_indirect;
GARCH_API extern bool GARCH_GL_NV_bindless_multi_draw_indirect_count;
GARCH_API extern bool GARCH_GL_NV_bindless_texture;
GARCH_API extern bool GARCH_GL_NV_blend_equation_advanced;
GARCH_API extern bool GARCH_GL_NV_blend_equation_advanced_coherent;
GARCH_API extern bool GARCH_GL_NV_blend_minmax_factor;
GARCH_API extern bool GARCH_GL_NV_blend_square;
GARCH_API extern bool GARCH_GL_NV_clip_space_w_scaling;
GARCH_API extern bool GARCH_GL_NV_command_list;
GARCH_API extern bool GARCH_GL_NV_compute_program5;
GARCH_API extern bool GARCH_GL_NV_compute_shader_derivatives;
GARCH_API extern bool GARCH_GL_NV_conditional_render;
GARCH_API extern bool GARCH_GL_NV_conservative_raster;
GARCH_API extern bool GARCH_GL_NV_conservative_raster_dilate;
GARCH_API extern bool GARCH_GL_NV_conservative_raster_pre_snap;
GARCH_API extern bool GARCH_GL_NV_conservative_raster_pre_snap_triangles;
GARCH_API extern bool GARCH_GL_NV_conservative_raster_underestimation;
GARCH_API extern bool GARCH_GL_NV_copy_depth_to_color;
GARCH_API extern bool GARCH_GL_NV_copy_image;
GARCH_API extern bool GARCH_GL_NV_deep_texture3D;
GARCH_API extern bool GARCH_GL_NV_depth_buffer_float;
GARCH_API extern bool GARCH_GL_NV_depth_clamp;
GARCH_API extern bool GARCH_GL_NV_draw_texture;
GARCH_API extern bool GARCH_GL_NV_draw_vulkan_image;
GARCH_API extern bool GARCH_GL_NV_evaluators;
GARCH_API extern bool GARCH_GL_NV_explicit_multisample;
GARCH_API extern bool GARCH_GL_NV_fence;
GARCH_API extern bool GARCH_GL_NV_fill_rectangle;
GARCH_API extern bool GARCH_GL_NV_float_buffer;
GARCH_API extern bool GARCH_GL_NV_fog_distance;
GARCH_API extern bool GARCH_GL_NV_fragment_coverage_to_color;
GARCH_API extern bool GARCH_GL_NV_fragment_program;
GARCH_API extern bool GARCH_GL_NV_fragment_program2;
GARCH_API extern bool GARCH_GL_NV_fragment_program4;
GARCH_API extern bool GARCH_GL_NV_fragment_program_option;
GARCH_API extern bool GARCH_GL_NV_fragment_shader_barycentric;
GARCH_API extern bool GARCH_GL_NV_fragment_shader_interlock;
GARCH_API extern bool GARCH_GL_NV_framebuffer_mixed_samples;
GARCH_API extern bool GARCH_GL_NV_framebuffer_multisample_coverage;
GARCH_API extern bool GARCH_GL_NV_geometry_program4;
GARCH_API extern bool GARCH_GL_NV_geometry_shader4;
GARCH_API extern bool GARCH_GL_NV_geometry_shader_passthrough;
GARCH_API extern bool GARCH_GL_NV_gpu_program4;
GARCH_API extern bool GARCH_GL_NV_gpu_program5;
GARCH_API extern bool GARCH_GL_NV_gpu_program5_mem_extended;
GARCH_API extern bool GARCH_GL_NV_gpu_shader5;
GARCH_API extern bool GARCH_GL_NV_half_float;
GARCH_API extern bool GARCH_GL_NV_internalformat_sample_query;
GARCH_API extern bool GARCH_GL_NV_light_max_exponent;
GARCH_API extern bool GARCH_GL_NV_gpu_multicast;
GARCH_API extern bool GARCH_GL_NVX_gpu_multicast2;
GARCH_API extern bool GARCH_GL_NVX_progress_fence;
GARCH_API extern bool GARCH_GL_NV_memory_attachment;
GARCH_API extern bool GARCH_GL_NV_memory_object_sparse;
GARCH_API extern bool GARCH_GL_NV_mesh_shader;
GARCH_API extern bool GARCH_GL_NV_multisample_coverage;
GARCH_API extern bool GARCH_GL_NV_multisample_filter_hint;
GARCH_API extern bool GARCH_GL_NV_occlusion_query;
GARCH_API extern bool GARCH_GL_NV_packed_depth_stencil;
GARCH_API extern bool GARCH_GL_NV_parameter_buffer_object;
GARCH_API extern bool GARCH_GL_NV_parameter_buffer_object2;
GARCH_API extern bool GARCH_GL_NV_path_rendering;
GARCH_API extern bool GARCH_GL_NV_path_rendering_shared_edge;
GARCH_API extern bool GARCH_GL_NV_pixel_data_range;
GARCH_API extern bool GARCH_GL_NV_point_sprite;
GARCH_API extern bool GARCH_GL_NV_present_video;
GARCH_API extern bool GARCH_GL_NV_primitive_restart;
GARCH_API extern bool GARCH_GL_NV_query_resource;
GARCH_API extern bool GARCH_GL_NV_query_resource_tag;
GARCH_API extern bool GARCH_GL_NV_register_combiners;
GARCH_API extern bool GARCH_GL_NV_register_combiners2;
GARCH_API extern bool GARCH_GL_NV_representative_fragment_test;
GARCH_API extern bool GARCH_GL_NV_robustness_video_memory_purge;
GARCH_API extern bool GARCH_GL_NV_sample_locations;
GARCH_API extern bool GARCH_GL_NV_sample_mask_override_coverage;
GARCH_API extern bool GARCH_GL_NV_scissor_exclusive;
GARCH_API extern bool GARCH_GL_NV_shader_atomic_counters;
GARCH_API extern bool GARCH_GL_NV_shader_atomic_float;
GARCH_API extern bool GARCH_GL_NV_shader_atomic_float64;
GARCH_API extern bool GARCH_GL_NV_shader_atomic_fp16_vector;
GARCH_API extern bool GARCH_GL_NV_shader_atomic_int64;
GARCH_API extern bool GARCH_GL_NV_shader_buffer_load;
GARCH_API extern bool GARCH_GL_NV_shader_buffer_store;
GARCH_API extern bool GARCH_GL_NV_shader_storage_buffer_object;
GARCH_API extern bool GARCH_GL_NV_shader_subgroup_partitioned;
GARCH_API extern bool GARCH_GL_NV_shader_texture_footprint;
GARCH_API extern bool GARCH_GL_NV_shader_thread_group;
GARCH_API extern bool GARCH_GL_NV_shader_thread_shuffle;
GARCH_API extern bool GARCH_GL_NV_shading_rate_image;
GARCH_API extern bool GARCH_GL_NV_stereo_view_rendering;
GARCH_API extern bool GARCH_GL_NV_tessellation_program5;
GARCH_API extern bool GARCH_GL_NV_texgen_emboss;
GARCH_API extern bool GARCH_GL_NV_texgen_reflection;
GARCH_API extern bool GARCH_GL_NV_texture_barrier;
GARCH_API extern bool GARCH_GL_NV_texture_compression_vtc;
GARCH_API extern bool GARCH_GL_NV_texture_env_combine4;
GARCH_API extern bool GARCH_GL_NV_texture_expand_normal;
GARCH_API extern bool GARCH_GL_NV_texture_multisample;
GARCH_API extern bool GARCH_GL_NV_texture_rectangle;
GARCH_API extern bool GARCH_GL_NV_texture_rectangle_compressed;
GARCH_API extern bool GARCH_GL_NV_texture_shader;
GARCH_API extern bool GARCH_GL_NV_texture_shader2;
GARCH_API extern bool GARCH_GL_NV_texture_shader3;
GARCH_API extern bool GARCH_GL_NV_transform_feedback;
GARCH_API extern bool GARCH_GL_NV_transform_feedback2;
GARCH_API extern bool GARCH_GL_NV_uniform_buffer_unified_memory;
GARCH_API extern bool GARCH_GL_NV_vdpau_interop;
GARCH_API extern bool GARCH_GL_NV_vdpau_interop2;
GARCH_API extern bool GARCH_GL_NV_vertex_array_range;
GARCH_API extern bool GARCH_GL_NV_vertex_array_range2;
GARCH_API extern bool GARCH_GL_NV_vertex_attrib_integer_64bit;
GARCH_API extern bool GARCH_GL_NV_vertex_buffer_unified_memory;
GARCH_API extern bool GARCH_GL_NV_vertex_program;
GARCH_API extern bool GARCH_GL_NV_vertex_program1_1;
GARCH_API extern bool GARCH_GL_NV_vertex_program2;
GARCH_API extern bool GARCH_GL_NV_vertex_program2_option;
GARCH_API extern bool GARCH_GL_NV_vertex_program3;
GARCH_API extern bool GARCH_GL_NV_vertex_program4;
GARCH_API extern bool GARCH_GL_NV_video_capture;
GARCH_API extern bool GARCH_GL_NV_viewport_array2;
GARCH_API extern bool GARCH_GL_NV_viewport_swizzle;
GARCH_API extern bool GARCH_GL_EXT_texture_shadow_lod;


GARCH_API extern PFNGLACCUMPROC glAccum;
GARCH_API extern PFNGLACTIVEPROGRAMEXTPROC glActiveProgramEXT;
GARCH_API extern PFNGLACTIVESHADERPROGRAMPROC glActiveShaderProgram;
GARCH_API extern PFNGLACTIVESHADERPROGRAMEXTPROC glActiveShaderProgramEXT;
GARCH_API extern PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT;
GARCH_API extern PFNGLACTIVETEXTUREPROC glActiveTexture;
GARCH_API extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
GARCH_API extern PFNGLACTIVEVARYINGNVPROC glActiveVaryingNV;
GARCH_API extern PFNGLALPHAFUNCPROC glAlphaFunc;
GARCH_API extern PFNGLALPHATOCOVERAGEDITHERCONTROLNVPROC glAlphaToCoverageDitherControlNV;
GARCH_API extern PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC glApplyFramebufferAttachmentCMAAINTEL;
GARCH_API extern PFNGLAPPLYTEXTUREEXTPROC glApplyTextureEXT;
GARCH_API extern PFNGLACQUIREKEYEDMUTEXWIN32EXTPROC glAcquireKeyedMutexWin32EXT;
GARCH_API extern PFNGLAREPROGRAMSRESIDENTNVPROC glAreProgramsResidentNV;
GARCH_API extern PFNGLARETEXTURESRESIDENTPROC glAreTexturesResident;
GARCH_API extern PFNGLARETEXTURESRESIDENTEXTPROC glAreTexturesResidentEXT;
GARCH_API extern PFNGLARRAYELEMENTPROC glArrayElement;
GARCH_API extern PFNGLARRAYELEMENTEXTPROC glArrayElementEXT;
GARCH_API extern PFNGLASYNCCOPYBUFFERSUBDATANVXPROC glAsyncCopyBufferSubDataNVX;
GARCH_API extern PFNGLASYNCCOPYIMAGESUBDATANVXPROC glAsyncCopyImageSubDataNVX;
GARCH_API extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
GARCH_API extern PFNGLATTACHSHADERPROC glAttachShader;
GARCH_API extern PFNGLBEGINPROC glBegin;
GARCH_API extern PFNGLBEGINCONDITIONALRENDERPROC glBeginConditionalRender;
GARCH_API extern PFNGLBEGINCONDITIONALRENDERNVPROC glBeginConditionalRenderNV;
GARCH_API extern PFNGLBEGINCONDITIONALRENDERNVXPROC glBeginConditionalRenderNVX;
GARCH_API extern PFNGLBEGINOCCLUSIONQUERYNVPROC glBeginOcclusionQueryNV;
GARCH_API extern PFNGLBEGINPERFMONITORAMDPROC glBeginPerfMonitorAMD;
GARCH_API extern PFNGLBEGINPERFQUERYINTELPROC glBeginPerfQueryINTEL;
GARCH_API extern PFNGLBEGINQUERYPROC glBeginQuery;
GARCH_API extern PFNGLBEGINQUERYARBPROC glBeginQueryARB;
GARCH_API extern PFNGLBEGINQUERYINDEXEDPROC glBeginQueryIndexed;
GARCH_API extern PFNGLBEGINTRANSFORMFEEDBACKPROC glBeginTransformFeedback;
GARCH_API extern PFNGLBEGINTRANSFORMFEEDBACKEXTPROC glBeginTransformFeedbackEXT;
GARCH_API extern PFNGLBEGINTRANSFORMFEEDBACKNVPROC glBeginTransformFeedbackNV;
GARCH_API extern PFNGLBEGINVERTEXSHADEREXTPROC glBeginVertexShaderEXT;
GARCH_API extern PFNGLBEGINVIDEOCAPTURENVPROC glBeginVideoCaptureNV;
GARCH_API extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
GARCH_API extern PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
GARCH_API extern PFNGLBINDBUFFERPROC glBindBuffer;
GARCH_API extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
GARCH_API extern PFNGLBINDBUFFERBASEPROC glBindBufferBase;
GARCH_API extern PFNGLBINDBUFFERBASEEXTPROC glBindBufferBaseEXT;
GARCH_API extern PFNGLBINDBUFFERBASENVPROC glBindBufferBaseNV;
GARCH_API extern PFNGLBINDBUFFEROFFSETEXTPROC glBindBufferOffsetEXT;
GARCH_API extern PFNGLBINDBUFFEROFFSETNVPROC glBindBufferOffsetNV;
GARCH_API extern PFNGLBINDBUFFERRANGEPROC glBindBufferRange;
GARCH_API extern PFNGLBINDBUFFERRANGEEXTPROC glBindBufferRangeEXT;
GARCH_API extern PFNGLBINDBUFFERRANGENVPROC glBindBufferRangeNV;
GARCH_API extern PFNGLBINDBUFFERSBASEPROC glBindBuffersBase;
GARCH_API extern PFNGLBINDBUFFERSRANGEPROC glBindBuffersRange;
GARCH_API extern PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
GARCH_API extern PFNGLBINDFRAGDATALOCATIONEXTPROC glBindFragDataLocationEXT;
GARCH_API extern PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glBindFragDataLocationIndexed;
GARCH_API extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
GARCH_API extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
GARCH_API extern PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;
GARCH_API extern PFNGLBINDIMAGETEXTUREEXTPROC glBindImageTextureEXT;
GARCH_API extern PFNGLBINDIMAGETEXTURESPROC glBindImageTextures;
GARCH_API extern PFNGLBINDLIGHTPARAMETEREXTPROC glBindLightParameterEXT;
GARCH_API extern PFNGLBINDMATERIALPARAMETEREXTPROC glBindMaterialParameterEXT;
GARCH_API extern PFNGLBINDMULTITEXTUREEXTPROC glBindMultiTextureEXT;
GARCH_API extern PFNGLBINDPARAMETEREXTPROC glBindParameterEXT;
GARCH_API extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
GARCH_API extern PFNGLBINDPROGRAMNVPROC glBindProgramNV;
GARCH_API extern PFNGLBINDPROGRAMPIPELINEPROC glBindProgramPipeline;
GARCH_API extern PFNGLBINDPROGRAMPIPELINEEXTPROC glBindProgramPipelineEXT;
GARCH_API extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
GARCH_API extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
GARCH_API extern PFNGLBINDSAMPLERPROC glBindSampler;
GARCH_API extern PFNGLBINDSAMPLERSPROC glBindSamplers;
GARCH_API extern PFNGLBINDSHADINGRATEIMAGENVPROC glBindShadingRateImageNV;
GARCH_API extern PFNGLBINDTEXGENPARAMETEREXTPROC glBindTexGenParameterEXT;
GARCH_API extern PFNGLBINDTEXTUREPROC glBindTexture;
GARCH_API extern PFNGLBINDTEXTUREEXTPROC glBindTextureEXT;
GARCH_API extern PFNGLBINDTEXTUREUNITPROC glBindTextureUnit;
GARCH_API extern PFNGLBINDTEXTUREUNITPARAMETEREXTPROC glBindTextureUnitParameterEXT;
GARCH_API extern PFNGLBINDTEXTURESPROC glBindTextures;
GARCH_API extern PFNGLBINDTRANSFORMFEEDBACKPROC glBindTransformFeedback;
GARCH_API extern PFNGLBINDTRANSFORMFEEDBACKNVPROC glBindTransformFeedbackNV;
GARCH_API extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
GARCH_API extern PFNGLBINDVERTEXARRAYAPPLEPROC glBindVertexArrayAPPLE;
GARCH_API extern PFNGLBINDVERTEXBUFFERPROC glBindVertexBuffer;
GARCH_API extern PFNGLBINDVERTEXBUFFERSPROC glBindVertexBuffers;
GARCH_API extern PFNGLBINDVERTEXSHADEREXTPROC glBindVertexShaderEXT;
GARCH_API extern PFNGLBINDVIDEOCAPTURESTREAMBUFFERNVPROC glBindVideoCaptureStreamBufferNV;
GARCH_API extern PFNGLBINDVIDEOCAPTURESTREAMTEXTURENVPROC glBindVideoCaptureStreamTextureNV;
GARCH_API extern PFNGLBINORMAL3BEXTPROC glBinormal3bEXT;
GARCH_API extern PFNGLBINORMAL3BVEXTPROC glBinormal3bvEXT;
GARCH_API extern PFNGLBINORMAL3DEXTPROC glBinormal3dEXT;
GARCH_API extern PFNGLBINORMAL3DVEXTPROC glBinormal3dvEXT;
GARCH_API extern PFNGLBINORMAL3FEXTPROC glBinormal3fEXT;
GARCH_API extern PFNGLBINORMAL3FVEXTPROC glBinormal3fvEXT;
GARCH_API extern PFNGLBINORMAL3IEXTPROC glBinormal3iEXT;
GARCH_API extern PFNGLBINORMAL3IVEXTPROC glBinormal3ivEXT;
GARCH_API extern PFNGLBINORMAL3SEXTPROC glBinormal3sEXT;
GARCH_API extern PFNGLBINORMAL3SVEXTPROC glBinormal3svEXT;
GARCH_API extern PFNGLBINORMALPOINTEREXTPROC glBinormalPointerEXT;
GARCH_API extern PFNGLBITMAPPROC glBitmap;
GARCH_API extern PFNGLBLENDBARRIERKHRPROC glBlendBarrierKHR;
GARCH_API extern PFNGLBLENDBARRIERNVPROC glBlendBarrierNV;
GARCH_API extern PFNGLBLENDCOLORPROC glBlendColor;
GARCH_API extern PFNGLBLENDCOLOREXTPROC glBlendColorEXT;
GARCH_API extern PFNGLBLENDEQUATIONPROC glBlendEquation;
GARCH_API extern PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;
GARCH_API extern PFNGLBLENDEQUATIONINDEXEDAMDPROC glBlendEquationIndexedAMD;
GARCH_API extern PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate;
GARCH_API extern PFNGLBLENDEQUATIONSEPARATEEXTPROC glBlendEquationSeparateEXT;
GARCH_API extern PFNGLBLENDEQUATIONSEPARATEINDEXEDAMDPROC glBlendEquationSeparateIndexedAMD;
GARCH_API extern PFNGLBLENDEQUATIONSEPARATEIPROC glBlendEquationSeparatei;
GARCH_API extern PFNGLBLENDEQUATIONSEPARATEIARBPROC glBlendEquationSeparateiARB;
GARCH_API extern PFNGLBLENDEQUATIONIPROC glBlendEquationi;
GARCH_API extern PFNGLBLENDEQUATIONIARBPROC glBlendEquationiARB;
GARCH_API extern PFNGLBLENDFUNCPROC glBlendFunc;
GARCH_API extern PFNGLBLENDFUNCINDEXEDAMDPROC glBlendFuncIndexedAMD;
GARCH_API extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
GARCH_API extern PFNGLBLENDFUNCSEPARATEEXTPROC glBlendFuncSeparateEXT;
GARCH_API extern PFNGLBLENDFUNCSEPARATEINDEXEDAMDPROC glBlendFuncSeparateIndexedAMD;
GARCH_API extern PFNGLBLENDFUNCSEPARATEIPROC glBlendFuncSeparatei;
GARCH_API extern PFNGLBLENDFUNCSEPARATEIARBPROC glBlendFuncSeparateiARB;
GARCH_API extern PFNGLBLENDFUNCIPROC glBlendFunci;
GARCH_API extern PFNGLBLENDFUNCIARBPROC glBlendFunciARB;
GARCH_API extern PFNGLBLENDPARAMETERINVPROC glBlendParameteriNV;
GARCH_API extern PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
GARCH_API extern PFNGLBLITFRAMEBUFFEREXTPROC glBlitFramebufferEXT;
GARCH_API extern PFNGLBLITNAMEDFRAMEBUFFERPROC glBlitNamedFramebuffer;
GARCH_API extern PFNGLBUFFERADDRESSRANGENVPROC glBufferAddressRangeNV;
GARCH_API extern PFNGLBUFFERATTACHMEMORYNVPROC glBufferAttachMemoryNV;
GARCH_API extern PFNGLBUFFERDATAPROC glBufferData;
GARCH_API extern PFNGLBUFFERDATAARBPROC glBufferDataARB;
GARCH_API extern PFNGLBUFFERPAGECOMMITMENTARBPROC glBufferPageCommitmentARB;
GARCH_API extern PFNGLBUFFERPAGECOMMITMENTMEMNVPROC glBufferPageCommitmentMemNV;
GARCH_API extern PFNGLBUFFERPARAMETERIAPPLEPROC glBufferParameteriAPPLE;
GARCH_API extern PFNGLBUFFERSTORAGEPROC glBufferStorage;
GARCH_API extern PFNGLBUFFERSTORAGEEXTERNALEXTPROC glBufferStorageExternalEXT;
GARCH_API extern PFNGLBUFFERSTORAGEMEMEXTPROC glBufferStorageMemEXT;
GARCH_API extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
GARCH_API extern PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB;
GARCH_API extern PFNGLCALLCOMMANDLISTNVPROC glCallCommandListNV;
GARCH_API extern PFNGLCALLLISTPROC glCallList;
GARCH_API extern PFNGLCALLLISTSPROC glCallLists;
GARCH_API extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
GARCH_API extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
GARCH_API extern PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC glCheckNamedFramebufferStatus;
GARCH_API extern PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC glCheckNamedFramebufferStatusEXT;
GARCH_API extern PFNGLCLAMPCOLORPROC glClampColor;
GARCH_API extern PFNGLCLAMPCOLORARBPROC glClampColorARB;
GARCH_API extern PFNGLCLEARPROC glClear;
GARCH_API extern PFNGLCLEARACCUMPROC glClearAccum;
GARCH_API extern PFNGLCLEARBUFFERDATAPROC glClearBufferData;
GARCH_API extern PFNGLCLEARBUFFERSUBDATAPROC glClearBufferSubData;
GARCH_API extern PFNGLCLEARBUFFERFIPROC glClearBufferfi;
GARCH_API extern PFNGLCLEARBUFFERFVPROC glClearBufferfv;
GARCH_API extern PFNGLCLEARBUFFERIVPROC glClearBufferiv;
GARCH_API extern PFNGLCLEARBUFFERUIVPROC glClearBufferuiv;
GARCH_API extern PFNGLCLEARCOLORPROC glClearColor;
GARCH_API extern PFNGLCLEARCOLORIIEXTPROC glClearColorIiEXT;
GARCH_API extern PFNGLCLEARCOLORIUIEXTPROC glClearColorIuiEXT;
GARCH_API extern PFNGLCLEARDEPTHPROC glClearDepth;
GARCH_API extern PFNGLCLEARDEPTHDNVPROC glClearDepthdNV;
GARCH_API extern PFNGLCLEARDEPTHFPROC glClearDepthf;
GARCH_API extern PFNGLCLEARINDEXPROC glClearIndex;
GARCH_API extern PFNGLCLEARNAMEDBUFFERDATAPROC glClearNamedBufferData;
GARCH_API extern PFNGLCLEARNAMEDBUFFERDATAEXTPROC glClearNamedBufferDataEXT;
GARCH_API extern PFNGLCLEARNAMEDBUFFERSUBDATAPROC glClearNamedBufferSubData;
GARCH_API extern PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC glClearNamedBufferSubDataEXT;
GARCH_API extern PFNGLCLEARNAMEDFRAMEBUFFERFIPROC glClearNamedFramebufferfi;
GARCH_API extern PFNGLCLEARNAMEDFRAMEBUFFERFVPROC glClearNamedFramebufferfv;
GARCH_API extern PFNGLCLEARNAMEDFRAMEBUFFERIVPROC glClearNamedFramebufferiv;
GARCH_API extern PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC glClearNamedFramebufferuiv;
GARCH_API extern PFNGLCLEARSTENCILPROC glClearStencil;
GARCH_API extern PFNGLCLEARTEXIMAGEPROC glClearTexImage;
GARCH_API extern PFNGLCLEARTEXSUBIMAGEPROC glClearTexSubImage;
GARCH_API extern PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
GARCH_API extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
GARCH_API extern PFNGLCLIENTATTRIBDEFAULTEXTPROC glClientAttribDefaultEXT;
GARCH_API extern PFNGLCLIENTWAITSEMAPHOREUI64NVXPROC glClientWaitSemaphoreui64NVX;
GARCH_API extern PFNGLCLIENTWAITSYNCPROC glClientWaitSync;
GARCH_API extern PFNGLCLIPCONTROLPROC glClipControl;
GARCH_API extern PFNGLCLIPPLANEPROC glClipPlane;
GARCH_API extern PFNGLCOLOR3BPROC glColor3b;
GARCH_API extern PFNGLCOLOR3BVPROC glColor3bv;
GARCH_API extern PFNGLCOLOR3DPROC glColor3d;
GARCH_API extern PFNGLCOLOR3DVPROC glColor3dv;
GARCH_API extern PFNGLCOLOR3FPROC glColor3f;
GARCH_API extern PFNGLCOLOR3FVPROC glColor3fv;
GARCH_API extern PFNGLCOLOR3HNVPROC glColor3hNV;
GARCH_API extern PFNGLCOLOR3HVNVPROC glColor3hvNV;
GARCH_API extern PFNGLCOLOR3IPROC glColor3i;
GARCH_API extern PFNGLCOLOR3IVPROC glColor3iv;
GARCH_API extern PFNGLCOLOR3SPROC glColor3s;
GARCH_API extern PFNGLCOLOR3SVPROC glColor3sv;
GARCH_API extern PFNGLCOLOR3UBPROC glColor3ub;
GARCH_API extern PFNGLCOLOR3UBVPROC glColor3ubv;
GARCH_API extern PFNGLCOLOR3UIPROC glColor3ui;
GARCH_API extern PFNGLCOLOR3UIVPROC glColor3uiv;
GARCH_API extern PFNGLCOLOR3USPROC glColor3us;
GARCH_API extern PFNGLCOLOR3USVPROC glColor3usv;
GARCH_API extern PFNGLCOLOR4BPROC glColor4b;
GARCH_API extern PFNGLCOLOR4BVPROC glColor4bv;
GARCH_API extern PFNGLCOLOR4DPROC glColor4d;
GARCH_API extern PFNGLCOLOR4DVPROC glColor4dv;
GARCH_API extern PFNGLCOLOR4FPROC glColor4f;
GARCH_API extern PFNGLCOLOR4FVPROC glColor4fv;
GARCH_API extern PFNGLCOLOR4HNVPROC glColor4hNV;
GARCH_API extern PFNGLCOLOR4HVNVPROC glColor4hvNV;
GARCH_API extern PFNGLCOLOR4IPROC glColor4i;
GARCH_API extern PFNGLCOLOR4IVPROC glColor4iv;
GARCH_API extern PFNGLCOLOR4SPROC glColor4s;
GARCH_API extern PFNGLCOLOR4SVPROC glColor4sv;
GARCH_API extern PFNGLCOLOR4UBPROC glColor4ub;
GARCH_API extern PFNGLCOLOR4UBVPROC glColor4ubv;
GARCH_API extern PFNGLCOLOR4UIPROC glColor4ui;
GARCH_API extern PFNGLCOLOR4UIVPROC glColor4uiv;
GARCH_API extern PFNGLCOLOR4USPROC glColor4us;
GARCH_API extern PFNGLCOLOR4USVPROC glColor4usv;
GARCH_API extern PFNGLCOLORFORMATNVPROC glColorFormatNV;
GARCH_API extern PFNGLCOLORMASKPROC glColorMask;
GARCH_API extern PFNGLCOLORMASKINDEXEDEXTPROC glColorMaskIndexedEXT;
GARCH_API extern PFNGLCOLORMASKIPROC glColorMaski;
GARCH_API extern PFNGLCOLORMATERIALPROC glColorMaterial;
GARCH_API extern PFNGLCOLORP3UIPROC glColorP3ui;
GARCH_API extern PFNGLCOLORP3UIVPROC glColorP3uiv;
GARCH_API extern PFNGLCOLORP4UIPROC glColorP4ui;
GARCH_API extern PFNGLCOLORP4UIVPROC glColorP4uiv;
GARCH_API extern PFNGLCOLORPOINTERPROC glColorPointer;
GARCH_API extern PFNGLCOLORPOINTEREXTPROC glColorPointerEXT;
GARCH_API extern PFNGLCOLORPOINTERVINTELPROC glColorPointervINTEL;
GARCH_API extern PFNGLCOLORSUBTABLEPROC glColorSubTable;
GARCH_API extern PFNGLCOLORSUBTABLEEXTPROC glColorSubTableEXT;
GARCH_API extern PFNGLCOLORTABLEPROC glColorTable;
GARCH_API extern PFNGLCOLORTABLEEXTPROC glColorTableEXT;
GARCH_API extern PFNGLCOLORTABLEPARAMETERFVPROC glColorTableParameterfv;
GARCH_API extern PFNGLCOLORTABLEPARAMETERIVPROC glColorTableParameteriv;
GARCH_API extern PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
GARCH_API extern PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
GARCH_API extern PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
GARCH_API extern PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
GARCH_API extern PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
GARCH_API extern PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
GARCH_API extern PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glCombinerStageParameterfvNV;
GARCH_API extern PFNGLCOMMANDLISTSEGMENTSNVPROC glCommandListSegmentsNV;
GARCH_API extern PFNGLCOMPILECOMMANDLISTNVPROC glCompileCommandListNV;
GARCH_API extern PFNGLCOMPILESHADERPROC glCompileShader;
GARCH_API extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
GARCH_API extern PFNGLCOMPILESHADERINCLUDEARBPROC glCompileShaderIncludeARB;
GARCH_API extern PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC glCompressedMultiTexImage1DEXT;
GARCH_API extern PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC glCompressedMultiTexImage2DEXT;
GARCH_API extern PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC glCompressedMultiTexImage3DEXT;
GARCH_API extern PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC glCompressedMultiTexSubImage1DEXT;
GARCH_API extern PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC glCompressedMultiTexSubImage2DEXT;
GARCH_API extern PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC glCompressedMultiTexSubImage3DEXT;
GARCH_API extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glCompressedTexImage1D;
GARCH_API extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;
GARCH_API extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
GARCH_API extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
GARCH_API extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D;
GARCH_API extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;
GARCH_API extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D;
GARCH_API extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;
GARCH_API extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
GARCH_API extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
GARCH_API extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
GARCH_API extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
GARCH_API extern PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC glCompressedTextureImage1DEXT;
GARCH_API extern PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC glCompressedTextureImage2DEXT;
GARCH_API extern PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC glCompressedTextureImage3DEXT;
GARCH_API extern PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC glCompressedTextureSubImage1D;
GARCH_API extern PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC glCompressedTextureSubImage1DEXT;
GARCH_API extern PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC glCompressedTextureSubImage2D;
GARCH_API extern PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC glCompressedTextureSubImage2DEXT;
GARCH_API extern PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC glCompressedTextureSubImage3D;
GARCH_API extern PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC glCompressedTextureSubImage3DEXT;
GARCH_API extern PFNGLCONSERVATIVERASTERPARAMETERFNVPROC glConservativeRasterParameterfNV;
GARCH_API extern PFNGLCONSERVATIVERASTERPARAMETERINVPROC glConservativeRasterParameteriNV;
GARCH_API extern PFNGLCONVOLUTIONFILTER1DPROC glConvolutionFilter1D;
GARCH_API extern PFNGLCONVOLUTIONFILTER1DEXTPROC glConvolutionFilter1DEXT;
GARCH_API extern PFNGLCONVOLUTIONFILTER2DPROC glConvolutionFilter2D;
GARCH_API extern PFNGLCONVOLUTIONFILTER2DEXTPROC glConvolutionFilter2DEXT;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERFPROC glConvolutionParameterf;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERFEXTPROC glConvolutionParameterfEXT;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERFVPROC glConvolutionParameterfv;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERFVEXTPROC glConvolutionParameterfvEXT;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERIPROC glConvolutionParameteri;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERIEXTPROC glConvolutionParameteriEXT;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERIVPROC glConvolutionParameteriv;
GARCH_API extern PFNGLCONVOLUTIONPARAMETERIVEXTPROC glConvolutionParameterivEXT;
GARCH_API extern PFNGLCOPYBUFFERSUBDATAPROC glCopyBufferSubData;
GARCH_API extern PFNGLCOPYCOLORSUBTABLEPROC glCopyColorSubTable;
GARCH_API extern PFNGLCOPYCOLORSUBTABLEEXTPROC glCopyColorSubTableEXT;
GARCH_API extern PFNGLCOPYCOLORTABLEPROC glCopyColorTable;
GARCH_API extern PFNGLCOPYCONVOLUTIONFILTER1DPROC glCopyConvolutionFilter1D;
GARCH_API extern PFNGLCOPYCONVOLUTIONFILTER1DEXTPROC glCopyConvolutionFilter1DEXT;
GARCH_API extern PFNGLCOPYCONVOLUTIONFILTER2DPROC glCopyConvolutionFilter2D;
GARCH_API extern PFNGLCOPYCONVOLUTIONFILTER2DEXTPROC glCopyConvolutionFilter2DEXT;
GARCH_API extern PFNGLCOPYIMAGESUBDATAPROC glCopyImageSubData;
GARCH_API extern PFNGLCOPYIMAGESUBDATANVPROC glCopyImageSubDataNV;
GARCH_API extern PFNGLCOPYMULTITEXIMAGE1DEXTPROC glCopyMultiTexImage1DEXT;
GARCH_API extern PFNGLCOPYMULTITEXIMAGE2DEXTPROC glCopyMultiTexImage2DEXT;
GARCH_API extern PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC glCopyMultiTexSubImage1DEXT;
GARCH_API extern PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC glCopyMultiTexSubImage2DEXT;
GARCH_API extern PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC glCopyMultiTexSubImage3DEXT;
GARCH_API extern PFNGLCOPYNAMEDBUFFERSUBDATAPROC glCopyNamedBufferSubData;
GARCH_API extern PFNGLCOPYPATHNVPROC glCopyPathNV;
GARCH_API extern PFNGLCOPYPIXELSPROC glCopyPixels;
GARCH_API extern PFNGLCOPYTEXIMAGE1DPROC glCopyTexImage1D;
GARCH_API extern PFNGLCOPYTEXIMAGE1DEXTPROC glCopyTexImage1DEXT;
GARCH_API extern PFNGLCOPYTEXIMAGE2DPROC glCopyTexImage2D;
GARCH_API extern PFNGLCOPYTEXIMAGE2DEXTPROC glCopyTexImage2DEXT;
GARCH_API extern PFNGLCOPYTEXSUBIMAGE1DPROC glCopyTexSubImage1D;
GARCH_API extern PFNGLCOPYTEXSUBIMAGE1DEXTPROC glCopyTexSubImage1DEXT;
GARCH_API extern PFNGLCOPYTEXSUBIMAGE2DPROC glCopyTexSubImage2D;
GARCH_API extern PFNGLCOPYTEXSUBIMAGE2DEXTPROC glCopyTexSubImage2DEXT;
GARCH_API extern PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D;
GARCH_API extern PFNGLCOPYTEXSUBIMAGE3DEXTPROC glCopyTexSubImage3DEXT;
GARCH_API extern PFNGLCOPYTEXTUREIMAGE1DEXTPROC glCopyTextureImage1DEXT;
GARCH_API extern PFNGLCOPYTEXTUREIMAGE2DEXTPROC glCopyTextureImage2DEXT;
GARCH_API extern PFNGLCOPYTEXTURESUBIMAGE1DPROC glCopyTextureSubImage1D;
GARCH_API extern PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC glCopyTextureSubImage1DEXT;
GARCH_API extern PFNGLCOPYTEXTURESUBIMAGE2DPROC glCopyTextureSubImage2D;
GARCH_API extern PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC glCopyTextureSubImage2DEXT;
GARCH_API extern PFNGLCOPYTEXTURESUBIMAGE3DPROC glCopyTextureSubImage3D;
GARCH_API extern PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC glCopyTextureSubImage3DEXT;
GARCH_API extern PFNGLCOVERFILLPATHINSTANCEDNVPROC glCoverFillPathInstancedNV;
GARCH_API extern PFNGLCOVERFILLPATHNVPROC glCoverFillPathNV;
GARCH_API extern PFNGLCOVERSTROKEPATHINSTANCEDNVPROC glCoverStrokePathInstancedNV;
GARCH_API extern PFNGLCOVERSTROKEPATHNVPROC glCoverStrokePathNV;
GARCH_API extern PFNGLCOVERAGEMODULATIONNVPROC glCoverageModulationNV;
GARCH_API extern PFNGLCOVERAGEMODULATIONTABLENVPROC glCoverageModulationTableNV;
GARCH_API extern PFNGLCREATEBUFFERSPROC glCreateBuffers;
GARCH_API extern PFNGLCREATECOMMANDLISTSNVPROC glCreateCommandListsNV;
GARCH_API extern PFNGLCREATEFRAMEBUFFERSPROC glCreateFramebuffers;
GARCH_API extern PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT;
GARCH_API extern PFNGLCREATEPERFQUERYINTELPROC glCreatePerfQueryINTEL;
GARCH_API extern PFNGLCREATEPROGRAMPROC glCreateProgram;
GARCH_API extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
GARCH_API extern PFNGLCREATEPROGRAMPIPELINESPROC glCreateProgramPipelines;
GARCH_API extern PFNGLCREATEPROGRESSFENCENVXPROC glCreateProgressFenceNVX;
GARCH_API extern PFNGLCREATEQUERIESPROC glCreateQueries;
GARCH_API extern PFNGLCREATERENDERBUFFERSPROC glCreateRenderbuffers;
GARCH_API extern PFNGLCREATESAMPLERSPROC glCreateSamplers;
GARCH_API extern PFNGLCREATESEMAPHORESNVPROC glCreateSemaphoresNV;
GARCH_API extern PFNGLCREATESHADERPROC glCreateShader;
GARCH_API extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
GARCH_API extern PFNGLCREATESHADERPROGRAMEXTPROC glCreateShaderProgramEXT;
GARCH_API extern PFNGLCREATESHADERPROGRAMVPROC glCreateShaderProgramv;
GARCH_API extern PFNGLCREATESHADERPROGRAMVEXTPROC glCreateShaderProgramvEXT;
GARCH_API extern PFNGLCREATESTATESNVPROC glCreateStatesNV;
GARCH_API extern PFNGLCREATESYNCFROMCLEVENTARBPROC glCreateSyncFromCLeventARB;
GARCH_API extern PFNGLCREATETEXTURESPROC glCreateTextures;
GARCH_API extern PFNGLCREATETRANSFORMFEEDBACKSPROC glCreateTransformFeedbacks;
GARCH_API extern PFNGLCREATEVERTEXARRAYSPROC glCreateVertexArrays;
GARCH_API extern PFNGLCULLFACEPROC glCullFace;
GARCH_API extern PFNGLCULLPARAMETERDVEXTPROC glCullParameterdvEXT;
GARCH_API extern PFNGLCULLPARAMETERFVEXTPROC glCullParameterfvEXT;
GARCH_API extern PFNGLCURRENTPALETTEMATRIXARBPROC glCurrentPaletteMatrixARB;
GARCH_API extern PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
GARCH_API extern PFNGLDEBUGMESSAGECALLBACKAMDPROC glDebugMessageCallbackAMD;
GARCH_API extern PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB;
GARCH_API extern PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR;
GARCH_API extern PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
GARCH_API extern PFNGLDEBUGMESSAGECONTROLARBPROC glDebugMessageControlARB;
GARCH_API extern PFNGLDEBUGMESSAGECONTROLKHRPROC glDebugMessageControlKHR;
GARCH_API extern PFNGLDEBUGMESSAGEENABLEAMDPROC glDebugMessageEnableAMD;
GARCH_API extern PFNGLDEBUGMESSAGEINSERTPROC glDebugMessageInsert;
GARCH_API extern PFNGLDEBUGMESSAGEINSERTAMDPROC glDebugMessageInsertAMD;
GARCH_API extern PFNGLDEBUGMESSAGEINSERTARBPROC glDebugMessageInsertARB;
GARCH_API extern PFNGLDEBUGMESSAGEINSERTKHRPROC glDebugMessageInsertKHR;
GARCH_API extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
GARCH_API extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
GARCH_API extern PFNGLDELETECOMMANDLISTSNVPROC glDeleteCommandListsNV;
GARCH_API extern PFNGLDELETEFENCESAPPLEPROC glDeleteFencesAPPLE;
GARCH_API extern PFNGLDELETEFENCESNVPROC glDeleteFencesNV;
GARCH_API extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
GARCH_API extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
GARCH_API extern PFNGLDELETELISTSPROC glDeleteLists;
GARCH_API extern PFNGLDELETEMEMORYOBJECTSEXTPROC glDeleteMemoryObjectsEXT;
GARCH_API extern PFNGLDELETENAMEDSTRINGARBPROC glDeleteNamedStringARB;
GARCH_API extern PFNGLDELETENAMESAMDPROC glDeleteNamesAMD;
GARCH_API extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
GARCH_API extern PFNGLDELETEOCCLUSIONQUERIESNVPROC glDeleteOcclusionQueriesNV;
GARCH_API extern PFNGLDELETEPATHSNVPROC glDeletePathsNV;
GARCH_API extern PFNGLDELETEPERFMONITORSAMDPROC glDeletePerfMonitorsAMD;
GARCH_API extern PFNGLDELETEPERFQUERYINTELPROC glDeletePerfQueryINTEL;
GARCH_API extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
GARCH_API extern PFNGLDELETEPROGRAMPIPELINESPROC glDeleteProgramPipelines;
GARCH_API extern PFNGLDELETEPROGRAMPIPELINESEXTPROC glDeleteProgramPipelinesEXT;
GARCH_API extern PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
GARCH_API extern PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV;
GARCH_API extern PFNGLDELETEQUERIESPROC glDeleteQueries;
GARCH_API extern PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB;
GARCH_API extern PFNGLDELETEQUERYRESOURCETAGNVPROC glDeleteQueryResourceTagNV;
GARCH_API extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
GARCH_API extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
GARCH_API extern PFNGLDELETESAMPLERSPROC glDeleteSamplers;
GARCH_API extern PFNGLDELETESEMAPHORESEXTPROC glDeleteSemaphoresEXT;
GARCH_API extern PFNGLDELETESHADERPROC glDeleteShader;
GARCH_API extern PFNGLDELETESTATESNVPROC glDeleteStatesNV;
GARCH_API extern PFNGLDELETESYNCPROC glDeleteSync;
GARCH_API extern PFNGLDELETETEXTURESPROC glDeleteTextures;
GARCH_API extern PFNGLDELETETEXTURESEXTPROC glDeleteTexturesEXT;
GARCH_API extern PFNGLDELETETRANSFORMFEEDBACKSPROC glDeleteTransformFeedbacks;
GARCH_API extern PFNGLDELETETRANSFORMFEEDBACKSNVPROC glDeleteTransformFeedbacksNV;
GARCH_API extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
GARCH_API extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glDeleteVertexArraysAPPLE;
GARCH_API extern PFNGLDELETEVERTEXSHADEREXTPROC glDeleteVertexShaderEXT;
GARCH_API extern PFNGLDEPTHBOUNDSEXTPROC glDepthBoundsEXT;
GARCH_API extern PFNGLDEPTHBOUNDSDNVPROC glDepthBoundsdNV;
GARCH_API extern PFNGLDEPTHFUNCPROC glDepthFunc;
GARCH_API extern PFNGLDEPTHMASKPROC glDepthMask;
GARCH_API extern PFNGLDEPTHRANGEPROC glDepthRange;
GARCH_API extern PFNGLDEPTHRANGEARRAYDVNVPROC glDepthRangeArraydvNV;
GARCH_API extern PFNGLDEPTHRANGEARRAYVPROC glDepthRangeArrayv;
GARCH_API extern PFNGLDEPTHRANGEINDEXEDPROC glDepthRangeIndexed;
GARCH_API extern PFNGLDEPTHRANGEINDEXEDDNVPROC glDepthRangeIndexeddNV;
GARCH_API extern PFNGLDEPTHRANGEDNVPROC glDepthRangedNV;
GARCH_API extern PFNGLDEPTHRANGEFPROC glDepthRangef;
GARCH_API extern PFNGLDETACHOBJECTARBPROC glDetachObjectARB;
GARCH_API extern PFNGLDETACHSHADERPROC glDetachShader;
GARCH_API extern PFNGLDISABLEPROC glDisable;
GARCH_API extern PFNGLDISABLECLIENTSTATEPROC glDisableClientState;
GARCH_API extern PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC glDisableClientStateIndexedEXT;
GARCH_API extern PFNGLDISABLECLIENTSTATEIEXTPROC glDisableClientStateiEXT;
GARCH_API extern PFNGLDISABLEINDEXEDEXTPROC glDisableIndexedEXT;
GARCH_API extern PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC glDisableVariantClientStateEXT;
GARCH_API extern PFNGLDISABLEVERTEXARRAYATTRIBPROC glDisableVertexArrayAttrib;
GARCH_API extern PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC glDisableVertexArrayAttribEXT;
GARCH_API extern PFNGLDISABLEVERTEXARRAYEXTPROC glDisableVertexArrayEXT;
GARCH_API extern PFNGLDISABLEVERTEXATTRIBAPPLEPROC glDisableVertexAttribAPPLE;
GARCH_API extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
GARCH_API extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
GARCH_API extern PFNGLDISABLEIPROC glDisablei;
GARCH_API extern PFNGLDISPATCHCOMPUTEPROC glDispatchCompute;
GARCH_API extern PFNGLDISPATCHCOMPUTEGROUPSIZEARBPROC glDispatchComputeGroupSizeARB;
GARCH_API extern PFNGLDISPATCHCOMPUTEINDIRECTPROC glDispatchComputeIndirect;
GARCH_API extern PFNGLDRAWARRAYSPROC glDrawArrays;
GARCH_API extern PFNGLDRAWARRAYSEXTPROC glDrawArraysEXT;
GARCH_API extern PFNGLDRAWARRAYSINDIRECTPROC glDrawArraysIndirect;
GARCH_API extern PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;
GARCH_API extern PFNGLDRAWARRAYSINSTANCEDARBPROC glDrawArraysInstancedARB;
GARCH_API extern PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC glDrawArraysInstancedBaseInstance;
GARCH_API extern PFNGLDRAWARRAYSINSTANCEDEXTPROC glDrawArraysInstancedEXT;
GARCH_API extern PFNGLDRAWBUFFERPROC glDrawBuffer;
GARCH_API extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
GARCH_API extern PFNGLDRAWBUFFERSARBPROC glDrawBuffersARB;
GARCH_API extern PFNGLDRAWCOMMANDSADDRESSNVPROC glDrawCommandsAddressNV;
GARCH_API extern PFNGLDRAWCOMMANDSNVPROC glDrawCommandsNV;
GARCH_API extern PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC glDrawCommandsStatesAddressNV;
GARCH_API extern PFNGLDRAWCOMMANDSSTATESNVPROC glDrawCommandsStatesNV;
GARCH_API extern PFNGLDRAWELEMENTARRAYAPPLEPROC glDrawElementArrayAPPLE;
GARCH_API extern PFNGLDRAWELEMENTSPROC glDrawElements;
GARCH_API extern PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex;
GARCH_API extern PFNGLDRAWELEMENTSINDIRECTPROC glDrawElementsIndirect;
GARCH_API extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
GARCH_API extern PFNGLDRAWELEMENTSINSTANCEDARBPROC glDrawElementsInstancedARB;
GARCH_API extern PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC glDrawElementsInstancedBaseInstance;
GARCH_API extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex;
GARCH_API extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glDrawElementsInstancedBaseVertexBaseInstance;
GARCH_API extern PFNGLDRAWELEMENTSINSTANCEDEXTPROC glDrawElementsInstancedEXT;
GARCH_API extern PFNGLDRAWMESHTASKSNVPROC glDrawMeshTasksNV;
GARCH_API extern PFNGLDRAWMESHTASKSINDIRECTNVPROC glDrawMeshTasksIndirectNV;
GARCH_API extern PFNGLDRAWPIXELSPROC glDrawPixels;
GARCH_API extern PFNGLDRAWRANGEELEMENTARRAYAPPLEPROC glDrawRangeElementArrayAPPLE;
GARCH_API extern PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements;
GARCH_API extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glDrawRangeElementsBaseVertex;
GARCH_API extern PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElementsEXT;
GARCH_API extern PFNGLDRAWTEXTURENVPROC glDrawTextureNV;
GARCH_API extern PFNGLDRAWTRANSFORMFEEDBACKPROC glDrawTransformFeedback;
GARCH_API extern PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC glDrawTransformFeedbackInstanced;
GARCH_API extern PFNGLDRAWTRANSFORMFEEDBACKNVPROC glDrawTransformFeedbackNV;
GARCH_API extern PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC glDrawTransformFeedbackStream;
GARCH_API extern PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC glDrawTransformFeedbackStreamInstanced;
GARCH_API extern PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC glEGLImageTargetTexStorageEXT;
GARCH_API extern PFNGLEGLIMAGETARGETTEXTURESTORAGEEXTPROC glEGLImageTargetTextureStorageEXT;
GARCH_API extern PFNGLEDGEFLAGPROC glEdgeFlag;
GARCH_API extern PFNGLEDGEFLAGFORMATNVPROC glEdgeFlagFormatNV;
GARCH_API extern PFNGLEDGEFLAGPOINTERPROC glEdgeFlagPointer;
GARCH_API extern PFNGLEDGEFLAGPOINTEREXTPROC glEdgeFlagPointerEXT;
GARCH_API extern PFNGLEDGEFLAGVPROC glEdgeFlagv;
GARCH_API extern PFNGLELEMENTPOINTERAPPLEPROC glElementPointerAPPLE;
GARCH_API extern PFNGLENABLEPROC glEnable;
GARCH_API extern PFNGLENABLECLIENTSTATEPROC glEnableClientState;
GARCH_API extern PFNGLENABLECLIENTSTATEINDEXEDEXTPROC glEnableClientStateIndexedEXT;
GARCH_API extern PFNGLENABLECLIENTSTATEIEXTPROC glEnableClientStateiEXT;
GARCH_API extern PFNGLENABLEINDEXEDEXTPROC glEnableIndexedEXT;
GARCH_API extern PFNGLENABLEVARIANTCLIENTSTATEEXTPROC glEnableVariantClientStateEXT;
GARCH_API extern PFNGLENABLEVERTEXARRAYATTRIBPROC glEnableVertexArrayAttrib;
GARCH_API extern PFNGLENABLEVERTEXARRAYATTRIBEXTPROC glEnableVertexArrayAttribEXT;
GARCH_API extern PFNGLENABLEVERTEXARRAYEXTPROC glEnableVertexArrayEXT;
GARCH_API extern PFNGLENABLEVERTEXATTRIBAPPLEPROC glEnableVertexAttribAPPLE;
GARCH_API extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
GARCH_API extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
GARCH_API extern PFNGLENABLEIPROC glEnablei;
GARCH_API extern PFNGLENDPROC glEnd;
GARCH_API extern PFNGLENDCONDITIONALRENDERPROC glEndConditionalRender;
GARCH_API extern PFNGLENDCONDITIONALRENDERNVPROC glEndConditionalRenderNV;
GARCH_API extern PFNGLENDCONDITIONALRENDERNVXPROC glEndConditionalRenderNVX;
GARCH_API extern PFNGLENDLISTPROC glEndList;
GARCH_API extern PFNGLENDOCCLUSIONQUERYNVPROC glEndOcclusionQueryNV;
GARCH_API extern PFNGLENDPERFMONITORAMDPROC glEndPerfMonitorAMD;
GARCH_API extern PFNGLENDPERFQUERYINTELPROC glEndPerfQueryINTEL;
GARCH_API extern PFNGLENDQUERYPROC glEndQuery;
GARCH_API extern PFNGLENDQUERYARBPROC glEndQueryARB;
GARCH_API extern PFNGLENDQUERYINDEXEDPROC glEndQueryIndexed;
GARCH_API extern PFNGLENDTRANSFORMFEEDBACKPROC glEndTransformFeedback;
GARCH_API extern PFNGLENDTRANSFORMFEEDBACKEXTPROC glEndTransformFeedbackEXT;
GARCH_API extern PFNGLENDTRANSFORMFEEDBACKNVPROC glEndTransformFeedbackNV;
GARCH_API extern PFNGLENDVERTEXSHADEREXTPROC glEndVertexShaderEXT;
GARCH_API extern PFNGLENDVIDEOCAPTURENVPROC glEndVideoCaptureNV;
GARCH_API extern PFNGLEVALCOORD1DPROC glEvalCoord1d;
GARCH_API extern PFNGLEVALCOORD1DVPROC glEvalCoord1dv;
GARCH_API extern PFNGLEVALCOORD1FPROC glEvalCoord1f;
GARCH_API extern PFNGLEVALCOORD1FVPROC glEvalCoord1fv;
GARCH_API extern PFNGLEVALCOORD2DPROC glEvalCoord2d;
GARCH_API extern PFNGLEVALCOORD2DVPROC glEvalCoord2dv;
GARCH_API extern PFNGLEVALCOORD2FPROC glEvalCoord2f;
GARCH_API extern PFNGLEVALCOORD2FVPROC glEvalCoord2fv;
GARCH_API extern PFNGLEVALMAPSNVPROC glEvalMapsNV;
GARCH_API extern PFNGLEVALMESH1PROC glEvalMesh1;
GARCH_API extern PFNGLEVALMESH2PROC glEvalMesh2;
GARCH_API extern PFNGLEVALPOINT1PROC glEvalPoint1;
GARCH_API extern PFNGLEVALPOINT2PROC glEvalPoint2;
GARCH_API extern PFNGLEVALUATEDEPTHVALUESARBPROC glEvaluateDepthValuesARB;
GARCH_API extern PFNGLEXECUTEPROGRAMNVPROC glExecuteProgramNV;
GARCH_API extern PFNGLEXTRACTCOMPONENTEXTPROC glExtractComponentEXT;
GARCH_API extern PFNGLFEEDBACKBUFFERPROC glFeedbackBuffer;
GARCH_API extern PFNGLFENCESYNCPROC glFenceSync;
GARCH_API extern PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
GARCH_API extern PFNGLFINISHPROC glFinish;
GARCH_API extern PFNGLFINISHFENCEAPPLEPROC glFinishFenceAPPLE;
GARCH_API extern PFNGLFINISHFENCENVPROC glFinishFenceNV;
GARCH_API extern PFNGLFINISHOBJECTAPPLEPROC glFinishObjectAPPLE;
GARCH_API extern PFNGLFLUSHPROC glFlush;
GARCH_API extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC glFlushMappedBufferRange;
GARCH_API extern PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC glFlushMappedBufferRangeAPPLE;
GARCH_API extern PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC glFlushMappedNamedBufferRange;
GARCH_API extern PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC glFlushMappedNamedBufferRangeEXT;
GARCH_API extern PFNGLFLUSHPIXELDATARANGENVPROC glFlushPixelDataRangeNV;
GARCH_API extern PFNGLFLUSHVERTEXARRAYRANGEAPPLEPROC glFlushVertexArrayRangeAPPLE;
GARCH_API extern PFNGLFLUSHVERTEXARRAYRANGENVPROC glFlushVertexArrayRangeNV;
GARCH_API extern PFNGLFOGCOORDFORMATNVPROC glFogCoordFormatNV;
GARCH_API extern PFNGLFOGCOORDPOINTERPROC glFogCoordPointer;
GARCH_API extern PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;
GARCH_API extern PFNGLFOGCOORDDPROC glFogCoordd;
GARCH_API extern PFNGLFOGCOORDDEXTPROC glFogCoorddEXT;
GARCH_API extern PFNGLFOGCOORDDVPROC glFogCoorddv;
GARCH_API extern PFNGLFOGCOORDDVEXTPROC glFogCoorddvEXT;
GARCH_API extern PFNGLFOGCOORDFPROC glFogCoordf;
GARCH_API extern PFNGLFOGCOORDFEXTPROC glFogCoordfEXT;
GARCH_API extern PFNGLFOGCOORDFVPROC glFogCoordfv;
GARCH_API extern PFNGLFOGCOORDFVEXTPROC glFogCoordfvEXT;
GARCH_API extern PFNGLFOGCOORDHNVPROC glFogCoordhNV;
GARCH_API extern PFNGLFOGCOORDHVNVPROC glFogCoordhvNV;
GARCH_API extern PFNGLFOGFPROC glFogf;
GARCH_API extern PFNGLFOGFVPROC glFogfv;
GARCH_API extern PFNGLFOGIPROC glFogi;
GARCH_API extern PFNGLFOGIVPROC glFogiv;
GARCH_API extern PFNGLFRAGMENTCOVERAGECOLORNVPROC glFragmentCoverageColorNV;
GARCH_API extern PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC glFramebufferDrawBufferEXT;
GARCH_API extern PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC glFramebufferDrawBuffersEXT;
GARCH_API extern PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC glFramebufferFetchBarrierEXT;
GARCH_API extern PFNGLFRAMEBUFFERPARAMETERIPROC glFramebufferParameteri;
GARCH_API extern PFNGLFRAMEBUFFERREADBUFFEREXTPROC glFramebufferReadBufferEXT;
GARCH_API extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
GARCH_API extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
GARCH_API extern PFNGLFRAMEBUFFERSAMPLELOCATIONSFVARBPROC glFramebufferSampleLocationsfvARB;
GARCH_API extern PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC glFramebufferSampleLocationsfvNV;
GARCH_API extern PFNGLFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC glFramebufferSamplePositionsfvAMD;
GARCH_API extern PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
GARCH_API extern PFNGLFRAMEBUFFERTEXTUREARBPROC glFramebufferTextureARB;
GARCH_API extern PFNGLFRAMEBUFFERTEXTUREEXTPROC glFramebufferTextureEXT;
GARCH_API extern PFNGLFRAMEBUFFERTEXTUREFACEARBPROC glFramebufferTextureFaceARB;
GARCH_API extern PFNGLFRAMEBUFFERTEXTUREFACEEXTPROC glFramebufferTextureFaceEXT;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURELAYERARBPROC glFramebufferTextureLayerARB;
GARCH_API extern PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC glFramebufferTextureLayerEXT;
GARCH_API extern PFNGLFRONTFACEPROC glFrontFace;
GARCH_API extern PFNGLFRUSTUMPROC glFrustum;
GARCH_API extern PFNGLGENBUFFERSPROC glGenBuffers;
GARCH_API extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;
GARCH_API extern PFNGLGENFENCESAPPLEPROC glGenFencesAPPLE;
GARCH_API extern PFNGLGENFENCESNVPROC glGenFencesNV;
GARCH_API extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
GARCH_API extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
GARCH_API extern PFNGLGENLISTSPROC glGenLists;
GARCH_API extern PFNGLGENNAMESAMDPROC glGenNamesAMD;
GARCH_API extern PFNGLGENOCCLUSIONQUERIESNVPROC glGenOcclusionQueriesNV;
GARCH_API extern PFNGLGENPATHSNVPROC glGenPathsNV;
GARCH_API extern PFNGLGENPERFMONITORSAMDPROC glGenPerfMonitorsAMD;
GARCH_API extern PFNGLGENPROGRAMPIPELINESPROC glGenProgramPipelines;
GARCH_API extern PFNGLGENPROGRAMPIPELINESEXTPROC glGenProgramPipelinesEXT;
GARCH_API extern PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
GARCH_API extern PFNGLGENPROGRAMSNVPROC glGenProgramsNV;
GARCH_API extern PFNGLGENQUERIESPROC glGenQueries;
GARCH_API extern PFNGLGENQUERIESARBPROC glGenQueriesARB;
GARCH_API extern PFNGLGENQUERYRESOURCETAGNVPROC glGenQueryResourceTagNV;
GARCH_API extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
GARCH_API extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
GARCH_API extern PFNGLGENSAMPLERSPROC glGenSamplers;
GARCH_API extern PFNGLGENSEMAPHORESEXTPROC glGenSemaphoresEXT;
GARCH_API extern PFNGLGENSYMBOLSEXTPROC glGenSymbolsEXT;
GARCH_API extern PFNGLGENTEXTURESPROC glGenTextures;
GARCH_API extern PFNGLGENTEXTURESEXTPROC glGenTexturesEXT;
GARCH_API extern PFNGLGENTRANSFORMFEEDBACKSPROC glGenTransformFeedbacks;
GARCH_API extern PFNGLGENTRANSFORMFEEDBACKSNVPROC glGenTransformFeedbacksNV;
GARCH_API extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
GARCH_API extern PFNGLGENVERTEXARRAYSAPPLEPROC glGenVertexArraysAPPLE;
GARCH_API extern PFNGLGENVERTEXSHADERSEXTPROC glGenVertexShadersEXT;
GARCH_API extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
GARCH_API extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
GARCH_API extern PFNGLGENERATEMULTITEXMIPMAPEXTPROC glGenerateMultiTexMipmapEXT;
GARCH_API extern PFNGLGENERATETEXTUREMIPMAPPROC glGenerateTextureMipmap;
GARCH_API extern PFNGLGENERATETEXTUREMIPMAPEXTPROC glGenerateTextureMipmapEXT;
GARCH_API extern PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC glGetActiveAtomicCounterBufferiv;
GARCH_API extern PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
GARCH_API extern PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB;
GARCH_API extern PFNGLGETACTIVESUBROUTINENAMEPROC glGetActiveSubroutineName;
GARCH_API extern PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC glGetActiveSubroutineUniformName;
GARCH_API extern PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC glGetActiveSubroutineUniformiv;
GARCH_API extern PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
GARCH_API extern PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB;
GARCH_API extern PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glGetActiveUniformBlockName;
GARCH_API extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv;
GARCH_API extern PFNGLGETACTIVEUNIFORMNAMEPROC glGetActiveUniformName;
GARCH_API extern PFNGLGETACTIVEUNIFORMSIVPROC glGetActiveUniformsiv;
GARCH_API extern PFNGLGETACTIVEVARYINGNVPROC glGetActiveVaryingNV;
GARCH_API extern PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB;
GARCH_API extern PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders;
GARCH_API extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
GARCH_API extern PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;
GARCH_API extern PFNGLGETBOOLEANINDEXEDVEXTPROC glGetBooleanIndexedvEXT;
GARCH_API extern PFNGLGETBOOLEANI_VPROC glGetBooleani_v;
GARCH_API extern PFNGLGETBOOLEANVPROC glGetBooleanv;
GARCH_API extern PFNGLGETBUFFERPARAMETERI64VPROC glGetBufferParameteri64v;
GARCH_API extern PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
GARCH_API extern PFNGLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB;
GARCH_API extern PFNGLGETBUFFERPARAMETERUI64VNVPROC glGetBufferParameterui64vNV;
GARCH_API extern PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv;
GARCH_API extern PFNGLGETBUFFERPOINTERVARBPROC glGetBufferPointervARB;
GARCH_API extern PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
GARCH_API extern PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubDataARB;
GARCH_API extern PFNGLGETCLIPPLANEPROC glGetClipPlane;
GARCH_API extern PFNGLGETCOLORTABLEPROC glGetColorTable;
GARCH_API extern PFNGLGETCOLORTABLEEXTPROC glGetColorTableEXT;
GARCH_API extern PFNGLGETCOLORTABLEPARAMETERFVPROC glGetColorTableParameterfv;
GARCH_API extern PFNGLGETCOLORTABLEPARAMETERFVEXTPROC glGetColorTableParameterfvEXT;
GARCH_API extern PFNGLGETCOLORTABLEPARAMETERIVPROC glGetColorTableParameteriv;
GARCH_API extern PFNGLGETCOLORTABLEPARAMETERIVEXTPROC glGetColorTableParameterivEXT;
GARCH_API extern PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
GARCH_API extern PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
GARCH_API extern PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
GARCH_API extern PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
GARCH_API extern PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC glGetCombinerStageParameterfvNV;
GARCH_API extern PFNGLGETCOMMANDHEADERNVPROC glGetCommandHeaderNV;
GARCH_API extern PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC glGetCompressedMultiTexImageEXT;
GARCH_API extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glGetCompressedTexImage;
GARCH_API extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC glGetCompressedTexImageARB;
GARCH_API extern PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC glGetCompressedTextureImage;
GARCH_API extern PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC glGetCompressedTextureImageEXT;
GARCH_API extern PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC glGetCompressedTextureSubImage;
GARCH_API extern PFNGLGETCONVOLUTIONFILTERPROC glGetConvolutionFilter;
GARCH_API extern PFNGLGETCONVOLUTIONFILTEREXTPROC glGetConvolutionFilterEXT;
GARCH_API extern PFNGLGETCONVOLUTIONPARAMETERFVPROC glGetConvolutionParameterfv;
GARCH_API extern PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC glGetConvolutionParameterfvEXT;
GARCH_API extern PFNGLGETCONVOLUTIONPARAMETERIVPROC glGetConvolutionParameteriv;
GARCH_API extern PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC glGetConvolutionParameterivEXT;
GARCH_API extern PFNGLGETCOVERAGEMODULATIONTABLENVPROC glGetCoverageModulationTableNV;
GARCH_API extern PFNGLGETDEBUGMESSAGELOGPROC glGetDebugMessageLog;
GARCH_API extern PFNGLGETDEBUGMESSAGELOGAMDPROC glGetDebugMessageLogAMD;
GARCH_API extern PFNGLGETDEBUGMESSAGELOGARBPROC glGetDebugMessageLogARB;
GARCH_API extern PFNGLGETDEBUGMESSAGELOGKHRPROC glGetDebugMessageLogKHR;
GARCH_API extern PFNGLGETDOUBLEINDEXEDVEXTPROC glGetDoubleIndexedvEXT;
GARCH_API extern PFNGLGETDOUBLEI_VPROC glGetDoublei_v;
GARCH_API extern PFNGLGETDOUBLEI_VEXTPROC glGetDoublei_vEXT;
GARCH_API extern PFNGLGETDOUBLEVPROC glGetDoublev;
GARCH_API extern PFNGLGETERRORPROC glGetError;
GARCH_API extern PFNGLGETFENCEIVNVPROC glGetFenceivNV;
GARCH_API extern PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
GARCH_API extern PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;
GARCH_API extern PFNGLGETFIRSTPERFQUERYIDINTELPROC glGetFirstPerfQueryIdINTEL;
GARCH_API extern PFNGLGETFLOATINDEXEDVEXTPROC glGetFloatIndexedvEXT;
GARCH_API extern PFNGLGETFLOATI_VPROC glGetFloati_v;
GARCH_API extern PFNGLGETFLOATI_VEXTPROC glGetFloati_vEXT;
GARCH_API extern PFNGLGETFLOATVPROC glGetFloatv;
GARCH_API extern PFNGLGETFRAGDATAINDEXPROC glGetFragDataIndex;
GARCH_API extern PFNGLGETFRAGDATALOCATIONPROC glGetFragDataLocation;
GARCH_API extern PFNGLGETFRAGDATALOCATIONEXTPROC glGetFragDataLocationEXT;
GARCH_API extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
GARCH_API extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
GARCH_API extern PFNGLGETFRAMEBUFFERPARAMETERFVAMDPROC glGetFramebufferParameterfvAMD;
GARCH_API extern PFNGLGETFRAMEBUFFERPARAMETERIVPROC glGetFramebufferParameteriv;
GARCH_API extern PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC glGetFramebufferParameterivEXT;
GARCH_API extern PFNGLGETGRAPHICSRESETSTATUSPROC glGetGraphicsResetStatus;
GARCH_API extern PFNGLGETGRAPHICSRESETSTATUSARBPROC glGetGraphicsResetStatusARB;
GARCH_API extern PFNGLGETGRAPHICSRESETSTATUSKHRPROC glGetGraphicsResetStatusKHR;
GARCH_API extern PFNGLGETHANDLEARBPROC glGetHandleARB;
GARCH_API extern PFNGLGETHISTOGRAMPROC glGetHistogram;
GARCH_API extern PFNGLGETHISTOGRAMEXTPROC glGetHistogramEXT;
GARCH_API extern PFNGLGETHISTOGRAMPARAMETERFVPROC glGetHistogramParameterfv;
GARCH_API extern PFNGLGETHISTOGRAMPARAMETERFVEXTPROC glGetHistogramParameterfvEXT;
GARCH_API extern PFNGLGETHISTOGRAMPARAMETERIVPROC glGetHistogramParameteriv;
GARCH_API extern PFNGLGETHISTOGRAMPARAMETERIVEXTPROC glGetHistogramParameterivEXT;
GARCH_API extern PFNGLGETIMAGEHANDLEARBPROC glGetImageHandleARB;
GARCH_API extern PFNGLGETIMAGEHANDLENVPROC glGetImageHandleNV;
GARCH_API extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
GARCH_API extern PFNGLGETINTEGER64I_VPROC glGetInteger64i_v;
GARCH_API extern PFNGLGETINTEGER64VPROC glGetInteger64v;
GARCH_API extern PFNGLGETINTEGERINDEXEDVEXTPROC glGetIntegerIndexedvEXT;
GARCH_API extern PFNGLGETINTEGERI_VPROC glGetIntegeri_v;
GARCH_API extern PFNGLGETINTEGERUI64I_VNVPROC glGetIntegerui64i_vNV;
GARCH_API extern PFNGLGETINTEGERUI64VNVPROC glGetIntegerui64vNV;
GARCH_API extern PFNGLGETINTEGERVPROC glGetIntegerv;
GARCH_API extern PFNGLGETINTERNALFORMATSAMPLEIVNVPROC glGetInternalformatSampleivNV;
GARCH_API extern PFNGLGETINTERNALFORMATI64VPROC glGetInternalformati64v;
GARCH_API extern PFNGLGETINTERNALFORMATIVPROC glGetInternalformativ;
GARCH_API extern PFNGLGETINVARIANTBOOLEANVEXTPROC glGetInvariantBooleanvEXT;
GARCH_API extern PFNGLGETINVARIANTFLOATVEXTPROC glGetInvariantFloatvEXT;
GARCH_API extern PFNGLGETINVARIANTINTEGERVEXTPROC glGetInvariantIntegervEXT;
GARCH_API extern PFNGLGETLIGHTFVPROC glGetLightfv;
GARCH_API extern PFNGLGETLIGHTIVPROC glGetLightiv;
GARCH_API extern PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC glGetLocalConstantBooleanvEXT;
GARCH_API extern PFNGLGETLOCALCONSTANTFLOATVEXTPROC glGetLocalConstantFloatvEXT;
GARCH_API extern PFNGLGETLOCALCONSTANTINTEGERVEXTPROC glGetLocalConstantIntegervEXT;
GARCH_API extern PFNGLGETMAPATTRIBPARAMETERFVNVPROC glGetMapAttribParameterfvNV;
GARCH_API extern PFNGLGETMAPATTRIBPARAMETERIVNVPROC glGetMapAttribParameterivNV;
GARCH_API extern PFNGLGETMAPCONTROLPOINTSNVPROC glGetMapControlPointsNV;
GARCH_API extern PFNGLGETMAPPARAMETERFVNVPROC glGetMapParameterfvNV;
GARCH_API extern PFNGLGETMAPPARAMETERIVNVPROC glGetMapParameterivNV;
GARCH_API extern PFNGLGETMAPDVPROC glGetMapdv;
GARCH_API extern PFNGLGETMAPFVPROC glGetMapfv;
GARCH_API extern PFNGLGETMAPIVPROC glGetMapiv;
GARCH_API extern PFNGLGETMATERIALFVPROC glGetMaterialfv;
GARCH_API extern PFNGLGETMATERIALIVPROC glGetMaterialiv;
GARCH_API extern PFNGLGETMEMORYOBJECTDETACHEDRESOURCESUIVNVPROC glGetMemoryObjectDetachedResourcesuivNV;
GARCH_API extern PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC glGetMemoryObjectParameterivEXT;
GARCH_API extern PFNGLGETMINMAXPROC glGetMinmax;
GARCH_API extern PFNGLGETMINMAXEXTPROC glGetMinmaxEXT;
GARCH_API extern PFNGLGETMINMAXPARAMETERFVPROC glGetMinmaxParameterfv;
GARCH_API extern PFNGLGETMINMAXPARAMETERFVEXTPROC glGetMinmaxParameterfvEXT;
GARCH_API extern PFNGLGETMINMAXPARAMETERIVPROC glGetMinmaxParameteriv;
GARCH_API extern PFNGLGETMINMAXPARAMETERIVEXTPROC glGetMinmaxParameterivEXT;
GARCH_API extern PFNGLGETMULTITEXENVFVEXTPROC glGetMultiTexEnvfvEXT;
GARCH_API extern PFNGLGETMULTITEXENVIVEXTPROC glGetMultiTexEnvivEXT;
GARCH_API extern PFNGLGETMULTITEXGENDVEXTPROC glGetMultiTexGendvEXT;
GARCH_API extern PFNGLGETMULTITEXGENFVEXTPROC glGetMultiTexGenfvEXT;
GARCH_API extern PFNGLGETMULTITEXGENIVEXTPROC glGetMultiTexGenivEXT;
GARCH_API extern PFNGLGETMULTITEXIMAGEEXTPROC glGetMultiTexImageEXT;
GARCH_API extern PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC glGetMultiTexLevelParameterfvEXT;
GARCH_API extern PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC glGetMultiTexLevelParameterivEXT;
GARCH_API extern PFNGLGETMULTITEXPARAMETERIIVEXTPROC glGetMultiTexParameterIivEXT;
GARCH_API extern PFNGLGETMULTITEXPARAMETERIUIVEXTPROC glGetMultiTexParameterIuivEXT;
GARCH_API extern PFNGLGETMULTITEXPARAMETERFVEXTPROC glGetMultiTexParameterfvEXT;
GARCH_API extern PFNGLGETMULTITEXPARAMETERIVEXTPROC glGetMultiTexParameterivEXT;
GARCH_API extern PFNGLGETMULTISAMPLEFVPROC glGetMultisamplefv;
GARCH_API extern PFNGLGETMULTISAMPLEFVNVPROC glGetMultisamplefvNV;
GARCH_API extern PFNGLGETNAMEDBUFFERPARAMETERI64VPROC glGetNamedBufferParameteri64v;
GARCH_API extern PFNGLGETNAMEDBUFFERPARAMETERIVPROC glGetNamedBufferParameteriv;
GARCH_API extern PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC glGetNamedBufferParameterivEXT;
GARCH_API extern PFNGLGETNAMEDBUFFERPARAMETERUI64VNVPROC glGetNamedBufferParameterui64vNV;
GARCH_API extern PFNGLGETNAMEDBUFFERPOINTERVPROC glGetNamedBufferPointerv;
GARCH_API extern PFNGLGETNAMEDBUFFERPOINTERVEXTPROC glGetNamedBufferPointervEXT;
GARCH_API extern PFNGLGETNAMEDBUFFERSUBDATAPROC glGetNamedBufferSubData;
GARCH_API extern PFNGLGETNAMEDBUFFERSUBDATAEXTPROC glGetNamedBufferSubDataEXT;
GARCH_API extern PFNGLGETNAMEDFRAMEBUFFERPARAMETERFVAMDPROC glGetNamedFramebufferParameterfvAMD;
GARCH_API extern PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetNamedFramebufferAttachmentParameteriv;
GARCH_API extern PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetNamedFramebufferAttachmentParameterivEXT;
GARCH_API extern PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC glGetNamedFramebufferParameteriv;
GARCH_API extern PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC glGetNamedFramebufferParameterivEXT;
GARCH_API extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC glGetNamedProgramLocalParameterIivEXT;
GARCH_API extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC glGetNamedProgramLocalParameterIuivEXT;
GARCH_API extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC glGetNamedProgramLocalParameterdvEXT;
GARCH_API extern PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC glGetNamedProgramLocalParameterfvEXT;
GARCH_API extern PFNGLGETNAMEDPROGRAMSTRINGEXTPROC glGetNamedProgramStringEXT;
GARCH_API extern PFNGLGETNAMEDPROGRAMIVEXTPROC glGetNamedProgramivEXT;
GARCH_API extern PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC glGetNamedRenderbufferParameteriv;
GARCH_API extern PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC glGetNamedRenderbufferParameterivEXT;
GARCH_API extern PFNGLGETNAMEDSTRINGARBPROC glGetNamedStringARB;
GARCH_API extern PFNGLGETNAMEDSTRINGIVARBPROC glGetNamedStringivARB;
GARCH_API extern PFNGLGETNEXTPERFQUERYIDINTELPROC glGetNextPerfQueryIdINTEL;
GARCH_API extern PFNGLGETOBJECTLABELPROC glGetObjectLabel;
GARCH_API extern PFNGLGETOBJECTLABELEXTPROC glGetObjectLabelEXT;
GARCH_API extern PFNGLGETOBJECTLABELKHRPROC glGetObjectLabelKHR;
GARCH_API extern PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB;
GARCH_API extern PFNGLGETOBJECTPARAMETERIVAPPLEPROC glGetObjectParameterivAPPLE;
GARCH_API extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
GARCH_API extern PFNGLGETOBJECTPTRLABELPROC glGetObjectPtrLabel;
GARCH_API extern PFNGLGETOBJECTPTRLABELKHRPROC glGetObjectPtrLabelKHR;
GARCH_API extern PFNGLGETOCCLUSIONQUERYIVNVPROC glGetOcclusionQueryivNV;
GARCH_API extern PFNGLGETOCCLUSIONQUERYUIVNVPROC glGetOcclusionQueryuivNV;
GARCH_API extern PFNGLGETPATHCOLORGENFVNVPROC glGetPathColorGenfvNV;
GARCH_API extern PFNGLGETPATHCOLORGENIVNVPROC glGetPathColorGenivNV;
GARCH_API extern PFNGLGETPATHCOMMANDSNVPROC glGetPathCommandsNV;
GARCH_API extern PFNGLGETPATHCOORDSNVPROC glGetPathCoordsNV;
GARCH_API extern PFNGLGETPATHDASHARRAYNVPROC glGetPathDashArrayNV;
GARCH_API extern PFNGLGETPATHLENGTHNVPROC glGetPathLengthNV;
GARCH_API extern PFNGLGETPATHMETRICRANGENVPROC glGetPathMetricRangeNV;
GARCH_API extern PFNGLGETPATHMETRICSNVPROC glGetPathMetricsNV;
GARCH_API extern PFNGLGETPATHPARAMETERFVNVPROC glGetPathParameterfvNV;
GARCH_API extern PFNGLGETPATHPARAMETERIVNVPROC glGetPathParameterivNV;
GARCH_API extern PFNGLGETPATHSPACINGNVPROC glGetPathSpacingNV;
GARCH_API extern PFNGLGETPATHTEXGENFVNVPROC glGetPathTexGenfvNV;
GARCH_API extern PFNGLGETPATHTEXGENIVNVPROC glGetPathTexGenivNV;
GARCH_API extern PFNGLGETPERFCOUNTERINFOINTELPROC glGetPerfCounterInfoINTEL;
GARCH_API extern PFNGLGETPERFMONITORCOUNTERDATAAMDPROC glGetPerfMonitorCounterDataAMD;
GARCH_API extern PFNGLGETPERFMONITORCOUNTERINFOAMDPROC glGetPerfMonitorCounterInfoAMD;
GARCH_API extern PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC glGetPerfMonitorCounterStringAMD;
GARCH_API extern PFNGLGETPERFMONITORCOUNTERSAMDPROC glGetPerfMonitorCountersAMD;
GARCH_API extern PFNGLGETPERFMONITORGROUPSTRINGAMDPROC glGetPerfMonitorGroupStringAMD;
GARCH_API extern PFNGLGETPERFMONITORGROUPSAMDPROC glGetPerfMonitorGroupsAMD;
GARCH_API extern PFNGLGETPERFQUERYDATAINTELPROC glGetPerfQueryDataINTEL;
GARCH_API extern PFNGLGETPERFQUERYIDBYNAMEINTELPROC glGetPerfQueryIdByNameINTEL;
GARCH_API extern PFNGLGETPERFQUERYINFOINTELPROC glGetPerfQueryInfoINTEL;
GARCH_API extern PFNGLGETPIXELMAPFVPROC glGetPixelMapfv;
GARCH_API extern PFNGLGETPIXELMAPUIVPROC glGetPixelMapuiv;
GARCH_API extern PFNGLGETPIXELMAPUSVPROC glGetPixelMapusv;
GARCH_API extern PFNGLGETPIXELTRANSFORMPARAMETERFVEXTPROC glGetPixelTransformParameterfvEXT;
GARCH_API extern PFNGLGETPIXELTRANSFORMPARAMETERIVEXTPROC glGetPixelTransformParameterivEXT;
GARCH_API extern PFNGLGETPOINTERINDEXEDVEXTPROC glGetPointerIndexedvEXT;
GARCH_API extern PFNGLGETPOINTERI_VEXTPROC glGetPointeri_vEXT;
GARCH_API extern PFNGLGETPOINTERVPROC glGetPointerv;
GARCH_API extern PFNGLGETPOINTERVEXTPROC glGetPointervEXT;
GARCH_API extern PFNGLGETPOINTERVKHRPROC glGetPointervKHR;
GARCH_API extern PFNGLGETPOLYGONSTIPPLEPROC glGetPolygonStipple;
GARCH_API extern PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
GARCH_API extern PFNGLGETPROGRAMENVPARAMETERIIVNVPROC glGetProgramEnvParameterIivNV;
GARCH_API extern PFNGLGETPROGRAMENVPARAMETERIUIVNVPROC glGetProgramEnvParameterIuivNV;
GARCH_API extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC glGetProgramEnvParameterdvARB;
GARCH_API extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC glGetProgramEnvParameterfvARB;
GARCH_API extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
GARCH_API extern PFNGLGETPROGRAMINTERFACEIVPROC glGetProgramInterfaceiv;
GARCH_API extern PFNGLGETPROGRAMLOCALPARAMETERIIVNVPROC glGetProgramLocalParameterIivNV;
GARCH_API extern PFNGLGETPROGRAMLOCALPARAMETERIUIVNVPROC glGetProgramLocalParameterIuivNV;
GARCH_API extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB;
GARCH_API extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glGetProgramLocalParameterfvARB;
GARCH_API extern PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC glGetProgramNamedParameterdvNV;
GARCH_API extern PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC glGetProgramNamedParameterfvNV;
GARCH_API extern PFNGLGETPROGRAMPARAMETERDVNVPROC glGetProgramParameterdvNV;
GARCH_API extern PFNGLGETPROGRAMPARAMETERFVNVPROC glGetProgramParameterfvNV;
GARCH_API extern PFNGLGETPROGRAMPIPELINEINFOLOGPROC glGetProgramPipelineInfoLog;
GARCH_API extern PFNGLGETPROGRAMPIPELINEINFOLOGEXTPROC glGetProgramPipelineInfoLogEXT;
GARCH_API extern PFNGLGETPROGRAMPIPELINEIVPROC glGetProgramPipelineiv;
GARCH_API extern PFNGLGETPROGRAMPIPELINEIVEXTPROC glGetProgramPipelineivEXT;
GARCH_API extern PFNGLGETPROGRAMRESOURCEINDEXPROC glGetProgramResourceIndex;
GARCH_API extern PFNGLGETPROGRAMRESOURCELOCATIONPROC glGetProgramResourceLocation;
GARCH_API extern PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC glGetProgramResourceLocationIndex;
GARCH_API extern PFNGLGETPROGRAMRESOURCENAMEPROC glGetProgramResourceName;
GARCH_API extern PFNGLGETPROGRAMRESOURCEFVNVPROC glGetProgramResourcefvNV;
GARCH_API extern PFNGLGETPROGRAMRESOURCEIVPROC glGetProgramResourceiv;
GARCH_API extern PFNGLGETPROGRAMSTAGEIVPROC glGetProgramStageiv;
GARCH_API extern PFNGLGETPROGRAMSTRINGARBPROC glGetProgramStringARB;
GARCH_API extern PFNGLGETPROGRAMSTRINGNVPROC glGetProgramStringNV;
GARCH_API extern PFNGLGETPROGRAMSUBROUTINEPARAMETERUIVNVPROC glGetProgramSubroutineParameteruivNV;
GARCH_API extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
GARCH_API extern PFNGLGETPROGRAMIVARBPROC glGetProgramivARB;
GARCH_API extern PFNGLGETPROGRAMIVNVPROC glGetProgramivNV;
GARCH_API extern PFNGLGETQUERYBUFFEROBJECTI64VPROC glGetQueryBufferObjecti64v;
GARCH_API extern PFNGLGETQUERYBUFFEROBJECTIVPROC glGetQueryBufferObjectiv;
GARCH_API extern PFNGLGETQUERYBUFFEROBJECTUI64VPROC glGetQueryBufferObjectui64v;
GARCH_API extern PFNGLGETQUERYBUFFEROBJECTUIVPROC glGetQueryBufferObjectuiv;
GARCH_API extern PFNGLGETQUERYINDEXEDIVPROC glGetQueryIndexediv;
GARCH_API extern PFNGLGETQUERYOBJECTI64VPROC glGetQueryObjecti64v;
GARCH_API extern PFNGLGETQUERYOBJECTI64VEXTPROC glGetQueryObjecti64vEXT;
GARCH_API extern PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
GARCH_API extern PFNGLGETQUERYOBJECTIVARBPROC glGetQueryObjectivARB;
GARCH_API extern PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;
GARCH_API extern PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT;
GARCH_API extern PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;
GARCH_API extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB;
GARCH_API extern PFNGLGETQUERYIVPROC glGetQueryiv;
GARCH_API extern PFNGLGETQUERYIVARBPROC glGetQueryivARB;
GARCH_API extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
GARCH_API extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
GARCH_API extern PFNGLGETSAMPLERPARAMETERIIVPROC glGetSamplerParameterIiv;
GARCH_API extern PFNGLGETSAMPLERPARAMETERIUIVPROC glGetSamplerParameterIuiv;
GARCH_API extern PFNGLGETSAMPLERPARAMETERFVPROC glGetSamplerParameterfv;
GARCH_API extern PFNGLGETSAMPLERPARAMETERIVPROC glGetSamplerParameteriv;
GARCH_API extern PFNGLGETSEMAPHOREPARAMETERIVNVPROC glGetSemaphoreParameterivNV;
GARCH_API extern PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC glGetSemaphoreParameterui64vEXT;
GARCH_API extern PFNGLGETSEPARABLEFILTERPROC glGetSeparableFilter;
GARCH_API extern PFNGLGETSEPARABLEFILTEREXTPROC glGetSeparableFilterEXT;
GARCH_API extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
GARCH_API extern PFNGLGETSHADERPRECISIONFORMATPROC glGetShaderPrecisionFormat;
GARCH_API extern PFNGLGETSHADERSOURCEPROC glGetShaderSource;
GARCH_API extern PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB;
GARCH_API extern PFNGLGETSHADERIVPROC glGetShaderiv;
GARCH_API extern PFNGLGETSHADINGRATEIMAGEPALETTENVPROC glGetShadingRateImagePaletteNV;
GARCH_API extern PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC glGetShadingRateSampleLocationivNV;
GARCH_API extern PFNGLGETSTAGEINDEXNVPROC glGetStageIndexNV;
GARCH_API extern PFNGLGETSTRINGPROC glGetString;
GARCH_API extern PFNGLGETSTRINGIPROC glGetStringi;
GARCH_API extern PFNGLGETSUBROUTINEINDEXPROC glGetSubroutineIndex;
GARCH_API extern PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC glGetSubroutineUniformLocation;
GARCH_API extern PFNGLGETSYNCIVPROC glGetSynciv;
GARCH_API extern PFNGLGETTEXENVFVPROC glGetTexEnvfv;
GARCH_API extern PFNGLGETTEXENVIVPROC glGetTexEnviv;
GARCH_API extern PFNGLGETTEXGENDVPROC glGetTexGendv;
GARCH_API extern PFNGLGETTEXGENFVPROC glGetTexGenfv;
GARCH_API extern PFNGLGETTEXGENIVPROC glGetTexGeniv;
GARCH_API extern PFNGLGETTEXIMAGEPROC glGetTexImage;
GARCH_API extern PFNGLGETTEXLEVELPARAMETERFVPROC glGetTexLevelParameterfv;
GARCH_API extern PFNGLGETTEXLEVELPARAMETERIVPROC glGetTexLevelParameteriv;
GARCH_API extern PFNGLGETTEXPARAMETERIIVPROC glGetTexParameterIiv;
GARCH_API extern PFNGLGETTEXPARAMETERIIVEXTPROC glGetTexParameterIivEXT;
GARCH_API extern PFNGLGETTEXPARAMETERIUIVPROC glGetTexParameterIuiv;
GARCH_API extern PFNGLGETTEXPARAMETERIUIVEXTPROC glGetTexParameterIuivEXT;
GARCH_API extern PFNGLGETTEXPARAMETERPOINTERVAPPLEPROC glGetTexParameterPointervAPPLE;
GARCH_API extern PFNGLGETTEXPARAMETERFVPROC glGetTexParameterfv;
GARCH_API extern PFNGLGETTEXPARAMETERIVPROC glGetTexParameteriv;
GARCH_API extern PFNGLGETTEXTUREHANDLEARBPROC glGetTextureHandleARB;
GARCH_API extern PFNGLGETTEXTUREHANDLENVPROC glGetTextureHandleNV;
GARCH_API extern PFNGLGETTEXTUREIMAGEPROC glGetTextureImage;
GARCH_API extern PFNGLGETTEXTUREIMAGEEXTPROC glGetTextureImageEXT;
GARCH_API extern PFNGLGETTEXTURELEVELPARAMETERFVPROC glGetTextureLevelParameterfv;
GARCH_API extern PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC glGetTextureLevelParameterfvEXT;
GARCH_API extern PFNGLGETTEXTURELEVELPARAMETERIVPROC glGetTextureLevelParameteriv;
GARCH_API extern PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC glGetTextureLevelParameterivEXT;
GARCH_API extern PFNGLGETTEXTUREPARAMETERIIVPROC glGetTextureParameterIiv;
GARCH_API extern PFNGLGETTEXTUREPARAMETERIIVEXTPROC glGetTextureParameterIivEXT;
GARCH_API extern PFNGLGETTEXTUREPARAMETERIUIVPROC glGetTextureParameterIuiv;
GARCH_API extern PFNGLGETTEXTUREPARAMETERIUIVEXTPROC glGetTextureParameterIuivEXT;
GARCH_API extern PFNGLGETTEXTUREPARAMETERFVPROC glGetTextureParameterfv;
GARCH_API extern PFNGLGETTEXTUREPARAMETERFVEXTPROC glGetTextureParameterfvEXT;
GARCH_API extern PFNGLGETTEXTUREPARAMETERIVPROC glGetTextureParameteriv;
GARCH_API extern PFNGLGETTEXTUREPARAMETERIVEXTPROC glGetTextureParameterivEXT;
GARCH_API extern PFNGLGETTEXTURESAMPLERHANDLEARBPROC glGetTextureSamplerHandleARB;
GARCH_API extern PFNGLGETTEXTURESAMPLERHANDLENVPROC glGetTextureSamplerHandleNV;
GARCH_API extern PFNGLGETTEXTURESUBIMAGEPROC glGetTextureSubImage;
GARCH_API extern PFNGLGETTRACKMATRIXIVNVPROC glGetTrackMatrixivNV;
GARCH_API extern PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glGetTransformFeedbackVarying;
GARCH_API extern PFNGLGETTRANSFORMFEEDBACKVARYINGEXTPROC glGetTransformFeedbackVaryingEXT;
GARCH_API extern PFNGLGETTRANSFORMFEEDBACKVARYINGNVPROC glGetTransformFeedbackVaryingNV;
GARCH_API extern PFNGLGETTRANSFORMFEEDBACKI64_VPROC glGetTransformFeedbacki64_v;
GARCH_API extern PFNGLGETTRANSFORMFEEDBACKI_VPROC glGetTransformFeedbacki_v;
GARCH_API extern PFNGLGETTRANSFORMFEEDBACKIVPROC glGetTransformFeedbackiv;
GARCH_API extern PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
GARCH_API extern PFNGLGETUNIFORMBUFFERSIZEEXTPROC glGetUniformBufferSizeEXT;
GARCH_API extern PFNGLGETUNIFORMINDICESPROC glGetUniformIndices;
GARCH_API extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
GARCH_API extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
GARCH_API extern PFNGLGETUNIFORMOFFSETEXTPROC glGetUniformOffsetEXT;
GARCH_API extern PFNGLGETUNIFORMSUBROUTINEUIVPROC glGetUniformSubroutineuiv;
GARCH_API extern PFNGLGETUNIFORMDVPROC glGetUniformdv;
GARCH_API extern PFNGLGETUNIFORMFVPROC glGetUniformfv;
GARCH_API extern PFNGLGETUNIFORMFVARBPROC glGetUniformfvARB;
GARCH_API extern PFNGLGETUNIFORMI64VARBPROC glGetUniformi64vARB;
GARCH_API extern PFNGLGETUNIFORMI64VNVPROC glGetUniformi64vNV;
GARCH_API extern PFNGLGETUNIFORMIVPROC glGetUniformiv;
GARCH_API extern PFNGLGETUNIFORMIVARBPROC glGetUniformivARB;
GARCH_API extern PFNGLGETUNIFORMUI64VARBPROC glGetUniformui64vARB;
GARCH_API extern PFNGLGETUNIFORMUI64VNVPROC glGetUniformui64vNV;
GARCH_API extern PFNGLGETUNIFORMUIVPROC glGetUniformuiv;
GARCH_API extern PFNGLGETUNIFORMUIVEXTPROC glGetUniformuivEXT;
GARCH_API extern PFNGLGETUNSIGNEDBYTEVEXTPROC glGetUnsignedBytevEXT;
GARCH_API extern PFNGLGETUNSIGNEDBYTEI_VEXTPROC glGetUnsignedBytei_vEXT;
GARCH_API extern PFNGLGETVARIANTBOOLEANVEXTPROC glGetVariantBooleanvEXT;
GARCH_API extern PFNGLGETVARIANTFLOATVEXTPROC glGetVariantFloatvEXT;
GARCH_API extern PFNGLGETVARIANTINTEGERVEXTPROC glGetVariantIntegervEXT;
GARCH_API extern PFNGLGETVARIANTPOINTERVEXTPROC glGetVariantPointervEXT;
GARCH_API extern PFNGLGETVARYINGLOCATIONNVPROC glGetVaryingLocationNV;
GARCH_API extern PFNGLGETVERTEXARRAYINDEXED64IVPROC glGetVertexArrayIndexed64iv;
GARCH_API extern PFNGLGETVERTEXARRAYINDEXEDIVPROC glGetVertexArrayIndexediv;
GARCH_API extern PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC glGetVertexArrayIntegeri_vEXT;
GARCH_API extern PFNGLGETVERTEXARRAYINTEGERVEXTPROC glGetVertexArrayIntegervEXT;
GARCH_API extern PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC glGetVertexArrayPointeri_vEXT;
GARCH_API extern PFNGLGETVERTEXARRAYPOINTERVEXTPROC glGetVertexArrayPointervEXT;
GARCH_API extern PFNGLGETVERTEXARRAYIVPROC glGetVertexArrayiv;
GARCH_API extern PFNGLGETVERTEXATTRIBIIVPROC glGetVertexAttribIiv;
GARCH_API extern PFNGLGETVERTEXATTRIBIIVEXTPROC glGetVertexAttribIivEXT;
GARCH_API extern PFNGLGETVERTEXATTRIBIUIVPROC glGetVertexAttribIuiv;
GARCH_API extern PFNGLGETVERTEXATTRIBIUIVEXTPROC glGetVertexAttribIuivEXT;
GARCH_API extern PFNGLGETVERTEXATTRIBLDVPROC glGetVertexAttribLdv;
GARCH_API extern PFNGLGETVERTEXATTRIBLDVEXTPROC glGetVertexAttribLdvEXT;
GARCH_API extern PFNGLGETVERTEXATTRIBLI64VNVPROC glGetVertexAttribLi64vNV;
GARCH_API extern PFNGLGETVERTEXATTRIBLUI64VARBPROC glGetVertexAttribLui64vARB;
GARCH_API extern PFNGLGETVERTEXATTRIBLUI64VNVPROC glGetVertexAttribLui64vNV;
GARCH_API extern PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv;
GARCH_API extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC glGetVertexAttribPointervARB;
GARCH_API extern PFNGLGETVERTEXATTRIBPOINTERVNVPROC glGetVertexAttribPointervNV;
GARCH_API extern PFNGLGETVERTEXATTRIBDVPROC glGetVertexAttribdv;
GARCH_API extern PFNGLGETVERTEXATTRIBDVARBPROC glGetVertexAttribdvARB;
GARCH_API extern PFNGLGETVERTEXATTRIBDVNVPROC glGetVertexAttribdvNV;
GARCH_API extern PFNGLGETVERTEXATTRIBFVPROC glGetVertexAttribfv;
GARCH_API extern PFNGLGETVERTEXATTRIBFVARBPROC glGetVertexAttribfvARB;
GARCH_API extern PFNGLGETVERTEXATTRIBFVNVPROC glGetVertexAttribfvNV;
GARCH_API extern PFNGLGETVERTEXATTRIBIVPROC glGetVertexAttribiv;
GARCH_API extern PFNGLGETVERTEXATTRIBIVARBPROC glGetVertexAttribivARB;
GARCH_API extern PFNGLGETVERTEXATTRIBIVNVPROC glGetVertexAttribivNV;
GARCH_API extern PFNGLGETVIDEOCAPTURESTREAMDVNVPROC glGetVideoCaptureStreamdvNV;
GARCH_API extern PFNGLGETVIDEOCAPTURESTREAMFVNVPROC glGetVideoCaptureStreamfvNV;
GARCH_API extern PFNGLGETVIDEOCAPTURESTREAMIVNVPROC glGetVideoCaptureStreamivNV;
GARCH_API extern PFNGLGETVIDEOCAPTUREIVNVPROC glGetVideoCaptureivNV;
GARCH_API extern PFNGLGETVIDEOI64VNVPROC glGetVideoi64vNV;
GARCH_API extern PFNGLGETVIDEOIVNVPROC glGetVideoivNV;
GARCH_API extern PFNGLGETVIDEOUI64VNVPROC glGetVideoui64vNV;
GARCH_API extern PFNGLGETVIDEOUIVNVPROC glGetVideouivNV;
GARCH_API extern PFNGLGETNCOLORTABLEPROC glGetnColorTable;
GARCH_API extern PFNGLGETNCOLORTABLEARBPROC glGetnColorTableARB;
GARCH_API extern PFNGLGETNCOMPRESSEDTEXIMAGEPROC glGetnCompressedTexImage;
GARCH_API extern PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC glGetnCompressedTexImageARB;
GARCH_API extern PFNGLGETNCONVOLUTIONFILTERPROC glGetnConvolutionFilter;
GARCH_API extern PFNGLGETNCONVOLUTIONFILTERARBPROC glGetnConvolutionFilterARB;
GARCH_API extern PFNGLGETNHISTOGRAMPROC glGetnHistogram;
GARCH_API extern PFNGLGETNHISTOGRAMARBPROC glGetnHistogramARB;
GARCH_API extern PFNGLGETNMAPDVPROC glGetnMapdv;
GARCH_API extern PFNGLGETNMAPDVARBPROC glGetnMapdvARB;
GARCH_API extern PFNGLGETNMAPFVPROC glGetnMapfv;
GARCH_API extern PFNGLGETNMAPFVARBPROC glGetnMapfvARB;
GARCH_API extern PFNGLGETNMAPIVPROC glGetnMapiv;
GARCH_API extern PFNGLGETNMAPIVARBPROC glGetnMapivARB;
GARCH_API extern PFNGLGETNMINMAXPROC glGetnMinmax;
GARCH_API extern PFNGLGETNMINMAXARBPROC glGetnMinmaxARB;
GARCH_API extern PFNGLGETNPIXELMAPFVPROC glGetnPixelMapfv;
GARCH_API extern PFNGLGETNPIXELMAPFVARBPROC glGetnPixelMapfvARB;
GARCH_API extern PFNGLGETNPIXELMAPUIVPROC glGetnPixelMapuiv;
GARCH_API extern PFNGLGETNPIXELMAPUIVARBPROC glGetnPixelMapuivARB;
GARCH_API extern PFNGLGETNPIXELMAPUSVPROC glGetnPixelMapusv;
GARCH_API extern PFNGLGETNPIXELMAPUSVARBPROC glGetnPixelMapusvARB;
GARCH_API extern PFNGLGETNPOLYGONSTIPPLEPROC glGetnPolygonStipple;
GARCH_API extern PFNGLGETNPOLYGONSTIPPLEARBPROC glGetnPolygonStippleARB;
GARCH_API extern PFNGLGETNSEPARABLEFILTERPROC glGetnSeparableFilter;
GARCH_API extern PFNGLGETNSEPARABLEFILTERARBPROC glGetnSeparableFilterARB;
GARCH_API extern PFNGLGETNTEXIMAGEPROC glGetnTexImage;
GARCH_API extern PFNGLGETNTEXIMAGEARBPROC glGetnTexImageARB;
GARCH_API extern PFNGLGETNUNIFORMDVPROC glGetnUniformdv;
GARCH_API extern PFNGLGETNUNIFORMDVARBPROC glGetnUniformdvARB;
GARCH_API extern PFNGLGETNUNIFORMFVPROC glGetnUniformfv;
GARCH_API extern PFNGLGETNUNIFORMFVARBPROC glGetnUniformfvARB;
GARCH_API extern PFNGLGETNUNIFORMFVKHRPROC glGetnUniformfvKHR;
GARCH_API extern PFNGLGETNUNIFORMI64VARBPROC glGetnUniformi64vARB;
GARCH_API extern PFNGLGETNUNIFORMIVPROC glGetnUniformiv;
GARCH_API extern PFNGLGETNUNIFORMIVARBPROC glGetnUniformivARB;
GARCH_API extern PFNGLGETNUNIFORMIVKHRPROC glGetnUniformivKHR;
GARCH_API extern PFNGLGETNUNIFORMUI64VARBPROC glGetnUniformui64vARB;
GARCH_API extern PFNGLGETNUNIFORMUIVPROC glGetnUniformuiv;
GARCH_API extern PFNGLGETNUNIFORMUIVARBPROC glGetnUniformuivARB;
GARCH_API extern PFNGLGETNUNIFORMUIVKHRPROC glGetnUniformuivKHR;
GARCH_API extern PFNGLHINTPROC glHint;
GARCH_API extern PFNGLHISTOGRAMPROC glHistogram;
GARCH_API extern PFNGLHISTOGRAMEXTPROC glHistogramEXT;
GARCH_API extern PFNGLIMPORTMEMORYFDEXTPROC glImportMemoryFdEXT;
GARCH_API extern PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC glImportMemoryWin32HandleEXT;
GARCH_API extern PFNGLIMPORTMEMORYWIN32NAMEEXTPROC glImportMemoryWin32NameEXT;
GARCH_API extern PFNGLIMPORTSEMAPHOREFDEXTPROC glImportSemaphoreFdEXT;
GARCH_API extern PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC glImportSemaphoreWin32HandleEXT;
GARCH_API extern PFNGLIMPORTSEMAPHOREWIN32NAMEEXTPROC glImportSemaphoreWin32NameEXT;
GARCH_API extern PFNGLIMPORTSYNCEXTPROC glImportSyncEXT;
GARCH_API extern PFNGLINDEXFORMATNVPROC glIndexFormatNV;
GARCH_API extern PFNGLINDEXFUNCEXTPROC glIndexFuncEXT;
GARCH_API extern PFNGLINDEXMASKPROC glIndexMask;
GARCH_API extern PFNGLINDEXMATERIALEXTPROC glIndexMaterialEXT;
GARCH_API extern PFNGLINDEXPOINTERPROC glIndexPointer;
GARCH_API extern PFNGLINDEXPOINTEREXTPROC glIndexPointerEXT;
GARCH_API extern PFNGLINDEXDPROC glIndexd;
GARCH_API extern PFNGLINDEXDVPROC glIndexdv;
GARCH_API extern PFNGLINDEXFPROC glIndexf;
GARCH_API extern PFNGLINDEXFVPROC glIndexfv;
GARCH_API extern PFNGLINDEXIPROC glIndexi;
GARCH_API extern PFNGLINDEXIVPROC glIndexiv;
GARCH_API extern PFNGLINDEXSPROC glIndexs;
GARCH_API extern PFNGLINDEXSVPROC glIndexsv;
GARCH_API extern PFNGLINDEXUBPROC glIndexub;
GARCH_API extern PFNGLINDEXUBVPROC glIndexubv;
GARCH_API extern PFNGLINITNAMESPROC glInitNames;
GARCH_API extern PFNGLINSERTCOMPONENTEXTPROC glInsertComponentEXT;
GARCH_API extern PFNGLINSERTEVENTMARKEREXTPROC glInsertEventMarkerEXT;
GARCH_API extern PFNGLINTERLEAVEDARRAYSPROC glInterleavedArrays;
GARCH_API extern PFNGLINTERPOLATEPATHSNVPROC glInterpolatePathsNV;
GARCH_API extern PFNGLINVALIDATEBUFFERDATAPROC glInvalidateBufferData;
GARCH_API extern PFNGLINVALIDATEBUFFERSUBDATAPROC glInvalidateBufferSubData;
GARCH_API extern PFNGLINVALIDATEFRAMEBUFFERPROC glInvalidateFramebuffer;
GARCH_API extern PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC glInvalidateNamedFramebufferData;
GARCH_API extern PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC glInvalidateNamedFramebufferSubData;
GARCH_API extern PFNGLINVALIDATESUBFRAMEBUFFERPROC glInvalidateSubFramebuffer;
GARCH_API extern PFNGLINVALIDATETEXIMAGEPROC glInvalidateTexImage;
GARCH_API extern PFNGLINVALIDATETEXSUBIMAGEPROC glInvalidateTexSubImage;
GARCH_API extern PFNGLISBUFFERPROC glIsBuffer;
GARCH_API extern PFNGLISBUFFERARBPROC glIsBufferARB;
GARCH_API extern PFNGLISBUFFERRESIDENTNVPROC glIsBufferResidentNV;
GARCH_API extern PFNGLISCOMMANDLISTNVPROC glIsCommandListNV;
GARCH_API extern PFNGLISENABLEDPROC glIsEnabled;
GARCH_API extern PFNGLISENABLEDINDEXEDEXTPROC glIsEnabledIndexedEXT;
GARCH_API extern PFNGLISENABLEDIPROC glIsEnabledi;
GARCH_API extern PFNGLISFENCEAPPLEPROC glIsFenceAPPLE;
GARCH_API extern PFNGLISFENCENVPROC glIsFenceNV;
GARCH_API extern PFNGLISFRAMEBUFFERPROC glIsFramebuffer;
GARCH_API extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
GARCH_API extern PFNGLISIMAGEHANDLERESIDENTARBPROC glIsImageHandleResidentARB;
GARCH_API extern PFNGLISIMAGEHANDLERESIDENTNVPROC glIsImageHandleResidentNV;
GARCH_API extern PFNGLISLISTPROC glIsList;
GARCH_API extern PFNGLISMEMORYOBJECTEXTPROC glIsMemoryObjectEXT;
GARCH_API extern PFNGLISNAMEAMDPROC glIsNameAMD;
GARCH_API extern PFNGLISNAMEDBUFFERRESIDENTNVPROC glIsNamedBufferResidentNV;
GARCH_API extern PFNGLISNAMEDSTRINGARBPROC glIsNamedStringARB;
GARCH_API extern PFNGLISOCCLUSIONQUERYNVPROC glIsOcclusionQueryNV;
GARCH_API extern PFNGLISPATHNVPROC glIsPathNV;
GARCH_API extern PFNGLISPOINTINFILLPATHNVPROC glIsPointInFillPathNV;
GARCH_API extern PFNGLISPOINTINSTROKEPATHNVPROC glIsPointInStrokePathNV;
GARCH_API extern PFNGLISPROGRAMPROC glIsProgram;
GARCH_API extern PFNGLISPROGRAMARBPROC glIsProgramARB;
GARCH_API extern PFNGLISPROGRAMNVPROC glIsProgramNV;
GARCH_API extern PFNGLISPROGRAMPIPELINEPROC glIsProgramPipeline;
GARCH_API extern PFNGLISPROGRAMPIPELINEEXTPROC glIsProgramPipelineEXT;
GARCH_API extern PFNGLISQUERYPROC glIsQuery;
GARCH_API extern PFNGLISQUERYARBPROC glIsQueryARB;
GARCH_API extern PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
GARCH_API extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
GARCH_API extern PFNGLISSEMAPHOREEXTPROC glIsSemaphoreEXT;
GARCH_API extern PFNGLISSAMPLERPROC glIsSampler;
GARCH_API extern PFNGLISSHADERPROC glIsShader;
GARCH_API extern PFNGLISSTATENVPROC glIsStateNV;
GARCH_API extern PFNGLISSYNCPROC glIsSync;
GARCH_API extern PFNGLISTEXTUREPROC glIsTexture;
GARCH_API extern PFNGLISTEXTUREEXTPROC glIsTextureEXT;
GARCH_API extern PFNGLISTEXTUREHANDLERESIDENTARBPROC glIsTextureHandleResidentARB;
GARCH_API extern PFNGLISTEXTUREHANDLERESIDENTNVPROC glIsTextureHandleResidentNV;
GARCH_API extern PFNGLISTRANSFORMFEEDBACKPROC glIsTransformFeedback;
GARCH_API extern PFNGLISTRANSFORMFEEDBACKNVPROC glIsTransformFeedbackNV;
GARCH_API extern PFNGLISVARIANTENABLEDEXTPROC glIsVariantEnabledEXT;
GARCH_API extern PFNGLISVERTEXARRAYPROC glIsVertexArray;
GARCH_API extern PFNGLISVERTEXARRAYAPPLEPROC glIsVertexArrayAPPLE;
GARCH_API extern PFNGLISVERTEXATTRIBENABLEDAPPLEPROC glIsVertexAttribEnabledAPPLE;
GARCH_API extern PFNGLLGPUCOPYIMAGESUBDATANVXPROC glLGPUCopyImageSubDataNVX;
GARCH_API extern PFNGLLGPUINTERLOCKNVXPROC glLGPUInterlockNVX;
GARCH_API extern PFNGLLGPUNAMEDBUFFERSUBDATANVXPROC glLGPUNamedBufferSubDataNVX;
GARCH_API extern PFNGLLABELOBJECTEXTPROC glLabelObjectEXT;
GARCH_API extern PFNGLLIGHTMODELFPROC glLightModelf;
GARCH_API extern PFNGLLIGHTMODELFVPROC glLightModelfv;
GARCH_API extern PFNGLLIGHTMODELIPROC glLightModeli;
GARCH_API extern PFNGLLIGHTMODELIVPROC glLightModeliv;
GARCH_API extern PFNGLLIGHTFPROC glLightf;
GARCH_API extern PFNGLLIGHTFVPROC glLightfv;
GARCH_API extern PFNGLLIGHTIPROC glLighti;
GARCH_API extern PFNGLLIGHTIVPROC glLightiv;
GARCH_API extern PFNGLLINESTIPPLEPROC glLineStipple;
GARCH_API extern PFNGLLINEWIDTHPROC glLineWidth;
GARCH_API extern PFNGLLINKPROGRAMPROC glLinkProgram;
GARCH_API extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
GARCH_API extern PFNGLLISTBASEPROC glListBase;
GARCH_API extern PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC glListDrawCommandsStatesClientNV;
GARCH_API extern PFNGLLOADIDENTITYPROC glLoadIdentity;
GARCH_API extern PFNGLLOADMATRIXDPROC glLoadMatrixd;
GARCH_API extern PFNGLLOADMATRIXFPROC glLoadMatrixf;
GARCH_API extern PFNGLLOADNAMEPROC glLoadName;
GARCH_API extern PFNGLLOADPROGRAMNVPROC glLoadProgramNV;
GARCH_API extern PFNGLLOADTRANSPOSEMATRIXDPROC glLoadTransposeMatrixd;
GARCH_API extern PFNGLLOADTRANSPOSEMATRIXDARBPROC glLoadTransposeMatrixdARB;
GARCH_API extern PFNGLLOADTRANSPOSEMATRIXFPROC glLoadTransposeMatrixf;
GARCH_API extern PFNGLLOADTRANSPOSEMATRIXFARBPROC glLoadTransposeMatrixfARB;
GARCH_API extern PFNGLLOCKARRAYSEXTPROC glLockArraysEXT;
GARCH_API extern PFNGLLOGICOPPROC glLogicOp;
GARCH_API extern PFNGLMAKEBUFFERNONRESIDENTNVPROC glMakeBufferNonResidentNV;
GARCH_API extern PFNGLMAKEBUFFERRESIDENTNVPROC glMakeBufferResidentNV;
GARCH_API extern PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC glMakeImageHandleNonResidentARB;
GARCH_API extern PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC glMakeImageHandleNonResidentNV;
GARCH_API extern PFNGLMAKEIMAGEHANDLERESIDENTARBPROC glMakeImageHandleResidentARB;
GARCH_API extern PFNGLMAKEIMAGEHANDLERESIDENTNVPROC glMakeImageHandleResidentNV;
GARCH_API extern PFNGLMAKENAMEDBUFFERNONRESIDENTNVPROC glMakeNamedBufferNonResidentNV;
GARCH_API extern PFNGLMAKENAMEDBUFFERRESIDENTNVPROC glMakeNamedBufferResidentNV;
GARCH_API extern PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC glMakeTextureHandleNonResidentARB;
GARCH_API extern PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC glMakeTextureHandleNonResidentNV;
GARCH_API extern PFNGLMAKETEXTUREHANDLERESIDENTARBPROC glMakeTextureHandleResidentARB;
GARCH_API extern PFNGLMAKETEXTUREHANDLERESIDENTNVPROC glMakeTextureHandleResidentNV;
GARCH_API extern PFNGLMAP1DPROC glMap1d;
GARCH_API extern PFNGLMAP1FPROC glMap1f;
GARCH_API extern PFNGLMAP2DPROC glMap2d;
GARCH_API extern PFNGLMAP2FPROC glMap2f;
GARCH_API extern PFNGLMAPBUFFERPROC glMapBuffer;
GARCH_API extern PFNGLMAPBUFFERARBPROC glMapBufferARB;
GARCH_API extern PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
GARCH_API extern PFNGLMAPCONTROLPOINTSNVPROC glMapControlPointsNV;
GARCH_API extern PFNGLMAPGRID1DPROC glMapGrid1d;
GARCH_API extern PFNGLMAPGRID1FPROC glMapGrid1f;
GARCH_API extern PFNGLMAPGRID2DPROC glMapGrid2d;
GARCH_API extern PFNGLMAPGRID2FPROC glMapGrid2f;
GARCH_API extern PFNGLMAPNAMEDBUFFERPROC glMapNamedBuffer;
GARCH_API extern PFNGLMAPNAMEDBUFFEREXTPROC glMapNamedBufferEXT;
GARCH_API extern PFNGLMAPNAMEDBUFFERRANGEPROC glMapNamedBufferRange;
GARCH_API extern PFNGLMAPNAMEDBUFFERRANGEEXTPROC glMapNamedBufferRangeEXT;
GARCH_API extern PFNGLMAPPARAMETERFVNVPROC glMapParameterfvNV;
GARCH_API extern PFNGLMAPPARAMETERIVNVPROC glMapParameterivNV;
GARCH_API extern PFNGLMAPTEXTURE2DINTELPROC glMapTexture2DINTEL;
GARCH_API extern PFNGLMAPVERTEXATTRIB1DAPPLEPROC glMapVertexAttrib1dAPPLE;
GARCH_API extern PFNGLMAPVERTEXATTRIB1FAPPLEPROC glMapVertexAttrib1fAPPLE;
GARCH_API extern PFNGLMAPVERTEXATTRIB2DAPPLEPROC glMapVertexAttrib2dAPPLE;
GARCH_API extern PFNGLMAPVERTEXATTRIB2FAPPLEPROC glMapVertexAttrib2fAPPLE;
GARCH_API extern PFNGLMATERIALFPROC glMaterialf;
GARCH_API extern PFNGLMATERIALFVPROC glMaterialfv;
GARCH_API extern PFNGLMATERIALIPROC glMateriali;
GARCH_API extern PFNGLMATERIALIVPROC glMaterialiv;
GARCH_API extern PFNGLMATRIXFRUSTUMEXTPROC glMatrixFrustumEXT;
GARCH_API extern PFNGLMATRIXINDEXPOINTERARBPROC glMatrixIndexPointerARB;
GARCH_API extern PFNGLMATRIXINDEXUBVARBPROC glMatrixIndexubvARB;
GARCH_API extern PFNGLMATRIXINDEXUIVARBPROC glMatrixIndexuivARB;
GARCH_API extern PFNGLMATRIXINDEXUSVARBPROC glMatrixIndexusvARB;
GARCH_API extern PFNGLMATRIXLOAD3X2FNVPROC glMatrixLoad3x2fNV;
GARCH_API extern PFNGLMATRIXLOAD3X3FNVPROC glMatrixLoad3x3fNV;
GARCH_API extern PFNGLMATRIXLOADIDENTITYEXTPROC glMatrixLoadIdentityEXT;
GARCH_API extern PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC glMatrixLoadTranspose3x3fNV;
GARCH_API extern PFNGLMATRIXLOADTRANSPOSEDEXTPROC glMatrixLoadTransposedEXT;
GARCH_API extern PFNGLMATRIXLOADTRANSPOSEFEXTPROC glMatrixLoadTransposefEXT;
GARCH_API extern PFNGLMATRIXLOADDEXTPROC glMatrixLoaddEXT;
GARCH_API extern PFNGLMATRIXLOADFEXTPROC glMatrixLoadfEXT;
GARCH_API extern PFNGLMATRIXMODEPROC glMatrixMode;
GARCH_API extern PFNGLMATRIXMULT3X2FNVPROC glMatrixMult3x2fNV;
GARCH_API extern PFNGLMATRIXMULT3X3FNVPROC glMatrixMult3x3fNV;
GARCH_API extern PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC glMatrixMultTranspose3x3fNV;
GARCH_API extern PFNGLMATRIXMULTTRANSPOSEDEXTPROC glMatrixMultTransposedEXT;
GARCH_API extern PFNGLMATRIXMULTTRANSPOSEFEXTPROC glMatrixMultTransposefEXT;
GARCH_API extern PFNGLMATRIXMULTDEXTPROC glMatrixMultdEXT;
GARCH_API extern PFNGLMATRIXMULTFEXTPROC glMatrixMultfEXT;
GARCH_API extern PFNGLMATRIXORTHOEXTPROC glMatrixOrthoEXT;
GARCH_API extern PFNGLMATRIXPOPEXTPROC glMatrixPopEXT;
GARCH_API extern PFNGLMATRIXPUSHEXTPROC glMatrixPushEXT;
GARCH_API extern PFNGLMATRIXROTATEDEXTPROC glMatrixRotatedEXT;
GARCH_API extern PFNGLMATRIXROTATEFEXTPROC glMatrixRotatefEXT;
GARCH_API extern PFNGLMATRIXSCALEDEXTPROC glMatrixScaledEXT;
GARCH_API extern PFNGLMATRIXSCALEFEXTPROC glMatrixScalefEXT;
GARCH_API extern PFNGLMATRIXTRANSLATEDEXTPROC glMatrixTranslatedEXT;
GARCH_API extern PFNGLMATRIXTRANSLATEFEXTPROC glMatrixTranslatefEXT;
GARCH_API extern PFNGLMAXSHADERCOMPILERTHREADSKHRPROC glMaxShaderCompilerThreadsKHR;
GARCH_API extern PFNGLMAXSHADERCOMPILERTHREADSARBPROC glMaxShaderCompilerThreadsARB;
GARCH_API extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;
GARCH_API extern PFNGLMEMORYBARRIERBYREGIONPROC glMemoryBarrierByRegion;
GARCH_API extern PFNGLMEMORYBARRIEREXTPROC glMemoryBarrierEXT;
GARCH_API extern PFNGLMEMORYOBJECTPARAMETERIVEXTPROC glMemoryObjectParameterivEXT;
GARCH_API extern PFNGLMINSAMPLESHADINGPROC glMinSampleShading;
GARCH_API extern PFNGLMINSAMPLESHADINGARBPROC glMinSampleShadingARB;
GARCH_API extern PFNGLMINMAXPROC glMinmax;
GARCH_API extern PFNGLMINMAXEXTPROC glMinmaxEXT;
GARCH_API extern PFNGLMULTMATRIXDPROC glMultMatrixd;
GARCH_API extern PFNGLMULTMATRIXFPROC glMultMatrixf;
GARCH_API extern PFNGLMULTTRANSPOSEMATRIXDPROC glMultTransposeMatrixd;
GARCH_API extern PFNGLMULTTRANSPOSEMATRIXDARBPROC glMultTransposeMatrixdARB;
GARCH_API extern PFNGLMULTTRANSPOSEMATRIXFPROC glMultTransposeMatrixf;
GARCH_API extern PFNGLMULTTRANSPOSEMATRIXFARBPROC glMultTransposeMatrixfARB;
GARCH_API extern PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays;
GARCH_API extern PFNGLMULTIDRAWARRAYSEXTPROC glMultiDrawArraysEXT;
GARCH_API extern PFNGLMULTIDRAWARRAYSINDIRECTPROC glMultiDrawArraysIndirect;
GARCH_API extern PFNGLMULTIDRAWARRAYSINDIRECTAMDPROC glMultiDrawArraysIndirectAMD;
GARCH_API extern PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNVPROC glMultiDrawArraysIndirectBindlessCountNV;
GARCH_API extern PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSNVPROC glMultiDrawArraysIndirectBindlessNV;
GARCH_API extern PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC glMultiDrawArraysIndirectCount;
GARCH_API extern PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC glMultiDrawArraysIndirectCountARB;
GARCH_API extern PFNGLMULTIDRAWELEMENTARRAYAPPLEPROC glMultiDrawElementArrayAPPLE;
GARCH_API extern PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements;
GARCH_API extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glMultiDrawElementsBaseVertex;
GARCH_API extern PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElementsEXT;
GARCH_API extern PFNGLMULTIDRAWELEMENTSINDIRECTPROC glMultiDrawElementsIndirect;
GARCH_API extern PFNGLMULTIDRAWELEMENTSINDIRECTAMDPROC glMultiDrawElementsIndirectAMD;
GARCH_API extern PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNVPROC glMultiDrawElementsIndirectBindlessCountNV;
GARCH_API extern PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSNVPROC glMultiDrawElementsIndirectBindlessNV;
GARCH_API extern PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC glMultiDrawElementsIndirectCount;
GARCH_API extern PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC glMultiDrawElementsIndirectCountARB;
GARCH_API extern PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC glMultiDrawMeshTasksIndirectNV;
GARCH_API extern PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC glMultiDrawMeshTasksIndirectCountNV;
GARCH_API extern PFNGLMULTIDRAWRANGEELEMENTARRAYAPPLEPROC glMultiDrawRangeElementArrayAPPLE;
GARCH_API extern PFNGLMULTITEXBUFFEREXTPROC glMultiTexBufferEXT;
GARCH_API extern PFNGLMULTITEXCOORD1DPROC glMultiTexCoord1d;
GARCH_API extern PFNGLMULTITEXCOORD1DARBPROC glMultiTexCoord1dARB;
GARCH_API extern PFNGLMULTITEXCOORD1DVPROC glMultiTexCoord1dv;
GARCH_API extern PFNGLMULTITEXCOORD1DVARBPROC glMultiTexCoord1dvARB;
GARCH_API extern PFNGLMULTITEXCOORD1FPROC glMultiTexCoord1f;
GARCH_API extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;
GARCH_API extern PFNGLMULTITEXCOORD1FVPROC glMultiTexCoord1fv;
GARCH_API extern PFNGLMULTITEXCOORD1FVARBPROC glMultiTexCoord1fvARB;
GARCH_API extern PFNGLMULTITEXCOORD1HNVPROC glMultiTexCoord1hNV;
GARCH_API extern PFNGLMULTITEXCOORD1HVNVPROC glMultiTexCoord1hvNV;
GARCH_API extern PFNGLMULTITEXCOORD1IPROC glMultiTexCoord1i;
GARCH_API extern PFNGLMULTITEXCOORD1IARBPROC glMultiTexCoord1iARB;
GARCH_API extern PFNGLMULTITEXCOORD1IVPROC glMultiTexCoord1iv;
GARCH_API extern PFNGLMULTITEXCOORD1IVARBPROC glMultiTexCoord1ivARB;
GARCH_API extern PFNGLMULTITEXCOORD1SPROC glMultiTexCoord1s;
GARCH_API extern PFNGLMULTITEXCOORD1SARBPROC glMultiTexCoord1sARB;
GARCH_API extern PFNGLMULTITEXCOORD1SVPROC glMultiTexCoord1sv;
GARCH_API extern PFNGLMULTITEXCOORD1SVARBPROC glMultiTexCoord1svARB;
GARCH_API extern PFNGLMULTITEXCOORD2DPROC glMultiTexCoord2d;
GARCH_API extern PFNGLMULTITEXCOORD2DARBPROC glMultiTexCoord2dARB;
GARCH_API extern PFNGLMULTITEXCOORD2DVPROC glMultiTexCoord2dv;
GARCH_API extern PFNGLMULTITEXCOORD2DVARBPROC glMultiTexCoord2dvARB;
GARCH_API extern PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;
GARCH_API extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
GARCH_API extern PFNGLMULTITEXCOORD2FVPROC glMultiTexCoord2fv;
GARCH_API extern PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB;
GARCH_API extern PFNGLMULTITEXCOORD2HNVPROC glMultiTexCoord2hNV;
GARCH_API extern PFNGLMULTITEXCOORD2HVNVPROC glMultiTexCoord2hvNV;
GARCH_API extern PFNGLMULTITEXCOORD2IPROC glMultiTexCoord2i;
GARCH_API extern PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
GARCH_API extern PFNGLMULTITEXCOORD2IVPROC glMultiTexCoord2iv;
GARCH_API extern PFNGLMULTITEXCOORD2IVARBPROC glMultiTexCoord2ivARB;
GARCH_API extern PFNGLMULTITEXCOORD2SPROC glMultiTexCoord2s;
GARCH_API extern PFNGLMULTITEXCOORD2SARBPROC glMultiTexCoord2sARB;
GARCH_API extern PFNGLMULTITEXCOORD2SVPROC glMultiTexCoord2sv;
GARCH_API extern PFNGLMULTITEXCOORD2SVARBPROC glMultiTexCoord2svARB;
GARCH_API extern PFNGLMULTITEXCOORD3DPROC glMultiTexCoord3d;
GARCH_API extern PFNGLMULTITEXCOORD3DARBPROC glMultiTexCoord3dARB;
GARCH_API extern PFNGLMULTITEXCOORD3DVPROC glMultiTexCoord3dv;
GARCH_API extern PFNGLMULTITEXCOORD3DVARBPROC glMultiTexCoord3dvARB;
GARCH_API extern PFNGLMULTITEXCOORD3FPROC glMultiTexCoord3f;
GARCH_API extern PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
GARCH_API extern PFNGLMULTITEXCOORD3FVPROC glMultiTexCoord3fv;
GARCH_API extern PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
GARCH_API extern PFNGLMULTITEXCOORD3HNVPROC glMultiTexCoord3hNV;
GARCH_API extern PFNGLMULTITEXCOORD3HVNVPROC glMultiTexCoord3hvNV;
GARCH_API extern PFNGLMULTITEXCOORD3IPROC glMultiTexCoord3i;
GARCH_API extern PFNGLMULTITEXCOORD3IARBPROC glMultiTexCoord3iARB;
GARCH_API extern PFNGLMULTITEXCOORD3IVPROC glMultiTexCoord3iv;
GARCH_API extern PFNGLMULTITEXCOORD3IVARBPROC glMultiTexCoord3ivARB;
GARCH_API extern PFNGLMULTITEXCOORD3SPROC glMultiTexCoord3s;
GARCH_API extern PFNGLMULTITEXCOORD3SARBPROC glMultiTexCoord3sARB;
GARCH_API extern PFNGLMULTITEXCOORD3SVPROC glMultiTexCoord3sv;
GARCH_API extern PFNGLMULTITEXCOORD3SVARBPROC glMultiTexCoord3svARB;
GARCH_API extern PFNGLMULTITEXCOORD4DPROC glMultiTexCoord4d;
GARCH_API extern PFNGLMULTITEXCOORD4DARBPROC glMultiTexCoord4dARB;
GARCH_API extern PFNGLMULTITEXCOORD4DVPROC glMultiTexCoord4dv;
GARCH_API extern PFNGLMULTITEXCOORD4DVARBPROC glMultiTexCoord4dvARB;
GARCH_API extern PFNGLMULTITEXCOORD4FPROC glMultiTexCoord4f;
GARCH_API extern PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
GARCH_API extern PFNGLMULTITEXCOORD4FVPROC glMultiTexCoord4fv;
GARCH_API extern PFNGLMULTITEXCOORD4FVARBPROC glMultiTexCoord4fvARB;
GARCH_API extern PFNGLMULTITEXCOORD4HNVPROC glMultiTexCoord4hNV;
GARCH_API extern PFNGLMULTITEXCOORD4HVNVPROC glMultiTexCoord4hvNV;
GARCH_API extern PFNGLMULTITEXCOORD4IPROC glMultiTexCoord4i;
GARCH_API extern PFNGLMULTITEXCOORD4IARBPROC glMultiTexCoord4iARB;
GARCH_API extern PFNGLMULTITEXCOORD4IVPROC glMultiTexCoord4iv;
GARCH_API extern PFNGLMULTITEXCOORD4IVARBPROC glMultiTexCoord4ivARB;
GARCH_API extern PFNGLMULTITEXCOORD4SPROC glMultiTexCoord4s;
GARCH_API extern PFNGLMULTITEXCOORD4SARBPROC glMultiTexCoord4sARB;
GARCH_API extern PFNGLMULTITEXCOORD4SVPROC glMultiTexCoord4sv;
GARCH_API extern PFNGLMULTITEXCOORD4SVARBPROC glMultiTexCoord4svARB;
GARCH_API extern PFNGLMULTITEXCOORDP1UIPROC glMultiTexCoordP1ui;
GARCH_API extern PFNGLMULTITEXCOORDP1UIVPROC glMultiTexCoordP1uiv;
GARCH_API extern PFNGLMULTITEXCOORDP2UIPROC glMultiTexCoordP2ui;
GARCH_API extern PFNGLMULTITEXCOORDP2UIVPROC glMultiTexCoordP2uiv;
GARCH_API extern PFNGLMULTITEXCOORDP3UIPROC glMultiTexCoordP3ui;
GARCH_API extern PFNGLMULTITEXCOORDP3UIVPROC glMultiTexCoordP3uiv;
GARCH_API extern PFNGLMULTITEXCOORDP4UIPROC glMultiTexCoordP4ui;
GARCH_API extern PFNGLMULTITEXCOORDP4UIVPROC glMultiTexCoordP4uiv;
GARCH_API extern PFNGLMULTITEXCOORDPOINTEREXTPROC glMultiTexCoordPointerEXT;
GARCH_API extern PFNGLMULTITEXENVFEXTPROC glMultiTexEnvfEXT;
GARCH_API extern PFNGLMULTITEXENVFVEXTPROC glMultiTexEnvfvEXT;
GARCH_API extern PFNGLMULTITEXENVIEXTPROC glMultiTexEnviEXT;
GARCH_API extern PFNGLMULTITEXENVIVEXTPROC glMultiTexEnvivEXT;
GARCH_API extern PFNGLMULTITEXGENDEXTPROC glMultiTexGendEXT;
GARCH_API extern PFNGLMULTITEXGENDVEXTPROC glMultiTexGendvEXT;
GARCH_API extern PFNGLMULTITEXGENFEXTPROC glMultiTexGenfEXT;
GARCH_API extern PFNGLMULTITEXGENFVEXTPROC glMultiTexGenfvEXT;
GARCH_API extern PFNGLMULTITEXGENIEXTPROC glMultiTexGeniEXT;
GARCH_API extern PFNGLMULTITEXGENIVEXTPROC glMultiTexGenivEXT;
GARCH_API extern PFNGLMULTITEXIMAGE1DEXTPROC glMultiTexImage1DEXT;
GARCH_API extern PFNGLMULTITEXIMAGE2DEXTPROC glMultiTexImage2DEXT;
GARCH_API extern PFNGLMULTITEXIMAGE3DEXTPROC glMultiTexImage3DEXT;
GARCH_API extern PFNGLMULTITEXPARAMETERIIVEXTPROC glMultiTexParameterIivEXT;
GARCH_API extern PFNGLMULTITEXPARAMETERIUIVEXTPROC glMultiTexParameterIuivEXT;
GARCH_API extern PFNGLMULTITEXPARAMETERFEXTPROC glMultiTexParameterfEXT;
GARCH_API extern PFNGLMULTITEXPARAMETERFVEXTPROC glMultiTexParameterfvEXT;
GARCH_API extern PFNGLMULTITEXPARAMETERIEXTPROC glMultiTexParameteriEXT;
GARCH_API extern PFNGLMULTITEXPARAMETERIVEXTPROC glMultiTexParameterivEXT;
GARCH_API extern PFNGLMULTITEXRENDERBUFFEREXTPROC glMultiTexRenderbufferEXT;
GARCH_API extern PFNGLMULTITEXSUBIMAGE1DEXTPROC glMultiTexSubImage1DEXT;
GARCH_API extern PFNGLMULTITEXSUBIMAGE2DEXTPROC glMultiTexSubImage2DEXT;
GARCH_API extern PFNGLMULTITEXSUBIMAGE3DEXTPROC glMultiTexSubImage3DEXT;
GARCH_API extern PFNGLMULTICASTBARRIERNVPROC glMulticastBarrierNV;
GARCH_API extern PFNGLMULTICASTBLITFRAMEBUFFERNVPROC glMulticastBlitFramebufferNV;
GARCH_API extern PFNGLMULTICASTBUFFERSUBDATANVPROC glMulticastBufferSubDataNV;
GARCH_API extern PFNGLMULTICASTCOPYBUFFERSUBDATANVPROC glMulticastCopyBufferSubDataNV;
GARCH_API extern PFNGLMULTICASTCOPYIMAGESUBDATANVPROC glMulticastCopyImageSubDataNV;
GARCH_API extern PFNGLMULTICASTFRAMEBUFFERSAMPLELOCATIONSFVNVPROC glMulticastFramebufferSampleLocationsfvNV;
GARCH_API extern PFNGLMULTICASTGETQUERYOBJECTI64VNVPROC glMulticastGetQueryObjecti64vNV;
GARCH_API extern PFNGLMULTICASTGETQUERYOBJECTIVNVPROC glMulticastGetQueryObjectivNV;
GARCH_API extern PFNGLMULTICASTGETQUERYOBJECTUI64VNVPROC glMulticastGetQueryObjectui64vNV;
GARCH_API extern PFNGLMULTICASTGETQUERYOBJECTUIVNVPROC glMulticastGetQueryObjectuivNV;
GARCH_API extern PFNGLMULTICASTSCISSORARRAYVNVXPROC glMulticastScissorArrayvNVX;
GARCH_API extern PFNGLMULTICASTVIEWPORTARRAYVNVXPROC glMulticastViewportArrayvNVX;
GARCH_API extern PFNGLMULTICASTVIEWPORTPOSITIONWSCALENVXPROC glMulticastViewportPositionWScaleNVX;
GARCH_API extern PFNGLMULTICASTWAITSYNCNVPROC glMulticastWaitSyncNV;
GARCH_API extern PFNGLNAMEDBUFFERATTACHMEMORYNVPROC glNamedBufferAttachMemoryNV;
GARCH_API extern PFNGLNAMEDBUFFERDATAPROC glNamedBufferData;
GARCH_API extern PFNGLNAMEDBUFFERDATAEXTPROC glNamedBufferDataEXT;
GARCH_API extern PFNGLNAMEDBUFFERPAGECOMMITMENTARBPROC glNamedBufferPageCommitmentARB;
GARCH_API extern PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC glNamedBufferPageCommitmentEXT;
GARCH_API extern PFNGLNAMEDBUFFERPAGECOMMITMENTMEMNVPROC glNamedBufferPageCommitmentMemNV;
GARCH_API extern PFNGLNAMEDBUFFERSTORAGEPROC glNamedBufferStorage;
GARCH_API extern PFNGLNAMEDBUFFERSTORAGEEXTERNALEXTPROC glNamedBufferStorageExternalEXT;
GARCH_API extern PFNGLNAMEDBUFFERSTORAGEEXTPROC glNamedBufferStorageEXT;
GARCH_API extern PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC glNamedBufferStorageMemEXT;
GARCH_API extern PFNGLNAMEDBUFFERSUBDATAPROC glNamedBufferSubData;
GARCH_API extern PFNGLNAMEDBUFFERSUBDATAEXTPROC glNamedBufferSubDataEXT;
GARCH_API extern PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC glNamedCopyBufferSubDataEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC glNamedFramebufferDrawBuffer;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC glNamedFramebufferDrawBuffers;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC glNamedFramebufferParameteri;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC glNamedFramebufferParameteriEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC glNamedFramebufferReadBuffer;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC glNamedFramebufferRenderbuffer;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC glNamedFramebufferRenderbufferEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC glNamedFramebufferSampleLocationsfvARB;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC glNamedFramebufferSampleLocationsfvNV;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTUREPROC glNamedFramebufferTexture;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERSAMPLEPOSITIONSFVAMDPROC glNamedFramebufferSamplePositionsfvAMD;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC glNamedFramebufferTexture1DEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC glNamedFramebufferTexture2DEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC glNamedFramebufferTexture3DEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC glNamedFramebufferTextureEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC glNamedFramebufferTextureFaceEXT;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC glNamedFramebufferTextureLayer;
GARCH_API extern PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC glNamedFramebufferTextureLayerEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC glNamedProgramLocalParameter4dEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC glNamedProgramLocalParameter4dvEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC glNamedProgramLocalParameter4fEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC glNamedProgramLocalParameter4fvEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC glNamedProgramLocalParameterI4iEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC glNamedProgramLocalParameterI4ivEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC glNamedProgramLocalParameterI4uiEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC glNamedProgramLocalParameterI4uivEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC glNamedProgramLocalParameters4fvEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC glNamedProgramLocalParametersI4ivEXT;
GARCH_API extern PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC glNamedProgramLocalParametersI4uivEXT;
GARCH_API extern PFNGLNAMEDPROGRAMSTRINGEXTPROC glNamedProgramStringEXT;
GARCH_API extern PFNGLNAMEDRENDERBUFFERSTORAGEPROC glNamedRenderbufferStorage;
GARCH_API extern PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC glNamedRenderbufferStorageEXT;
GARCH_API extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC glNamedRenderbufferStorageMultisample;
GARCH_API extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC glNamedRenderbufferStorageMultisampleAdvancedAMD;
GARCH_API extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC glNamedRenderbufferStorageMultisampleCoverageEXT;
GARCH_API extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glNamedRenderbufferStorageMultisampleEXT;
GARCH_API extern PFNGLNAMEDSTRINGARBPROC glNamedStringARB;
GARCH_API extern PFNGLNEWLISTPROC glNewList;
GARCH_API extern PFNGLNORMAL3BPROC glNormal3b;
GARCH_API extern PFNGLNORMAL3BVPROC glNormal3bv;
GARCH_API extern PFNGLNORMAL3DPROC glNormal3d;
GARCH_API extern PFNGLNORMAL3DVPROC glNormal3dv;
GARCH_API extern PFNGLNORMAL3FPROC glNormal3f;
GARCH_API extern PFNGLNORMAL3FVPROC glNormal3fv;
GARCH_API extern PFNGLNORMAL3HNVPROC glNormal3hNV;
GARCH_API extern PFNGLNORMAL3HVNVPROC glNormal3hvNV;
GARCH_API extern PFNGLNORMAL3IPROC glNormal3i;
GARCH_API extern PFNGLNORMAL3IVPROC glNormal3iv;
GARCH_API extern PFNGLNORMAL3SPROC glNormal3s;
GARCH_API extern PFNGLNORMAL3SVPROC glNormal3sv;
GARCH_API extern PFNGLNORMALFORMATNVPROC glNormalFormatNV;
GARCH_API extern PFNGLNORMALP3UIPROC glNormalP3ui;
GARCH_API extern PFNGLNORMALP3UIVPROC glNormalP3uiv;
GARCH_API extern PFNGLNORMALPOINTERPROC glNormalPointer;
GARCH_API extern PFNGLNORMALPOINTEREXTPROC glNormalPointerEXT;
GARCH_API extern PFNGLNORMALPOINTERVINTELPROC glNormalPointervINTEL;
GARCH_API extern PFNGLOBJECTLABELPROC glObjectLabel;
GARCH_API extern PFNGLOBJECTLABELKHRPROC glObjectLabelKHR;
GARCH_API extern PFNGLOBJECTPTRLABELPROC glObjectPtrLabel;
GARCH_API extern PFNGLOBJECTPTRLABELKHRPROC glObjectPtrLabelKHR;
GARCH_API extern PFNGLOBJECTPURGEABLEAPPLEPROC glObjectPurgeableAPPLE;
GARCH_API extern PFNGLOBJECTUNPURGEABLEAPPLEPROC glObjectUnpurgeableAPPLE;
GARCH_API extern PFNGLORTHOPROC glOrtho;
GARCH_API extern PFNGLPASSTHROUGHPROC glPassThrough;
GARCH_API extern PFNGLPATCHPARAMETERFVPROC glPatchParameterfv;
GARCH_API extern PFNGLPATCHPARAMETERIPROC glPatchParameteri;
GARCH_API extern PFNGLPATHCOLORGENNVPROC glPathColorGenNV;
GARCH_API extern PFNGLPATHCOMMANDSNVPROC glPathCommandsNV;
GARCH_API extern PFNGLPATHCOORDSNVPROC glPathCoordsNV;
GARCH_API extern PFNGLPATHCOVERDEPTHFUNCNVPROC glPathCoverDepthFuncNV;
GARCH_API extern PFNGLPATHDASHARRAYNVPROC glPathDashArrayNV;
GARCH_API extern PFNGLPATHFOGGENNVPROC glPathFogGenNV;
GARCH_API extern PFNGLPATHGLYPHINDEXARRAYNVPROC glPathGlyphIndexArrayNV;
GARCH_API extern PFNGLPATHGLYPHINDEXRANGENVPROC glPathGlyphIndexRangeNV;
GARCH_API extern PFNGLPATHGLYPHRANGENVPROC glPathGlyphRangeNV;
GARCH_API extern PFNGLPATHGLYPHSNVPROC glPathGlyphsNV;
GARCH_API extern PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC glPathMemoryGlyphIndexArrayNV;
GARCH_API extern PFNGLPATHPARAMETERFNVPROC glPathParameterfNV;
GARCH_API extern PFNGLPATHPARAMETERFVNVPROC glPathParameterfvNV;
GARCH_API extern PFNGLPATHPARAMETERINVPROC glPathParameteriNV;
GARCH_API extern PFNGLPATHPARAMETERIVNVPROC glPathParameterivNV;
GARCH_API extern PFNGLPATHSTENCILDEPTHOFFSETNVPROC glPathStencilDepthOffsetNV;
GARCH_API extern PFNGLPATHSTENCILFUNCNVPROC glPathStencilFuncNV;
GARCH_API extern PFNGLPATHSTRINGNVPROC glPathStringNV;
GARCH_API extern PFNGLPATHSUBCOMMANDSNVPROC glPathSubCommandsNV;
GARCH_API extern PFNGLPATHSUBCOORDSNVPROC glPathSubCoordsNV;
GARCH_API extern PFNGLPATHTEXGENNVPROC glPathTexGenNV;
GARCH_API extern PFNGLPAUSETRANSFORMFEEDBACKPROC glPauseTransformFeedback;
GARCH_API extern PFNGLPAUSETRANSFORMFEEDBACKNVPROC glPauseTransformFeedbackNV;
GARCH_API extern PFNGLPIXELDATARANGENVPROC glPixelDataRangeNV;
GARCH_API extern PFNGLPIXELMAPFVPROC glPixelMapfv;
GARCH_API extern PFNGLPIXELMAPUIVPROC glPixelMapuiv;
GARCH_API extern PFNGLPIXELMAPUSVPROC glPixelMapusv;
GARCH_API extern PFNGLPIXELSTOREFPROC glPixelStoref;
GARCH_API extern PFNGLPIXELSTOREIPROC glPixelStorei;
GARCH_API extern PFNGLPIXELTRANSFERFPROC glPixelTransferf;
GARCH_API extern PFNGLPIXELTRANSFERIPROC glPixelTransferi;
GARCH_API extern PFNGLPIXELTRANSFORMPARAMETERFEXTPROC glPixelTransformParameterfEXT;
GARCH_API extern PFNGLPIXELTRANSFORMPARAMETERFVEXTPROC glPixelTransformParameterfvEXT;
GARCH_API extern PFNGLPIXELTRANSFORMPARAMETERIEXTPROC glPixelTransformParameteriEXT;
GARCH_API extern PFNGLPIXELTRANSFORMPARAMETERIVEXTPROC glPixelTransformParameterivEXT;
GARCH_API extern PFNGLPIXELZOOMPROC glPixelZoom;
GARCH_API extern PFNGLPOINTALONGPATHNVPROC glPointAlongPathNV;
GARCH_API extern PFNGLPOINTPARAMETERFPROC glPointParameterf;
GARCH_API extern PFNGLPOINTPARAMETERFARBPROC glPointParameterfARB;
GARCH_API extern PFNGLPOINTPARAMETERFEXTPROC glPointParameterfEXT;
GARCH_API extern PFNGLPOINTPARAMETERFVPROC glPointParameterfv;
GARCH_API extern PFNGLPOINTPARAMETERFVARBPROC glPointParameterfvARB;
GARCH_API extern PFNGLPOINTPARAMETERFVEXTPROC glPointParameterfvEXT;
GARCH_API extern PFNGLPOINTPARAMETERIPROC glPointParameteri;
GARCH_API extern PFNGLPOINTPARAMETERINVPROC glPointParameteriNV;
GARCH_API extern PFNGLPOINTPARAMETERIVPROC glPointParameteriv;
GARCH_API extern PFNGLPOINTPARAMETERIVNVPROC glPointParameterivNV;
GARCH_API extern PFNGLPOINTSIZEPROC glPointSize;
GARCH_API extern PFNGLPOLYGONMODEPROC glPolygonMode;
GARCH_API extern PFNGLPOLYGONOFFSETPROC glPolygonOffset;
GARCH_API extern PFNGLPOLYGONOFFSETCLAMPPROC glPolygonOffsetClamp;
GARCH_API extern PFNGLPOLYGONOFFSETCLAMPEXTPROC glPolygonOffsetClampEXT;
GARCH_API extern PFNGLPOLYGONOFFSETEXTPROC glPolygonOffsetEXT;
GARCH_API extern PFNGLPOLYGONSTIPPLEPROC glPolygonStipple;
GARCH_API extern PFNGLPOPATTRIBPROC glPopAttrib;
GARCH_API extern PFNGLPOPCLIENTATTRIBPROC glPopClientAttrib;
GARCH_API extern PFNGLPOPDEBUGGROUPPROC glPopDebugGroup;
GARCH_API extern PFNGLPOPDEBUGGROUPKHRPROC glPopDebugGroupKHR;
GARCH_API extern PFNGLPOPGROUPMARKEREXTPROC glPopGroupMarkerEXT;
GARCH_API extern PFNGLPOPMATRIXPROC glPopMatrix;
GARCH_API extern PFNGLPOPNAMEPROC glPopName;
GARCH_API extern PFNGLPRESENTFRAMEDUALFILLNVPROC glPresentFrameDualFillNV;
GARCH_API extern PFNGLPRESENTFRAMEKEYEDNVPROC glPresentFrameKeyedNV;
GARCH_API extern PFNGLPRIMITIVEBOUNDINGBOXARBPROC glPrimitiveBoundingBoxARB;
GARCH_API extern PFNGLPRIMITIVERESTARTINDEXPROC glPrimitiveRestartIndex;
GARCH_API extern PFNGLPRIMITIVERESTARTINDEXNVPROC glPrimitiveRestartIndexNV;
GARCH_API extern PFNGLPRIMITIVERESTARTNVPROC glPrimitiveRestartNV;
GARCH_API extern PFNGLPRIORITIZETEXTURESPROC glPrioritizeTextures;
GARCH_API extern PFNGLPRIORITIZETEXTURESEXTPROC glPrioritizeTexturesEXT;
GARCH_API extern PFNGLPROGRAMBINARYPROC glProgramBinary;
GARCH_API extern PFNGLPROGRAMBUFFERPARAMETERSIIVNVPROC glProgramBufferParametersIivNV;
GARCH_API extern PFNGLPROGRAMBUFFERPARAMETERSIUIVNVPROC glProgramBufferParametersIuivNV;
GARCH_API extern PFNGLPROGRAMBUFFERPARAMETERSFVNVPROC glProgramBufferParametersfvNV;
GARCH_API extern PFNGLPROGRAMENVPARAMETER4DARBPROC glProgramEnvParameter4dARB;
GARCH_API extern PFNGLPROGRAMENVPARAMETER4DVARBPROC glProgramEnvParameter4dvARB;
GARCH_API extern PFNGLPROGRAMENVPARAMETER4FARBPROC glProgramEnvParameter4fARB;
GARCH_API extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB;
GARCH_API extern PFNGLPROGRAMENVPARAMETERI4INVPROC glProgramEnvParameterI4iNV;
GARCH_API extern PFNGLPROGRAMENVPARAMETERI4IVNVPROC glProgramEnvParameterI4ivNV;
GARCH_API extern PFNGLPROGRAMENVPARAMETERI4UINVPROC glProgramEnvParameterI4uiNV;
GARCH_API extern PFNGLPROGRAMENVPARAMETERI4UIVNVPROC glProgramEnvParameterI4uivNV;
GARCH_API extern PFNGLPROGRAMENVPARAMETERS4FVEXTPROC glProgramEnvParameters4fvEXT;
GARCH_API extern PFNGLPROGRAMENVPARAMETERSI4IVNVPROC glProgramEnvParametersI4ivNV;
GARCH_API extern PFNGLPROGRAMENVPARAMETERSI4UIVNVPROC glProgramEnvParametersI4uivNV;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glProgramLocalParameter4dvARB;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC glProgramLocalParameter4fARB;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERI4INVPROC glProgramLocalParameterI4iNV;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERI4IVNVPROC glProgramLocalParameterI4ivNV;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERI4UINVPROC glProgramLocalParameterI4uiNV;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERI4UIVNVPROC glProgramLocalParameterI4uivNV;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC glProgramLocalParameters4fvEXT;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERSI4IVNVPROC glProgramLocalParametersI4ivNV;
GARCH_API extern PFNGLPROGRAMLOCALPARAMETERSI4UIVNVPROC glProgramLocalParametersI4uivNV;
GARCH_API extern PFNGLPROGRAMNAMEDPARAMETER4DNVPROC glProgramNamedParameter4dNV;
GARCH_API extern PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC glProgramNamedParameter4dvNV;
GARCH_API extern PFNGLPROGRAMNAMEDPARAMETER4FNVPROC glProgramNamedParameter4fNV;
GARCH_API extern PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC glProgramNamedParameter4fvNV;
GARCH_API extern PFNGLPROGRAMPARAMETER4DNVPROC glProgramParameter4dNV;
GARCH_API extern PFNGLPROGRAMPARAMETER4DVNVPROC glProgramParameter4dvNV;
GARCH_API extern PFNGLPROGRAMPARAMETER4FNVPROC glProgramParameter4fNV;
GARCH_API extern PFNGLPROGRAMPARAMETER4FVNVPROC glProgramParameter4fvNV;
GARCH_API extern PFNGLPROGRAMPARAMETERIPROC glProgramParameteri;
GARCH_API extern PFNGLPROGRAMPARAMETERIARBPROC glProgramParameteriARB;
GARCH_API extern PFNGLPROGRAMPARAMETERIEXTPROC glProgramParameteriEXT;
GARCH_API extern PFNGLPROGRAMPARAMETERS4DVNVPROC glProgramParameters4dvNV;
GARCH_API extern PFNGLPROGRAMPARAMETERS4FVNVPROC glProgramParameters4fvNV;
GARCH_API extern PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC glProgramPathFragmentInputGenNV;
GARCH_API extern PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
GARCH_API extern PFNGLPROGRAMSUBROUTINEPARAMETERSUIVNVPROC glProgramSubroutineParametersuivNV;
GARCH_API extern PFNGLPROGRAMUNIFORM1DPROC glProgramUniform1d;
GARCH_API extern PFNGLPROGRAMUNIFORM1DEXTPROC glProgramUniform1dEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1DVPROC glProgramUniform1dv;
GARCH_API extern PFNGLPROGRAMUNIFORM1DVEXTPROC glProgramUniform1dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
GARCH_API extern PFNGLPROGRAMUNIFORM1FEXTPROC glProgramUniform1fEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1FVPROC glProgramUniform1fv;
GARCH_API extern PFNGLPROGRAMUNIFORM1FVEXTPROC glProgramUniform1fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
GARCH_API extern PFNGLPROGRAMUNIFORM1I64ARBPROC glProgramUniform1i64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM1I64NVPROC glProgramUniform1i64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM1I64VARBPROC glProgramUniform1i64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM1I64VNVPROC glProgramUniform1i64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM1IEXTPROC glProgramUniform1iEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1IVPROC glProgramUniform1iv;
GARCH_API extern PFNGLPROGRAMUNIFORM1IVEXTPROC glProgramUniform1ivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1UIPROC glProgramUniform1ui;
GARCH_API extern PFNGLPROGRAMUNIFORM1UI64ARBPROC glProgramUniform1ui64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM1UI64NVPROC glProgramUniform1ui64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM1UI64VARBPROC glProgramUniform1ui64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM1UI64VNVPROC glProgramUniform1ui64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM1UIEXTPROC glProgramUniform1uiEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM1UIVPROC glProgramUniform1uiv;
GARCH_API extern PFNGLPROGRAMUNIFORM1UIVEXTPROC glProgramUniform1uivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2DPROC glProgramUniform2d;
GARCH_API extern PFNGLPROGRAMUNIFORM2DEXTPROC glProgramUniform2dEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2DVPROC glProgramUniform2dv;
GARCH_API extern PFNGLPROGRAMUNIFORM2DVEXTPROC glProgramUniform2dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f;
GARCH_API extern PFNGLPROGRAMUNIFORM2FEXTPROC glProgramUniform2fEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2FVPROC glProgramUniform2fv;
GARCH_API extern PFNGLPROGRAMUNIFORM2FVEXTPROC glProgramUniform2fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2IPROC glProgramUniform2i;
GARCH_API extern PFNGLPROGRAMUNIFORM2I64ARBPROC glProgramUniform2i64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM2I64NVPROC glProgramUniform2i64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM2I64VARBPROC glProgramUniform2i64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM2I64VNVPROC glProgramUniform2i64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM2IEXTPROC glProgramUniform2iEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2IVPROC glProgramUniform2iv;
GARCH_API extern PFNGLPROGRAMUNIFORM2IVEXTPROC glProgramUniform2ivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2UIPROC glProgramUniform2ui;
GARCH_API extern PFNGLPROGRAMUNIFORM2UI64ARBPROC glProgramUniform2ui64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM2UI64NVPROC glProgramUniform2ui64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM2UI64VARBPROC glProgramUniform2ui64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM2UI64VNVPROC glProgramUniform2ui64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM2UIEXTPROC glProgramUniform2uiEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM2UIVPROC glProgramUniform2uiv;
GARCH_API extern PFNGLPROGRAMUNIFORM2UIVEXTPROC glProgramUniform2uivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3DPROC glProgramUniform3d;
GARCH_API extern PFNGLPROGRAMUNIFORM3DEXTPROC glProgramUniform3dEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3DVPROC glProgramUniform3dv;
GARCH_API extern PFNGLPROGRAMUNIFORM3DVEXTPROC glProgramUniform3dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;
GARCH_API extern PFNGLPROGRAMUNIFORM3FEXTPROC glProgramUniform3fEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3FVPROC glProgramUniform3fv;
GARCH_API extern PFNGLPROGRAMUNIFORM3FVEXTPROC glProgramUniform3fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3IPROC glProgramUniform3i;
GARCH_API extern PFNGLPROGRAMUNIFORM3I64ARBPROC glProgramUniform3i64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM3I64NVPROC glProgramUniform3i64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM3I64VARBPROC glProgramUniform3i64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM3I64VNVPROC glProgramUniform3i64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM3IEXTPROC glProgramUniform3iEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3IVPROC glProgramUniform3iv;
GARCH_API extern PFNGLPROGRAMUNIFORM3IVEXTPROC glProgramUniform3ivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3UIPROC glProgramUniform3ui;
GARCH_API extern PFNGLPROGRAMUNIFORM3UI64ARBPROC glProgramUniform3ui64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM3UI64NVPROC glProgramUniform3ui64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM3UI64VARBPROC glProgramUniform3ui64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM3UI64VNVPROC glProgramUniform3ui64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM3UIEXTPROC glProgramUniform3uiEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM3UIVPROC glProgramUniform3uiv;
GARCH_API extern PFNGLPROGRAMUNIFORM3UIVEXTPROC glProgramUniform3uivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4DPROC glProgramUniform4d;
GARCH_API extern PFNGLPROGRAMUNIFORM4DEXTPROC glProgramUniform4dEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4DVPROC glProgramUniform4dv;
GARCH_API extern PFNGLPROGRAMUNIFORM4DVEXTPROC glProgramUniform4dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4FPROC glProgramUniform4f;
GARCH_API extern PFNGLPROGRAMUNIFORM4FEXTPROC glProgramUniform4fEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4FVPROC glProgramUniform4fv;
GARCH_API extern PFNGLPROGRAMUNIFORM4FVEXTPROC glProgramUniform4fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4IPROC glProgramUniform4i;
GARCH_API extern PFNGLPROGRAMUNIFORM4I64ARBPROC glProgramUniform4i64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM4I64NVPROC glProgramUniform4i64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM4I64VARBPROC glProgramUniform4i64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM4I64VNVPROC glProgramUniform4i64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM4IEXTPROC glProgramUniform4iEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4IVPROC glProgramUniform4iv;
GARCH_API extern PFNGLPROGRAMUNIFORM4IVEXTPROC glProgramUniform4ivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4UIPROC glProgramUniform4ui;
GARCH_API extern PFNGLPROGRAMUNIFORM4UI64ARBPROC glProgramUniform4ui64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORM4UI64NVPROC glProgramUniform4ui64NV;
GARCH_API extern PFNGLPROGRAMUNIFORM4UI64VARBPROC glProgramUniform4ui64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORM4UI64VNVPROC glProgramUniform4ui64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORM4UIEXTPROC glProgramUniform4uiEXT;
GARCH_API extern PFNGLPROGRAMUNIFORM4UIVPROC glProgramUniform4uiv;
GARCH_API extern PFNGLPROGRAMUNIFORM4UIVEXTPROC glProgramUniform4uivEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC glProgramUniformHandleui64ARB;
GARCH_API extern PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC glProgramUniformHandleui64NV;
GARCH_API extern PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC glProgramUniformHandleui64vARB;
GARCH_API extern PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC glProgramUniformHandleui64vNV;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2DVPROC glProgramUniformMatrix2dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC glProgramUniformMatrix2dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2FVPROC glProgramUniformMatrix2fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC glProgramUniformMatrix2fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC glProgramUniformMatrix2x3dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC glProgramUniformMatrix2x3dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC glProgramUniformMatrix2x3fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC glProgramUniformMatrix2x3fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC glProgramUniformMatrix2x4dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC glProgramUniformMatrix2x4dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC glProgramUniformMatrix2x4fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC glProgramUniformMatrix2x4fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3DVPROC glProgramUniformMatrix3dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC glProgramUniformMatrix3dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3FVPROC glProgramUniformMatrix3fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC glProgramUniformMatrix3fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC glProgramUniformMatrix3x2dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC glProgramUniformMatrix3x2dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC glProgramUniformMatrix3x2fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC glProgramUniformMatrix3x2fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC glProgramUniformMatrix3x4dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC glProgramUniformMatrix3x4dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC glProgramUniformMatrix3x4fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC glProgramUniformMatrix3x4fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4DVPROC glProgramUniformMatrix4dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC glProgramUniformMatrix4dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4FVPROC glProgramUniformMatrix4fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC glProgramUniformMatrix4fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC glProgramUniformMatrix4x2dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC glProgramUniformMatrix4x2dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC glProgramUniformMatrix4x2fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC glProgramUniformMatrix4x2fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC glProgramUniformMatrix4x3dv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC glProgramUniformMatrix4x3dvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC glProgramUniformMatrix4x3fv;
GARCH_API extern PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC glProgramUniformMatrix4x3fvEXT;
GARCH_API extern PFNGLPROGRAMUNIFORMUI64NVPROC glProgramUniformui64NV;
GARCH_API extern PFNGLPROGRAMUNIFORMUI64VNVPROC glProgramUniformui64vNV;
GARCH_API extern PFNGLPROGRAMVERTEXLIMITNVPROC glProgramVertexLimitNV;
GARCH_API extern PFNGLPROVOKINGVERTEXPROC glProvokingVertex;
GARCH_API extern PFNGLPROVOKINGVERTEXEXTPROC glProvokingVertexEXT;
GARCH_API extern PFNGLPUSHATTRIBPROC glPushAttrib;
GARCH_API extern PFNGLPUSHCLIENTATTRIBPROC glPushClientAttrib;
GARCH_API extern PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC glPushClientAttribDefaultEXT;
GARCH_API extern PFNGLPUSHDEBUGGROUPPROC glPushDebugGroup;
GARCH_API extern PFNGLPUSHDEBUGGROUPKHRPROC glPushDebugGroupKHR;
GARCH_API extern PFNGLPUSHGROUPMARKEREXTPROC glPushGroupMarkerEXT;
GARCH_API extern PFNGLPUSHMATRIXPROC glPushMatrix;
GARCH_API extern PFNGLPUSHNAMEPROC glPushName;
GARCH_API extern PFNGLQUERYCOUNTERPROC glQueryCounter;
GARCH_API extern PFNGLQUERYOBJECTPARAMETERUIAMDPROC glQueryObjectParameteruiAMD;
GARCH_API extern PFNGLQUERYRESOURCENVPROC glQueryResourceNV;
GARCH_API extern PFNGLQUERYRESOURCETAGNVPROC glQueryResourceTagNV;
GARCH_API extern PFNGLRASTERPOS2DPROC glRasterPos2d;
GARCH_API extern PFNGLRASTERPOS2DVPROC glRasterPos2dv;
GARCH_API extern PFNGLRASTERPOS2FPROC glRasterPos2f;
GARCH_API extern PFNGLRASTERPOS2FVPROC glRasterPos2fv;
GARCH_API extern PFNGLRASTERPOS2IPROC glRasterPos2i;
GARCH_API extern PFNGLRASTERPOS2IVPROC glRasterPos2iv;
GARCH_API extern PFNGLRASTERPOS2SPROC glRasterPos2s;
GARCH_API extern PFNGLRASTERPOS2SVPROC glRasterPos2sv;
GARCH_API extern PFNGLRASTERPOS3DPROC glRasterPos3d;
GARCH_API extern PFNGLRASTERPOS3DVPROC glRasterPos3dv;
GARCH_API extern PFNGLRASTERPOS3FPROC glRasterPos3f;
GARCH_API extern PFNGLRASTERPOS3FVPROC glRasterPos3fv;
GARCH_API extern PFNGLRASTERPOS3IPROC glRasterPos3i;
GARCH_API extern PFNGLRASTERPOS3IVPROC glRasterPos3iv;
GARCH_API extern PFNGLRASTERPOS3SPROC glRasterPos3s;
GARCH_API extern PFNGLRASTERPOS3SVPROC glRasterPos3sv;
GARCH_API extern PFNGLRASTERPOS4DPROC glRasterPos4d;
GARCH_API extern PFNGLRASTERPOS4DVPROC glRasterPos4dv;
GARCH_API extern PFNGLRASTERPOS4FPROC glRasterPos4f;
GARCH_API extern PFNGLRASTERPOS4FVPROC glRasterPos4fv;
GARCH_API extern PFNGLRASTERPOS4IPROC glRasterPos4i;
GARCH_API extern PFNGLRASTERPOS4IVPROC glRasterPos4iv;
GARCH_API extern PFNGLRASTERPOS4SPROC glRasterPos4s;
GARCH_API extern PFNGLRASTERPOS4SVPROC glRasterPos4sv;
GARCH_API extern PFNGLRASTERSAMPLESEXTPROC glRasterSamplesEXT;
GARCH_API extern PFNGLREADBUFFERPROC glReadBuffer;
GARCH_API extern PFNGLREADPIXELSPROC glReadPixels;
GARCH_API extern PFNGLREADNPIXELSPROC glReadnPixels;
GARCH_API extern PFNGLREADNPIXELSARBPROC glReadnPixelsARB;
GARCH_API extern PFNGLREADNPIXELSKHRPROC glReadnPixelsKHR;
GARCH_API extern PFNGLRELEASEKEYEDMUTEXWIN32EXTPROC glReleaseKeyedMutexWin32EXT;
GARCH_API extern PFNGLRECTDPROC glRectd;
GARCH_API extern PFNGLRECTDVPROC glRectdv;
GARCH_API extern PFNGLRECTFPROC glRectf;
GARCH_API extern PFNGLRECTFVPROC glRectfv;
GARCH_API extern PFNGLRECTIPROC glRecti;
GARCH_API extern PFNGLRECTIVPROC glRectiv;
GARCH_API extern PFNGLRECTSPROC glRects;
GARCH_API extern PFNGLRECTSVPROC glRectsv;
GARCH_API extern PFNGLRELEASESHADERCOMPILERPROC glReleaseShaderCompiler;
GARCH_API extern PFNGLRENDERGPUMASKNVPROC glRenderGpuMaskNV;
GARCH_API extern PFNGLRENDERMODEPROC glRenderMode;
GARCH_API extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
GARCH_API extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
GARCH_API extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
GARCH_API extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC glRenderbufferStorageMultisampleAdvancedAMD;
GARCH_API extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC glRenderbufferStorageMultisampleCoverageNV;
GARCH_API extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT;
GARCH_API extern PFNGLREQUESTRESIDENTPROGRAMSNVPROC glRequestResidentProgramsNV;
GARCH_API extern PFNGLRESETHISTOGRAMPROC glResetHistogram;
GARCH_API extern PFNGLRESETHISTOGRAMEXTPROC glResetHistogramEXT;
GARCH_API extern PFNGLRESETMEMORYOBJECTPARAMETERNVPROC glResetMemoryObjectParameterNV;
GARCH_API extern PFNGLRESETMINMAXPROC glResetMinmax;
GARCH_API extern PFNGLRESETMINMAXEXTPROC glResetMinmaxEXT;
GARCH_API extern PFNGLRESOLVEDEPTHVALUESNVPROC glResolveDepthValuesNV;
GARCH_API extern PFNGLRESUMETRANSFORMFEEDBACKPROC glResumeTransformFeedback;
GARCH_API extern PFNGLRESUMETRANSFORMFEEDBACKNVPROC glResumeTransformFeedbackNV;
GARCH_API extern PFNGLROTATEDPROC glRotated;
GARCH_API extern PFNGLROTATEFPROC glRotatef;
GARCH_API extern PFNGLSAMPLECOVERAGEPROC glSampleCoverage;
GARCH_API extern PFNGLSAMPLECOVERAGEARBPROC glSampleCoverageARB;
GARCH_API extern PFNGLSAMPLEMASKEXTPROC glSampleMaskEXT;
GARCH_API extern PFNGLSAMPLEMASKINDEXEDNVPROC glSampleMaskIndexedNV;
GARCH_API extern PFNGLSAMPLEMASKIPROC glSampleMaski;
GARCH_API extern PFNGLSAMPLEPATTERNEXTPROC glSamplePatternEXT;
GARCH_API extern PFNGLSAMPLERPARAMETERIIVPROC glSamplerParameterIiv;
GARCH_API extern PFNGLSAMPLERPARAMETERIUIVPROC glSamplerParameterIuiv;
GARCH_API extern PFNGLSAMPLERPARAMETERFPROC glSamplerParameterf;
GARCH_API extern PFNGLSAMPLERPARAMETERFVPROC glSamplerParameterfv;
GARCH_API extern PFNGLSAMPLERPARAMETERIPROC glSamplerParameteri;
GARCH_API extern PFNGLSAMPLERPARAMETERIVPROC glSamplerParameteriv;
GARCH_API extern PFNGLSCALEDPROC glScaled;
GARCH_API extern PFNGLSCALEFPROC glScalef;
GARCH_API extern PFNGLSCISSORPROC glScissor;
GARCH_API extern PFNGLSCISSORARRAYVPROC glScissorArrayv;
GARCH_API extern PFNGLSCISSOREXCLUSIVEARRAYVNVPROC glScissorExclusiveArrayvNV;
GARCH_API extern PFNGLSCISSOREXCLUSIVENVPROC glScissorExclusiveNV;
GARCH_API extern PFNGLSCISSORINDEXEDPROC glScissorIndexed;
GARCH_API extern PFNGLSCISSORINDEXEDVPROC glScissorIndexedv;
GARCH_API extern PFNGLSECONDARYCOLOR3BPROC glSecondaryColor3b;
GARCH_API extern PFNGLSECONDARYCOLOR3BEXTPROC glSecondaryColor3bEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3BVPROC glSecondaryColor3bv;
GARCH_API extern PFNGLSECONDARYCOLOR3BVEXTPROC glSecondaryColor3bvEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3DPROC glSecondaryColor3d;
GARCH_API extern PFNGLSECONDARYCOLOR3DEXTPROC glSecondaryColor3dEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3DVPROC glSecondaryColor3dv;
GARCH_API extern PFNGLSECONDARYCOLOR3DVEXTPROC glSecondaryColor3dvEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3FPROC glSecondaryColor3f;
GARCH_API extern PFNGLSECONDARYCOLOR3FEXTPROC glSecondaryColor3fEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3FVPROC glSecondaryColor3fv;
GARCH_API extern PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3HNVPROC glSecondaryColor3hNV;
GARCH_API extern PFNGLSECONDARYCOLOR3HVNVPROC glSecondaryColor3hvNV;
GARCH_API extern PFNGLSECONDARYCOLOR3IPROC glSecondaryColor3i;
GARCH_API extern PFNGLSECONDARYCOLOR3IEXTPROC glSecondaryColor3iEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3IVPROC glSecondaryColor3iv;
GARCH_API extern PFNGLSECONDARYCOLOR3IVEXTPROC glSecondaryColor3ivEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3SPROC glSecondaryColor3s;
GARCH_API extern PFNGLSECONDARYCOLOR3SEXTPROC glSecondaryColor3sEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3SVPROC glSecondaryColor3sv;
GARCH_API extern PFNGLSECONDARYCOLOR3SVEXTPROC glSecondaryColor3svEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3UBPROC glSecondaryColor3ub;
GARCH_API extern PFNGLSECONDARYCOLOR3UBEXTPROC glSecondaryColor3ubEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3UBVPROC glSecondaryColor3ubv;
GARCH_API extern PFNGLSECONDARYCOLOR3UBVEXTPROC glSecondaryColor3ubvEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3UIPROC glSecondaryColor3ui;
GARCH_API extern PFNGLSECONDARYCOLOR3UIEXTPROC glSecondaryColor3uiEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3UIVPROC glSecondaryColor3uiv;
GARCH_API extern PFNGLSECONDARYCOLOR3UIVEXTPROC glSecondaryColor3uivEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3USPROC glSecondaryColor3us;
GARCH_API extern PFNGLSECONDARYCOLOR3USEXTPROC glSecondaryColor3usEXT;
GARCH_API extern PFNGLSECONDARYCOLOR3USVPROC glSecondaryColor3usv;
GARCH_API extern PFNGLSECONDARYCOLOR3USVEXTPROC glSecondaryColor3usvEXT;
GARCH_API extern PFNGLSECONDARYCOLORFORMATNVPROC glSecondaryColorFormatNV;
GARCH_API extern PFNGLSECONDARYCOLORP3UIPROC glSecondaryColorP3ui;
GARCH_API extern PFNGLSECONDARYCOLORP3UIVPROC glSecondaryColorP3uiv;
GARCH_API extern PFNGLSECONDARYCOLORPOINTERPROC glSecondaryColorPointer;
GARCH_API extern PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointerEXT;
GARCH_API extern PFNGLSELECTBUFFERPROC glSelectBuffer;
GARCH_API extern PFNGLSELECTPERFMONITORCOUNTERSAMDPROC glSelectPerfMonitorCountersAMD;
GARCH_API extern PFNGLSEMAPHOREPARAMETERIVNVPROC glSemaphoreParameterivNV;
GARCH_API extern PFNGLSEMAPHOREPARAMETERUI64VEXTPROC glSemaphoreParameterui64vEXT;
GARCH_API extern PFNGLSEPARABLEFILTER2DPROC glSeparableFilter2D;
GARCH_API extern PFNGLSEPARABLEFILTER2DEXTPROC glSeparableFilter2DEXT;
GARCH_API extern PFNGLSETFENCEAPPLEPROC glSetFenceAPPLE;
GARCH_API extern PFNGLSETFENCENVPROC glSetFenceNV;
GARCH_API extern PFNGLSETINVARIANTEXTPROC glSetInvariantEXT;
GARCH_API extern PFNGLSETLOCALCONSTANTEXTPROC glSetLocalConstantEXT;
GARCH_API extern PFNGLSETMULTISAMPLEFVAMDPROC glSetMultisamplefvAMD;
GARCH_API extern PFNGLSHADEMODELPROC glShadeModel;
GARCH_API extern PFNGLSHADERBINARYPROC glShaderBinary;
GARCH_API extern PFNGLSHADEROP1EXTPROC glShaderOp1EXT;
GARCH_API extern PFNGLSHADEROP2EXTPROC glShaderOp2EXT;
GARCH_API extern PFNGLSHADEROP3EXTPROC glShaderOp3EXT;
GARCH_API extern PFNGLSHADERSOURCEPROC glShaderSource;
GARCH_API extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
GARCH_API extern PFNGLSHADERSTORAGEBLOCKBINDINGPROC glShaderStorageBlockBinding;
GARCH_API extern PFNGLSHADINGRATEIMAGEBARRIERNVPROC glShadingRateImageBarrierNV;
GARCH_API extern PFNGLSHADINGRATEIMAGEPALETTENVPROC glShadingRateImagePaletteNV;
GARCH_API extern PFNGLSHADINGRATESAMPLEORDERNVPROC glShadingRateSampleOrderNV;
GARCH_API extern PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC glShadingRateSampleOrderCustomNV;
GARCH_API extern PFNGLSIGNALSEMAPHOREEXTPROC glSignalSemaphoreEXT;
GARCH_API extern PFNGLSIGNALSEMAPHOREUI64NVXPROC glSignalSemaphoreui64NVX;
GARCH_API extern PFNGLSPECIALIZESHADERPROC glSpecializeShader;
GARCH_API extern PFNGLSPECIALIZESHADERARBPROC glSpecializeShaderARB;
GARCH_API extern PFNGLSTATECAPTURENVPROC glStateCaptureNV;
GARCH_API extern PFNGLSTENCILCLEARTAGEXTPROC glStencilClearTagEXT;
GARCH_API extern PFNGLSTENCILFILLPATHINSTANCEDNVPROC glStencilFillPathInstancedNV;
GARCH_API extern PFNGLSTENCILFILLPATHNVPROC glStencilFillPathNV;
GARCH_API extern PFNGLSTENCILFUNCPROC glStencilFunc;
GARCH_API extern PFNGLSTENCILFUNCSEPARATEPROC glStencilFuncSeparate;
GARCH_API extern PFNGLSTENCILMASKPROC glStencilMask;
GARCH_API extern PFNGLSTENCILMASKSEPARATEPROC glStencilMaskSeparate;
GARCH_API extern PFNGLSTENCILOPPROC glStencilOp;
GARCH_API extern PFNGLSTENCILOPSEPARATEPROC glStencilOpSeparate;
GARCH_API extern PFNGLSTENCILOPVALUEAMDPROC glStencilOpValueAMD;
GARCH_API extern PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC glStencilStrokePathInstancedNV;
GARCH_API extern PFNGLSTENCILSTROKEPATHNVPROC glStencilStrokePathNV;
GARCH_API extern PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC glStencilThenCoverFillPathInstancedNV;
GARCH_API extern PFNGLSTENCILTHENCOVERFILLPATHNVPROC glStencilThenCoverFillPathNV;
GARCH_API extern PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC glStencilThenCoverStrokePathInstancedNV;
GARCH_API extern PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC glStencilThenCoverStrokePathNV;
GARCH_API extern PFNGLSUBPIXELPRECISIONBIASNVPROC glSubpixelPrecisionBiasNV;
GARCH_API extern PFNGLSWIZZLEEXTPROC glSwizzleEXT;
GARCH_API extern PFNGLSYNCTEXTUREINTELPROC glSyncTextureINTEL;
GARCH_API extern PFNGLTANGENT3BEXTPROC glTangent3bEXT;
GARCH_API extern PFNGLTANGENT3BVEXTPROC glTangent3bvEXT;
GARCH_API extern PFNGLTANGENT3DEXTPROC glTangent3dEXT;
GARCH_API extern PFNGLTANGENT3DVEXTPROC glTangent3dvEXT;
GARCH_API extern PFNGLTANGENT3FEXTPROC glTangent3fEXT;
GARCH_API extern PFNGLTANGENT3FVEXTPROC glTangent3fvEXT;
GARCH_API extern PFNGLTANGENT3IEXTPROC glTangent3iEXT;
GARCH_API extern PFNGLTANGENT3IVEXTPROC glTangent3ivEXT;
GARCH_API extern PFNGLTANGENT3SEXTPROC glTangent3sEXT;
GARCH_API extern PFNGLTANGENT3SVEXTPROC glTangent3svEXT;
GARCH_API extern PFNGLTANGENTPOINTEREXTPROC glTangentPointerEXT;
GARCH_API extern PFNGLTESSELLATIONFACTORAMDPROC glTessellationFactorAMD;
GARCH_API extern PFNGLTESSELLATIONMODEAMDPROC glTessellationModeAMD;
GARCH_API extern PFNGLTESTFENCEAPPLEPROC glTestFenceAPPLE;
GARCH_API extern PFNGLTESTFENCENVPROC glTestFenceNV;
GARCH_API extern PFNGLTESTOBJECTAPPLEPROC glTestObjectAPPLE;
GARCH_API extern PFNGLTEXATTACHMEMORYNVPROC glTexAttachMemoryNV;
GARCH_API extern PFNGLTEXBUFFERPROC glTexBuffer;
GARCH_API extern PFNGLTEXBUFFERARBPROC glTexBufferARB;
GARCH_API extern PFNGLTEXBUFFEREXTPROC glTexBufferEXT;
GARCH_API extern PFNGLTEXBUFFERRANGEPROC glTexBufferRange;
GARCH_API extern PFNGLTEXCOORD1DPROC glTexCoord1d;
GARCH_API extern PFNGLTEXCOORD1DVPROC glTexCoord1dv;
GARCH_API extern PFNGLTEXCOORD1FPROC glTexCoord1f;
GARCH_API extern PFNGLTEXCOORD1FVPROC glTexCoord1fv;
GARCH_API extern PFNGLTEXCOORD1HNVPROC glTexCoord1hNV;
GARCH_API extern PFNGLTEXCOORD1HVNVPROC glTexCoord1hvNV;
GARCH_API extern PFNGLTEXCOORD1IPROC glTexCoord1i;
GARCH_API extern PFNGLTEXCOORD1IVPROC glTexCoord1iv;
GARCH_API extern PFNGLTEXCOORD1SPROC glTexCoord1s;
GARCH_API extern PFNGLTEXCOORD1SVPROC glTexCoord1sv;
GARCH_API extern PFNGLTEXCOORD2DPROC glTexCoord2d;
GARCH_API extern PFNGLTEXCOORD2DVPROC glTexCoord2dv;
GARCH_API extern PFNGLTEXCOORD2FPROC glTexCoord2f;
GARCH_API extern PFNGLTEXCOORD2FVPROC glTexCoord2fv;
GARCH_API extern PFNGLTEXCOORD2HNVPROC glTexCoord2hNV;
GARCH_API extern PFNGLTEXCOORD2HVNVPROC glTexCoord2hvNV;
GARCH_API extern PFNGLTEXCOORD2IPROC glTexCoord2i;
GARCH_API extern PFNGLTEXCOORD2IVPROC glTexCoord2iv;
GARCH_API extern PFNGLTEXCOORD2SPROC glTexCoord2s;
GARCH_API extern PFNGLTEXCOORD2SVPROC glTexCoord2sv;
GARCH_API extern PFNGLTEXCOORD3DPROC glTexCoord3d;
GARCH_API extern PFNGLTEXCOORD3DVPROC glTexCoord3dv;
GARCH_API extern PFNGLTEXCOORD3FPROC glTexCoord3f;
GARCH_API extern PFNGLTEXCOORD3FVPROC glTexCoord3fv;
GARCH_API extern PFNGLTEXCOORD3HNVPROC glTexCoord3hNV;
GARCH_API extern PFNGLTEXCOORD3HVNVPROC glTexCoord3hvNV;
GARCH_API extern PFNGLTEXCOORD3IPROC glTexCoord3i;
GARCH_API extern PFNGLTEXCOORD3IVPROC glTexCoord3iv;
GARCH_API extern PFNGLTEXCOORD3SPROC glTexCoord3s;
GARCH_API extern PFNGLTEXCOORD3SVPROC glTexCoord3sv;
GARCH_API extern PFNGLTEXCOORD4DPROC glTexCoord4d;
GARCH_API extern PFNGLTEXCOORD4DVPROC glTexCoord4dv;
GARCH_API extern PFNGLTEXCOORD4FPROC glTexCoord4f;
GARCH_API extern PFNGLTEXCOORD4FVPROC glTexCoord4fv;
GARCH_API extern PFNGLTEXCOORD4HNVPROC glTexCoord4hNV;
GARCH_API extern PFNGLTEXCOORD4HVNVPROC glTexCoord4hvNV;
GARCH_API extern PFNGLTEXCOORD4IPROC glTexCoord4i;
GARCH_API extern PFNGLTEXCOORD4IVPROC glTexCoord4iv;
GARCH_API extern PFNGLTEXCOORD4SPROC glTexCoord4s;
GARCH_API extern PFNGLTEXCOORD4SVPROC glTexCoord4sv;
GARCH_API extern PFNGLTEXCOORDFORMATNVPROC glTexCoordFormatNV;
GARCH_API extern PFNGLTEXCOORDP1UIPROC glTexCoordP1ui;
GARCH_API extern PFNGLTEXCOORDP1UIVPROC glTexCoordP1uiv;
GARCH_API extern PFNGLTEXCOORDP2UIPROC glTexCoordP2ui;
GARCH_API extern PFNGLTEXCOORDP2UIVPROC glTexCoordP2uiv;
GARCH_API extern PFNGLTEXCOORDP3UIPROC glTexCoordP3ui;
GARCH_API extern PFNGLTEXCOORDP3UIVPROC glTexCoordP3uiv;
GARCH_API extern PFNGLTEXCOORDP4UIPROC glTexCoordP4ui;
GARCH_API extern PFNGLTEXCOORDP4UIVPROC glTexCoordP4uiv;
GARCH_API extern PFNGLTEXCOORDPOINTERPROC glTexCoordPointer;
GARCH_API extern PFNGLTEXCOORDPOINTEREXTPROC glTexCoordPointerEXT;
GARCH_API extern PFNGLTEXCOORDPOINTERVINTELPROC glTexCoordPointervINTEL;
GARCH_API extern PFNGLTEXENVFPROC glTexEnvf;
GARCH_API extern PFNGLTEXENVFVPROC glTexEnvfv;
GARCH_API extern PFNGLTEXENVIPROC glTexEnvi;
GARCH_API extern PFNGLTEXENVIVPROC glTexEnviv;
GARCH_API extern PFNGLTEXGENDPROC glTexGend;
GARCH_API extern PFNGLTEXGENDVPROC glTexGendv;
GARCH_API extern PFNGLTEXGENFPROC glTexGenf;
GARCH_API extern PFNGLTEXGENFVPROC glTexGenfv;
GARCH_API extern PFNGLTEXGENIPROC glTexGeni;
GARCH_API extern PFNGLTEXGENIVPROC glTexGeniv;
GARCH_API extern PFNGLTEXIMAGE1DPROC glTexImage1D;
GARCH_API extern PFNGLTEXIMAGE2DPROC glTexImage2D;
GARCH_API extern PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample;
GARCH_API extern PFNGLTEXIMAGE2DMULTISAMPLECOVERAGENVPROC glTexImage2DMultisampleCoverageNV;
GARCH_API extern PFNGLTEXIMAGE3DPROC glTexImage3D;
GARCH_API extern PFNGLTEXIMAGE3DEXTPROC glTexImage3DEXT;
GARCH_API extern PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample;
GARCH_API extern PFNGLTEXIMAGE3DMULTISAMPLECOVERAGENVPROC glTexImage3DMultisampleCoverageNV;
GARCH_API extern PFNGLTEXPAGECOMMITMENTARBPROC glTexPageCommitmentARB;
GARCH_API extern PFNGLTEXPAGECOMMITMENTMEMNVPROC glTexPageCommitmentMemNV;
GARCH_API extern PFNGLTEXPARAMETERIIVPROC glTexParameterIiv;
GARCH_API extern PFNGLTEXPARAMETERIIVEXTPROC glTexParameterIivEXT;
GARCH_API extern PFNGLTEXPARAMETERIUIVPROC glTexParameterIuiv;
GARCH_API extern PFNGLTEXPARAMETERIUIVEXTPROC glTexParameterIuivEXT;
GARCH_API extern PFNGLTEXPARAMETERFPROC glTexParameterf;
GARCH_API extern PFNGLTEXPARAMETERFVPROC glTexParameterfv;
GARCH_API extern PFNGLTEXPARAMETERIPROC glTexParameteri;
GARCH_API extern PFNGLTEXPARAMETERIVPROC glTexParameteriv;
GARCH_API extern PFNGLTEXRENDERBUFFERNVPROC glTexRenderbufferNV;
GARCH_API extern PFNGLTEXSTORAGE1DPROC glTexStorage1D;
GARCH_API extern PFNGLTEXSTORAGE2DPROC glTexStorage2D;
GARCH_API extern PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexStorage2DMultisample;
GARCH_API extern PFNGLTEXSTORAGE3DPROC glTexStorage3D;
GARCH_API extern PFNGLTEXSTORAGE3DMULTISAMPLEPROC glTexStorage3DMultisample;
GARCH_API extern PFNGLTEXSTORAGEMEM1DEXTPROC glTexStorageMem1DEXT;
GARCH_API extern PFNGLTEXSTORAGEMEM2DEXTPROC glTexStorageMem2DEXT;
GARCH_API extern PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC glTexStorageMem2DMultisampleEXT;
GARCH_API extern PFNGLTEXSTORAGEMEM3DEXTPROC glTexStorageMem3DEXT;
GARCH_API extern PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC glTexStorageMem3DMultisampleEXT;
GARCH_API extern PFNGLTEXSTORAGESPARSEAMDPROC glTexStorageSparseAMD;
GARCH_API extern PFNGLTEXSUBIMAGE1DPROC glTexSubImage1D;
GARCH_API extern PFNGLTEXSUBIMAGE1DEXTPROC glTexSubImage1DEXT;
GARCH_API extern PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
GARCH_API extern PFNGLTEXSUBIMAGE2DEXTPROC glTexSubImage2DEXT;
GARCH_API extern PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;
GARCH_API extern PFNGLTEXSUBIMAGE3DEXTPROC glTexSubImage3DEXT;
GARCH_API extern PFNGLTEXTUREATTACHMEMORYNVPROC glTextureAttachMemoryNV;
GARCH_API extern PFNGLTEXTUREBARRIERPROC glTextureBarrier;
GARCH_API extern PFNGLTEXTUREBARRIERNVPROC glTextureBarrierNV;
GARCH_API extern PFNGLTEXTUREBUFFERPROC glTextureBuffer;
GARCH_API extern PFNGLTEXTUREBUFFEREXTPROC glTextureBufferEXT;
GARCH_API extern PFNGLTEXTUREBUFFERRANGEPROC glTextureBufferRange;
GARCH_API extern PFNGLTEXTUREBUFFERRANGEEXTPROC glTextureBufferRangeEXT;
GARCH_API extern PFNGLTEXTUREIMAGE1DEXTPROC glTextureImage1DEXT;
GARCH_API extern PFNGLTEXTUREIMAGE2DEXTPROC glTextureImage2DEXT;
GARCH_API extern PFNGLTEXTUREIMAGE2DMULTISAMPLECOVERAGENVPROC glTextureImage2DMultisampleCoverageNV;
GARCH_API extern PFNGLTEXTUREIMAGE2DMULTISAMPLENVPROC glTextureImage2DMultisampleNV;
GARCH_API extern PFNGLTEXTUREIMAGE3DEXTPROC glTextureImage3DEXT;
GARCH_API extern PFNGLTEXTUREIMAGE3DMULTISAMPLECOVERAGENVPROC glTextureImage3DMultisampleCoverageNV;
GARCH_API extern PFNGLTEXTUREIMAGE3DMULTISAMPLENVPROC glTextureImage3DMultisampleNV;
GARCH_API extern PFNGLTEXTURELIGHTEXTPROC glTextureLightEXT;
GARCH_API extern PFNGLTEXTUREMATERIALEXTPROC glTextureMaterialEXT;
GARCH_API extern PFNGLTEXTURENORMALEXTPROC glTextureNormalEXT;
GARCH_API extern PFNGLTEXTUREPAGECOMMITMENTEXTPROC glTexturePageCommitmentEXT;
GARCH_API extern PFNGLTEXTUREPAGECOMMITMENTMEMNVPROC glTexturePageCommitmentMemNV;
GARCH_API extern PFNGLTEXTUREPARAMETERIIVPROC glTextureParameterIiv;
GARCH_API extern PFNGLTEXTUREPARAMETERIIVEXTPROC glTextureParameterIivEXT;
GARCH_API extern PFNGLTEXTUREPARAMETERIUIVPROC glTextureParameterIuiv;
GARCH_API extern PFNGLTEXTUREPARAMETERIUIVEXTPROC glTextureParameterIuivEXT;
GARCH_API extern PFNGLTEXTUREPARAMETERFPROC glTextureParameterf;
GARCH_API extern PFNGLTEXTUREPARAMETERFEXTPROC glTextureParameterfEXT;
GARCH_API extern PFNGLTEXTUREPARAMETERFVPROC glTextureParameterfv;
GARCH_API extern PFNGLTEXTUREPARAMETERFVEXTPROC glTextureParameterfvEXT;
GARCH_API extern PFNGLTEXTUREPARAMETERIPROC glTextureParameteri;
GARCH_API extern PFNGLTEXTUREPARAMETERIEXTPROC glTextureParameteriEXT;
GARCH_API extern PFNGLTEXTUREPARAMETERIVPROC glTextureParameteriv;
GARCH_API extern PFNGLTEXTUREPARAMETERIVEXTPROC glTextureParameterivEXT;
GARCH_API extern PFNGLTEXTURERANGEAPPLEPROC glTextureRangeAPPLE;
GARCH_API extern PFNGLTEXTURERENDERBUFFEREXTPROC glTextureRenderbufferEXT;
GARCH_API extern PFNGLTEXTURESTORAGE1DPROC glTextureStorage1D;
GARCH_API extern PFNGLTEXTURESTORAGE1DEXTPROC glTextureStorage1DEXT;
GARCH_API extern PFNGLTEXTURESTORAGE2DPROC glTextureStorage2D;
GARCH_API extern PFNGLTEXTURESTORAGE2DEXTPROC glTextureStorage2DEXT;
GARCH_API extern PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC glTextureStorage2DMultisample;
GARCH_API extern PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC glTextureStorage2DMultisampleEXT;
GARCH_API extern PFNGLTEXTURESTORAGE3DPROC glTextureStorage3D;
GARCH_API extern PFNGLTEXTURESTORAGE3DEXTPROC glTextureStorage3DEXT;
GARCH_API extern PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC glTextureStorage3DMultisample;
GARCH_API extern PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC glTextureStorage3DMultisampleEXT;
GARCH_API extern PFNGLTEXTURESTORAGEMEM1DEXTPROC glTextureStorageMem1DEXT;
GARCH_API extern PFNGLTEXTURESTORAGEMEM2DEXTPROC glTextureStorageMem2DEXT;
GARCH_API extern PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC glTextureStorageMem2DMultisampleEXT;
GARCH_API extern PFNGLTEXTURESTORAGEMEM3DEXTPROC glTextureStorageMem3DEXT;
GARCH_API extern PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC glTextureStorageMem3DMultisampleEXT;
GARCH_API extern PFNGLTEXTURESTORAGESPARSEAMDPROC glTextureStorageSparseAMD;
GARCH_API extern PFNGLTEXTURESUBIMAGE1DPROC glTextureSubImage1D;
GARCH_API extern PFNGLTEXTURESUBIMAGE1DEXTPROC glTextureSubImage1DEXT;
GARCH_API extern PFNGLTEXTURESUBIMAGE2DPROC glTextureSubImage2D;
GARCH_API extern PFNGLTEXTURESUBIMAGE2DEXTPROC glTextureSubImage2DEXT;
GARCH_API extern PFNGLTEXTURESUBIMAGE3DPROC glTextureSubImage3D;
GARCH_API extern PFNGLTEXTURESUBIMAGE3DEXTPROC glTextureSubImage3DEXT;
GARCH_API extern PFNGLTEXTUREVIEWPROC glTextureView;
GARCH_API extern PFNGLTRACKMATRIXNVPROC glTrackMatrixNV;
GARCH_API extern PFNGLTRANSFORMFEEDBACKATTRIBSNVPROC glTransformFeedbackAttribsNV;
GARCH_API extern PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC glTransformFeedbackBufferBase;
GARCH_API extern PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC glTransformFeedbackBufferRange;
GARCH_API extern PFNGLTRANSFORMFEEDBACKSTREAMATTRIBSNVPROC glTransformFeedbackStreamAttribsNV;
GARCH_API extern PFNGLTRANSFORMFEEDBACKVARYINGSPROC glTransformFeedbackVaryings;
GARCH_API extern PFNGLTRANSFORMFEEDBACKVARYINGSEXTPROC glTransformFeedbackVaryingsEXT;
GARCH_API extern PFNGLTRANSFORMFEEDBACKVARYINGSNVPROC glTransformFeedbackVaryingsNV;
GARCH_API extern PFNGLTRANSFORMPATHNVPROC glTransformPathNV;
GARCH_API extern PFNGLTRANSLATEDPROC glTranslated;
GARCH_API extern PFNGLTRANSLATEFPROC glTranslatef;
GARCH_API extern PFNGLUNIFORM1DPROC glUniform1d;
GARCH_API extern PFNGLUNIFORM1DVPROC glUniform1dv;
GARCH_API extern PFNGLUNIFORM1FPROC glUniform1f;
GARCH_API extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
GARCH_API extern PFNGLUNIFORM1FVPROC glUniform1fv;
GARCH_API extern PFNGLUNIFORM1FVARBPROC glUniform1fvARB;
GARCH_API extern PFNGLUNIFORM1IPROC glUniform1i;
GARCH_API extern PFNGLUNIFORM1I64ARBPROC glUniform1i64ARB;
GARCH_API extern PFNGLUNIFORM1I64NVPROC glUniform1i64NV;
GARCH_API extern PFNGLUNIFORM1I64VARBPROC glUniform1i64vARB;
GARCH_API extern PFNGLUNIFORM1I64VNVPROC glUniform1i64vNV;
GARCH_API extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
GARCH_API extern PFNGLUNIFORM1IVPROC glUniform1iv;
GARCH_API extern PFNGLUNIFORM1IVARBPROC glUniform1ivARB;
GARCH_API extern PFNGLUNIFORM1UIPROC glUniform1ui;
GARCH_API extern PFNGLUNIFORM1UI64ARBPROC glUniform1ui64ARB;
GARCH_API extern PFNGLUNIFORM1UI64NVPROC glUniform1ui64NV;
GARCH_API extern PFNGLUNIFORM1UI64VARBPROC glUniform1ui64vARB;
GARCH_API extern PFNGLUNIFORM1UI64VNVPROC glUniform1ui64vNV;
GARCH_API extern PFNGLUNIFORM1UIEXTPROC glUniform1uiEXT;
GARCH_API extern PFNGLUNIFORM1UIVPROC glUniform1uiv;
GARCH_API extern PFNGLUNIFORM1UIVEXTPROC glUniform1uivEXT;
GARCH_API extern PFNGLUNIFORM2DPROC glUniform2d;
GARCH_API extern PFNGLUNIFORM2DVPROC glUniform2dv;
GARCH_API extern PFNGLUNIFORM2FPROC glUniform2f;
GARCH_API extern PFNGLUNIFORM2FARBPROC glUniform2fARB;
GARCH_API extern PFNGLUNIFORM2FVPROC glUniform2fv;
GARCH_API extern PFNGLUNIFORM2FVARBPROC glUniform2fvARB;
GARCH_API extern PFNGLUNIFORM2IPROC glUniform2i;
GARCH_API extern PFNGLUNIFORM2I64ARBPROC glUniform2i64ARB;
GARCH_API extern PFNGLUNIFORM2I64NVPROC glUniform2i64NV;
GARCH_API extern PFNGLUNIFORM2I64VARBPROC glUniform2i64vARB;
GARCH_API extern PFNGLUNIFORM2I64VNVPROC glUniform2i64vNV;
GARCH_API extern PFNGLUNIFORM2IARBPROC glUniform2iARB;
GARCH_API extern PFNGLUNIFORM2IVPROC glUniform2iv;
GARCH_API extern PFNGLUNIFORM2IVARBPROC glUniform2ivARB;
GARCH_API extern PFNGLUNIFORM2UIPROC glUniform2ui;
GARCH_API extern PFNGLUNIFORM2UI64ARBPROC glUniform2ui64ARB;
GARCH_API extern PFNGLUNIFORM2UI64NVPROC glUniform2ui64NV;
GARCH_API extern PFNGLUNIFORM2UI64VARBPROC glUniform2ui64vARB;
GARCH_API extern PFNGLUNIFORM2UI64VNVPROC glUniform2ui64vNV;
GARCH_API extern PFNGLUNIFORM2UIEXTPROC glUniform2uiEXT;
GARCH_API extern PFNGLUNIFORM2UIVPROC glUniform2uiv;
GARCH_API extern PFNGLUNIFORM2UIVEXTPROC glUniform2uivEXT;
GARCH_API extern PFNGLUNIFORM3DPROC glUniform3d;
GARCH_API extern PFNGLUNIFORM3DVPROC glUniform3dv;
GARCH_API extern PFNGLUNIFORM3FPROC glUniform3f;
GARCH_API extern PFNGLUNIFORM3FARBPROC glUniform3fARB;
GARCH_API extern PFNGLUNIFORM3FVPROC glUniform3fv;
GARCH_API extern PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
GARCH_API extern PFNGLUNIFORM3IPROC glUniform3i;
GARCH_API extern PFNGLUNIFORM3I64ARBPROC glUniform3i64ARB;
GARCH_API extern PFNGLUNIFORM3I64NVPROC glUniform3i64NV;
GARCH_API extern PFNGLUNIFORM3I64VARBPROC glUniform3i64vARB;
GARCH_API extern PFNGLUNIFORM3I64VNVPROC glUniform3i64vNV;
GARCH_API extern PFNGLUNIFORM3IARBPROC glUniform3iARB;
GARCH_API extern PFNGLUNIFORM3IVPROC glUniform3iv;
GARCH_API extern PFNGLUNIFORM3IVARBPROC glUniform3ivARB;
GARCH_API extern PFNGLUNIFORM3UIPROC glUniform3ui;
GARCH_API extern PFNGLUNIFORM3UI64ARBPROC glUniform3ui64ARB;
GARCH_API extern PFNGLUNIFORM3UI64NVPROC glUniform3ui64NV;
GARCH_API extern PFNGLUNIFORM3UI64VARBPROC glUniform3ui64vARB;
GARCH_API extern PFNGLUNIFORM3UI64VNVPROC glUniform3ui64vNV;
GARCH_API extern PFNGLUNIFORM3UIEXTPROC glUniform3uiEXT;
GARCH_API extern PFNGLUNIFORM3UIVPROC glUniform3uiv;
GARCH_API extern PFNGLUNIFORM3UIVEXTPROC glUniform3uivEXT;
GARCH_API extern PFNGLUNIFORM4DPROC glUniform4d;
GARCH_API extern PFNGLUNIFORM4DVPROC glUniform4dv;
GARCH_API extern PFNGLUNIFORM4FPROC glUniform4f;
GARCH_API extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
GARCH_API extern PFNGLUNIFORM4FVPROC glUniform4fv;
GARCH_API extern PFNGLUNIFORM4FVARBPROC glUniform4fvARB;
GARCH_API extern PFNGLUNIFORM4IPROC glUniform4i;
GARCH_API extern PFNGLUNIFORM4I64ARBPROC glUniform4i64ARB;
GARCH_API extern PFNGLUNIFORM4I64NVPROC glUniform4i64NV;
GARCH_API extern PFNGLUNIFORM4I64VARBPROC glUniform4i64vARB;
GARCH_API extern PFNGLUNIFORM4I64VNVPROC glUniform4i64vNV;
GARCH_API extern PFNGLUNIFORM4IARBPROC glUniform4iARB;
GARCH_API extern PFNGLUNIFORM4IVPROC glUniform4iv;
GARCH_API extern PFNGLUNIFORM4IVARBPROC glUniform4ivARB;
GARCH_API extern PFNGLUNIFORM4UIPROC glUniform4ui;
GARCH_API extern PFNGLUNIFORM4UI64ARBPROC glUniform4ui64ARB;
GARCH_API extern PFNGLUNIFORM4UI64NVPROC glUniform4ui64NV;
GARCH_API extern PFNGLUNIFORM4UI64VARBPROC glUniform4ui64vARB;
GARCH_API extern PFNGLUNIFORM4UI64VNVPROC glUniform4ui64vNV;
GARCH_API extern PFNGLUNIFORM4UIEXTPROC glUniform4uiEXT;
GARCH_API extern PFNGLUNIFORM4UIVPROC glUniform4uiv;
GARCH_API extern PFNGLUNIFORM4UIVEXTPROC glUniform4uivEXT;
GARCH_API extern PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
GARCH_API extern PFNGLUNIFORMBUFFEREXTPROC glUniformBufferEXT;
GARCH_API extern PFNGLUNIFORMHANDLEUI64ARBPROC glUniformHandleui64ARB;
GARCH_API extern PFNGLUNIFORMHANDLEUI64NVPROC glUniformHandleui64NV;
GARCH_API extern PFNGLUNIFORMHANDLEUI64VARBPROC glUniformHandleui64vARB;
GARCH_API extern PFNGLUNIFORMHANDLEUI64VNVPROC glUniformHandleui64vNV;
GARCH_API extern PFNGLUNIFORMMATRIX2DVPROC glUniformMatrix2dv;
GARCH_API extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
GARCH_API extern PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB;
GARCH_API extern PFNGLUNIFORMMATRIX2X3DVPROC glUniformMatrix2x3dv;
GARCH_API extern PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
GARCH_API extern PFNGLUNIFORMMATRIX2X4DVPROC glUniformMatrix2x4dv;
GARCH_API extern PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
GARCH_API extern PFNGLUNIFORMMATRIX3DVPROC glUniformMatrix3dv;
GARCH_API extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
GARCH_API extern PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
GARCH_API extern PFNGLUNIFORMMATRIX3X2DVPROC glUniformMatrix3x2dv;
GARCH_API extern PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
GARCH_API extern PFNGLUNIFORMMATRIX3X4DVPROC glUniformMatrix3x4dv;
GARCH_API extern PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
GARCH_API extern PFNGLUNIFORMMATRIX4DVPROC glUniformMatrix4dv;
GARCH_API extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
GARCH_API extern PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;
GARCH_API extern PFNGLUNIFORMMATRIX4X2DVPROC glUniformMatrix4x2dv;
GARCH_API extern PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
GARCH_API extern PFNGLUNIFORMMATRIX4X3DVPROC glUniformMatrix4x3dv;
GARCH_API extern PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;
GARCH_API extern PFNGLUNIFORMSUBROUTINESUIVPROC glUniformSubroutinesuiv;
GARCH_API extern PFNGLUNIFORMUI64NVPROC glUniformui64NV;
GARCH_API extern PFNGLUNIFORMUI64VNVPROC glUniformui64vNV;
GARCH_API extern PFNGLUNLOCKARRAYSEXTPROC glUnlockArraysEXT;
GARCH_API extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
GARCH_API extern PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB;
GARCH_API extern PFNGLUNMAPNAMEDBUFFERPROC glUnmapNamedBuffer;
GARCH_API extern PFNGLUNMAPNAMEDBUFFEREXTPROC glUnmapNamedBufferEXT;
GARCH_API extern PFNGLUNMAPTEXTURE2DINTELPROC glUnmapTexture2DINTEL;
GARCH_API extern PFNGLUPLOADGPUMASKNVXPROC glUploadGpuMaskNVX;
GARCH_API extern PFNGLUSEPROGRAMPROC glUseProgram;
GARCH_API extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
GARCH_API extern PFNGLUSEPROGRAMSTAGESPROC glUseProgramStages;
GARCH_API extern PFNGLUSEPROGRAMSTAGESEXTPROC glUseProgramStagesEXT;
GARCH_API extern PFNGLUSESHADERPROGRAMEXTPROC glUseShaderProgramEXT;
GARCH_API extern PFNGLVDPAUFININVPROC glVDPAUFiniNV;
GARCH_API extern PFNGLVDPAUGETSURFACEIVNVPROC glVDPAUGetSurfaceivNV;
GARCH_API extern PFNGLVDPAUINITNVPROC glVDPAUInitNV;
GARCH_API extern PFNGLVDPAUISSURFACENVPROC glVDPAUIsSurfaceNV;
GARCH_API extern PFNGLVDPAUMAPSURFACESNVPROC glVDPAUMapSurfacesNV;
GARCH_API extern PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC glVDPAURegisterOutputSurfaceNV;
GARCH_API extern PFNGLVDPAUREGISTERVIDEOSURFACENVPROC glVDPAURegisterVideoSurfaceNV;
GARCH_API extern PFNGLVDPAUREGISTERVIDEOSURFACEWITHPICTURESTRUCTURENVPROC glVDPAURegisterVideoSurfaceWithPictureStructureNV;
GARCH_API extern PFNGLVDPAUSURFACEACCESSNVPROC glVDPAUSurfaceAccessNV;
GARCH_API extern PFNGLVDPAUUNMAPSURFACESNVPROC glVDPAUUnmapSurfacesNV;
GARCH_API extern PFNGLVDPAUUNREGISTERSURFACENVPROC glVDPAUUnregisterSurfaceNV;
GARCH_API extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
GARCH_API extern PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB;
GARCH_API extern PFNGLVALIDATEPROGRAMPIPELINEPROC glValidateProgramPipeline;
GARCH_API extern PFNGLVALIDATEPROGRAMPIPELINEEXTPROC glValidateProgramPipelineEXT;
GARCH_API extern PFNGLVARIANTPOINTEREXTPROC glVariantPointerEXT;
GARCH_API extern PFNGLVARIANTBVEXTPROC glVariantbvEXT;
GARCH_API extern PFNGLVARIANTDVEXTPROC glVariantdvEXT;
GARCH_API extern PFNGLVARIANTFVEXTPROC glVariantfvEXT;
GARCH_API extern PFNGLVARIANTIVEXTPROC glVariantivEXT;
GARCH_API extern PFNGLVARIANTSVEXTPROC glVariantsvEXT;
GARCH_API extern PFNGLVARIANTUBVEXTPROC glVariantubvEXT;
GARCH_API extern PFNGLVARIANTUIVEXTPROC glVariantuivEXT;
GARCH_API extern PFNGLVARIANTUSVEXTPROC glVariantusvEXT;
GARCH_API extern PFNGLVERTEX2DPROC glVertex2d;
GARCH_API extern PFNGLVERTEX2DVPROC glVertex2dv;
GARCH_API extern PFNGLVERTEX2FPROC glVertex2f;
GARCH_API extern PFNGLVERTEX2FVPROC glVertex2fv;
GARCH_API extern PFNGLVERTEX2HNVPROC glVertex2hNV;
GARCH_API extern PFNGLVERTEX2HVNVPROC glVertex2hvNV;
GARCH_API extern PFNGLVERTEX2IPROC glVertex2i;
GARCH_API extern PFNGLVERTEX2IVPROC glVertex2iv;
GARCH_API extern PFNGLVERTEX2SPROC glVertex2s;
GARCH_API extern PFNGLVERTEX2SVPROC glVertex2sv;
GARCH_API extern PFNGLVERTEX3DPROC glVertex3d;
GARCH_API extern PFNGLVERTEX3DVPROC glVertex3dv;
GARCH_API extern PFNGLVERTEX3FPROC glVertex3f;
GARCH_API extern PFNGLVERTEX3FVPROC glVertex3fv;
GARCH_API extern PFNGLVERTEX3HNVPROC glVertex3hNV;
GARCH_API extern PFNGLVERTEX3HVNVPROC glVertex3hvNV;
GARCH_API extern PFNGLVERTEX3IPROC glVertex3i;
GARCH_API extern PFNGLVERTEX3IVPROC glVertex3iv;
GARCH_API extern PFNGLVERTEX3SPROC glVertex3s;
GARCH_API extern PFNGLVERTEX3SVPROC glVertex3sv;
GARCH_API extern PFNGLVERTEX4DPROC glVertex4d;
GARCH_API extern PFNGLVERTEX4DVPROC glVertex4dv;
GARCH_API extern PFNGLVERTEX4FPROC glVertex4f;
GARCH_API extern PFNGLVERTEX4FVPROC glVertex4fv;
GARCH_API extern PFNGLVERTEX4HNVPROC glVertex4hNV;
GARCH_API extern PFNGLVERTEX4HVNVPROC glVertex4hvNV;
GARCH_API extern PFNGLVERTEX4IPROC glVertex4i;
GARCH_API extern PFNGLVERTEX4IVPROC glVertex4iv;
GARCH_API extern PFNGLVERTEX4SPROC glVertex4s;
GARCH_API extern PFNGLVERTEX4SVPROC glVertex4sv;
GARCH_API extern PFNGLVERTEXARRAYATTRIBBINDINGPROC glVertexArrayAttribBinding;
GARCH_API extern PFNGLVERTEXARRAYATTRIBFORMATPROC glVertexArrayAttribFormat;
GARCH_API extern PFNGLVERTEXARRAYATTRIBIFORMATPROC glVertexArrayAttribIFormat;
GARCH_API extern PFNGLVERTEXARRAYATTRIBLFORMATPROC glVertexArrayAttribLFormat;
GARCH_API extern PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC glVertexArrayBindVertexBufferEXT;
GARCH_API extern PFNGLVERTEXARRAYBINDINGDIVISORPROC glVertexArrayBindingDivisor;
GARCH_API extern PFNGLVERTEXARRAYCOLOROFFSETEXTPROC glVertexArrayColorOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC glVertexArrayEdgeFlagOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYELEMENTBUFFERPROC glVertexArrayElementBuffer;
GARCH_API extern PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC glVertexArrayFogCoordOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYINDEXOFFSETEXTPROC glVertexArrayIndexOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC glVertexArrayMultiTexCoordOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYNORMALOFFSETEXTPROC glVertexArrayNormalOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYPARAMETERIAPPLEPROC glVertexArrayParameteriAPPLE;
GARCH_API extern PFNGLVERTEXARRAYRANGEAPPLEPROC glVertexArrayRangeAPPLE;
GARCH_API extern PFNGLVERTEXARRAYRANGENVPROC glVertexArrayRangeNV;
GARCH_API extern PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC glVertexArraySecondaryColorOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC glVertexArrayTexCoordOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC glVertexArrayVertexAttribBindingEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC glVertexArrayVertexAttribDivisorEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC glVertexArrayVertexAttribFormatEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC glVertexArrayVertexAttribIFormatEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC glVertexArrayVertexAttribIOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC glVertexArrayVertexAttribLFormatEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC glVertexArrayVertexAttribLOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC glVertexArrayVertexAttribOffsetEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC glVertexArrayVertexBindingDivisorEXT;
GARCH_API extern PFNGLVERTEXARRAYVERTEXBUFFERPROC glVertexArrayVertexBuffer;
GARCH_API extern PFNGLVERTEXARRAYVERTEXBUFFERSPROC glVertexArrayVertexBuffers;
GARCH_API extern PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC glVertexArrayVertexOffsetEXT;
GARCH_API extern PFNGLVERTEXATTRIB1DPROC glVertexAttrib1d;
GARCH_API extern PFNGLVERTEXATTRIB1DARBPROC glVertexAttrib1dARB;
GARCH_API extern PFNGLVERTEXATTRIB1DNVPROC glVertexAttrib1dNV;
GARCH_API extern PFNGLVERTEXATTRIB1DVPROC glVertexAttrib1dv;
GARCH_API extern PFNGLVERTEXATTRIB1DVARBPROC glVertexAttrib1dvARB;
GARCH_API extern PFNGLVERTEXATTRIB1DVNVPROC glVertexAttrib1dvNV;
GARCH_API extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
GARCH_API extern PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB;
GARCH_API extern PFNGLVERTEXATTRIB1FNVPROC glVertexAttrib1fNV;
GARCH_API extern PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
GARCH_API extern PFNGLVERTEXATTRIB1FVARBPROC glVertexAttrib1fvARB;
GARCH_API extern PFNGLVERTEXATTRIB1FVNVPROC glVertexAttrib1fvNV;
GARCH_API extern PFNGLVERTEXATTRIB1HNVPROC glVertexAttrib1hNV;
GARCH_API extern PFNGLVERTEXATTRIB1HVNVPROC glVertexAttrib1hvNV;
GARCH_API extern PFNGLVERTEXATTRIB1SPROC glVertexAttrib1s;
GARCH_API extern PFNGLVERTEXATTRIB1SARBPROC glVertexAttrib1sARB;
GARCH_API extern PFNGLVERTEXATTRIB1SNVPROC glVertexAttrib1sNV;
GARCH_API extern PFNGLVERTEXATTRIB1SVPROC glVertexAttrib1sv;
GARCH_API extern PFNGLVERTEXATTRIB1SVARBPROC glVertexAttrib1svARB;
GARCH_API extern PFNGLVERTEXATTRIB1SVNVPROC glVertexAttrib1svNV;
GARCH_API extern PFNGLVERTEXATTRIB2DPROC glVertexAttrib2d;
GARCH_API extern PFNGLVERTEXATTRIB2DARBPROC glVertexAttrib2dARB;
GARCH_API extern PFNGLVERTEXATTRIB2DNVPROC glVertexAttrib2dNV;
GARCH_API extern PFNGLVERTEXATTRIB2DVPROC glVertexAttrib2dv;
GARCH_API extern PFNGLVERTEXATTRIB2DVARBPROC glVertexAttrib2dvARB;
GARCH_API extern PFNGLVERTEXATTRIB2DVNVPROC glVertexAttrib2dvNV;
GARCH_API extern PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
GARCH_API extern PFNGLVERTEXATTRIB2FARBPROC glVertexAttrib2fARB;
GARCH_API extern PFNGLVERTEXATTRIB2FNVPROC glVertexAttrib2fNV;
GARCH_API extern PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
GARCH_API extern PFNGLVERTEXATTRIB2FVARBPROC glVertexAttrib2fvARB;
GARCH_API extern PFNGLVERTEXATTRIB2FVNVPROC glVertexAttrib2fvNV;
GARCH_API extern PFNGLVERTEXATTRIB2HNVPROC glVertexAttrib2hNV;
GARCH_API extern PFNGLVERTEXATTRIB2HVNVPROC glVertexAttrib2hvNV;
GARCH_API extern PFNGLVERTEXATTRIB2SPROC glVertexAttrib2s;
GARCH_API extern PFNGLVERTEXATTRIB2SARBPROC glVertexAttrib2sARB;
GARCH_API extern PFNGLVERTEXATTRIB2SNVPROC glVertexAttrib2sNV;
GARCH_API extern PFNGLVERTEXATTRIB2SVPROC glVertexAttrib2sv;
GARCH_API extern PFNGLVERTEXATTRIB2SVARBPROC glVertexAttrib2svARB;
GARCH_API extern PFNGLVERTEXATTRIB2SVNVPROC glVertexAttrib2svNV;
GARCH_API extern PFNGLVERTEXATTRIB3DPROC glVertexAttrib3d;
GARCH_API extern PFNGLVERTEXATTRIB3DARBPROC glVertexAttrib3dARB;
GARCH_API extern PFNGLVERTEXATTRIB3DNVPROC glVertexAttrib3dNV;
GARCH_API extern PFNGLVERTEXATTRIB3DVPROC glVertexAttrib3dv;
GARCH_API extern PFNGLVERTEXATTRIB3DVARBPROC glVertexAttrib3dvARB;
GARCH_API extern PFNGLVERTEXATTRIB3DVNVPROC glVertexAttrib3dvNV;
GARCH_API extern PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
GARCH_API extern PFNGLVERTEXATTRIB3FARBPROC glVertexAttrib3fARB;
GARCH_API extern PFNGLVERTEXATTRIB3FNVPROC glVertexAttrib3fNV;
GARCH_API extern PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
GARCH_API extern PFNGLVERTEXATTRIB3FVARBPROC glVertexAttrib3fvARB;
GARCH_API extern PFNGLVERTEXATTRIB3FVNVPROC glVertexAttrib3fvNV;
GARCH_API extern PFNGLVERTEXATTRIB3HNVPROC glVertexAttrib3hNV;
GARCH_API extern PFNGLVERTEXATTRIB3HVNVPROC glVertexAttrib3hvNV;
GARCH_API extern PFNGLVERTEXATTRIB3SPROC glVertexAttrib3s;
GARCH_API extern PFNGLVERTEXATTRIB3SARBPROC glVertexAttrib3sARB;
GARCH_API extern PFNGLVERTEXATTRIB3SNVPROC glVertexAttrib3sNV;
GARCH_API extern PFNGLVERTEXATTRIB3SVPROC glVertexAttrib3sv;
GARCH_API extern PFNGLVERTEXATTRIB3SVARBPROC glVertexAttrib3svARB;
GARCH_API extern PFNGLVERTEXATTRIB3SVNVPROC glVertexAttrib3svNV;
GARCH_API extern PFNGLVERTEXATTRIB4NBVPROC glVertexAttrib4Nbv;
GARCH_API extern PFNGLVERTEXATTRIB4NBVARBPROC glVertexAttrib4NbvARB;
GARCH_API extern PFNGLVERTEXATTRIB4NIVPROC glVertexAttrib4Niv;
GARCH_API extern PFNGLVERTEXATTRIB4NIVARBPROC glVertexAttrib4NivARB;
GARCH_API extern PFNGLVERTEXATTRIB4NSVPROC glVertexAttrib4Nsv;
GARCH_API extern PFNGLVERTEXATTRIB4NSVARBPROC glVertexAttrib4NsvARB;
GARCH_API extern PFNGLVERTEXATTRIB4NUBPROC glVertexAttrib4Nub;
GARCH_API extern PFNGLVERTEXATTRIB4NUBARBPROC glVertexAttrib4NubARB;
GARCH_API extern PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv;
GARCH_API extern PFNGLVERTEXATTRIB4NUBVARBPROC glVertexAttrib4NubvARB;
GARCH_API extern PFNGLVERTEXATTRIB4NUIVPROC glVertexAttrib4Nuiv;
GARCH_API extern PFNGLVERTEXATTRIB4NUIVARBPROC glVertexAttrib4NuivARB;
GARCH_API extern PFNGLVERTEXATTRIB4NUSVPROC glVertexAttrib4Nusv;
GARCH_API extern PFNGLVERTEXATTRIB4NUSVARBPROC glVertexAttrib4NusvARB;
GARCH_API extern PFNGLVERTEXATTRIB4BVPROC glVertexAttrib4bv;
GARCH_API extern PFNGLVERTEXATTRIB4BVARBPROC glVertexAttrib4bvARB;
GARCH_API extern PFNGLVERTEXATTRIB4DPROC glVertexAttrib4d;
GARCH_API extern PFNGLVERTEXATTRIB4DARBPROC glVertexAttrib4dARB;
GARCH_API extern PFNGLVERTEXATTRIB4DNVPROC glVertexAttrib4dNV;
GARCH_API extern PFNGLVERTEXATTRIB4DVPROC glVertexAttrib4dv;
GARCH_API extern PFNGLVERTEXATTRIB4DVARBPROC glVertexAttrib4dvARB;
GARCH_API extern PFNGLVERTEXATTRIB4DVNVPROC glVertexAttrib4dvNV;
GARCH_API extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
GARCH_API extern PFNGLVERTEXATTRIB4FARBPROC glVertexAttrib4fARB;
GARCH_API extern PFNGLVERTEXATTRIB4FNVPROC glVertexAttrib4fNV;
GARCH_API extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
GARCH_API extern PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;
GARCH_API extern PFNGLVERTEXATTRIB4FVNVPROC glVertexAttrib4fvNV;
GARCH_API extern PFNGLVERTEXATTRIB4HNVPROC glVertexAttrib4hNV;
GARCH_API extern PFNGLVERTEXATTRIB4HVNVPROC glVertexAttrib4hvNV;
GARCH_API extern PFNGLVERTEXATTRIB4IVPROC glVertexAttrib4iv;
GARCH_API extern PFNGLVERTEXATTRIB4IVARBPROC glVertexAttrib4ivARB;
GARCH_API extern PFNGLVERTEXATTRIB4SPROC glVertexAttrib4s;
GARCH_API extern PFNGLVERTEXATTRIB4SARBPROC glVertexAttrib4sARB;
GARCH_API extern PFNGLVERTEXATTRIB4SNVPROC glVertexAttrib4sNV;
GARCH_API extern PFNGLVERTEXATTRIB4SVPROC glVertexAttrib4sv;
GARCH_API extern PFNGLVERTEXATTRIB4SVARBPROC glVertexAttrib4svARB;
GARCH_API extern PFNGLVERTEXATTRIB4SVNVPROC glVertexAttrib4svNV;
GARCH_API extern PFNGLVERTEXATTRIB4UBNVPROC glVertexAttrib4ubNV;
GARCH_API extern PFNGLVERTEXATTRIB4UBVPROC glVertexAttrib4ubv;
GARCH_API extern PFNGLVERTEXATTRIB4UBVARBPROC glVertexAttrib4ubvARB;
GARCH_API extern PFNGLVERTEXATTRIB4UBVNVPROC glVertexAttrib4ubvNV;
GARCH_API extern PFNGLVERTEXATTRIB4UIVPROC glVertexAttrib4uiv;
GARCH_API extern PFNGLVERTEXATTRIB4UIVARBPROC glVertexAttrib4uivARB;
GARCH_API extern PFNGLVERTEXATTRIB4USVPROC glVertexAttrib4usv;
GARCH_API extern PFNGLVERTEXATTRIB4USVARBPROC glVertexAttrib4usvARB;
GARCH_API extern PFNGLVERTEXATTRIBBINDINGPROC glVertexAttribBinding;
GARCH_API extern PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
GARCH_API extern PFNGLVERTEXATTRIBDIVISORARBPROC glVertexAttribDivisorARB;
GARCH_API extern PFNGLVERTEXATTRIBFORMATPROC glVertexAttribFormat;
GARCH_API extern PFNGLVERTEXATTRIBFORMATNVPROC glVertexAttribFormatNV;
GARCH_API extern PFNGLVERTEXATTRIBI1IPROC glVertexAttribI1i;
GARCH_API extern PFNGLVERTEXATTRIBI1IEXTPROC glVertexAttribI1iEXT;
GARCH_API extern PFNGLVERTEXATTRIBI1IVPROC glVertexAttribI1iv;
GARCH_API extern PFNGLVERTEXATTRIBI1IVEXTPROC glVertexAttribI1ivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI1UIPROC glVertexAttribI1ui;
GARCH_API extern PFNGLVERTEXATTRIBI1UIEXTPROC glVertexAttribI1uiEXT;
GARCH_API extern PFNGLVERTEXATTRIBI1UIVPROC glVertexAttribI1uiv;
GARCH_API extern PFNGLVERTEXATTRIBI1UIVEXTPROC glVertexAttribI1uivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI2IPROC glVertexAttribI2i;
GARCH_API extern PFNGLVERTEXATTRIBI2IEXTPROC glVertexAttribI2iEXT;
GARCH_API extern PFNGLVERTEXATTRIBI2IVPROC glVertexAttribI2iv;
GARCH_API extern PFNGLVERTEXATTRIBI2IVEXTPROC glVertexAttribI2ivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI2UIPROC glVertexAttribI2ui;
GARCH_API extern PFNGLVERTEXATTRIBI2UIEXTPROC glVertexAttribI2uiEXT;
GARCH_API extern PFNGLVERTEXATTRIBI2UIVPROC glVertexAttribI2uiv;
GARCH_API extern PFNGLVERTEXATTRIBI2UIVEXTPROC glVertexAttribI2uivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI3IPROC glVertexAttribI3i;
GARCH_API extern PFNGLVERTEXATTRIBI3IEXTPROC glVertexAttribI3iEXT;
GARCH_API extern PFNGLVERTEXATTRIBI3IVPROC glVertexAttribI3iv;
GARCH_API extern PFNGLVERTEXATTRIBI3IVEXTPROC glVertexAttribI3ivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI3UIPROC glVertexAttribI3ui;
GARCH_API extern PFNGLVERTEXATTRIBI3UIEXTPROC glVertexAttribI3uiEXT;
GARCH_API extern PFNGLVERTEXATTRIBI3UIVPROC glVertexAttribI3uiv;
GARCH_API extern PFNGLVERTEXATTRIBI3UIVEXTPROC glVertexAttribI3uivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4BVPROC glVertexAttribI4bv;
GARCH_API extern PFNGLVERTEXATTRIBI4BVEXTPROC glVertexAttribI4bvEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4IPROC glVertexAttribI4i;
GARCH_API extern PFNGLVERTEXATTRIBI4IEXTPROC glVertexAttribI4iEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4IVPROC glVertexAttribI4iv;
GARCH_API extern PFNGLVERTEXATTRIBI4IVEXTPROC glVertexAttribI4ivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4SVPROC glVertexAttribI4sv;
GARCH_API extern PFNGLVERTEXATTRIBI4SVEXTPROC glVertexAttribI4svEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4UBVPROC glVertexAttribI4ubv;
GARCH_API extern PFNGLVERTEXATTRIBI4UBVEXTPROC glVertexAttribI4ubvEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4UIPROC glVertexAttribI4ui;
GARCH_API extern PFNGLVERTEXATTRIBI4UIEXTPROC glVertexAttribI4uiEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4UIVPROC glVertexAttribI4uiv;
GARCH_API extern PFNGLVERTEXATTRIBI4UIVEXTPROC glVertexAttribI4uivEXT;
GARCH_API extern PFNGLVERTEXATTRIBI4USVPROC glVertexAttribI4usv;
GARCH_API extern PFNGLVERTEXATTRIBI4USVEXTPROC glVertexAttribI4usvEXT;
GARCH_API extern PFNGLVERTEXATTRIBIFORMATPROC glVertexAttribIFormat;
GARCH_API extern PFNGLVERTEXATTRIBIFORMATNVPROC glVertexAttribIFormatNV;
GARCH_API extern PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;
GARCH_API extern PFNGLVERTEXATTRIBIPOINTEREXTPROC glVertexAttribIPointerEXT;
GARCH_API extern PFNGLVERTEXATTRIBL1DPROC glVertexAttribL1d;
GARCH_API extern PFNGLVERTEXATTRIBL1DEXTPROC glVertexAttribL1dEXT;
GARCH_API extern PFNGLVERTEXATTRIBL1DVPROC glVertexAttribL1dv;
GARCH_API extern PFNGLVERTEXATTRIBL1DVEXTPROC glVertexAttribL1dvEXT;
GARCH_API extern PFNGLVERTEXATTRIBL1I64NVPROC glVertexAttribL1i64NV;
GARCH_API extern PFNGLVERTEXATTRIBL1I64VNVPROC glVertexAttribL1i64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL1UI64ARBPROC glVertexAttribL1ui64ARB;
GARCH_API extern PFNGLVERTEXATTRIBL1UI64NVPROC glVertexAttribL1ui64NV;
GARCH_API extern PFNGLVERTEXATTRIBL1UI64VARBPROC glVertexAttribL1ui64vARB;
GARCH_API extern PFNGLVERTEXATTRIBL1UI64VNVPROC glVertexAttribL1ui64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL2DPROC glVertexAttribL2d;
GARCH_API extern PFNGLVERTEXATTRIBL2DEXTPROC glVertexAttribL2dEXT;
GARCH_API extern PFNGLVERTEXATTRIBL2DVPROC glVertexAttribL2dv;
GARCH_API extern PFNGLVERTEXATTRIBL2DVEXTPROC glVertexAttribL2dvEXT;
GARCH_API extern PFNGLVERTEXATTRIBL2I64NVPROC glVertexAttribL2i64NV;
GARCH_API extern PFNGLVERTEXATTRIBL2I64VNVPROC glVertexAttribL2i64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL2UI64NVPROC glVertexAttribL2ui64NV;
GARCH_API extern PFNGLVERTEXATTRIBL2UI64VNVPROC glVertexAttribL2ui64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL3DPROC glVertexAttribL3d;
GARCH_API extern PFNGLVERTEXATTRIBL3DEXTPROC glVertexAttribL3dEXT;
GARCH_API extern PFNGLVERTEXATTRIBL3DVPROC glVertexAttribL3dv;
GARCH_API extern PFNGLVERTEXATTRIBL3DVEXTPROC glVertexAttribL3dvEXT;
GARCH_API extern PFNGLVERTEXATTRIBL3I64NVPROC glVertexAttribL3i64NV;
GARCH_API extern PFNGLVERTEXATTRIBL3I64VNVPROC glVertexAttribL3i64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL3UI64NVPROC glVertexAttribL3ui64NV;
GARCH_API extern PFNGLVERTEXATTRIBL3UI64VNVPROC glVertexAttribL3ui64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL4DPROC glVertexAttribL4d;
GARCH_API extern PFNGLVERTEXATTRIBL4DEXTPROC glVertexAttribL4dEXT;
GARCH_API extern PFNGLVERTEXATTRIBL4DVPROC glVertexAttribL4dv;
GARCH_API extern PFNGLVERTEXATTRIBL4DVEXTPROC glVertexAttribL4dvEXT;
GARCH_API extern PFNGLVERTEXATTRIBL4I64NVPROC glVertexAttribL4i64NV;
GARCH_API extern PFNGLVERTEXATTRIBL4I64VNVPROC glVertexAttribL4i64vNV;
GARCH_API extern PFNGLVERTEXATTRIBL4UI64NVPROC glVertexAttribL4ui64NV;
GARCH_API extern PFNGLVERTEXATTRIBL4UI64VNVPROC glVertexAttribL4ui64vNV;
GARCH_API extern PFNGLVERTEXATTRIBLFORMATPROC glVertexAttribLFormat;
GARCH_API extern PFNGLVERTEXATTRIBLFORMATNVPROC glVertexAttribLFormatNV;
GARCH_API extern PFNGLVERTEXATTRIBLPOINTERPROC glVertexAttribLPointer;
GARCH_API extern PFNGLVERTEXATTRIBLPOINTEREXTPROC glVertexAttribLPointerEXT;
GARCH_API extern PFNGLVERTEXATTRIBP1UIPROC glVertexAttribP1ui;
GARCH_API extern PFNGLVERTEXATTRIBP1UIVPROC glVertexAttribP1uiv;
GARCH_API extern PFNGLVERTEXATTRIBP2UIPROC glVertexAttribP2ui;
GARCH_API extern PFNGLVERTEXATTRIBP2UIVPROC glVertexAttribP2uiv;
GARCH_API extern PFNGLVERTEXATTRIBP3UIPROC glVertexAttribP3ui;
GARCH_API extern PFNGLVERTEXATTRIBP3UIVPROC glVertexAttribP3uiv;
GARCH_API extern PFNGLVERTEXATTRIBP4UIPROC glVertexAttribP4ui;
GARCH_API extern PFNGLVERTEXATTRIBP4UIVPROC glVertexAttribP4uiv;
GARCH_API extern PFNGLVERTEXATTRIBPARAMETERIAMDPROC glVertexAttribParameteriAMD;
GARCH_API extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
GARCH_API extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
GARCH_API extern PFNGLVERTEXATTRIBPOINTERNVPROC glVertexAttribPointerNV;
GARCH_API extern PFNGLVERTEXATTRIBS1DVNVPROC glVertexAttribs1dvNV;
GARCH_API extern PFNGLVERTEXATTRIBS1FVNVPROC glVertexAttribs1fvNV;
GARCH_API extern PFNGLVERTEXATTRIBS1HVNVPROC glVertexAttribs1hvNV;
GARCH_API extern PFNGLVERTEXATTRIBS1SVNVPROC glVertexAttribs1svNV;
GARCH_API extern PFNGLVERTEXATTRIBS2DVNVPROC glVertexAttribs2dvNV;
GARCH_API extern PFNGLVERTEXATTRIBS2FVNVPROC glVertexAttribs2fvNV;
GARCH_API extern PFNGLVERTEXATTRIBS2HVNVPROC glVertexAttribs2hvNV;
GARCH_API extern PFNGLVERTEXATTRIBS2SVNVPROC glVertexAttribs2svNV;
GARCH_API extern PFNGLVERTEXATTRIBS3DVNVPROC glVertexAttribs3dvNV;
GARCH_API extern PFNGLVERTEXATTRIBS3FVNVPROC glVertexAttribs3fvNV;
GARCH_API extern PFNGLVERTEXATTRIBS3HVNVPROC glVertexAttribs3hvNV;
GARCH_API extern PFNGLVERTEXATTRIBS3SVNVPROC glVertexAttribs3svNV;
GARCH_API extern PFNGLVERTEXATTRIBS4DVNVPROC glVertexAttribs4dvNV;
GARCH_API extern PFNGLVERTEXATTRIBS4FVNVPROC glVertexAttribs4fvNV;
GARCH_API extern PFNGLVERTEXATTRIBS4HVNVPROC glVertexAttribs4hvNV;
GARCH_API extern PFNGLVERTEXATTRIBS4SVNVPROC glVertexAttribs4svNV;
GARCH_API extern PFNGLVERTEXATTRIBS4UBVNVPROC glVertexAttribs4ubvNV;
GARCH_API extern PFNGLVERTEXBINDINGDIVISORPROC glVertexBindingDivisor;
GARCH_API extern PFNGLVERTEXBLENDARBPROC glVertexBlendARB;
GARCH_API extern PFNGLVERTEXFORMATNVPROC glVertexFormatNV;
GARCH_API extern PFNGLVERTEXP2UIPROC glVertexP2ui;
GARCH_API extern PFNGLVERTEXP2UIVPROC glVertexP2uiv;
GARCH_API extern PFNGLVERTEXP3UIPROC glVertexP3ui;
GARCH_API extern PFNGLVERTEXP3UIVPROC glVertexP3uiv;
GARCH_API extern PFNGLVERTEXP4UIPROC glVertexP4ui;
GARCH_API extern PFNGLVERTEXP4UIVPROC glVertexP4uiv;
GARCH_API extern PFNGLVERTEXPOINTERPROC glVertexPointer;
GARCH_API extern PFNGLVERTEXPOINTEREXTPROC glVertexPointerEXT;
GARCH_API extern PFNGLVERTEXPOINTERVINTELPROC glVertexPointervINTEL;
GARCH_API extern PFNGLVERTEXWEIGHTPOINTEREXTPROC glVertexWeightPointerEXT;
GARCH_API extern PFNGLVERTEXWEIGHTFEXTPROC glVertexWeightfEXT;
GARCH_API extern PFNGLVERTEXWEIGHTFVEXTPROC glVertexWeightfvEXT;
GARCH_API extern PFNGLVERTEXWEIGHTHNVPROC glVertexWeighthNV;
GARCH_API extern PFNGLVERTEXWEIGHTHVNVPROC glVertexWeighthvNV;
GARCH_API extern PFNGLVIDEOCAPTURENVPROC glVideoCaptureNV;
GARCH_API extern PFNGLVIDEOCAPTURESTREAMPARAMETERDVNVPROC glVideoCaptureStreamParameterdvNV;
GARCH_API extern PFNGLVIDEOCAPTURESTREAMPARAMETERFVNVPROC glVideoCaptureStreamParameterfvNV;
GARCH_API extern PFNGLVIDEOCAPTURESTREAMPARAMETERIVNVPROC glVideoCaptureStreamParameterivNV;
GARCH_API extern PFNGLVIEWPORTPROC glViewport;
GARCH_API extern PFNGLVIEWPORTARRAYVPROC glViewportArrayv;
GARCH_API extern PFNGLVIEWPORTINDEXEDFPROC glViewportIndexedf;
GARCH_API extern PFNGLVIEWPORTINDEXEDFVPROC glViewportIndexedfv;
GARCH_API extern PFNGLVIEWPORTPOSITIONWSCALENVPROC glViewportPositionWScaleNV;
GARCH_API extern PFNGLVIEWPORTSWIZZLENVPROC glViewportSwizzleNV;
GARCH_API extern PFNGLWAITSEMAPHOREEXTPROC glWaitSemaphoreEXT;
GARCH_API extern PFNGLWAITSEMAPHOREUI64NVXPROC glWaitSemaphoreui64NVX;
GARCH_API extern PFNGLWAITSYNCPROC glWaitSync;
GARCH_API extern PFNGLWEIGHTPATHSNVPROC glWeightPathsNV;
GARCH_API extern PFNGLWEIGHTPOINTERARBPROC glWeightPointerARB;
GARCH_API extern PFNGLWEIGHTBVARBPROC glWeightbvARB;
GARCH_API extern PFNGLWEIGHTDVARBPROC glWeightdvARB;
GARCH_API extern PFNGLWEIGHTFVARBPROC glWeightfvARB;
GARCH_API extern PFNGLWEIGHTIVARBPROC glWeightivARB;
GARCH_API extern PFNGLWEIGHTSVARBPROC glWeightsvARB;
GARCH_API extern PFNGLWEIGHTUBVARBPROC glWeightubvARB;
GARCH_API extern PFNGLWEIGHTUIVARBPROC glWeightuivARB;
GARCH_API extern PFNGLWEIGHTUSVARBPROC glWeightusvARB;
GARCH_API extern PFNGLWINDOWPOS2DPROC glWindowPos2d;
GARCH_API extern PFNGLWINDOWPOS2DARBPROC glWindowPos2dARB;
GARCH_API extern PFNGLWINDOWPOS2DVPROC glWindowPos2dv;
GARCH_API extern PFNGLWINDOWPOS2DVARBPROC glWindowPos2dvARB;
GARCH_API extern PFNGLWINDOWPOS2FPROC glWindowPos2f;
GARCH_API extern PFNGLWINDOWPOS2FARBPROC glWindowPos2fARB;
GARCH_API extern PFNGLWINDOWPOS2FVPROC glWindowPos2fv;
GARCH_API extern PFNGLWINDOWPOS2FVARBPROC glWindowPos2fvARB;
GARCH_API extern PFNGLWINDOWPOS2IPROC glWindowPos2i;
GARCH_API extern PFNGLWINDOWPOS2IARBPROC glWindowPos2iARB;
GARCH_API extern PFNGLWINDOWPOS2IVPROC glWindowPos2iv;
GARCH_API extern PFNGLWINDOWPOS2IVARBPROC glWindowPos2ivARB;
GARCH_API extern PFNGLWINDOWPOS2SPROC glWindowPos2s;
GARCH_API extern PFNGLWINDOWPOS2SARBPROC glWindowPos2sARB;
GARCH_API extern PFNGLWINDOWPOS2SVPROC glWindowPos2sv;
GARCH_API extern PFNGLWINDOWPOS2SVARBPROC glWindowPos2svARB;
GARCH_API extern PFNGLWINDOWPOS3DPROC glWindowPos3d;
GARCH_API extern PFNGLWINDOWPOS3DARBPROC glWindowPos3dARB;
GARCH_API extern PFNGLWINDOWPOS3DVPROC glWindowPos3dv;
GARCH_API extern PFNGLWINDOWPOS3DVARBPROC glWindowPos3dvARB;
GARCH_API extern PFNGLWINDOWPOS3FPROC glWindowPos3f;
GARCH_API extern PFNGLWINDOWPOS3FARBPROC glWindowPos3fARB;
GARCH_API extern PFNGLWINDOWPOS3FVPROC glWindowPos3fv;
GARCH_API extern PFNGLWINDOWPOS3FVARBPROC glWindowPos3fvARB;
GARCH_API extern PFNGLWINDOWPOS3IPROC glWindowPos3i;
GARCH_API extern PFNGLWINDOWPOS3IARBPROC glWindowPos3iARB;
GARCH_API extern PFNGLWINDOWPOS3IVPROC glWindowPos3iv;
GARCH_API extern PFNGLWINDOWPOS3IVARBPROC glWindowPos3ivARB;
GARCH_API extern PFNGLWINDOWPOS3SPROC glWindowPos3s;
GARCH_API extern PFNGLWINDOWPOS3SARBPROC glWindowPos3sARB;
GARCH_API extern PFNGLWINDOWPOS3SVPROC glWindowPos3sv;
GARCH_API extern PFNGLWINDOWPOS3SVARBPROC glWindowPos3svARB;
GARCH_API extern PFNGLWINDOWRECTANGLESEXTPROC glWindowRectanglesEXT;
GARCH_API extern PFNGLWRITEMASKEXTPROC glWriteMaskEXT;
GARCH_API extern PFNGLDRAWVKIMAGENVPROC glDrawVkImageNV;
GARCH_API extern PFNGLGETVKPROCADDRNVPROC glGetVkProcAddrNV;
GARCH_API extern PFNGLWAITVKSEMAPHORENVPROC glWaitVkSemaphoreNV;
GARCH_API extern PFNGLSIGNALVKSEMAPHORENVPROC glSignalVkSemaphoreNV;
GARCH_API extern PFNGLSIGNALVKFENCENVPROC glSignalVkFenceNV;


GARCH_API const GLubyte * gluErrorString(GLenum error);


GARCH_API bool GarchGLApiLoad();
GARCH_API void GarchGLApiUnload();


}  // namespace GLApi
}  // namespace internal
PXR_NAMESPACE_CLOSE_SCOPE

//
// Initialization:
//
//     #include "pxr/imaging/garch/glApi.h"
//     ...
//     GarchGLApiLoad();
//     ...
//
// GL version or GL extension dependent code:
//
// The best practice is to check for both runtime and
// compile time availabilty, e.g.
//
//     #if defined(GL_VERSION_4_0)
//     if (GARCH_GLAPI_HAS(VERSION_4_0)) {
//         // ... GL_VERSION_4_0 specific code
//     }
//     #endif
//
// and
//
//     #if defined(GL_ARB_tessellation_shader)
//     if (GARCH_GLAPI_HAS(ARB_tessellation_shader)) {
//         // ... ARB_tessellation_shader specific code
//     }
//     #endif
//

#define GARCH_GLAPI_HAS(token) (GARCH_GL_##token)

using namespace PXR_NS::internal::GLApi;

#endif // __gl3_h_
#endif // __gl_h_

#endif
