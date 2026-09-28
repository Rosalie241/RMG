#include "sr_field_output.h"

#include <stdlib.h>
#include <string.h>

void sr_field_output_reset(sr_field_output *output)
{
    if (!output) return;
    free(output->pixels);
    memset(output, 0, sizeof(*output));
}

bool sr_field_output_compose(sr_field_output *output,
                             const sr_rgba8 *field_pixels,
                             uint32_t width, uint32_t height,
                             uint32_t stride, bool interlaced, bool field,
                             const sr_rgba8 **pixels, uint32_t *output_height)
{
    if (!output || !field_pixels || !width || !height || stride < width ||
        !pixels || !output_height) return false;

    if (!interlaced) {
        /* Invalidate retained fields. */
        output->width = 0;
        output->height = 0;
        *pixels = field_pixels;
        *output_height = height;
        return true;
    }

    if (height > UINT32_MAX / 2u ||
        (size_t)width > SIZE_MAX / (size_t)(height * 2u) / sizeof(sr_rgba8))
        return false;

    const uint32_t full_height = height * 2u;
    if (output->width != width || output->height != full_height) {
        sr_rgba8 *buffer = realloc(output->pixels,
                                  (size_t)width * full_height * sizeof(*buffer));
        if (!buffer) return false;
        output->pixels = buffer;
        output->width = width;
        output->height = full_height;
        /* Seed both parities until the next field arrives. */
        for (uint32_t y = 0; y < height; y++) {
            const sr_rgba8 *source = field_pixels + (size_t)y * stride;
            memcpy(buffer + (size_t)(y * 2u) * width, source,
                   (size_t)width * sizeof(*source));
            memcpy(buffer + (size_t)(y * 2u + 1u) * width, source,
                   (size_t)width * sizeof(*source));
        }
    }

    /* Field 0 occupies odd rows. */
    const uint32_t parity = !field;
    for (uint32_t y = 0; y < height; y++)
        memcpy(output->pixels + ((size_t)y * 2u + parity) * width,
               field_pixels + (size_t)y * stride,
               (size_t)width * sizeof(*field_pixels));

    *pixels = output->pixels;
    *output_height = full_height;
    return true;
}
