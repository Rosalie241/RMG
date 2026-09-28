#include "pj64_gfx.h"
#include "pj64_log.h"
#include "../plugin_common.h"

#if SOFTRDP_ENABLE_LOG
/* For the command ids only; the core's functions are namespaced per scale
 * and not callable from here. */
#include "../../core/rdp_commands.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GFX_INFO g_gfx;
/*
 * Renderer options, read once per ROM from softrdp.ini beside this DLL. The
 * defaults stand in until then, so a missing or unreadable file still runs.
 */
static sr_plugin_config g_config;
/* Outlives g_context: the renderer's interrupt callback reads through it. */
static sr_plugin_machine g_machine;
static sr_context *g_context;
static sr_present g_present;
static sr_plugin_screen g_screen;
static bool g_runtime_started;
static uint32_t g_process_dlist_calls;
static uint32_t g_process_rdp_calls;
static uint32_t g_update_screen_calls;
static uint32_t g_uploaded_frames;

static bool g_fullscreen = false;
#ifdef _WIN32
static HMENU g_old_menu = NULL;
static LONG g_old_style = 0;
static WINDOWPLACEMENT g_old_pos;
#endif

static void acknowledge_dp_list(void)
{
    if (g_gfx.DPC_END_REG) {
        if (g_gfx.DPC_START_REG) *g_gfx.DPC_START_REG = *g_gfx.DPC_END_REG;
        if (g_gfx.DPC_CURRENT_REG) *g_gfx.DPC_CURRENT_REG = *g_gfx.DPC_END_REG;
    }
    if (g_gfx.MI_INTR_REG) *g_gfx.MI_INTR_REG |= 0x20u;
    if (g_gfx.CheckInterrupts) g_gfx.CheckInterrupts();
}

#if SOFTRDP_ENABLE_LOG
static const char *result_name(sr_result result)
{
    switch (result) {
    case SR_OK: return "OK";
    case SR_ERROR_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case SR_ERROR_BAD_COMMAND: return "BAD_COMMAND";
    case SR_ERROR_UNSUPPORTED: return "UNSUPPORTED";
    default: return "UNKNOWN";
    }
}

static bool command_has_texture_debug(uint32_t command_id)
{
    switch ((rdp_command_id)command_id) {
    case RDP_CMD_LOAD_TLUT:
    case RDP_CMD_LOAD_BLOCK:
    case RDP_CMD_LOAD_TILE:
    case RDP_CMD_TEXTURE_RECTANGLE:
    case RDP_CMD_TEXTURE_RECTANGLE_FLIP:
    case RDP_CMD_TEXTURE_TRIANGLE:
    case RDP_CMD_TEXTURE_ZBUFFER_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_TRIANGLE:
    case RDP_CMD_SHADE_TEXTURE_ZBUFFER_TRIANGLE:
        return true;
    default:
        return false;
    }
}

static void log_texture_debug(const sr_debug_stats *stats)
{
    if (!stats || !command_has_texture_debug(stats->last_command_id)) return;

    pj64_log_printf("  tex-debug ti fmt=%u size=%u width=%u addr=%08x tile=%u fmt=%u size=%u tmem=%u line=%u tile_st=%u,%u-%u,%u",
                    stats->last_texture_image_format,
                    stats->last_texture_image_size,
                    stats->last_texture_image_width,
                    stats->last_texture_image_address,
                    stats->last_tile_index,
                    stats->last_tile_format,
                    stats->last_tile_size,
                    stats->last_tile_tmem,
                    stats->last_tile_line,
                    stats->last_tile_sl,
                    stats->last_tile_tl,
                    stats->last_tile_sh,
                    stats->last_tile_th);

    if (stats->last_command_id == RDP_CMD_LOAD_TLUT ||
        stats->last_command_id == RDP_CMD_LOAD_BLOCK ||
        stats->last_command_id == RDP_CMD_LOAD_TILE) {
        pj64_log_printf("  load-debug sl=%u tl=%u sh=%u th/dxt=%u",
                        stats->last_load_sl, stats->last_load_tl,
                        stats->last_load_sh, stats->last_load_th);
    } else if (stats->last_command_id == RDP_CMD_TEXTURE_RECTANGLE ||
               stats->last_command_id == RDP_CMD_TEXTURE_RECTANGLE_FLIP) {
        pj64_log_printf("  rect-debug s0=%d t0=%d dsdx=%d dtdy=%d",
                        stats->last_rect_s0, stats->last_rect_t0,
                        stats->last_rect_dsdx, stats->last_rect_dtdy);
    }
}

static bool log_sample_u32(uint32_t count, uint32_t early_count, uint32_t interval)
{
    return count <= early_count || (interval != 0u && (count % interval) == 0u);
}

static void dump_log_sink(const char *message)
{
    pj64_log_printf("%s", message);
}
#else
#define dump_log_sink NULL
#endif

/*
 * Project64 does not say how much RDRAM it allocated; the mapping the pointer
 * lives in does. 4 MiB unless the region is large enough for the expansion
 * pak.
 */
static uint32_t detect_rdram_size(const uint8_t *rdram)
{
    uint32_t size = 0x400000u;
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION mbi;
    if (rdram && VirtualQuery(rdram, &mbi, sizeof(mbi)) != 0) {
        const size_t offset = (size_t)(rdram - (const uint8_t *)mbi.BaseAddress);
        if (mbi.RegionSize > offset && mbi.RegionSize - offset >= 0x800000u)
            size = 0x800000u;
    }
#endif
    return size;
}

static void ensure_window_size(HWND hwnd)
{
#ifdef _WIN32
    if (!hwnd || g_fullscreen) return;
    RECT rect;
    if (GetClientRect(hwnd, &rect)) {
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        if (width != 800 || height != 600) {
            RECT wr = {0, 0, 800, 600};
            const DWORD style = (DWORD)GetWindowLongA(hwnd, GWL_STYLE);
            const DWORD ex_style = (DWORD)GetWindowLongA(hwnd, GWL_EXSTYLE);
            const BOOL menu = (GetMenu(hwnd) != NULL);
            AdjustWindowRectEx(&wr, style, menu, ex_style);
            SetWindowPos(hwnd, NULL, 0, 0, wr.right - wr.left, wr.bottom - wr.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
#else
    (void)hwnd;
#endif
}

/*
 * Configuration: a plain ini beside the DLL, no dialog.
 *
 * GetPrivateProfile* is the API Project64 plugins conventionally use and needs
 * no parser of our own. The path is resolved from THIS module rather than the
 * working directory, which the emulator is free to change; passing a bare file
 * name would read whatever happened to sit next to the exe.
 */
static void config_path(char *out, size_t size)
{
    HMODULE module = NULL;
    out[0] = '\0';
    /* Address inside this DLL -> this DLL's handle, without needing DllMain.
     * UNCHANGED_REFCOUNT because we are not keeping the handle. */
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)(const void *)&config_path, &module))
        return;
    const DWORD length = GetModuleFileNameA(module, out, (DWORD)size);
    if (length == 0u || length >= size) {
        out[0] = '\0';
        return;
    }
    char *const slash = strrchr(out, '\\');
    if (!slash) {
        out[0] = '\0';
        return;
    }
    slash[1] = '\0';
    if (strlen(out) + sizeof("softrdp.ini") > size) {
        out[0] = '\0';
        return;
    }
    strcat(out, "softrdp.ini");
}

/*
 * One key: read it, or write its default into the file when it is absent so
 * the file documents itself rather than requiring the keys to be known in
 * advance.
 */
static long config_read(const char *path, const char *key, long fallback)
{
    const int value = GetPrivateProfileIntA(SR_CONFIG_SECTION, key, -1, path);
    if (value >= 0) return value;
    char text[16];
    snprintf(text, sizeof(text), "%ld", fallback);
    WritePrivateProfileStringA(SR_CONFIG_SECTION, key, text, path);
    return fallback;
}

static void load_config(void)
{
    const sr_plugin_config defaults = sr_config_defaults();
    g_config = defaults;

    char path[MAX_PATH];
    config_path(path, sizeof(path));
    if (path[0] == '\0') return;

    g_config.workers = sr_config_clamp_workers(
        config_read(path, SR_CONFIG_KEY_WORKERS, (long)defaults.workers));
    g_config.scale = sr_config_clamp_scale(
        config_read(path, SR_CONFIG_KEY_SCALE, (long)defaults.scale));
    g_config.disable_vi_dither_filter = config_read(path,
        SR_CONFIG_KEY_DISABLE_VI_DITHER_FILTER, defaults.disable_vi_dither_filter) != 0;
    g_config.disable_vi_divot_filter = config_read(path,
        SR_CONFIG_KEY_DISABLE_VI_DIVOT_FILTER, defaults.disable_vi_divot_filter) != 0;
    g_config.disable_vi_gamma_dither = config_read(path,
        SR_CONFIG_KEY_DISABLE_VI_GAMMA_DITHER, defaults.disable_vi_gamma_dither) != 0;
    g_config.disable_vi_aa = config_read(path,
        SR_CONFIG_KEY_DISABLE_VI_AA, defaults.disable_vi_aa) != 0;
}

static bool start_runtime(void)
{
    if (g_runtime_started) return true;

    /*
     * Per ROM, not per DLL load: Project64 keeps the plugin loaded between ROM
     * loads, so reading here means editing the ini and reloading the ROM is
     * enough - no emulator restart.
     */
    load_config();
    ensure_window_size(g_gfx.hWnd);

    pj64_log_open();
    pj64_log_printf("config scale=%ux workers=%u", g_config.scale, g_config.workers);
    pj64_log_printf("RomOpen/start_runtime hwnd=%p swapped=%d rdram=%p dmem=%p",
                    (void *)g_gfx.hWnd, g_gfx.MemoryBswaped ? 1 : 0,
                    (void *)g_gfx.RDRAM, (void *)g_gfx.DMEM);

    if (!g_gfx.MemoryBswaped) {
        pj64_log_printf("start_runtime: host did not provide word-swapped RDRAM");
        MessageBoxA(g_gfx.hWnd,
                    "SoftRDP requires word-swapped RDRAM from the emulator.",
                    "SoftRDP", MB_ICONERROR | MB_OK);
        return false;
    }

    SR_PLUGIN_MACHINE_FROM_GFX(&g_machine, &g_gfx);
    g_machine.rdram_size = detect_rdram_size(g_gfx.RDRAM);
    g_machine.rdram_bswapped = true;

    sr_plugin_context_destroy(g_context);
    g_context = sr_plugin_context_create(&g_machine, &g_config, dump_log_sink);
    if (!g_context) {
        pj64_log_printf("start_runtime: sr_create failed");
        return false;
    }

    if (!sr_present_init(&g_present, g_gfx.hWnd)) {
        pj64_log_printf("start_runtime: OpenGL presentation init failed");
        MessageBoxA(g_gfx.hWnd,
                    "SoftRDP could not initialize the OpenGL 3.3 presentation backend.",
                    "SoftRDP", MB_ICONERROR | MB_OK);
        return false;
    }

    g_runtime_started = true;
    pj64_log_printf("start_runtime: ready");
    return true;
}

static void stop_runtime(void)
{
    if (g_runtime_started || pj64_log_is_open()) {
        pj64_log_printf("stop_runtime rdp_calls=%u update_calls=%u uploaded_frames=%u",
                        g_process_rdp_calls, g_update_screen_calls, g_uploaded_frames);
    }
    sr_present_shutdown(&g_present);
    sr_plugin_screen_reset(&g_screen);
    sr_plugin_context_destroy(g_context);
    g_context = NULL;
    g_runtime_started = false;
    g_process_rdp_calls = 0;
    g_process_dlist_calls = 0;
    g_update_screen_calls = 0;
    g_uploaded_frames = 0;
    pj64_log_close();
}

void PJ64_CALL GetDllInfo(PLUGIN_INFO *plugin_info)
{
    if (!plugin_info) return;
    memset(plugin_info, 0, sizeof(*plugin_info));
    plugin_info->Version = PLUGIN_VERSION;
    plugin_info->Type = PLUGIN_TYPE_GFX;
    snprintf(plugin_info->Name, sizeof(plugin_info->Name), "SoftRDP");
    plugin_info->NormalMemory = 1;
    plugin_info->MemoryBswaped = 1;
}

void PJ64_CALL CaptureScreen(char *directory)
{
    (void)directory;
}

void PJ64_CALL ChangeWindow(void)
{
#ifdef _WIN32
    HWND hwnd = g_gfx.hWnd;
    if (!hwnd || !IsWindow(hwnd)) return;

    g_fullscreen = !g_fullscreen;

    if (g_fullscreen) {
        ShowCursor(FALSE);
        if (g_gfx.hStatusBar) ShowWindow(g_gfx.hStatusBar, SW_HIDE);

        /* Drop the menu and remember it, the placement and the style so
         * leaving fullscreen restores the window exactly. */
        g_old_menu = GetMenu(hwnd);
        if (g_old_menu) SetMenu(hwnd, NULL);
        g_old_pos.length = sizeof(g_old_pos);
        GetWindowPlacement(hwnd, &g_old_pos);
        g_old_style = GetWindowLongA(hwnd, GWL_STYLE);

        /* A borderless popup covering the whole screen. */
        SetWindowLongA(hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        SetWindowPos(hwnd, HWND_TOP, 0, 0,
                     GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                     SWP_SHOWWINDOW);
    } else {
        ShowCursor(TRUE);
        if (g_gfx.hStatusBar) ShowWindow(g_gfx.hStatusBar, SW_SHOW);
        if (g_old_menu) {
            SetMenu(hwnd, g_old_menu);
            g_old_menu = NULL;
        }
        SetWindowLongA(hwnd, GWL_STYLE, g_old_style);
        g_old_pos.length = sizeof(g_old_pos);
        SetWindowPlacement(hwnd, &g_old_pos);
    }
#endif
}

BOOL PJ64_CALL InitiateGFX(GFX_INFO gfx_info)
{
    g_gfx = gfx_info;
    return TRUE;
}

void PJ64_CALL CloseDLL(void)
{
    stop_runtime();
    memset(&g_gfx, 0, sizeof(g_gfx));
}

void PJ64_CALL RomOpen(void)
{
    (void)start_runtime();
}

void PJ64_CALL RomClosed(void)
{
    stop_runtime();
}

void PJ64_CALL ProcessDList(void)
{
    /*
     * This is the HLE graphics-list entry point in the PJ64 API. SoftRDP is
     * an LLE RDP plugin, so treating it as an RDP command buffer can make the
     * emulator wait forever on bogus DP state.
     */
    g_process_dlist_calls++;
    if (PJ64_LOG_ENABLED &&
        (g_process_dlist_calls <= 8u || (g_process_dlist_calls % 120u) == 0u)) {
        pj64_log_printf("ProcessDList ignored call=%u", g_process_dlist_calls);
    }
}

void PJ64_CALL ProcessRDPList(void)
{
    (void)start_runtime();
    g_process_rdp_calls++;

    if (!g_context) {
        pj64_log_printf("RDP call=%u no context, acknowledging", g_process_rdp_calls);
        acknowledge_dp_list();
        return;
    }

    const sr_result result = sr_plugin_process_list(g_context);
    (void)result;
#if SOFTRDP_ENABLE_LOG
    if (PJ64_LOG_ENABLED &&
        (log_sample_u32(g_process_rdp_calls, 32u, 600u) ||
         (result != SR_OK && log_sample_u32(g_process_rdp_calls, 128u, 120u)))) {
        const sr_debug_stats stats = sr_get_debug_stats(g_context);
        pj64_log_printf("RDP call=%u result=%s list=%08x-%08x bytes=%u last=%08x cmd=%02x ci=%08x/%ux%u/%u",
                        g_process_rdp_calls, result_name(result),
                        stats.last_list_current, stats.last_list_end,
                        stats.last_list_bytes, stats.last_command_address,
                        stats.last_command_id,
                        stats.color_image_address, stats.color_image_width,
                        stats.color_image_size, stats.color_image_format);
        if (result != SR_OK || command_has_texture_debug(stats.last_command_id))
            log_texture_debug(&stats);
    }
#endif
}

void PJ64_CALL DrawScreen(void)
{
    /* UpdateScreen owns presentation. PJ64 may call DrawScreen separately;
     * swapping here as well presents one frame twice and can block twice on
     * the display driver. */
}

void PJ64_CALL ReadScreen(void **dest, long *width, long *height)
{
    if (dest) *dest = NULL;
    if (width) *width = 0;
    if (height) *height = 0;
}

void PJ64_CALL UpdateScreen(void)
{
    ensure_window_size(g_gfx.hWnd);
    (void)start_runtime();
    g_update_screen_calls++;

    const sr_plugin_present_result shown =
        sr_plugin_present(&g_screen, g_context, &g_present);
    if (shown == SR_PLUGIN_PRESENTED) g_uploaded_frames++;

#if SOFTRDP_ENABLE_LOG
    if (PJ64_LOG_ENABLED &&
        (log_sample_u32(g_update_screen_calls, 32u, 600u) ||
         (shown != SR_PLUGIN_PRESENTED && log_sample_u32(g_update_screen_calls, 128u, 120u)))) {
        const sr_debug_stats st = sr_get_debug_stats(g_context);
        pj64_log_printf("UpdateScreen call=%u shown=%d origin=%06x state=%u size=%ux%u "
                        "bound_color=%06x bound_depth=%06x frames=%u",
                        g_update_screen_calls, (int)shown, st.scanout_origin,
                        st.scanout_state, st.scanout_width, st.scanout_height,
                        st.surface_color_address, st.surface_depth_address,
                        g_uploaded_frames);
    }
#endif
}

void PJ64_CALL ViStatusChanged(void)
{
}

void PJ64_CALL ViWidthChanged(void)
{
}

void PJ64_CALL ShowCFB(void)
{
}

void PJ64_CALL MoveScreen(int xpos, int ypos)
{
    (void)xpos;
    (void)ypos;
}

void PJ64_CALL FBWrite(DWORD addr, DWORD size)
{
    (void)addr;
    (void)size;
}

void PJ64_CALL FBWList(FrameBufferModifyEntry *plist, DWORD size)
{
    (void)plist;
    (void)size;
}

void PJ64_CALL FBRead(DWORD addr)
{
    (void)addr;
}

void PJ64_CALL FBGetFrameBufferInfo(void *pinfo)
{
    (void)pinfo;
}

void PJ64_CALL DllAbout(HWND hwnd)
{
    (void)hwnd;
}

#ifdef HAS_CONFIG
void PJ64_CALL DllConfig(HWND hwnd)
{
    (void)hwnd;
}
#endif

void PJ64_CALL DllTest(HWND hwnd)
{
    (void)hwnd;
}
