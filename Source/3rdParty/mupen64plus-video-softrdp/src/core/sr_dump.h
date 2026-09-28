#ifndef SR_DUMP_H
#define SR_DUMP_H

/*
 * Frame dump recorder: the file format writer, with no host in it.
 *
 * A dump is a state snapshot plus the initial RDRAM, then one record per RDP
 * list holding the RDRAM pages that changed since the previous record, the DP
 * and VI registers, and the command words themselves; a final copy of RDRAM
 * closes it out as the reference result.
 *
 * Nothing here knows what an emulator is. The caller owns the registers and the
 * command bytes and hands over values, so a host with no GFX_INFO - or no
 * Windows - drives this the same way the plugins do. The rules it cannot
 * enforce itself:
 *
 *   - Record a list BEFORE executing it. Afterwards DP_CURRENT == DP_END and
 *     every command size is zero, which produces a dump that replays as
 *     nothing at all.
 *   - Call sr_dump_sync_shadow() after executing it, so the renderer's own
 *     writes are not handed to the next record as if the CPU had made them.
 *     Each replay renderer has to reproduce them independently.
 */

#include <stdbool.h>
#include <stdint.h>

typedef struct sr_context sr_context;
typedef struct sr_dump sr_dump;

enum { SR_DUMP_DP_REGS = 8u };  /* START END CURRENT STATUS CLOCK BUFBUSY PIPEBUSY TMEM */
enum { SR_DUMP_VI_REGS = 14u }; /* STATUS ORIGIN WIDTH INTR V_CURRENT TIMING V_SYNC
                                 * H_SYNC LEAP H_START V_START V_BURST X_SCALE Y_SCALE */

/*
 * Writes the header and the initial RDRAM. Returns NULL if the file cannot be
 * opened or the state snapshot fails; nothing is left behind on failure.
 *
 * rdram must stay valid and stay put until the dump is closed - it is read on
 * every record - and rdram_size must be a multiple of 4096. rdram_bswapped
 * records whether the host keeps RDRAM byte-swapped rather than in host word
 * order; a replayer that guesses wrong renders scrambled colours.
 */
sr_dump *sr_dump_open(const char *path, const sr_context *ctx,
                      const uint8_t *rdram, uint32_t rdram_size,
                      bool rdram_bswapped);

/*
 * One record, written whole or not at all. command points at command_size bytes
 * of RDP command words; where they came from - RDRAM, or DMEM unwrapped by an
 * XBUS caller - is the caller's business. command may be NULL when the size is
 * zero. False means the dump is unusable: abort it.
 */
bool sr_dump_record_list(sr_dump *dump,
                         const uint32_t dp[SR_DUMP_DP_REGS],
                         const uint32_t vi[SR_DUMP_VI_REGS],
                         const uint8_t *command, uint32_t command_size);

/* Re-baselines the page shadow against RDRAM. Call after executing a list. */
void sr_dump_sync_shadow(sr_dump *dump);

/* Number of records written so far, for the caller's log line. */
uint32_t sr_dump_list_count(const sr_dump *dump);

/*
 * Writes the reference RDRAM, backpatches the list count and closes. Frees the
 * dump either way; false means the file on disk is incomplete.
 */
bool sr_dump_close(sr_dump *dump);

/* Frees the dump and leaves the partial file behind. NULL is fine. */
void sr_dump_abort(sr_dump *dump);

#endif
