#include "sr_dump.h"

#include "sr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SR_DUMP_PAGE_SIZE = 4096u };

struct sr_dump {
    FILE *file;
    const uint8_t *rdram;
    uint32_t rdram_size;
    uint32_t page_count;
    uint8_t *shadow;      /* RDRAM as of the last record, for the page diff */
    uint32_t *changed;    /* page_count entries, reused by every record */
    uint8_t *record;      /* reused by every record, grown when one needs more */
    size_t record_capacity;
    uint32_t list_count;
    long list_count_offset;
};

/*
 * The record buffer is sized for every page being dirty plus the largest
 * command seen so far. A non-XBUS list is bounded only by RDRAM, so there is no
 * constant to size it against up front; in practice it is allocated once on the
 * first record and never grows again.
 */
static bool ensure_record_capacity(sr_dump *dump, size_t needed)
{
    if (needed <= dump->record_capacity) return true;
    uint8_t *grown = realloc(dump->record, needed);
    if (!grown) return false;
    dump->record = grown;
    dump->record_capacity = needed;
    return true;
}

static bool write_all(FILE *file, const void *data, size_t size)
{
    const uint8_t *cursor = (const uint8_t *)data;
    while (size) {
        const size_t chunk = size > 64u * 1024u ? 64u * 1024u : size;
        const size_t written = fwrite(cursor, 1, chunk, file);
        if (written != chunk) return false;
        cursor += written;
        size -= written;
    }
    return true;
}

void sr_dump_abort(sr_dump *dump)
{
    if (!dump) return;
    if (dump->file) fclose(dump->file);
    free(dump->shadow);
    free(dump->changed);
    free(dump->record);
    free(dump);
}

sr_dump *sr_dump_open(const char *path, const sr_context *ctx,
                      const uint8_t *rdram, uint32_t rdram_size,
                      bool rdram_bswapped)
{
    if (!path || !rdram || !rdram_size || rdram_size % SR_DUMP_PAGE_SIZE) return NULL;

    sr_dump *dump = calloc(1, sizeof(*dump));
    if (!dump) return NULL;
    dump->rdram = rdram;
    dump->rdram_size = rdram_size;
    dump->page_count = rdram_size / SR_DUMP_PAGE_SIZE;

    /*
     * One allocation each, held for the life of the dump: a record is written
     * with a single fwrite so a short write cannot leave a half-record behind,
     * and the worst case - every page dirty - is what that buffer has to hold.
     */
    dump->shadow = malloc(rdram_size);
    dump->changed = malloc((size_t)dump->page_count * sizeof(*dump->changed));
    if (!dump->shadow || !dump->changed) {
        sr_dump_abort(dump);
        return NULL;
    }

    const uint32_t state_size = (uint32_t)sr_state_snapshot_size();
    void *state = malloc(state_size);
    if (!state) {
        sr_dump_abort(dump);
        return NULL;
    }
    if (sr_save_state(ctx, state, state_size) != SR_OK) {
        free(state);
        sr_dump_abort(dump);
        return NULL;
    }

    dump->file = fopen(path, "wb");
    if (!dump->file) {
        free(state);
        sr_dump_abort(dump);
        return NULL;
    }

    const uint32_t magic = 0x34444653; /* "SFD4" */
    const uint32_t bswapped = rdram_bswapped ? 1u : 0u;
    const uint32_t zero = 0;
    bool ok = fwrite(&magic, 4, 1, dump->file) == 1 &&
              fwrite(&rdram_size, 4, 1, dump->file) == 1 &&
              fwrite(&bswapped, 4, 1, dump->file) == 1;
    dump->list_count_offset = ok ? ftell(dump->file) : -1;
    ok = ok && dump->list_count_offset >= 0 &&
         fwrite(&zero, 4, 1, dump->file) == 1 && /* list_count, backpatched at close */
         fwrite(&state_size, 4, 1, dump->file) == 1 &&
         write_all(dump->file, state, state_size) &&
         write_all(dump->file, rdram, rdram_size);
    free(state);
    if (!ok) {
        sr_dump_abort(dump);
        return NULL;
    }

    memcpy(dump->shadow, rdram, rdram_size);
    return dump;
}

bool sr_dump_record_list(sr_dump *dump,
                         const uint32_t dp[SR_DUMP_DP_REGS],
                         const uint32_t vi[SR_DUMP_VI_REGS],
                         const uint8_t *command, uint32_t command_size)
{
    if (!dump || !dump->file || !dp || !vi) return false;
    if (command_size && !command) return false;

    uint32_t changed_count = 0;
    for (uint32_t page = 0; page < dump->page_count; page++) {
        const uint32_t offset = page * SR_DUMP_PAGE_SIZE;
        if (memcmp(dump->shadow + offset, dump->rdram + offset, SR_DUMP_PAGE_SIZE) != 0)
            dump->changed[changed_count++] = page;
    }

    const size_t record_size = sizeof(uint32_t) +
        (size_t)changed_count * (sizeof(uint32_t) + SR_DUMP_PAGE_SIZE) +
        sizeof(uint32_t) +
        (SR_DUMP_DP_REGS + SR_DUMP_VI_REGS) * sizeof(uint32_t) +
        command_size;
    if (!ensure_record_capacity(dump, record_size)) return false;

    uint8_t *cursor = dump->record;
    memcpy(cursor, &changed_count, sizeof(changed_count));
    cursor += sizeof(changed_count);
    for (uint32_t i = 0; i < changed_count; i++) {
        const uint32_t page = dump->changed[i];
        const uint32_t offset = page * SR_DUMP_PAGE_SIZE;
        memcpy(cursor, &page, sizeof(page));
        cursor += sizeof(page);
        memcpy(cursor, dump->rdram + offset, SR_DUMP_PAGE_SIZE);
        memcpy(dump->shadow + offset, cursor, SR_DUMP_PAGE_SIZE);
        cursor += SR_DUMP_PAGE_SIZE;
    }
    memcpy(cursor, &command_size, sizeof(command_size));
    cursor += sizeof(command_size);
    memcpy(cursor, dp, SR_DUMP_DP_REGS * sizeof(uint32_t));
    cursor += SR_DUMP_DP_REGS * sizeof(uint32_t);
    memcpy(cursor, vi, SR_DUMP_VI_REGS * sizeof(uint32_t));
    cursor += SR_DUMP_VI_REGS * sizeof(uint32_t);
    if (command_size) {
        memcpy(cursor, command, command_size);
        cursor += command_size;
    }

    if ((size_t)(cursor - dump->record) != record_size) return false;
    if (fwrite(dump->record, 1, record_size, dump->file) != record_size) return false;
    dump->list_count++;
    return true;
}

void sr_dump_sync_shadow(sr_dump *dump)
{
    if (dump && dump->shadow && dump->rdram)
        memcpy(dump->shadow, dump->rdram, dump->rdram_size);
}

uint32_t sr_dump_list_count(const sr_dump *dump)
{
    return dump ? dump->list_count : 0u;
}

bool sr_dump_close(sr_dump *dump)
{
    if (!dump) return false;
    FILE *file = dump->file;
    bool ok = false;
    if (file) {
        ok = write_all(file, dump->rdram, dump->rdram_size) &&
             fseek(file, dump->list_count_offset, SEEK_SET) == 0 &&
             fwrite(&dump->list_count, 4, 1, file) == 1;
        ok = (fclose(file) == 0) && ok;
        dump->file = NULL;
    }
    sr_dump_abort(dump);
    return ok;
}
