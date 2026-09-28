#ifndef SR_NAMESPACE_H
#define SR_NAMESPACE_H

/*
 * Symbol namespacing for the per-scale core builds.
 *
 * The renderer core is compiled ONCE PER SCALE, each time with SOFTRDP_SCALE as
 * a literal, so each copy is exactly the code a single-scale build produces -
 * the preprocessor removes the other scale before the compiler ever sees it.
 * That is the only way to get both scales into one binary with neither paying
 * for the other; asking the optimiser to reconstruct the constant through a
 * runtime argument works until some function stops being inlined, and then it
 * silently does not.
 *
 * The two copies would collide at link time, so this header renames every
 * symbol the core exports. It is force-included by the build (-include) with
 * SR_SCALE_SUFFIX set per copy, so no source file mentions it.
 *
 * Deliberately NOT applied to the platform thread pool: it has no scale
 * dependency and the pool is process-wide, so both cores must share one.
 *
 * If a new non-static function is added to the core and not listed here, the
 * link fails with a duplicate symbol. That is the intended failure mode - loud,
 * at build time, rather than two cores quietly sharing one function.
 *
 * Regenerate with:
 *   nm -g --defined-only <core objects> | awk '$2=="T"{print $3}' | sort -u
 */

#ifndef SR_SCALE_SUFFIX
#error "SR_SCALE_SUFFIX must be defined when building a core copy"
#endif

#define SR_NS_JOIN(a, b) a##b
#define SR_NS_EVAL(a, b) SR_NS_JOIN(a, b)
#define SR_NS(name) SR_NS_EVAL(name, SR_SCALE_SUFFIX)

#define framebuffer_fill_rect SR_NS(framebuffer_fill_rect)
#define kernel_render_span SR_NS(kernel_render_span)
#define primitive_cache_invalidate SR_NS(primitive_cache_invalidate)
#define primitive_cache_reset SR_NS(primitive_cache_reset)
#define primitive_compile_color_rectangle SR_NS(primitive_compile_color_rectangle)
#define primitive_compile_framebuffer SR_NS(primitive_compile_framebuffer)
#define primitive_compile_rectangle SR_NS(primitive_compile_rectangle)
#define primitive_compile_triangle SR_NS(primitive_compile_triangle)
#define primitive_compile_triangle_cached SR_NS(primitive_compile_triangle_cached)
#define primitive_tile_bounds SR_NS(primitive_tile_bounds)
#define raster_setup_rectangle_span SR_NS(raster_setup_rectangle_span)
#define raster_setup_triangle_span SR_NS(raster_setup_triangle_span)
#define raster_decode_triangle SR_NS(raster_decode_triangle)
#define raster_submit_rectangle SR_NS(raster_submit_rectangle)
#define raster_submit_fill_rectangle SR_NS(raster_submit_fill_rectangle)
#define raster_submit_triangle SR_NS(raster_submit_triangle)
#define rdp_blender_decode SR_NS(rdp_blender_decode)
#define rdp_combiner_decode SR_NS(rdp_combiner_decode)
#define rdp_combiner_make_passthrough SR_NS(rdp_combiner_make_passthrough)
#define rdp_command_is_draw SR_NS(rdp_command_is_draw)
#define rdp_command_name SR_NS(rdp_command_name)
#define rdp_command_word_count SR_NS(rdp_command_word_count)
#define rdp_decode_command SR_NS(rdp_decode_command)
#define rdp_execute_command SR_NS(rdp_execute_command)
#define sr_create SR_NS(sr_create)
#define sr_debug_read_sample SR_NS(sr_debug_read_sample)
#define sr_destroy SR_NS(sr_destroy)
#define sr_flush SR_NS(sr_flush)
#define sr_get_debug_stats SR_NS(sr_get_debug_stats)
#define sr_get_vi_frame_info SR_NS(sr_get_vi_frame_info)
#define sr_load_state SR_NS(sr_load_state)
#define sr_memory_init SR_NS(sr_memory_init)
#define sr_memory_release SR_NS(sr_memory_release)
#define sr_process_rdp_list SR_NS(sr_process_rdp_list)
#define sr_save_state SR_NS(sr_save_state)
#define sr_set_host SR_NS(sr_set_host)
#define sr_set_vi_scanline_registers SR_NS(sr_set_vi_scanline_registers)
#define sr_state_snapshot_size SR_NS(sr_state_snapshot_size)
#define sr_update_screen SR_NS(sr_update_screen)
#define stage_blend_divider_table SR_NS(stage_blend_divider_table)
#define tmem_init SR_NS(tmem_init)
#define tmem_load_tile SR_NS(tmem_load_tile)
#define tmem_palette_refresh SR_NS(tmem_palette_refresh)
#define tmem_rgba16_decode_table SR_NS(tmem_rgba16_decode_table)
#define vi_build_scanout_plan SR_NS(vi_build_scanout_plan)
#define vi_execute_scanout SR_NS(vi_execute_scanout)
#define vi_execute_scanout_threaded SR_NS(vi_execute_scanout_threaded)
#define vi_init SR_NS(vi_init)
#define vi_latch_registers SR_NS(vi_latch_registers)

#endif
