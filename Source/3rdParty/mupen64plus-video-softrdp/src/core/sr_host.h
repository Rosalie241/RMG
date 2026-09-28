#ifndef SR_HOST_H
#define SR_HOST_H

#include "sr_defs.h"

typedef enum sr_dp_register {
    SR_DP_START = 0,
    SR_DP_END,
    SR_DP_CURRENT,
    SR_DP_STATUS,
    SR_DP_CLOCK,
    SR_DP_BUFBUSY,
    SR_DP_PIPEBUSY,
    SR_DP_TMEM,
    SR_DP_REGISTER_COUNT
} sr_dp_register;

typedef enum sr_vi_register {
    SR_VI_STATUS = 0,
    SR_VI_ORIGIN,
    SR_VI_WIDTH,
    SR_VI_INTR,
    SR_VI_CURRENT,
    SR_VI_TIMING,
    SR_VI_V_SYNC,
    SR_VI_H_SYNC,
    SR_VI_LEAP,
    SR_VI_H_START,
    SR_VI_V_START,
    SR_VI_V_BURST,
    SR_VI_X_SCALE,
    SR_VI_Y_SCALE,
    SR_VI_REGISTER_COUNT
} sr_vi_register;

typedef struct sr_host_interface {
    /*
     * Internal resolution: 1 = native, 2 = a 2x2 sample grid per pixel. 0 means
     * "no preference" and is read as 1; out-of-range values are clamped.
     *
     * The renderer core is compiled once per scale and this selects WHICH core
     * runs - see sr_dispatch.c. A core itself never reads it: inside a core the
     * scale is a compile-time constant, exactly as it was when the plugin could
     * only be built for one scale.
     */
    uint32_t scale;

    uint32_t workers;
    bool disable_vi_dither_filter;
    bool disable_vi_divot_filter;
    bool disable_vi_gamma_dither;
    bool disable_vi_aa;
    uint8_t *rdram;
    uint32_t rdram_size;
    uint8_t *hidden_rdram;
    uint32_t hidden_rdram_size;
    uint8_t *dmem;
    bool dmem_big_endian;
    uint32_t *dp_regs[SR_DP_REGISTER_COUNT];
    uint32_t *vi_regs[SR_VI_REGISTER_COUNT];
    uint32_t *mi_intr_reg;
    void (*raise_mi_interrupt)(void *userdata);
    void (*trace_rdp_command)(void *userdata, uint32_t address,
                              uint32_t id, const uint32_t *words,
                              uint32_t word_count);
    void *userdata;
} sr_host_interface;

#endif
