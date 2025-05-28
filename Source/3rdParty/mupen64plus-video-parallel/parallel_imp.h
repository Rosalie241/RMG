#pragma once

#include "gfx_m64p.h"
#include "screen.h"
#include "gfxstructdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern int32_t vk_rescaling;
    extern bool vk_ssreadbacks;
    extern bool vk_ssdither;

    extern uint32_t vk_overscan_left;
    extern uint32_t vk_overscan_right;
    extern uint32_t vk_overscan_top;
    extern uint32_t vk_overscan_bottom;
    extern bool vk_overscan_enable;
    extern uint32_t vk_vertical_stretch;
    extern uint32_t vk_downscaling_steps;
    extern bool vk_native_texture_lod;
    extern bool vk_native_tex_rect;
    extern bool vk_divot_filter, vk_gamma_dither;
    extern bool vk_vi_aa, vk_vi_scale, vk_dither_filter;
    extern bool vk_interlacing;
    extern bool vk_synchronous;

    void vk_rasterize();
    void vk_process_commands();
    bool vk_init();
    void vk_destroy();
    void vk_resize();
    void vk_read_screen(unsigned char* dest);

#ifdef __cplusplus
}
#endif
