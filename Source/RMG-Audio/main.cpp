/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-sdl-audio - main.c                                        *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2007-2009 Richard Goedeken                              *
 *   Copyright (C) 2007-2008 Ebenblues                                     *
 *   Copyright (C) 2003 JttL                                               *
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

#include <SDL.h>
#include <SDL_audio.h>
#include <stdio.h>
#include <stdarg.h>

#include "RMG-Core/Settings/Settings.hpp"
#include "RMG-Core/Settings/SettingsID.hpp"
#include "main.hpp"

#include "sdl_backend.hpp"
#include "Resamplers/resamplers.hpp"

#define M64P_PLUGIN_PROTOTYPES 1
#define CORE_PLUGIN
#include <RMG-Core/Core.hpp>

#include "UserInterface//MainDialog.hpp"

/* version info */
#define SDL_AUDIO_PLUGIN_VERSION 0x020509
#define AUDIO_PLUGIN_API_VERSION 0x020000
#define CONFIG_PARAM_VERSION     1.00

#if SDL_VERSION_ATLEAST(2,0,0)
#define SDL_MixAudio(A, B, C, D) SDL_MixAudioFormat(A, B, AUDIO_S16SYS, C, D)
#endif

/* local variables */
static void (*l_DebugCallback)(void *, int, const char *) = nullptr;
static void *l_DebugCallContext = nullptr;
static int l_PluginInit = 0;

static struct sdl_backend* l_sdl_backend = nullptr;

/* Read header for type definition */
static AUDIO_INFO AudioInfo;
// volume to scale the audio by, range of 0..100
// if muted, this holds the volume when not muted
static int VolPercent = 80;
// how much percent to increment/decrement volume by
static int VolDelta = 5;
// the actual volume passed into SDL, range of 0..SDL_MIX_MAXVOLUME
static int VolSDL = SDL_MIX_MAXVOLUME;
// Muted or not
static int VolIsMuted = 0;

/* Helper functions */
static void ApplyVolumeSettings(void)
{
    if (VolIsMuted)
    {
        VolSDL = 0;
    }
    else
    {
        VolSDL = SDL_MIX_MAXVOLUME * VolPercent / 100;
    }
}

static void LoadVolumeSettings(void)
{
    VolIsMuted = CoreSettingsGetBoolValue(SettingsID::Audio_Muted) ? 1 : 0;
    VolPercent = CoreSettingsGetIntValue(SettingsID::Audio_Volume);

    ApplyVolumeSettings();
}

/* Global functions */
void DebugMessage(int level, const char *message, ...)
{
  char msgbuf[1024];
  va_list args;

  if (l_DebugCallback == nullptr)
      return;

  va_start(args, message);
  vsprintf(msgbuf, message, args);

  (*l_DebugCallback)(l_DebugCallContext, level, msgbuf);

  va_end(args);
}

/* Mupen64Plus plugin functions */
EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                   void (*DebugCallback)(void *, int, const char *))
{
    ptr_CoreGetAPIVersions CoreAPIVersionFunc;

    int ConfigAPIVersion, DebugAPIVersion, VidextAPIVersion;
    float fConfigParamsVersion = 0.0f;

    if (l_PluginInit)
        return M64ERR_ALREADY_INIT;

    /* first thing is to set the callback function for debug info */
    l_DebugCallback = DebugCallback;
    l_DebugCallContext = Context;

    /* Init RMG-Core */
    if (!CoreInit(CoreLibHandle))
    {
        return M64ERR_SYSTEM_FAIL;
    }

    // apply volume settings
    LoadVolumeSettings();

    l_PluginInit = 1;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (!l_PluginInit)
        return M64ERR_NOT_INIT;

    /* reset some local variables */
    l_DebugCallback = nullptr;
    l_DebugCallContext = nullptr;

    l_PluginInit = 0;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    if (PluginType != nullptr)
    {
        *PluginType = M64PLUGIN_AUDIO;
    }

    if (PluginVersion != nullptr)
    {
        *PluginVersion = SDL_AUDIO_PLUGIN_VERSION;
    }

    if (APIVersion != nullptr)
    {
        *APIVersion = AUDIO_PLUGIN_API_VERSION;
    }

    if (PluginNamePtr != nullptr)
    {
        *PluginNamePtr = "Rosalie's Mupen GUI - Audio Plugin";
    }

    if (Capabilities != nullptr)
    {
        *Capabilities = 0;
    }

    return M64ERR_SUCCESS;
}

/* ----------- Custom Functions ------------ */

EXPORT m64p_error CALL PluginConfig(void)
{
    if (!l_PluginInit)
    {
        return M64ERR_NOT_INIT;
    }

    UserInterface::MainDialog dialog(nullptr);
    dialog.exec();

    // apply volume settings
    LoadVolumeSettings();

    return M64ERR_SUCCESS;
}

/* ----------- Audio Functions ------------- */
static unsigned int vi_clock_from_system_type(int system_type)
{
    switch (system_type)
    {
    default:
        DebugMessage(M64MSG_WARNING, "Invalid system_type %d. Assuming NTSC", system_type);
        /* fallback */
    case SYSTEM_NTSC: return 48681812;
    case SYSTEM_PAL:  return 49656530;
    case SYSTEM_MPAL: return 48628316;
    }
}

static unsigned int dacrate2freq(unsigned int vi_clock, uint32_t dacrate)
{
    return vi_clock / (dacrate + 1);
}

EXPORT void CALL AiDacrateChanged(int SystemType)
{
    if (!l_PluginInit || l_sdl_backend == nullptr)
        return;

    unsigned int frequency = dacrate2freq(vi_clock_from_system_type(SystemType), *AudioInfo.AI_DACRATE_REG);

    sdl_set_frequency(l_sdl_backend, frequency);
}

EXPORT void CALL AiLenChanged(void)
{
    if (!l_PluginInit || l_sdl_backend == nullptr)
        return;

    sdl_push_samples(l_sdl_backend, AudioInfo.RDRAM + (*AudioInfo.AI_DRAM_ADDR_REG & 0xffffff), *AudioInfo.AI_LEN_REG);

    sdl_synchronize_audio(l_sdl_backend);
}

EXPORT int CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
    if (!l_PluginInit)
        return 0;

    AudioInfo = Audio_Info;
    return 1;
}

EXPORT int CALL RomOpen(void)
{
    if (!l_PluginInit || l_sdl_backend != nullptr)
        return 0;

    l_sdl_backend = init_sdl_backend();
    return 1;
}

EXPORT void CALL RomClosed(void)
{
    if (!l_PluginInit)
        return;

    release_sdl_backend(l_sdl_backend);
    l_sdl_backend = nullptr;
}

EXPORT void CALL ProcessAList(void)
{
}

EXPORT void CALL SetSpeedFactor(int percentage)
{
    if (!l_PluginInit || l_sdl_backend == nullptr)
        return;

    sdl_set_speed_factor(l_sdl_backend, percentage);
}

size_t ResampleAndMix(void* resampler, const struct resampler_interface* iresampler,
        void* mix_buffer,
        const void* src, size_t src_size, unsigned int src_freq,
        void* dst, size_t dst_size, unsigned int dst_freq)
{
    size_t consumed;

    consumed = iresampler->resample(resampler, src, src_size, src_freq, mix_buffer, dst_size, dst_freq);
    memset(dst, 0, dst_size);
    SDL_MixAudio((Uint8*)dst, (Uint8*)mix_buffer, dst_size, VolSDL);

    return consumed;
}

EXPORT void CALL VolumeMute(void)
{
    VolIsMuted = !VolIsMuted;
    ApplyVolumeSettings();
}

EXPORT void CALL VolumeUp(void)
{
    VolPercent += 10;
    ApplyVolumeSettings();
}

EXPORT void CALL VolumeDown(void)
{
    VolPercent -= 10;
    ApplyVolumeSettings();
}

EXPORT int CALL VolumeGetLevel(void)
{
    return VolIsMuted ? 0 : VolPercent;
}

EXPORT void CALL VolumeSetLevel(int level)
{
    VolPercent = level;
    ApplyVolumeSettings();
}

EXPORT const char * CALL VolumeGetString(void)
{
    return "";
}

