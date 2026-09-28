#ifndef RDP_COMMANDS_H
#define RDP_COMMANDS_H

#include "rdp_state.h"

typedef struct sr_memory sr_memory;
typedef struct tmem_state tmem_state;

typedef enum rdp_command_id {
    RDP_CMD_NO_OP = 0x00,
    RDP_CMD_FILL_TRIANGLE = 0x08,
    RDP_CMD_FILL_ZBUFFER_TRIANGLE = 0x09,
    RDP_CMD_TEXTURE_TRIANGLE = 0x0a,
    RDP_CMD_TEXTURE_ZBUFFER_TRIANGLE = 0x0b,
    RDP_CMD_SHADE_TRIANGLE = 0x0c,
    RDP_CMD_SHADE_ZBUFFER_TRIANGLE = 0x0d,
    RDP_CMD_SHADE_TEXTURE_TRIANGLE = 0x0e,
    RDP_CMD_SHADE_TEXTURE_ZBUFFER_TRIANGLE = 0x0f,
    RDP_CMD_TEXTURE_RECTANGLE = 0x24,
    RDP_CMD_TEXTURE_RECTANGLE_FLIP = 0x25,
    RDP_CMD_SYNC_LOAD = 0x26,
    RDP_CMD_SYNC_PIPE = 0x27,
    RDP_CMD_SYNC_TILE = 0x28,
    RDP_CMD_SYNC_FULL = 0x29,
    RDP_CMD_SET_KEY_GB = 0x2a,
    RDP_CMD_SET_KEY_R = 0x2b,
    RDP_CMD_SET_CONVERT = 0x2c,
    RDP_CMD_SET_SCISSOR = 0x2d,
    RDP_CMD_SET_PRIM_DEPTH = 0x2e,
    RDP_CMD_SET_OTHER_MODES = 0x2f,
    RDP_CMD_LOAD_TLUT = 0x30,
    RDP_CMD_SET_TILE_SIZE = 0x32,
    RDP_CMD_LOAD_BLOCK = 0x33,
    RDP_CMD_LOAD_TILE = 0x34,
    RDP_CMD_SET_TILE = 0x35,
    RDP_CMD_FILL_RECTANGLE = 0x36,
    RDP_CMD_SET_FILL_COLOR = 0x37,
    RDP_CMD_SET_FOG_COLOR = 0x38,
    RDP_CMD_SET_BLEND_COLOR = 0x39,
    RDP_CMD_SET_PRIM_COLOR = 0x3a,
    RDP_CMD_SET_ENV_COLOR = 0x3b,
    RDP_CMD_SET_COMBINE = 0x3c,
    RDP_CMD_SET_TEXTURE_IMAGE = 0x3d,
    RDP_CMD_SET_MASK_IMAGE = 0x3e,
    RDP_CMD_SET_COLOR_IMAGE = 0x3f
} rdp_command_id;

typedef struct raster_triangle_setup {
    int32_t xh, xm, xl;
    int16_t yh, ym, yl;
    int32_t dxhdy, dxmdy, dxldy;
    uint8_t tile;
    uint8_t max_level;
    bool flip;
} raster_triangle_setup;

typedef struct raster_shade_setup {
    int32_t r, g, b, a;
    int32_t drdx, dgdx, dbdx, dadx;
    int32_t drde, dgde, dbde, dade;
    int32_t drdy, dgdy, dbdy, dady;
} raster_shade_setup;

typedef struct raster_texture_setup {
    int32_t s, t, w;
    int32_t dsdx, dtdx, dwdx;
    int32_t dsde, dtde, dwde;
    int32_t dsdy, dtdy, dwdy;
} raster_texture_setup;

typedef struct raster_depth_setup {
    int32_t z, dzdx, dzde, dzdy;
} raster_depth_setup;

typedef struct raster_decoded_triangle {
    raster_triangle_setup position;
    raster_shade_setup shade;
    raster_texture_setup texture;
    raster_depth_setup depth;
    bool has_shade;
    bool has_texture;
    bool has_depth;
} raster_decoded_triangle;

typedef rdp_image rdp_set_color_image_cmd;
typedef rdp_image rdp_set_texture_image_cmd;
typedef rdp_other_modes rdp_set_other_modes_cmd;

typedef struct rdp_set_mask_image_cmd {
    uint32_t address;
} rdp_set_mask_image_cmd;

typedef struct rdp_set_scissor_cmd {
    uint16_t x0, y0, x1, y1;
    bool field;
    bool keep_odd;
} rdp_set_scissor_cmd;

typedef struct rdp_set_tile_cmd {
    uint8_t tile_index;
    uint16_t tmem, line;
    rdp_texture_size size;
    rdp_texture_format format;
    uint8_t palette, shift_t, mask_t, mirror_t, clamp_t, shift_s, mask_s, mirror_s, clamp_s;
} rdp_set_tile_cmd;

typedef struct rdp_set_tile_size_cmd {
    uint8_t tile_index;
    uint16_t sl, tl, sh, th;
} rdp_set_tile_size_cmd;

typedef struct rdp_set_fill_color_cmd {
    uint32_t fill_color;
} rdp_set_fill_color_cmd;

typedef struct rdp_set_color_cmd {
    rdp_color color;
} rdp_set_color_cmd;

typedef struct rdp_set_prim_color_cmd {
    uint8_t min_lod, lod_fraction;
    rdp_color color;
} rdp_set_prim_color_cmd;

typedef struct rdp_set_prim_depth_cmd {
    uint16_t depth, delta_z;
} rdp_set_prim_depth_cmd;

typedef struct rdp_set_key_cmd {
    rdp_color center;
    rdp_color scale;
} rdp_set_key_cmd;

typedef rdp_combiner_program rdp_set_combine_cmd;

typedef struct rdp_rect_cmd {
    uint32_t x0, y0, x1, y1;
    /* Raw 10.2 edges. x0/y0/x1/y1 above are the whole-pixel extent the span
     * loops walk; these keep the quarter-pixel fraction that edge coverage
     * needs. xl/yl are the exclusive far edges, xh/yh the inclusive near ones. */
    uint16_t xl, yl;
    uint16_t xh, yh;
    uint8_t tile_index;
    int32_t s0, t0, dsdx, dtdy;
    bool flip;
} rdp_rect_cmd;

typedef struct rdp_load_cmd {
    uint8_t tile_index;
    uint16_t sl, tl, sh;
    union {
        uint16_t th;
        uint16_t dxt;
    };
} rdp_load_cmd;

typedef struct rdp_command {
    rdp_command_id id;
    uint8_t word_count;
    uint32_t words[SR_MAX_COMMAND_WORDS];
    union {
        rdp_set_color_image_cmd set_color_image;
        rdp_set_texture_image_cmd set_texture_image;
        rdp_set_mask_image_cmd set_mask_image;
        rdp_set_scissor_cmd set_scissor;
        rdp_set_other_modes_cmd set_other_modes;
        rdp_set_tile_cmd set_tile;
        rdp_set_tile_size_cmd set_tile_size;
        rdp_set_fill_color_cmd set_fill_color;
        rdp_set_color_cmd set_fog_color;
        rdp_set_color_cmd set_blend_color;
        rdp_set_color_cmd set_env_color;
        rdp_set_prim_color_cmd set_prim_color;
        rdp_set_prim_depth_cmd set_prim_depth;
        rdp_set_key_cmd set_key;
        rdp_set_combine_cmd set_combine;
        raster_decoded_triangle triangle;
        rdp_rect_cmd rect;
        rdp_load_cmd load;
    } decoded;
} rdp_command;

uint8_t rdp_command_word_count(rdp_command_id id);
void rdp_combiner_decode(rdp_combiner_program *program, uint32_t w0, uint32_t w1);
void rdp_combiner_make_passthrough(rdp_combiner_program *program,
                                   rdp_combiner_source rgb,
                                   rdp_combiner_source alpha);
void rdp_blender_decode(rdp_blender_program *program, uint32_t w1);

bool rdp_command_is_draw(rdp_command_id id);
const char *rdp_command_name(rdp_command_id id);
sr_result rdp_decode_command(rdp_command *cmd);
sr_result rdp_execute_command(sr_memory *memory,
                              tmem_state *tmem,
                              rdp_state *state,
                              const rdp_command *cmd);

#endif
