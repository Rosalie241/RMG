/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_PLUGIN 1
#define M64P_PLUGIN_PROTOTYPES 1
#define AUDIO_PLUGIN_API_VERSION 0x020100

#include <SDL.h>
#include <SDL_audio.h>

#include <UserInterface/MainDialog.hpp>

#include <RMG-Core/Core.hpp>

//
// Local variables
//

static SDL_AudioDeviceID l_SDLDevice;
static SDL_AudioSpec* l_HardwareSpec  = nullptr;

static bool l_PluginInit              = false;
static int l_GameFreq                 = 0;
static AUDIO_INFO l_AudioInfo;
static bool l_VolIsMuted              = false;
static bool l_Paused                  = false;
static bool l_FastForward             = false;
static int  l_VolSDL                  = SDL_MIX_MAXVOLUME;
static SDL_AudioStream* l_AudioStream = nullptr;

static uint8_t l_PrimaryBuffer[0x40000];
static uint8_t l_OutputBuffer[0x40000];
static uint8_t l_MixBuffer[0x40000];

//
// Local Functions
//

static void load_settings(void)
{
    l_VolIsMuted = CoreSettingsGetBoolValue(SettingsID::Audio_Muted);
    l_VolSDL = SDL_MIX_MAXVOLUME * CoreSettingsGetIntValue(SettingsID::Audio_Volume) / 100;
    l_FastForward = false;
}

//
// Basic Plugin Functions
//

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context, void (*DebugCallback)(void *, int, const char *))
{
    if (l_PluginInit)
    {
        return M64ERR_ALREADY_INIT;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        return M64ERR_SYSTEM_FAIL;
    }

    if (!CoreInit(CoreLibHandle))
    {
        return M64ERR_SYSTEM_FAIL;
    }

    load_settings();

    l_PluginInit = true;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (!l_PluginInit)
    {
        return M64ERR_NOT_INIT;
    }

    if (l_HardwareSpec != nullptr)\
    { 
        free(l_HardwareSpec);
        l_HardwareSpec = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    l_PluginInit = false;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *pluginType, int *pluginVersion, 
    int *apiVersion, const char **pluginNamePtr, int *capabilities)
{
    if (pluginType != nullptr)
    {
        *pluginType = M64PLUGIN_AUDIO;
    }

    if (pluginVersion != nullptr)
    {
        *pluginVersion = 0x010000;
    }

    if (apiVersion != nullptr)
    {
        *apiVersion = AUDIO_PLUGIN_API_VERSION;
    }

    if (pluginNamePtr != nullptr)
    {
        *pluginNamePtr = "Rosalie's Mupen GUI - Audio Plugin";
    }

    if (capabilities != nullptr)
    {
        *capabilities = 0;
    }

    return M64ERR_SUCCESS;
}

//
// Custom Plugin Functions
//

EXPORT m64p_error CALL PluginConfig(void)
{
    if (!l_PluginInit)
    {
        return M64ERR_NOT_INIT;
    }

    UserInterface::MainDialog dialog(nullptr);
    dialog.exec();

    // reload settings
    load_settings();

    return M64ERR_SUCCESS;
}

//
// Audio Plugin Functions
//

EXPORT void CALL AiDacrateChanged( int SystemType )
{
    if (!l_PluginInit)
    {
        return;
    }

    switch (SystemType)
    {
        case SYSTEM_NTSC:
            l_GameFreq = 48681812 / (*l_AudioInfo.AI_DACRATE_REG + 1);
            break;
        case SYSTEM_PAL:
            l_GameFreq = 49656530 / (*l_AudioInfo.AI_DACRATE_REG + 1);
            break;
        case SYSTEM_MPAL:
            l_GameFreq = 48628316 / (*l_AudioInfo.AI_DACRATE_REG + 1);
            break;
    }
    SDL_FreeAudioStream(l_AudioStream);
    l_AudioStream = SDL_NewAudioStream(AUDIO_S16SYS, 2, l_GameFreq, l_HardwareSpec->format, 2, l_HardwareSpec->freq);
}

EXPORT void CALL AiLenChanged( void )
{
    if (!l_PluginInit)
    {
        return;
    }

    unsigned int LenReg = *l_AudioInfo.AI_LEN_REG;
    unsigned char *p = l_AudioInfo.RDRAM + (*l_AudioInfo.AI_DRAM_ADDR_REG & 0xFFFFFF);

    unsigned int i;

    for ( i = 0 ; i < LenReg ; i += 4 )
    {
        // Left channel
        l_PrimaryBuffer[ i ] = p[ i + 2 ];
        l_PrimaryBuffer[ i + 1 ] = p[ i + 3 ];

        // Right channel
        l_PrimaryBuffer[ i + 2 ] = p[ i ];
        l_PrimaryBuffer[ i + 3 ] = p[ i + 1 ];
    }

    if (!l_VolIsMuted && !l_FastForward)
    {
        unsigned int audio_queued = SDL_GetQueuedAudioSize(l_SDLDevice);
        unsigned int acceptable_latency = (l_HardwareSpec->freq * 0.2) * 4;
        unsigned int min_latency = (l_HardwareSpec->freq * 0.02) * 4;

        if (!l_Paused && audio_queued < min_latency)
        {
            SDL_PauseAudioDevice(l_SDLDevice, 1);
            l_Paused = true;
        }
        else if (l_Paused && audio_queued >= (min_latency * 2))
        {
            SDL_PauseAudioDevice(l_SDLDevice, 0);
            l_Paused = true;
        }

        if (audio_queued < acceptable_latency)
        {
            SDL_AudioStreamPut(l_AudioStream, l_PrimaryBuffer, LenReg);
        }

        int output_length = SDL_AudioStreamGet(l_AudioStream, l_OutputBuffer, sizeof(l_OutputBuffer));
        if (output_length > 0)
        {
            SDL_memset(l_MixBuffer, 0, output_length);
            SDL_MixAudioFormat(l_MixBuffer, l_OutputBuffer, l_HardwareSpec->format, output_length, l_VolSDL);
            SDL_QueueAudio(l_SDLDevice, l_MixBuffer, output_length);
        }
    }
}

EXPORT int CALL InitiateAudio( AUDIO_INFO Audio_Info )
{
    if (!l_PluginInit)
    {
        return 0;
    }

    l_GameFreq = 33600;
    l_AudioInfo = Audio_Info;

    return 1;
}

EXPORT int CALL RomOpen(void)
{
    if (!l_PluginInit)
    {
        return 0;
    }

    SDL_AudioSpec *desired, *obtained;
    if (l_HardwareSpec != nullptr)
    {
        free(l_HardwareSpec);
    }
    desired = (SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));
    obtained = (SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));
    desired->freq = 44100;
    desired->format = AUDIO_S16SYS;
    desired->channels = 2;
    desired->samples = 1024;
    desired->callback = nullptr;
    desired->userdata = nullptr;

    const char *dev_name = SDL_GetAudioDeviceName(-1, 0);
    l_SDLDevice = SDL_OpenAudioDevice(dev_name, 0, desired, obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    free(desired);
    l_HardwareSpec = obtained;
    SDL_PauseAudioDevice(l_SDLDevice, 0);
    l_Paused = 0;
    l_AudioStream = SDL_NewAudioStream(AUDIO_S16SYS, 2, l_GameFreq, l_HardwareSpec->format, 2, l_HardwareSpec->freq);

    return 1;
}

EXPORT void CALL RomClosed( void )
{
    if (!l_PluginInit)
    {
        return;
    }

    SDL_ClearQueuedAudio(l_SDLDevice);
    SDL_CloseAudioDevice(l_SDLDevice);

    if (l_HardwareSpec != nullptr)
    { 
        free(l_HardwareSpec);
        l_HardwareSpec = nullptr;
    }

    SDL_FreeAudioStream(l_AudioStream);
    l_AudioStream = nullptr;
}

EXPORT void CALL ProcessAList(void)
{
}

EXPORT void CALL SetSpeedFactor(int percentage)
{
    if (percentage > 100)
    {
        l_FastForward = true;
    }
    else
    {
        l_FastForward = false;
    }
}

EXPORT void CALL VolumeMute(void)
{
    if (!l_PluginInit)
    {
        return;
    }

    l_VolIsMuted = !l_VolIsMuted;
}

EXPORT void CALL VolumeUp(void)
{
}

EXPORT void CALL VolumeDown(void)
{
}

EXPORT int CALL VolumeGetLevel(void)
{
    return l_VolIsMuted ? 0 : 100;
}

EXPORT void CALL VolumeSetLevel(int level)
{
    if (level < 0)
    {
        level = 0;
    }
    else if (level > 100)
    {
        level = 100;
    }

    l_VolSDL = SDL_MIX_MAXVOLUME * level / 100;
}

EXPORT const char * CALL VolumeGetString(void)
{
    return "";
}
