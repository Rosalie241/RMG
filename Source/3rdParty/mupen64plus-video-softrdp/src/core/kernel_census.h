#ifndef KERNEL_CENSUS_H
#define KERNEL_CENSUS_H

/*
 * Development pixel census, for choosing fast paths from measured workloads
 * rather than by guess. Compiled only with SOFTRDP_CENSUS=1 and included only
 * by kernel.c. Every span adds its pixels to a table keyed by the draw-constant
 * state a fast path could depend on; the table is printed to stderr at exit as
 * projections onto combiner program, finish state, texture and the full key.
 *
 * The table is not synchronised: run single-threaded (one worker) and one
 * replay iteration. SOFTRDP_CENSUS_TOP sets how many rows each table shows.
 */

#include "primitive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* All bytes, so the key has no padding and compares with memcmp. */
typedef struct census_key {
    uint8_t kernel, two_cycle, texture, lod;
    uint8_t sampler_class, tex_format, tex_size, bilerp;
    uint8_t perspective, fb_size, depth, depth_late;
    uint8_t z_compare, z_update, z_source_primitive, blend_cycles;
    uint8_t z_mode;
    uint8_t force_blend, image_read, alpha_compare, antialias;
    uint8_t cvg_times_alpha, alpha_cvg_select, color_on_cvg, coverage_dest;
    uint8_t rgb_dither, alpha_dither, blend_op0, blend_op1;
    /* How the tile-1 texel is obtained; see census_texel1. */
    uint8_t texel1, tex1_format, tex1_size, tex1_class;
    rdp_blender_cycle blend[2];
    rdp_combiner_cycle comb[2];
} census_key;

typedef struct census_counts {
    uint64_t spans, covered, full, early_reject, alpha_reject, late_reject, written;
} census_counts;

typedef struct census_entry {
    census_key key;
    census_counts n;
} census_entry;

enum { CENSUS_CAP = 1u << 14, CENSUS_KERNEL_FILL = 5u };

static census_entry *census_table;
static census_counts census_total;

static void census_print(void);

static void census_counts_add(census_counts *to, const census_counts *from)
{
    to->spans += from->spans;
    to->covered += from->covered;
    to->full += from->full;
    to->early_reject += from->early_reject;
    to->alpha_reject += from->alpha_reject;
    to->late_reject += from->late_reject;
    to->written += from->written;
}

/*
 * 0: no separate tile-1 fetch. 1: a separate fetch whose sample state equals
 * tile 0's apart from the tile number, so it returns the tile-0 texel again.
 * 2: same TMEM address, format and size, but sampled differently. 3: other.
 */
static uint8_t census_texel1(const rdp_primitive_state *p)
{
    if (!stage_texel1_separate(p, (p->plan.stages & RDP_DRAW_LOD) != 0u)) return 0u;
    rdp_texture_sample_state a, b;
    memcpy(&a, &p->texture, sizeof(a));
    memcpy(&b, &p->texture_cycle1, sizeof(b));
    a.tile_index = b.tile_index = 0u;
    if (memcmp(&a, &b, sizeof(a)) == 0) return 1u;
    if (a.tile.tmem == b.tile.tmem && a.tile.format == b.tile.format &&
        a.tile.size == b.tile.size)
        return 2u;
    return 3u;
}

/* Fields a mode cannot observe are left zero so equivalent states merge. */
static census_key census_key_of(const rdp_primitive_state *p, uint8_t kernel)
{
    census_key k;
    memset(&k, 0, sizeof(k));
    const uint32_t stages = p->plan.stages;
    const bool bulk = kernel != RDP_KERNEL_TRIANGLE && kernel != RDP_KERNEL_TEXTURE_RECTANGLE;
    k.kernel = kernel;
    k.fb_size = (uint8_t)p->framebuffer.color_image.size;
    k.texture = (stages & RDP_DRAW_TEXTURE) != 0u ||
                kernel == RDP_KERNEL_TEXTURE_TRIANGLE_COPY ||
                kernel == RDP_KERNEL_TEXTURE_RECTANGLE_COPY;
    if (k.texture) {
        k.lod = (stages & RDP_DRAW_LOD) != 0u;
        k.sampler_class = (uint8_t)p->texture.sampler_class;
        k.tex_format = (uint8_t)p->texture.tile.format;
        k.tex_size = (uint8_t)p->texture.tile.size;
        k.bilerp = p->texture.bilerp;
        k.perspective = p->texture.perspective;
    }
    if (bulk) return k;
    if (k.texture) k.texel1 = census_texel1(p);
    if (k.texel1) {
        k.tex1_format = (uint8_t)p->texture_cycle1.tile.format;
        k.tex1_size = (uint8_t)p->texture_cycle1.tile.size;
        k.tex1_class = (uint8_t)p->texture_cycle1.sampler_class;
    }

    const rdp_fragment_state *f = &p->fragment;
    k.two_cycle = p->color.two_cycle;
    k.comb[1] = p->color.program.cycle[1];
    if (k.two_cycle) k.comb[0] = p->color.program.cycle[0];
    k.depth = (stages & RDP_DRAW_DEPTH) != 0u;
    k.depth_late = (stages & RDP_DRAW_DEPTH_LATE) != 0u;
    k.z_compare = f->depth.compare;
    k.z_update = f->depth.update;
    k.z_source_primitive = f->depth.source_primitive;
    k.z_mode = f->depth.mode & 3u;
    k.blend_cycles = f->blend.cycle_count;
    k.blend[0] = f->blend.program.cycle[0];
    k.blend_op0 = f->blend.program.operation[0];
    if (k.blend_cycles == 2u) {
        k.blend[1] = f->blend.program.cycle[1];
        k.blend_op1 = f->blend.program.operation[1];
    }
    k.force_blend = f->blend.force_blend;
    k.image_read = f->blend.image_read;
    k.alpha_compare = f->blend.alpha_compare;
    k.antialias = f->antialias;
    k.cvg_times_alpha = f->cvg_times_alpha;
    k.alpha_cvg_select = f->alpha_cvg_select;
    k.color_on_cvg = f->color_on_cvg;
    k.coverage_dest = f->coverage_dest & 3u;
    k.rgb_dither = f->rgb_dither;
    k.alpha_dither = f->alpha_dither;
    return k;
}

static uint32_t census_hash(const census_key *key)
{
    const uint8_t *bytes = (const uint8_t *)key;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < sizeof(*key); i++) h = (h ^ bytes[i]) * 16777619u;
    return h;
}

static void census_add(const census_key *key, const census_counts *counts)
{
    if (!census_table) {
        census_table = calloc(CENSUS_CAP, sizeof(*census_table));
        if (!census_table) return;
        atexit(census_print);
    }
    uint32_t i = census_hash(key) & (CENSUS_CAP - 1u);
    for (uint32_t probe = 0; probe < CENSUS_CAP; probe++, i = (i + 1u) & (CENSUS_CAP - 1u)) {
        census_entry *entry = &census_table[i];
        if (entry->n.spans && memcmp(&entry->key, key, sizeof(*key)) != 0) continue;
        entry->key = *key;
        census_counts_add(&entry->n, counts);
        census_counts_add(&census_total, counts);
        return;
    }
}

static void census_record(const rdp_primitive_state *primitive, const census_counts *counts)
{
    const census_key key = census_key_of(primitive, (uint8_t)primitive->kernel);
    census_add(&key, counts);
}

/* Copy and fill spans write every pixel of the span without the pipeline. */
static void census_record_bulk(const rdp_primitive_state *primitive, const rdp_span *work)
{
    if (work->x_end < work->x_begin) return;
    const uint64_t pixels = (uint64_t)(work->x_end - work->x_begin + 1);
    const census_counts counts = { 1u, pixels, pixels, 0u, 0u, 0u, pixels };
    const uint8_t kernel = (primitive->plan.stages & RDP_DRAW_FILL)
        ? (uint8_t)CENSUS_KERNEL_FILL : (uint8_t)primitive->kernel;
    const census_key key = census_key_of(primitive, kernel);
    census_add(&key, &counts);
}

/* ---- report ---- */

static const char *census_kernel_name(uint8_t kernel)
{
    static const char *const names[] = { "invalid", "tri", "tri-copy", "rect", "rect-copy", "tri-fill" };
    return kernel < 6u ? names[kernel] : "?";
}

static const char *census_source_name(uint8_t source)
{
    static const char *const names[] = {
        "0", "1", "COMB", "COMB_A", "T0", "T0_A", "T1", "T1_A", "PRIM", "PRIM_A",
        "SHADE", "SHADE_A", "ENV", "ENV_A", "LOD", "PLOD", "NOISE", "KCEN", "KSCL",
        "K4", "K5"
    };
    return source < 21u ? names[source] : "?";
}

static bool census_is_pipeline(uint8_t kernel)
{
    return kernel == RDP_KERNEL_TRIANGLE || kernel == RDP_KERNEL_TEXTURE_RECTANGLE;
}

static void census_project_combiner(census_key *k)
{
    census_key p;
    memset(&p, 0, sizeof(p));
    p.kernel = census_is_pipeline(k->kernel) ? 0u : k->kernel;
    p.two_cycle = k->two_cycle;
    memcpy(p.comb, k->comb, sizeof(p.comb));
    *k = p;
}

static void census_project_finish(census_key *k)
{
    census_key p = *k;
    p.kernel = census_is_pipeline(k->kernel) ? 0u : k->kernel;
    p.two_cycle = 0u;
    p.texture = p.lod = p.sampler_class = p.tex_format = p.tex_size = 0u;
    p.bilerp = p.perspective = p.texel1 = p.tex1_format = p.tex1_size = p.tex1_class = 0u;
    memset(p.comb, 0, sizeof(p.comb));
    *k = p;
}

static void census_project_texture(census_key *k)
{
    census_key p;
    memset(&p, 0, sizeof(p));
    p.kernel = k->kernel;
    p.texture = k->texture;
    p.lod = k->lod;
    p.sampler_class = k->sampler_class;
    p.tex_format = k->tex_format;
    p.tex_size = k->tex_size;
    p.bilerp = k->bilerp;
    p.perspective = k->perspective;
    p.texel1 = k->texel1;
    p.tex1_format = k->tex1_format;
    p.tex1_size = k->tex1_size;
    p.tex1_class = k->tex1_class;
    *k = p;
}

static void census_describe_combiner(FILE *f, const census_key *k)
{
    if (!census_is_pipeline(k->kernel) && k->kernel) {
        fprintf(f, "%s", census_kernel_name(k->kernel));
        return;
    }
    fprintf(f, "%s", k->two_cycle ? "2cyc" : "1cyc");
    for (uint32_t c = k->two_cycle ? 0u : 1u; c < 2u; c++) {
        const rdp_combiner_cycle *y = &k->comb[c];
        fprintf(f, "  c%u (%s-%s)*%s+%s | (%s-%s)*%s+%s", c,
                census_source_name(y->rgb_a), census_source_name(y->rgb_b),
                census_source_name(y->rgb_c), census_source_name(y->rgb_d),
                census_source_name(y->alpha_a), census_source_name(y->alpha_b),
                census_source_name(y->alpha_c), census_source_name(y->alpha_d));
    }
}

static void census_describe_finish(FILE *f, const census_key *k)
{
    if (!census_is_pipeline(k->kernel) && k->kernel) {
        fprintf(f, "%s fb%u", census_kernel_name(k->kernel), k->fb_size);
        return;
    }
    fprintf(f, "fb%u blend%u op%u/%u [%u%u%u%u|%u%u%u%u] force%u read%u ac%u aa%u "
               "cxa%u acs%u coc%u cd%u dith%u/%u z%s%s%s%s zm%u",
            k->fb_size, k->blend_cycles, k->blend_op0, k->blend_op1,
            k->blend[0].color_a, k->blend[0].factor_a, k->blend[0].color_b, k->blend[0].factor_b,
            k->blend[1].color_a, k->blend[1].factor_a, k->blend[1].color_b, k->blend[1].factor_b,
            k->force_blend, k->image_read, k->alpha_compare, k->antialias,
            k->cvg_times_alpha, k->alpha_cvg_select, k->color_on_cvg, k->coverage_dest,
            k->rgb_dither, k->alpha_dither,
            k->depth ? (k->depth_late ? "late" : "early") : "off",
            k->z_compare ? "+cmp" : "", k->z_update ? "+upd" : "",
            k->z_source_primitive ? "+prim" : "", k->z_mode);
}

static void census_describe_texture(FILE *f, const census_key *k)
{
    if (!k->texture) {
        fprintf(f, "%s untextured", census_kernel_name(k->kernel));
        return;
    }
    static const char *const texel1[] = { "none", "same-state", "same-tmem", "other" };
    fprintf(f, "%s class%u fmt%u size%u bilerp%u lod%u persp%u t1:%s",
            census_kernel_name(k->kernel), k->sampler_class, k->tex_format,
            k->tex_size, k->bilerp, k->lod, k->perspective,
            texel1[k->texel1 & 3u]);
    if (k->texel1)
        fprintf(f, " fmt%u size%u class%u", k->tex1_format, k->tex1_size, k->tex1_class);
}

static uint64_t census_shaded(const census_counts *n)
{
    return n->covered - n->early_reject;
}

static int census_by_shaded(const void *a, const void *b)
{
    const uint64_t x = census_shaded(&((const census_entry *)a)->n);
    const uint64_t y = census_shaded(&((const census_entry *)b)->n);
    return (x < y) - (x > y);
}

typedef void (*census_projection)(census_key *);
typedef void (*census_describer)(FILE *, const census_key *);

static void census_section(FILE *f, const char *title, census_projection project,
                           const census_describer *describe, uint32_t describers,
                           census_entry *scratch, uint32_t top)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < CENSUS_CAP; i++) {
        const census_entry *entry = &census_table[i];
        if (!entry->n.spans) continue;
        census_key key = entry->key;
        if (project) project(&key);
        uint32_t j = 0;
        while (j < count && memcmp(&scratch[j].key, &key, sizeof(key)) != 0) j++;
        if (j == count) {
            scratch[count].key = key;
            memset(&scratch[count].n, 0, sizeof(scratch[count].n));
            count++;
        }
        census_counts_add(&scratch[j].n, &entry->n);
    }
    qsort(scratch, count, sizeof(*scratch), census_by_shaded);
    const double shaded_total = (double)census_shaded(&census_total);
    const double written_total = (double)census_total.written;
    double cumulative = 0.0;
    fprintf(f, "\n== %s (%u distinct) ==\n", title, count);
    fprintf(f, " shaded  cumul  written  full  zrej  arej\n");
    for (uint32_t i = 0; i < count && i < top; i++) {
        const census_counts *n = &scratch[i].n;
        const double shaded = (double)census_shaded(n);
        cumulative += shaded;
        fprintf(f, "%6.2f%% %5.1f%% %6.2f%% %4.0f%% %4.0f%% %4.0f%%  ",
                shaded_total ? 100.0 * shaded / shaded_total : 0.0,
                shaded_total ? 100.0 * cumulative / shaded_total : 0.0,
                written_total ? 100.0 * (double)n->written / written_total : 0.0,
                n->covered ? 100.0 * (double)n->full / (double)n->covered : 0.0,
                n->covered ? 100.0 * (double)n->early_reject / (double)n->covered : 0.0,
                shaded ? 100.0 * (double)n->alpha_reject / shaded : 0.0);
        for (uint32_t d = 0; d < describers; d++) {
            if (d) fprintf(f, "  ||  ");
            describe[d](f, &scratch[i].key);
        }
        fprintf(f, "\n");
    }
}

static void census_print(void)
{
    if (!census_table || !census_total.spans) return;
    FILE *f = stderr;
    const char *top_env = getenv("SOFTRDP_CENSUS_TOP");
    const uint32_t top = top_env ? (uint32_t)strtoul(top_env, NULL, 10) : 15u;
    census_entry *scratch = calloc(CENSUS_CAP, sizeof(*scratch));
    if (!scratch) return;
    const census_counts *t = &census_total;
    fprintf(f, "\ncensus: spans %llu covered %llu full %.1f%% early-z reject %.1f%% "
               "shaded %llu alpha reject %.1f%% late-z reject %.1f%% written %llu\n",
            (unsigned long long)t->spans, (unsigned long long)t->covered,
            t->covered ? 100.0 * (double)t->full / (double)t->covered : 0.0,
            t->covered ? 100.0 * (double)t->early_reject / (double)t->covered : 0.0,
            (unsigned long long)census_shaded(t),
            census_shaded(t) ? 100.0 * (double)t->alpha_reject / (double)census_shaded(t) : 0.0,
            census_shaded(t) ? 100.0 * (double)t->late_reject / (double)census_shaded(t) : 0.0,
            (unsigned long long)t->written);
    fprintf(f, "columns: share of shaded pixels (reached the combiner), cumulative, share of "
               "written, full-coverage share, early-z reject share, alpha reject share\n");
    fprintf(f, "fb/tex size: 0=4 1=8 2=16 3=32 bpp; blend [color_a factor_a color_b factor_b]\n");
    const census_describer combiner[] = { census_describe_combiner };
    const census_describer finish[] = { census_describe_finish };
    const census_describer texture[] = { census_describe_texture };
    const census_describer joint[] = { census_describe_combiner, census_describe_finish,
                                       census_describe_texture };
    census_section(f, "combiner program", census_project_combiner, combiner, 1u, scratch, top);
    census_section(f, "finish state", census_project_finish, finish, 1u, scratch, top);
    census_section(f, "texture", census_project_texture, texture, 1u, scratch, top);
    census_section(f, "joint", NULL, joint, 3u, scratch, top);
    free(scratch);
}

#endif
