#define M64P_PLUGIN_PROTOTYPES 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#define DLSYM(a, b) GetProcAddress(a, b)
#else
#include <dlfcn.h>
#define DLSYM(a, b) dlsym(a, b)
#endif

#include "api/m64p_types.h"
#include "api/m64p_config.h"
#include "api/m64p_plugin.h"
#include "api/m64p_vidext.h"

#include "../plugin_common.h"

#define PLUGIN_VERSION              0x010600
#define VIDEO_PLUGIN_API_VERSION    0x020500

static m64p_dynlib_handle CoreLibHandle = NULL;
static void (*debug_callback)(void *, int, const char *) = NULL;
static void *debug_call_context = NULL;
static void (*render_callback)(int) = NULL;

static ptr_ConfigOpenSection      ConfigOpenSection = NULL;
static ptr_ConfigSaveSection      ConfigSaveSection = NULL;
static ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
static ptr_ConfigSetDefaultBool   ConfigSetDefaultBool = NULL;
static ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
static ptr_ConfigGetParamBool     ConfigGetParamBool = NULL;

static ptr_VidExt_Init                  CoreVideo_Init = NULL;
static ptr_VidExt_Quit                  CoreVideo_Quit = NULL;
static ptr_VidExt_SetVideoMode          CoreVideo_SetVideoMode = NULL;
static ptr_VidExt_SetCaption            CoreVideo_SetCaption = NULL;
static ptr_VidExt_ToggleFullScreen      CoreVideo_ToggleFullScreen = NULL;
static ptr_VidExt_ResizeWindow          CoreVideo_ResizeWindow = NULL;
static ptr_VidExt_GL_GetProcAddress     CoreVideo_GL_GetProcAddress = NULL;
static ptr_VidExt_GL_SetAttribute       CoreVideo_GL_SetAttribute = NULL;
static ptr_VidExt_GL_SwapBuffers        CoreVideo_GL_SwapBuffers = NULL;

static GFX_INFO g_gfx;
/*
 * Renderer options, read once per ROM in RomOpen. The defaults stand in until
 * then, so a core too old to expose the config API still runs.
 */
static sr_plugin_config g_config;
/* Outlives g_context: the renderer's interrupt callback reads through it. */
static sr_plugin_machine g_machine;
static sr_context *g_context = NULL;
static sr_present g_present;
static sr_plugin_screen g_screen;

static int32_t g_win_width = 640;
static int32_t g_win_height = 480;
static bool g_win_fullscreen = false;
static bool g_plugin_initialized = false;

static void msg_log(int level, const char *fmt, ...)
{
    if (!debug_callback) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debug_callback(debug_call_context, level, buf);
}

/* The frame dump recorder's messages, on the mupen64plus console. */
static void dump_log_sink(const char *message)
{
    if (debug_callback) debug_callback(debug_call_context, M64MSG_INFO, message);
}

static void *mupen_gl_proc_loader(const char *name)
{
    return CoreVideo_GL_GetProcAddress ? (void *)CoreVideo_GL_GetProcAddress(name) : NULL;
}

/*
 * Registering the defaults is what puts the keys into mupen64plus.cfg with
 * their descriptions. The VALUES are read in RomOpen; only their declaration
 * belongs at startup.
 *
 * Whether a front-end then offers them is its own business - RMG, for one,
 * only gives a settings button to plugins exporting its PluginConfig
 * extension, so the keys are edited in the cfg file, exactly as they are for
 * angrylion-plus. One warning if registration fails, because settings that
 * quietly never appear are the hard case to diagnose.
 */
static void register_config_defaults(void)
{
    m64p_handle general = NULL;
    if (ConfigOpenSection && ConfigOpenSection("Video-General", &general) == M64ERR_SUCCESS) {
        if (ConfigSetDefaultBool)
            ConfigSetDefaultBool(general, "Fullscreen", 0, "Use fullscreen mode if True, or windowed mode if False");
        if (ConfigSetDefaultInt) {
            ConfigSetDefaultInt(general, "ScreenWidth", 640, "Width of output window or fullscreen width");
            ConfigSetDefaultInt(general, "ScreenHeight", 480, "Height of output window or fullscreen height");
        }
        if (ConfigSaveSection) ConfigSaveSection("Video-General");
    }

    m64p_handle section = NULL;
    if (!ConfigOpenSection || !ConfigSetDefaultInt || !ConfigSaveSection ||
        ConfigOpenSection(SR_CONFIG_M64P_SECTION, &section) != M64ERR_SUCCESS) {
        msg_log(M64MSG_WARNING,
                "SoftRDP: could not register %s; built-in defaults will be used",
                SR_CONFIG_M64P_SECTION);
        return;
    }
    const sr_plugin_config defaults = sr_config_defaults();
    ConfigSetDefaultInt(section, SR_CONFIG_KEY_WORKERS, (int)defaults.workers,
                        SR_CONFIG_DESC_WORKERS);
    ConfigSetDefaultInt(section, SR_CONFIG_KEY_SCALE, (int)defaults.scale,
                        SR_CONFIG_DESC_SCALE);
    if (ConfigSetDefaultBool) {
        ConfigSetDefaultBool(section, SR_CONFIG_KEY_DISABLE_VI_DITHER_FILTER,
                             defaults.disable_vi_dither_filter,
                             SR_CONFIG_DESC_DISABLE_VI_DITHER_FILTER);
        ConfigSetDefaultBool(section, SR_CONFIG_KEY_DISABLE_VI_DIVOT_FILTER,
                             defaults.disable_vi_divot_filter,
                             SR_CONFIG_DESC_DISABLE_VI_DIVOT_FILTER);
        ConfigSetDefaultBool(section, SR_CONFIG_KEY_DISABLE_VI_GAMMA_DITHER,
                             defaults.disable_vi_gamma_dither,
                             SR_CONFIG_DESC_DISABLE_VI_GAMMA_DITHER);
        ConfigSetDefaultBool(section, SR_CONFIG_KEY_DISABLE_VI_AA,
                             defaults.disable_vi_aa,
                             SR_CONFIG_DESC_DISABLE_VI_AA);
    }
    ConfigSaveSection(SR_CONFIG_M64P_SECTION);
}

/*
 * Per ROM, not per plugin load: the core stays loaded across ROM launches, so
 * reading here means a changed setting takes effect on the next ROM rather
 * than on the next emulator start. Neither value can change while a ROM runs
 * - the scale sizes the sample planes when the context is created, and the
 * worker pool is built once.
 */
static void load_config(void)
{
    g_config = sr_config_defaults();

    m64p_handle general = NULL;
    if (ConfigOpenSection && ConfigOpenSection("Video-General", &general) == M64ERR_SUCCESS) {
        if (ConfigGetParamBool) g_win_fullscreen = ConfigGetParamBool(general, "Fullscreen");
        if (ConfigGetParamInt) {
            g_win_width = ConfigGetParamInt(general, "ScreenWidth");
            g_win_height = ConfigGetParamInt(general, "ScreenHeight");
        }
    }
    if (g_win_width <= 0) g_win_width = 640;
    if (g_win_height <= 0) g_win_height = 480;

    m64p_handle section = NULL;
    if (!ConfigOpenSection ||
        ConfigOpenSection(SR_CONFIG_M64P_SECTION, &section) != M64ERR_SUCCESS)
        return;
    if (ConfigGetParamInt) {
        g_config.workers = sr_config_clamp_workers(
            ConfigGetParamInt(section, SR_CONFIG_KEY_WORKERS));
        g_config.scale = sr_config_clamp_scale(
            ConfigGetParamInt(section, SR_CONFIG_KEY_SCALE));
    }
    if (ConfigGetParamBool) {
        g_config.disable_vi_dither_filter =
            ConfigGetParamBool(section, SR_CONFIG_KEY_DISABLE_VI_DITHER_FILTER) != 0;
        g_config.disable_vi_divot_filter =
            ConfigGetParamBool(section, SR_CONFIG_KEY_DISABLE_VI_DIVOT_FILTER) != 0;
        g_config.disable_vi_gamma_dither =
            ConfigGetParamBool(section, SR_CONFIG_KEY_DISABLE_VI_GAMMA_DITHER) != 0;
        g_config.disable_vi_aa =
            ConfigGetParamBool(section, SR_CONFIG_KEY_DISABLE_VI_AA) != 0;
    }
}

static bool load_video_extension(void)
{
    CoreVideo_Init = (ptr_VidExt_Init)DLSYM(CoreLibHandle, "VidExt_Init");
    CoreVideo_Quit = (ptr_VidExt_Quit)DLSYM(CoreLibHandle, "VidExt_Quit");
    CoreVideo_SetVideoMode = (ptr_VidExt_SetVideoMode)DLSYM(CoreLibHandle, "VidExt_SetVideoMode");
    CoreVideo_SetCaption = (ptr_VidExt_SetCaption)DLSYM(CoreLibHandle, "VidExt_SetCaption");
    CoreVideo_ToggleFullScreen = (ptr_VidExt_ToggleFullScreen)DLSYM(CoreLibHandle, "VidExt_ToggleFullScreen");
    CoreVideo_ResizeWindow = (ptr_VidExt_ResizeWindow)DLSYM(CoreLibHandle, "VidExt_ResizeWindow");
    CoreVideo_GL_GetProcAddress = (ptr_VidExt_GL_GetProcAddress)DLSYM(CoreLibHandle, "VidExt_GL_GetProcAddress");
    CoreVideo_GL_SetAttribute = (ptr_VidExt_GL_SetAttribute)DLSYM(CoreLibHandle, "VidExt_GL_SetAttribute");
    CoreVideo_GL_SwapBuffers = (ptr_VidExt_GL_SwapBuffers)DLSYM(CoreLibHandle, "VidExt_GL_SwapBuffers");

    return CoreVideo_Init && CoreVideo_Quit && CoreVideo_SetVideoMode &&
           CoreVideo_SetCaption && CoreVideo_ToggleFullScreen &&
           CoreVideo_GL_GetProcAddress && CoreVideo_GL_SetAttribute &&
           CoreVideo_GL_SwapBuffers;
}

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle _CoreLibHandle, void *Context,
                                     void (*DebugCallback)(void *, int, const char *))
{
    if (g_plugin_initialized) return M64ERR_ALREADY_INIT;

    debug_callback = DebugCallback;
    debug_call_context = Context;
    CoreLibHandle = _CoreLibHandle;

    ConfigOpenSection = (ptr_ConfigOpenSection)DLSYM(CoreLibHandle, "ConfigOpenSection");
    ConfigSaveSection = (ptr_ConfigSaveSection)DLSYM(CoreLibHandle, "ConfigSaveSection");
    ConfigSetDefaultInt = (ptr_ConfigSetDefaultInt)DLSYM(CoreLibHandle, "ConfigSetDefaultInt");
    ConfigSetDefaultBool = (ptr_ConfigSetDefaultBool)DLSYM(CoreLibHandle, "ConfigSetDefaultBool");
    ConfigGetParamInt = (ptr_ConfigGetParamInt)DLSYM(CoreLibHandle, "ConfigGetParamInt");
    ConfigGetParamBool = (ptr_ConfigGetParamBool)DLSYM(CoreLibHandle, "ConfigGetParamBool");

    register_config_defaults();

    g_plugin_initialized = true;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (!g_plugin_initialized) return M64ERR_NOT_INIT;

    sr_plugin_screen_reset(&g_screen);
    debug_callback = NULL;
    debug_call_context = NULL;
    g_plugin_initialized = false;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    if (PluginType) *PluginType = M64PLUGIN_GFX;
    if (PluginVersion) *PluginVersion = PLUGIN_VERSION;
    if (APIVersion) *APIVersion = VIDEO_PLUGIN_API_VERSION;
    if (PluginNamePtr) *PluginNamePtr = "SoftRDP-Mupen64Plus";
    if (Capabilities) *Capabilities = 0;
    return M64ERR_SUCCESS;
}

EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    g_gfx = Gfx_Info;
    return 1;
}

EXPORT void CALL MoveScreen(int xpos, int ypos)
{
    (void)xpos;
    (void)ypos;
}

EXPORT void CALL ProcessDList(void)
{
    /* HLE DList processing is not supported by SoftRDP. RSP must be run in LLE mode. */
}

EXPORT void CALL ProcessRDPList(void)
{
    (void)sr_plugin_process_list(g_context);
}

EXPORT int CALL RomOpen(void)
{
    if (!load_video_extension()) {
        msg_log(M64MSG_ERROR, "SoftRDP: Failed to load Mupen64Plus Video Extension functions.");
        return 0;
    }

    load_config();
    msg_log(M64MSG_INFO, "SoftRDP: scale %ux, workers %u", g_config.scale, g_config.workers);

    if (CoreVideo_Init() != M64ERR_SUCCESS) {
        msg_log(M64MSG_ERROR, "SoftRDP: VidExt_Init failed.");
        return 0;
    }
    CoreVideo_SetCaption("SoftRDP");

    CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_PROFILE_MASK, M64P_GL_CONTEXT_PROFILE_CORE);
    CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MAJOR_VERSION, 3);
    CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MINOR_VERSION, 3);
    CoreVideo_GL_SetAttribute(M64P_GL_DOUBLEBUFFER, 1);

    const m64p_video_mode mode = g_win_fullscreen ? M64VIDEO_FULLSCREEN : M64VIDEO_WINDOWED;
    if (CoreVideo_SetVideoMode(g_win_width, g_win_height, 0, mode,
                               M64VIDEOFLAG_SUPPORT_RESIZING) != M64ERR_SUCCESS) {
        msg_log(M64MSG_ERROR, "SoftRDP: VidExt_SetVideoMode failed.");
        CoreVideo_Quit();
        return 0;
    }

    /* mupen64plus keeps RDRAM in host word order unconditionally. The size
     * arrived with version 2 of GFX_INFO; older cores get the full 8 MiB. */
    SR_PLUGIN_MACHINE_FROM_GFX(&g_machine, &g_gfx);
    g_machine.rdram_bswapped = true;
    g_machine.rdram_size = (g_gfx.version >= 2 && g_gfx.RDRAM_SIZE)
        ? *g_gfx.RDRAM_SIZE : 0x800000u;

    g_context = sr_plugin_context_create(&g_machine, &g_config, dump_log_sink);
    if (!g_context) {
        msg_log(M64MSG_ERROR, "SoftRDP: Failed to create SoftRDP context.");
        CoreVideo_Quit();
        return 0;
    }

    if (!sr_present_init_external(&g_present, mupen_gl_proc_loader)) {
        msg_log(M64MSG_ERROR, "SoftRDP: Failed to initialize OpenGL presenter.");
        sr_plugin_context_destroy(g_context);
        g_context = NULL;
        CoreVideo_Quit();
        return 0;
    }

    sr_present_set_window_size(&g_present, (uint32_t)g_win_width, (uint32_t)g_win_height);
    sr_present_set_display_size(&g_present, 640, 480);
    sr_present_clear(&g_present);
    return 1;
}

EXPORT void CALL RomClosed(void)
{
    sr_present_shutdown(&g_present);
    sr_plugin_screen_reset(&g_screen);
    sr_plugin_context_destroy(g_context);
    g_context = NULL;
    if (CoreVideo_Quit) CoreVideo_Quit();
}

EXPORT void CALL ShowCFB(void)
{
}

EXPORT void CALL UpdateScreen(void)
{
    (void)sr_plugin_present(&g_screen, g_context, &g_present);
    if (render_callback) render_callback(1);
    if (CoreVideo_GL_SwapBuffers) CoreVideo_GL_SwapBuffers();
}

EXPORT void CALL ViStatusChanged(void)
{
}

EXPORT void CALL ViWidthChanged(void)
{
}

EXPORT void CALL ChangeWindow(void)
{
    if (CoreVideo_ToggleFullScreen) CoreVideo_ToggleFullScreen();
}

EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
    (void)front;
    if (!g_screen.shown_pixels || !dest || !width || !height) return;
    *width = (int)g_screen.shown_width;
    *height = (int)g_screen.shown_height;
    uint8_t *dst = dest;
    const uint32_t total = g_screen.shown_width * g_screen.shown_height;
    for (uint32_t i = 0; i < total; i++) {
        dst[i * 3 + 0] = g_screen.shown_pixels[i].r;
        dst[i * 3 + 1] = g_screen.shown_pixels[i].g;
        dst[i * 3 + 2] = g_screen.shown_pixels[i].b;
    }
}

EXPORT void CALL SetRenderingCallback(void (*callback)(int))
{
    render_callback = callback;
}

EXPORT void CALL ResizeVideoOutput(int width, int height)
{
    g_win_width = width;
    g_win_height = height;
    sr_present_set_window_size(&g_present, (uint32_t)width, (uint32_t)height);
}

EXPORT void CALL FBWrite(unsigned int addr, unsigned int size)
{
    (void)addr;
    (void)size;
}

EXPORT void CALL FBRead(unsigned int addr)
{
    (void)addr;
}

EXPORT void CALL FBGetFrameBufferInfo(void *pinfo)
{
    (void)pinfo;
}
