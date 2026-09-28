#include "tmem.h"

#include "rdp_commands.h"
#include "rdp_memory.h"

#include <string.h>

rdp_color tmem_rgba16_decode_table[UINT16_MAX + 1u];

void tmem_init(tmem_state *tmem)
{
    static bool decode_table_ready;
    if (!decode_table_ready) {
        for (uint32_t texel = 0; texel <= UINT16_MAX; texel++)
            tmem_rgba16_decode_table[texel] = rdp_color_from_rgba5551((uint16_t)texel);
        decode_table_ready = true;
    }
    memset(tmem, 0, sizeof(*tmem));
    tmem_palette_refresh(tmem);
}

/* Palette entry i's four TMEM bank copies are the halfwords of upper-TMEM word
 * i. Only words that differ from palette_known are decoded again, which keeps
 * the refresh after every TMEM load cheap. */
void tmem_palette_refresh(tmem_state *tmem)
{
    uint16_t mixed = 0u;
    for (uint32_t index = 0; index < 256u; index++) {
        const uint64_t copies = tmem->qwords[0x100u + index];
        if (((copies ^ (copies >> 16)) & 0x0000ffffffffffffull) != 0u)
            mixed |= (uint16_t)(1u << (index >> 4));
        if (copies == tmem->palette_known[index]) continue;
        tmem->palette_known[index] = copies;
        const uint16_t *lanes = &tmem->words[0x400u + (index << 2)];
        for (uint32_t lane = 0; lane < 4u; lane++)
            tmem->palette_lanes[(index << 2) | lane] = tmem_rgba16_decode_table[lanes[lane]];
        const uint8_t intensity = (uint8_t)(lanes[0] >> 8);
        tmem->palette[0][index] = tmem->palette_lanes[index << 2];
        tmem->palette[1][index] = (rdp_color){ intensity, intensity, intensity,
                                               (uint8_t)lanes[0] };
    }
    tmem->palette_mixed = mixed;
}

/* One RDRAM byte in big-endian order; the load unit's reads wrap like RDRAM. */
static inline uint8_t load_read_u8(const sr_memory *memory, uint32_t addr)
{
    return memory->rdram[mask_addr(memory, addr) ^ 3u];
}

/* The load unit reads eight bytes from any byte address. */
static inline uint64_t load_read_u64(const sr_memory *memory, uint32_t addr)
{
    const uint32_t masked = mask_addr(memory, addr);
    if (!(masked & 3u) && masked + 8u <= memory->rdram_size) {
        const uint32_t *words =
            (const uint32_t *)(const void *)&memory->rdram[masked];
        return ((uint64_t)words[0] << 32) | words[1];
    }
    uint64_t value = 0;
    for (uint32_t i = 0; i < 8u; i++)
        value = (value << 8) | load_read_u8(memory, addr + i);
    return value;
}

/* The eight bytes from a byte address, wrapping inside its aligned RDRAM
 * dword: the dword rotated left by the address's byte offset. */
static inline uint64_t load_read_u64_wrapped(const sr_memory *memory, uint32_t addr)
{
    const uint64_t dword = load_read_u64(memory, addr & ~7u);
    const uint32_t rotate = (addr & 7u) * 8u;
    return rotate ? (dword << rotate) | (dword >> (64u - rotate)) : dword;
}

/*
 * Every load runs through the load pipeline one 64-bit step at a time. A
 * line's steps start at its first texel's byte, and each lands where the
 * TILE's texel size, not the image's, puts the step's S and T - so a narrower
 * tile makes later steps overwrite earlier ones. S and T are the pipeline's
 * 16-bit 10.5 coordinates relative to the load origin. LoadTLUT and LoadBlock
 * read them in quarter units; LoadTLUT replicates each 16-bit entry, and
 * LoadBlock walks a single line whose T advances by DxT on every step.
 */
static sr_result load_lines(tmem_state *tmem, const sr_memory *memory,
                            const rdp_state *state, const rdp_tile *tile,
                            const rdp_command *cmd, bool *wrote_upper)
{
    const bool tlut = cmd->id == RDP_CMD_LOAD_TLUT;
    const bool block = cmd->id == RDP_CMD_LOAD_BLOCK;
    const uint32_t size = state->texture_image.size;
    /* LoadBlock's S and T are whole texels, the other loads' are 10.2. */
    const uint32_t coord_frac = block ? 0u : 2u;
    const uint32_t first_line = block ? cmd->decoded.load.tl & 0x3ffu
                                      : cmd->decoded.load.tl >> 2u;
    const uint32_t last_line = block ? first_line : cmd->decoded.load.th >> 2u;
    if (!memory->rdram) return SR_ERROR_INVALID_ARGUMENT;

    /* A 4bpp source takes four texels a step but moves the source on by one
     * byte, and every step reads the dword it starts in rotated to that byte.
     * A multi-line TLUT loads each line over the same rows. */
    const bool nibble_source = size == RDP_SIZE_4BPP;
    const bool entry_steps = tlut && size == RDP_SIZE_16BPP;
    const uint32_t step_bytes = entry_steps ? 2u : nibble_source ? 1u : 8u;
    const uint32_t step_texels = entry_steps ? 1u : nibble_source ? 4u : 16u >> size;
    const int32_t step_s = (block ? 0x80 : 0x200) >> size;
    /* DxT is 1.11 lines per step; T holds it in 1/256ths of a 10.5 unit. */
    const int32_t step_t = block ? (int32_t)cmd->decoded.load.dxt : 0;
    const uint32_t coord_shift = (tlut || block) ? 3u : 5u;
    const uint32_t first_texel = cmd->decoded.load.sl >> coord_frac;
    const uint32_t texels =
        ((cmd->decoded.load.sh >> coord_frac) - first_texel + 1u) & 0xfffu;
    const uint32_t steps = (texels + step_texels - 1u) / step_texels;

    const bool split_yuv = tile->format == RDP_FORMAT_YUV;
    const bool split_rgba32 = !split_yuv && tile->format == RDP_FORMAT_RGBA &&
                              tile->size == RDP_SIZE_32BPP;
    const uint32_t halfword_shift =
        (tile->size == RDP_SIZE_8BPP || split_yuv) ? 1u :
        tile->size == RDP_SIZE_4BPP ? 2u : 0u;
    const uint32_t line_words = tile->line >> 3u;
    const uint32_t base_words = tile->tmem >> 3u;
    const int32_t s_origin = (int32_t)cmd->decoded.load.sl << 3;
    const int32_t t_origin = (int32_t)cmd->decoded.load.tl << 3;
    bool upper_written = split_yuv || split_rgba32;
    if (!steps) return SR_OK;

    /*
     * The common load: every step fills one whole TMEM word, four halfwords
     * on from the last, and neither S nor T wraps its 16 bits anywhere in the
     * load. Then a step's word is the line's row plus the step, and T is the
     * line plus the whole lines DxT has accumulated - the general loop below
     * reduced to counters.
     */
    const bool whole_words = !split_yuv && !split_rgba32 &&
        (!tlut || (entry_steps && !(state->texture_image.address & 1u))) &&
        ((step_s >> coord_shift) >> halfword_shift) == 4 &&
        s_origin + (int32_t)(steps - 1u) * step_s <= 0x7fff &&
        t_origin + (int32_t)(last_line - first_line) * 32 +
            (((int32_t)(steps - 1u) * step_t) >> 8) <= 0x7fff;
    if (whole_words) {
        for (uint32_t line = first_line; line <= last_line; line++) {
            uint32_t source = state->texture_image.address +
                (((line * state->texture_image.width + first_texel) << size) >> 1);
            uint32_t accumulated = 0;
            for (uint32_t step = 0; step < steps; step++, source += step_bytes) {
                const uint32_t t = (line - first_line) + (accumulated >> 11);
                const uint32_t word =
                    (((line_words * t) & 0x1ffu) + base_words + step) & 0x1ffu;
                const uint64_t data = tlut
                    ? (((uint64_t)load_read_u8(memory, source) << 8) |
                       load_read_u8(memory, source + 1u)) * 0x0001000100010001ull
                    : nibble_source ? load_read_u64_wrapped(memory, source)
                    : load_read_u64(memory, source);
                tmem->qwords[word] = (t & 1u) ? data : (data << 32 | data >> 32);
                upper_written |= word >= 0x100u;
                accumulated += (uint32_t)step_t;
            }
        }
        *wrote_upper |= upper_written;
        return SR_OK;
    }

    for (uint32_t line = first_line; line <= last_line; line++) {
        const int32_t t_line = t_origin + (int32_t)(line - first_line) * 32;
        uint32_t source = state->texture_image.address +
            (((line * state->texture_image.width + first_texel) << size) >> 1);

        for (uint32_t step = 0; step < steps; step++, source += step_bytes) {
            const int32_t s = ((int32_t)(int16_t)(s_origin +
                (int32_t)step * step_s) - s_origin) >> coord_shift;
            const int32_t t = ((int32_t)(int16_t)(t_line +
                (((int32_t)step * step_t) >> 8)) - t_origin) >> coord_shift;
            const bool odd = (t & 1) != 0;
            const uint32_t row = ((line_words * (uint32_t)t) & 0x1ffu) + base_words;
            /* A YUV split lands two low-bank halfwords per 8-byte step, so its
             * position follows the source bytes: the image's texel size, not
             * the 16bpp rate. A 32bpp image (the illegal YUV size 3) moves S
             * by only two texels a step. */
            const uint32_t halfwords = split_yuv
                ? (uint32_t)((int32_t)((uint32_t)s << size) >> 3) & 0x7ffu
                : (uint32_t)(s >> halfword_shift) & 0x7ffu;
            const uint32_t first = ((row << 2) + halfwords) & 0x7fdu;

            uint64_t data;
            if (tlut && !(source & 1u)) {
                const uint64_t entry =
                    ((uint64_t)load_read_u8(memory, source) << 8) |
                    load_read_u8(memory, source + 1u);
                data = entry * 0x0001000100010001ull;
            } else if (tlut || nibble_source) {
                /* An odd TLUT source reads its four entries from the dword
                 * it starts in, wrapping past the dword's last byte. */
                data = load_read_u64_wrapped(memory, source);
            } else {
                data = load_read_u64(memory, source);
            }

            if (!(first & 1u) && !split_yuv && !split_rgba32) {
                /* A step that starts on a bank-0 halfword fills one whole TMEM
                 * word. Its halfwords sit in the word swapped pairwise, which
                 * on even lines also exchanges the 32-bit halves. */
                tmem->qwords[first >> 2] = odd ? data : (data << 32 | data >> 32);
                upper_written |= (first & 0x400u) != 0u;
                continue;
            }

            /* Bank b takes whichever of the four consecutive halfwords sits in
             * it; odd lines exchange the two 32-bit halves of every word. */
            uint32_t bank[4];
            for (uint32_t i = 0; i < 4u; i++) {
                const uint32_t address = ((first + i) & 0x7ffu) ^ (odd ? 2u : 0u);
                bank[address & 3u] = address & 0x3ffu;
            }

            if (split_yuv || split_rgba32) {
                /* The low TMEM half takes the first of each byte pair (YUV) or
                 * of each halfword pair (RGBA32), the high half the second. */
                uint32_t low, high;
                if (split_yuv) {
                    low = (uint32_t)(((data >> 56) & 0xffu) << 24 |
                                     ((data >> 40) & 0xffu) << 16 |
                                     ((data >> 24) & 0xffu) << 8 |
                                     ((data >> 8) & 0xffu));
                    high = (uint32_t)(((data >> 48) & 0xffu) << 24 |
                                      ((data >> 32) & 0xffu) << 16 |
                                      ((data >> 16) & 0xffu) << 8 |
                                      (data & 0xffu));
                } else {
                    low = (uint32_t)((data >> 48) << 16 | ((data >> 16) & 0xffffu));
                    high = (uint32_t)(((data >> 32) & 0xffffu) << 16 |
                                      (data & 0xffffu));
                }
                const uint32_t pair = (((halfwords & 2u) != 0u) != odd) ? 2u : 0u;
                tmem->words[bank[pair] ^ 1u] = (uint16_t)(low >> 16);
                tmem->words[bank[pair + 1u] ^ 1u] = (uint16_t)low;
                tmem->words[(bank[pair] | 0x400u) ^ 1u] = (uint16_t)(high >> 16);
                tmem->words[(bank[pair + 1u] | 0x400u) ^ 1u] = (uint16_t)high;
            } else {
                const uint32_t half = first & 0x400u;
                const uint32_t swap = odd ? 2u : 0u;
                for (uint32_t b = 0; b < 4u; b++)
                    tmem->words[(bank[b] | half) ^ 1u] =
                        (uint16_t)(data >> (48u - 16u * (b ^ swap)));
                upper_written |= half != 0u;
            }
        }
    }
    *wrote_upper |= upper_written;
    return SR_OK;
}

static sr_result tmem_load_tile_internal(tmem_state *tmem,
                                         sr_memory *memory,
                                         rdp_state *state,
                                         const rdp_command *cmd,
                                         bool *wrote_upper)
{
    if (!tmem || !memory || !state || !cmd) {
        return SR_ERROR_INVALID_ARGUMENT;
    }

    const rdp_tile *tile = &state->tiles[cmd->decoded.load.tile_index];

    /* The RDP's load unit accepts inverted bounds. They produce no valid
     * spans, so the command is an empty load rather than an execution error.
     * Games use this naturally after clipping; rejecting it can skip a later
     * SyncFull and leave the emulated CPU waiting forever. */
    if (cmd->id != RDP_CMD_LOAD_BLOCK &&
        (cmd->decoded.load.sh < cmd->decoded.load.sl ||
         (cmd->id == RDP_CMD_LOAD_TILE &&
          cmd->decoded.load.th < cmd->decoded.load.tl))) {
        return SR_OK;
    }
    /* Likewise an inverted or oversized LoadBlock loads nothing. */
    if (cmd->id == RDP_CMD_LOAD_BLOCK) {
        const int32_t texels = (int32_t)cmd->decoded.load.sh -
                               (int32_t)cmd->decoded.load.sl + 1;
        if (texels <= 0 || texels > 2048) return SR_OK;
    }

    return load_lines(tmem, memory, state, tile, cmd, wrote_upper);
}

sr_result tmem_load_tile(tmem_state *tmem,
                         sr_memory *memory,
                         rdp_state *state,
                         const rdp_command *cmd)
{
    bool upper = false;
    const sr_result result =
        tmem_load_tile_internal(tmem, memory, state, cmd, &upper);
    /* A load can fail after writing part of its range, so this runs either
     * way; it only decodes the entries that changed. */
    if (upper) tmem_palette_refresh(tmem);
    return result;
}
