/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-video-angrylionplus - plugin.c                            *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define M64P_PLUGIN_PROTOTYPES 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "gfx_m64p.h"
#include "screen.h"
#include "parallel_imp.h"
#include "util/logging.hpp"
#ifdef CONFIG_GUI
#include "UserInterface/MainDialog.hpp"
#endif // CONFIG_GUI

#include "m64p_types.h"
#include "m64p_config.h"
#include "m64p_plugin.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

ptr_ConfigOpenSection ConfigOpenSection = NULL;
ptr_ConfigSaveSection ConfigSaveSection = NULL;
ptr_ConfigSetDefaultInt ConfigSetDefaultInt = NULL;
ptr_ConfigSetDefaultBool ConfigSetDefaultBool = NULL;
ptr_ConfigGetParamInt ConfigGetParamInt = NULL;
ptr_ConfigGetParamBool ConfigGetParamBool = NULL;
ptr_ConfigSetParameter ConfigSetParameter = NULL;
ptr_ConfigReceiveNetplayConfig ConfigReceiveNetplayConfig = NULL;

static bool vk_initialized;
static bool warn_hle;
static bool plugin_initialized;
void (*debug_callback)(void *, int, const char *);
void *debug_call_context;

m64p_dynlib_handle CoreLibHandle;
GFX_INFO gfx;
void (*render_callback)(int);

m64p_handle configVideoParallel = NULL;

#define PLUGIN_VERSION 0x000001
#define VIDEO_PLUGIN_API_VERSION 0x020200
#define DP_INTERRUPT 0x20

uint32_t rdram_size;
static ptr_PluginGetVersion CoreGetVersion = NULL;

void plugin_init(void)
{
    CoreGetVersion = (ptr_PluginGetVersion)DLSYM(CoreLibHandle, "PluginGetVersion");

    int core_version;
    CoreGetVersion(NULL, &core_version, NULL, NULL, NULL);
    if (core_version >= 0x020501)
    {
        rdram_size = *gfx.RDRAM_SIZE;
    }
    else
    {
        rdram_size = 0x800000;
    }
}

void plugin_close(void)
{
}

void DebugMessage(int level, const char *message, ...)
{
    char msgbuf[1024];
    va_list args;

    if (debug_callback == nullptr)
    {
        return;
    }

    va_start(args, message);
    vsprintf(msgbuf, message, args);

    (*debug_callback)(debug_call_context, level, msgbuf);

    va_end(args);
}

class MupenLoggingInterface : public Util::LoggingInterface
{
public:
    bool log(const char *tag, const char *fmt, va_list va)
    {
        char buf[1024];
        int  buf_len = 0;
        int  level = M64MSG_VERBOSE;

        // copy string to buf
        vsnprintf(buf, sizeof(buf), fmt, va);

        // determine log level
        if (strncmp(tag, "[INFO]", 6) == 0)
        {
            level = M64MSG_INFO;
        }
        else if (strncmp(tag, "[WARN]", 6) == 0)
        {
            level = M64MSG_WARNING;
        }
        else if (strncmp(tag, "[ERROR]", 7) == 0)
        {
            level = M64MSG_ERROR;
        }

        // strip newline
        buf_len = strlen(buf); 
        if (buf[buf_len - 1] == '\n')
        {
            buf[buf_len - 1] = '\0';
        }

        DebugMessage(level, buf);
        return true;
    }
};

static MupenLoggingInterface l_LoggingInterface;

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle _CoreLibHandle, void *Context,
                                     void (*DebugCallback)(void *, int, const char *))
{
    if (plugin_initialized)
    {
        return M64ERR_ALREADY_INIT;
    }

    /* first thing is to set the callback function for debug info */
    debug_callback = DebugCallback;
    debug_call_context = Context;

    CoreLibHandle = _CoreLibHandle;

    ConfigOpenSection = (ptr_ConfigOpenSection)DLSYM(CoreLibHandle, "ConfigOpenSection");
    ConfigSaveSection = (ptr_ConfigSaveSection)DLSYM(CoreLibHandle, "ConfigSaveSection");
    ConfigSetDefaultInt = (ptr_ConfigSetDefaultInt)DLSYM(CoreLibHandle, "ConfigSetDefaultInt");
    ConfigSetDefaultBool = (ptr_ConfigSetDefaultBool)DLSYM(CoreLibHandle, "ConfigSetDefaultBool");
    ConfigGetParamInt = (ptr_ConfigGetParamInt)DLSYM(CoreLibHandle, "ConfigGetParamInt");
    ConfigGetParamBool = (ptr_ConfigGetParamBool)DLSYM(CoreLibHandle, "ConfigGetParamBool");
    ConfigSetParameter = (ptr_ConfigSetParameter)DLSYM(CoreLibHandle, "ConfigSetParameter");
    ConfigReceiveNetplayConfig = (ptr_ConfigReceiveNetplayConfig)DLSYM(CoreLibHandle, "ConfigReceiveNetplayConfig");

    ConfigOpenSection("Video-Parallel", &configVideoParallel);
    ConfigSetDefaultInt(configVideoParallel, KEY_UPSCALING, 1, "Amount of rescaling: 1=None, 2=2x, 4=4x, 8=8x");
    ConfigSetDefaultBool(configVideoParallel, KEY_VSYNC, 0, "Enable VSync");
    ConfigSetDefaultInt(configVideoParallel, KEY_SCREEN_WIDTH, 640, "Screen width");
    ConfigSetDefaultInt(configVideoParallel, KEY_SCREEN_HEIGHT, 480, "Screen height");
    ConfigSetDefaultBool(configVideoParallel, KEY_WIDESCREEN, 0, "Widescreen mode (stretched)");
    ConfigSetDefaultBool(configVideoParallel, KEY_DEINTERLACE, 0, "Deinterlacing method. False=Bob, True=Weave");
    ConfigSetDefaultBool(configVideoParallel, KEY_SSREADBACKS, 0, "Enable superscaling of readbacks when upsampling");
    ConfigSetDefaultBool(configVideoParallel, KEY_SSDITHER, 1, "Enable superscaling of dithering when upsampling");
    ConfigSetDefaultInt(configVideoParallel, KEY_OVERSCANCROPLEFT, 0, "Amount of overscan pixels to crop on the left side");
    ConfigSetDefaultInt(configVideoParallel, KEY_OVERSCANCROPRIGHT, 0, "Amount of overscan pixels to crop on the right side");
    ConfigSetDefaultInt(configVideoParallel, KEY_OVERSCANCROPTOP, 0, "Amount of overscan pixels to crop on the top side");
    ConfigSetDefaultInt(configVideoParallel, KEY_OVERSCANCROPBOTTOM, 0, "Amount of overscan pixels to crop on the bottom side");
    ConfigSetDefaultBool(configVideoParallel, KEY_OVERSCANCROPENABLE, 0, "Whether to enable overscan");
    ConfigSetDefaultInt(configVideoParallel, KEY_VERTICAL_STRETCH, 0, "Amount of pixels to stretch by vertically. Can fix PAL ports that didn't fill the PAL resolution of 288p (use value of 24 in that case).");
    ConfigSetDefaultBool(configVideoParallel, KEY_AA, 1, "VI anti-aliasing, smooths polygon edges.");
    ConfigSetDefaultBool(configVideoParallel, KEY_DIVOT, 1, "Allow VI divot filter, cleans up stray black pixels.");
    ConfigSetDefaultBool(configVideoParallel, KEY_GAMMADITHER, 1, "Allow VI gamma dither");
    ConfigSetDefaultBool(configVideoParallel, KEY_VIBILERP, 1, "Allow VI bilinear scaling");
    ConfigSetDefaultBool(configVideoParallel, KEY_VIDITHER, 1, "Allow VI dedither filter");
    ConfigSetDefaultInt(configVideoParallel, KEY_DOWNSCALE, 0, "Downsampling factor, downscales output after VI, equivalent to SSAA. 0=disabled, 1=1/2, 2=1/4, 3=1/8");
    ConfigSetDefaultBool(configVideoParallel, KEY_NATIVETEXTLOD, 0, "Use native texture LOD computation when upscaling, effectively a LOD bias");
    ConfigSetDefaultBool(configVideoParallel, KEY_NATIVETEXTRECT, 1, "Native resolution TEX_RECT. TEX_RECT primitives should generally be rendered at native resolution to avoid seams");
    ConfigSaveSection("Video-Parallel");

    // set logging interface
    Util::set_thread_logging_interface(&l_LoggingInterface);

    plugin_initialized = true;
    vk_initialized = false;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{

    if (!plugin_initialized)
    {
        return M64ERR_NOT_INIT;
    }

    /* reset some local variable */
    debug_callback = NULL;
    debug_call_context = NULL;

    plugin_initialized = false;
    vk_initialized = false;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    /* set version info */
    if (PluginType != NULL)
    {
        *PluginType = M64PLUGIN_GFX;
    }

    if (PluginVersion != NULL)
    {
        *PluginVersion = PLUGIN_VERSION;
    }

    if (APIVersion != NULL)
    {
        *APIVersion = VIDEO_PLUGIN_API_VERSION;
    }

    if (PluginNamePtr != NULL)
    {
        *PluginNamePtr = "parallel";
    }

    if (Capabilities != NULL)
    {
        *Capabilities = 0;
    }

    return M64ERR_SUCCESS;
}

#ifdef CONFIG_GUI
extern "C"
{
    EXPORT m64p_error CALL PluginConfig(void* parent)
    {
        if (!plugin_initialized)
        {
            return M64ERR_NOT_INIT;
        }

        UserInterface::MainDialog dialog((QWidget*)parent);
        dialog.exec();

        return M64ERR_SUCCESS;
    }
}
#endif // CONFIG_GUI

EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    gfx = Gfx_Info;

    return 1;
}

EXPORT void CALL MoveScreen(int xpos, int ypos)
{
}

EXPORT void CALL ProcessDList(void)
{
}

EXPORT void CALL ProcessRDPList(void)
{
    vk_process_commands();
}

EXPORT int CALL RomOpen(void)
{
    window_fullscreen = false;
    window_width = ConfigGetParamInt(configVideoParallel, KEY_SCREEN_WIDTH);
    window_height = ConfigGetParamInt(configVideoParallel, KEY_SCREEN_HEIGHT);
    window_widescreen = ConfigGetParamBool(configVideoParallel, KEY_WIDESCREEN);
    window_vsync = ConfigGetParamBool(configVideoParallel, KEY_VSYNC);
    vk_rescaling = ConfigGetParamInt(configVideoParallel, KEY_UPSCALING);
    vk_ssreadbacks = ConfigGetParamBool(configVideoParallel, KEY_SSREADBACKS);
    vk_ssdither = ConfigGetParamBool(configVideoParallel, KEY_SSDITHER);

    vk_divot_filter = ConfigGetParamBool(configVideoParallel, KEY_DIVOT);
    vk_gamma_dither = ConfigGetParamBool(configVideoParallel, KEY_GAMMADITHER);
    vk_vi_scale = ConfigGetParamBool(configVideoParallel, KEY_VIBILERP);
    vk_vi_aa = ConfigGetParamBool(configVideoParallel, KEY_AA);
    vk_dither_filter = ConfigGetParamBool(configVideoParallel, KEY_VIDITHER);
    vk_native_texture_lod = ConfigGetParamBool(configVideoParallel, KEY_NATIVETEXTLOD);
    vk_native_tex_rect = ConfigGetParamBool(configVideoParallel, KEY_NATIVETEXTRECT);
    vk_interlacing = ConfigGetParamBool(configVideoParallel, KEY_DEINTERLACE);
    vk_downscaling_steps = ConfigGetParamInt(configVideoParallel, KEY_DOWNSCALE);
    vk_overscan_left = ConfigGetParamInt(configVideoParallel, KEY_OVERSCANCROPLEFT);
    vk_overscan_right = ConfigGetParamInt(configVideoParallel, KEY_OVERSCANCROPRIGHT);
    vk_overscan_top = ConfigGetParamInt(configVideoParallel, KEY_OVERSCANCROPTOP);
    vk_overscan_bottom = ConfigGetParamInt(configVideoParallel, KEY_OVERSCANCROPBOTTOM);
    vk_overscan_enable = ConfigGetParamBool(configVideoParallel, KEY_OVERSCANCROPENABLE);
    vk_vertical_stretch = ConfigGetParamInt(configVideoParallel, KEY_VERTICAL_STRETCH);

    m64p_error netplay_init = ConfigReceiveNetplayConfig(NULL, 0);
    if (netplay_init != M64ERR_NOT_INIT)
    {
        DebugMessage(M64MSG_INFO, "Netplay enabled, disabling vsync");
        window_vsync   = 0; // force disable vsync during netplay
        vk_ssreadbacks = 0; // can cause desyncs
    }

    plugin_init();

    if (vk_init())
    {
        vk_initialized = true;
        return 1;
    }
    else
    {
        vk_initialized = false;
        DebugMessage(M64MSG_ERROR, "Could not start GFX plugin");
        return 0;
    }
}

EXPORT void CALL RomClosed(void)
{
    if (vk_initialized)
    {
        vk_destroy();
    }
}

EXPORT void CALL ShowCFB(void)
{
    vk_rasterize();
}

EXPORT void CALL UpdateScreen(void)
{
    vk_rasterize();
}

EXPORT void CALL ViStatusChanged(void)
{
}

EXPORT void CALL ViWidthChanged(void)
{
}

EXPORT void CALL ChangeWindow(void)
{
    screen_toggle_fullscreen();
}

EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
    *width = window_width;
    *height = window_height;
    if (dest)
    {
        vk_read_screen((unsigned char*)dest);
    }
}

EXPORT void CALL SetRenderingCallback(void (*callback)(int))
{
    render_callback = callback;
}

EXPORT void CALL ResizeVideoOutput(int width, int height)
{
    window_width = width;
    window_height = height;
    vk_resize();
}

EXPORT void CALL FBWrite(unsigned int addr, unsigned int size)
{
}

EXPORT void CALL FBRead(unsigned int addr)
{
}

EXPORT void CALL FBGetFrameBufferInfo(void *pinfo)
{
}
