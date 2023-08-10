/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "simple_sdl_backend.hpp"
#include "main.hpp"

#define CORE_PLUGIN
#include <RMG-Core/Core.hpp>

#include <SDL.h>

//
// Local Variables
//

static SDL_AudioDeviceID l_SDLDevice = 0;
static SDL_AudioSpec    l_HardwareSpec;
static SDL_AudioStream* l_AudioStream;

static uint8_t l_PrimaryBuffer[0x40000];
static uint8_t l_OutputBuffer[0x40000];
static uint8_t l_MixBuffer[0x40000];

static bool l_Paused = false;
static int l_Volume  = 0;
static uint32_t l_SpeedFactor = 100;
static bool l_Initialized = false;

//
// Exported Functions
//

bool init_simple_sdl_backend(void)
{
    SDL_AudioSpec desired;
    desired.freq     = 44100;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples  = 1024;
    desired.callback = nullptr;
    desired.userdata = nullptr;

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER);

    l_SDLDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &l_HardwareSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (l_SDLDevice == 0)
    {
        DebugMessage(M64MSG_ERROR, "init_simple_sdl_backend() SDL_OpenAudioDevice() Failed: %s", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(l_SDLDevice, 0);

    l_AudioStream = SDL_NewAudioStream(AUDIO_S16SYS, 2, 33600, l_HardwareSpec.format, 2, l_HardwareSpec.freq);
    if (l_AudioStream == nullptr)
    {
        DebugMessage(M64MSG_ERROR, "init_simple_sdl_backend() SDL_NewAudioStream() Failed: %s", SDL_GetError());
        release_simple_sdl_backend();
        return false;
    }

    l_Paused = false;
    l_Initialized = true;
    return true;
}

void release_simple_sdl_backend(void)
{
    if (l_AudioStream != nullptr)
    {
        SDL_FreeAudioStream(l_AudioStream);
        l_AudioStream = nullptr;
    }

    if (l_SDLDevice != 0)
    {
        SDL_CloseAudioDevice(l_SDLDevice);
        l_SDLDevice = 0;
    }
}

void simple_sdl_push_samples(const void* src, size_t size)
{
    if (!l_Initialized)
    {
        return;
    }

    const uint8_t* p_src = (const uint8_t*)src;

    for (uint32_t i = 0 ; i < size ; i += 4 )
    {
        // Left channel
        l_PrimaryBuffer[i] = p_src[i + 2 ];
        l_PrimaryBuffer[i + 1] = p_src[i + 3 ];

        // Right channel
        l_PrimaryBuffer[i + 2] = p_src[i];
        l_PrimaryBuffer[i + 3] = p_src[i + 1];
    }

    if (l_Volume != 0)
    {
        uint32_t audio_queued = SDL_GetQueuedAudioSize(l_SDLDevice);
        uint32_t acceptable_latency = (l_HardwareSpec.freq * 0.2) * 4;
        uint32_t min_latency = (l_HardwareSpec.freq * 0.02) * 4;

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
            SDL_AudioStreamPut(l_AudioStream, l_PrimaryBuffer, size);
        }

        int output_length = SDL_AudioStreamGet(l_AudioStream, l_OutputBuffer, sizeof(l_OutputBuffer));
        if (output_length > 0)
        {
            SDL_memset(l_MixBuffer, 0, output_length);
            SDL_MixAudioFormat(l_MixBuffer, l_OutputBuffer, l_HardwareSpec.format, output_length, l_Volume);
            SDL_QueueAudio(l_SDLDevice, l_MixBuffer, output_length);
        }
    }
}

void simple_sdl_set_frequency(unsigned int frequency)
{
    if (!l_Initialized)
    {
        return;
    }

    SDL_FreeAudioStream(l_AudioStream);
    l_AudioStream = SDL_NewAudioStream(AUDIO_S16SYS, 2, frequency, l_HardwareSpec.format, 2, l_HardwareSpec.freq);
    if (l_AudioStream == nullptr)
    {
        DebugMessage(M64MSG_ERROR, "simple_sdl_set_frequency() SDL_NewAudioStream() Failed: %s", SDL_GetError());
    }
}

void simple_sdl_set_volume(int volume)
{
    l_Volume = volume;
}

void simple_sdl_set_speed_factor(uint32_t speed_factor)
{
    l_SpeedFactor = speed_factor;
}