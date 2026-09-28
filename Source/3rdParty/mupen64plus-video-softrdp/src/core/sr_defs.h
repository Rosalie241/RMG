#ifndef SR_DEFS_H
#define SR_DEFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SR_RDRAM_MAX_SIZE (8u * 1024u * 1024u)
#define SR_DMEM_SIZE 0x1000u
#define SR_TMEM_SIZE 0x1000u
#define SR_MAX_COMMAND_WORDS 44u

#define SR_ALWAYS_INLINE __attribute__((always_inline)) inline

typedef enum sr_result {
    SR_OK = 0,
    SR_ERROR_INVALID_ARGUMENT,
    SR_ERROR_BAD_COMMAND,
    SR_ERROR_UNSUPPORTED
} sr_result;

typedef enum sr_pixel_format {
    SR_PIXEL_FORMAT_RGBA5551 = 0,
    SR_PIXEL_FORMAT_RGBA8888 = 1
} sr_pixel_format;

typedef struct sr_rgba8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} sr_rgba8;

/*
 * For sr_update_screen(), pixels/height/stride_pixels are inputs that describe
 * caller-owned storage. width may be zero to accept the VI width. On return,
 * width/height/stride_pixels describe the converted image and valid reports
 * whether pixels were written.
 */
typedef struct sr_framebuffer {
    sr_rgba8 *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t stride_pixels;
    bool valid;
} sr_framebuffer;

#endif
