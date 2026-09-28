#ifndef SR_FIELD_OUTPUT_H
#define SR_FIELD_OUTPUT_H

#include "../core/sr_defs.h"

#include <stdbool.h>
#include <stdint.h>

/* Interleave fields while retaining the opposite field's rows. */
typedef struct sr_field_output {
    sr_rgba8 *pixels;
    uint32_t width;
    uint32_t height;
} sr_field_output;

void sr_field_output_reset(sr_field_output *output);
bool sr_field_output_compose(sr_field_output *output,
                             const sr_rgba8 *field_pixels,
                             uint32_t width, uint32_t height,
                             uint32_t stride, bool interlaced, bool field,
                             const sr_rgba8 **pixels, uint32_t *output_height);

#endif
