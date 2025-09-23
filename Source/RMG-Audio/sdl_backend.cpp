/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-sdl-audio - sdl_backend.c                                 *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2017 Bobby Smiles                                       *
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

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include <stdlib.h>
#include <string.h>

#include "Resamplers/resamplers.hpp"
#include "main.hpp"

#include <RMG-Core/m64p/api/m64p_types.h>

#include <RMG-Core/Settings.hpp>
#include <RMG-Core/Netplay.hpp>

/* number of bytes per sample */
#define N64_SAMPLE_BYTES 4
#define SDL_SAMPLE_BYTES 4

struct sdl_backend
{
    /* Audio Stream */
    SDL_AudioStream* stream;

    /* Primary Buffer */
    void* primary_buffer;

    /* Resampling buffer */
    void* resample_buffer;

    /* Size of buffers */
    size_t buffers_size;

    unsigned int frequency;

    unsigned int speed_factor;

    unsigned int swap_channels;

    unsigned int error;

    float volume;

    /* Resampler */
    void* resampler;
    const struct resampler_interface* iresampler;
};

/* SDL_AudioFormat.format format specifier and args builder */
#define AFMT_FMTSPEC        "%c%d%s"
#define AFMT_ARGS(x) \
        ((SDL_AUDIO_ISFLOAT(x)) ? 'F' : (SDL_AUDIO_ISSIGNED(x)) ? 'S' : 'U'), \
        SDL_AUDIO_BITSIZE(x), \
        SDL_AUDIO_ISBIGENDIAN(x) ? "BE" : "LE"

static void sdl_init_audio_device(struct sdl_backend* sdl_backend)
{
    SDL_AudioSpec spec;

    sdl_backend->error = 0;

    if (SDL_WasInit(SDL_INIT_AUDIO))
    {
        DebugMessage(M64MSG_VERBOSE, "sdl_init_audio_device(): SDL Audio sub-system already initialized.");
        SDL_DestroyAudioStream(sdl_backend->stream);
    }
    else
    {
        if (!SDL_Init(SDL_INIT_AUDIO))
        {
            DebugMessage(M64MSG_ERROR, "Failed to initialize SDL audio subsystem.");
            sdl_backend->error = 1;
            return;
        }
    }

    DebugMessage(M64MSG_INFO, "Initializing SDL audio subsystem...");

    memset(&spec, 0, sizeof(spec));
    spec.freq     = sdl_backend->frequency;
    spec.format   = SDL_AUDIO_S16LE;
    spec.channels = 2;

    /* Open the audio device */
    sdl_backend->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (sdl_backend->stream == NULL)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't open audio stream: %s", SDL_GetError());
        sdl_backend->error = 1;
        return;
    }

    DebugMessage(M64MSG_VERBOSE, "Frequency: %i", spec.freq);
    DebugMessage(M64MSG_VERBOSE, "Format: " AFMT_FMTSPEC, AFMT_ARGS(spec.format));
    DebugMessage(M64MSG_VERBOSE, "Channels: %i", spec.channels);

    /* start audio stream */
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(sdl_backend->stream));
}

static void release_audio_device(struct sdl_backend* sdl_backend)
{
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_DestroyAudioStream(sdl_backend->stream);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

struct sdl_backend* init_sdl_backend(void)
{
    /* allocate memory for sdl_backend */
    struct sdl_backend* sdl_backend = (struct sdl_backend*)malloc(sizeof(*sdl_backend));
    if (sdl_backend == nullptr) {
        return nullptr;
    }

    /* reset sdl_backend */
    memset(sdl_backend, 0, sizeof(*sdl_backend));

    /* instanciate resampler */
    std::string resampler_id = CoreSettingsGetStringValue(SettingsID::Audio_Resampler);
    void* resampler = nullptr;
    const struct resampler_interface* iresampler = get_iresampler(resampler_id.c_str(), &resampler);
    if (iresampler == nullptr) {
        free(sdl_backend);
        return nullptr;
    }

    sdl_backend->frequency = CoreSettingsGetIntValue(SettingsID::Audio_DefaultFrequency);
    sdl_backend->swap_channels = CoreSettingsGetBoolValue(SettingsID::Audio_SwapChannels);
    sdl_backend->speed_factor = 100;
    sdl_backend->resampler = resampler;
    sdl_backend->iresampler = iresampler;

    sdl_init_audio_device(sdl_backend);

    return sdl_backend;
}

void sdl_apply_settings(struct sdl_backend* sdl_backend)
{
    sdl_backend->frequency = CoreSettingsGetIntValue(SettingsID::Audio_DefaultFrequency);
    sdl_backend->swap_channels = CoreSettingsGetBoolValue(SettingsID::Audio_SwapChannels);
}

void release_sdl_backend(struct sdl_backend* sdl_backend)
{
    if (sdl_backend == nullptr) {
        return;
    }

    if (sdl_backend->error == 0) {
        release_audio_device(sdl_backend);
    }

    /* release primary buffer */
    if (sdl_backend->primary_buffer != nullptr) {
        free(sdl_backend->primary_buffer);
    }

    /* release mix buffer */
    if (sdl_backend->resample_buffer != nullptr) {
        free(sdl_backend->resample_buffer);
    }

    /* release resampler */
    sdl_backend->iresampler->release(sdl_backend->resampler);

    /* release sdl backend */
    free(sdl_backend);
}

void sdl_set_frequency(struct sdl_backend* sdl_backend, unsigned int frequency)
{
    if (sdl_backend->error != 0)
        return;

    sdl_backend->frequency = frequency;
    sdl_init_audio_device(sdl_backend);
}

void sdl_push_samples(struct sdl_backend* sdl_backend, const void* src, size_t size)
{
    if (sdl_backend->error != 0)
        return;

    // taken from https://github.com/gopher64/gopher64/blob/f3271cba63571d4d42c84c4c2db891af125b6f8d/src/ui/audio.rs
    double audioQueued = (double)SDL_GetAudioStreamQueued(sdl_backend->stream);
    double acceptableLatency = ((double)sdl_backend->frequency * 0.2) * 4.0;
    if (audioQueued >= acceptableLatency)
    {
        return;
    }

    /* truncate to full samples */
    if (size & 0x3) {
        DebugMessage(M64MSG_VERBOSE, "sdl_push_samples: pushing non full samples: %zu bytes !", size);
    }
    size = (size / 4) * 4;

    /* resize buffers when required */
    if (size > sdl_backend->buffers_size)
    {
        sdl_backend->primary_buffer = realloc(sdl_backend->primary_buffer, size);
        sdl_backend->resample_buffer = realloc(sdl_backend->resample_buffer, size);

        sdl_backend->buffers_size = size;
    }

    /* Confusing logic but, for LittleEndian host using memcpy will result in swapped channels,
     * whereas the other branch will result in non-swapped channels.
     * For BigEndian host this logic is inverted, memcpy will result in non swapped channels
     * and the other branch will result in swapped channels.
     *
     * This is due to the fact that the core stores 32bit words in native order in RDRAM.
     * For instance N64 bytes "Lh Ll Rh Rl" will be stored as "Rl Rh Ll Lh" on LittleEndian host
     * and therefore should the non-memcpy path to get non swapped channels,
     * whereas on BigEndian host the bytes will be stored as "Lh Ll Rh Rl" and therefore
     * memcpy path results in the non-swapped channels outcome.
     */
    if (sdl_backend->swap_channels ^ (SDL_BYTEORDER == SDL_BIG_ENDIAN)) {
        memcpy(sdl_backend->primary_buffer, src, size);
    }
    else {
        size_t i;
        for (i = 0 ; i < size ; i += 4 )
        {
            memcpy((unsigned char*)sdl_backend->primary_buffer + i + 0, (const unsigned char*)src + i + 2, 2); /* Left */
            memcpy((unsigned char*)sdl_backend->primary_buffer + i + 2, (const unsigned char*)src + i + 0, 2); /* Right */
        }
    }

    /* resample audio */
    sdl_backend->iresampler->resample(sdl_backend->resampler, 
                                        sdl_backend->primary_buffer, size, 
                                        sdl_backend->frequency, 
                                        sdl_backend->resample_buffer, size,
                                        sdl_backend->frequency);

    /* push audio buffer to SDL */
    SDL_PutAudioStreamData(sdl_backend->stream, sdl_backend->resample_buffer, size);
}

void sdl_set_speed_factor(struct sdl_backend* sdl_backend, unsigned int speed_factor)
{
    if (speed_factor < 10 || speed_factor > 300)
        return;

    sdl_backend->speed_factor = speed_factor;
}

void sdl_apply_volume(struct sdl_backend* sdl_backend, float vol)
{
    if (sdl_backend->stream != nullptr)
    {
        SDL_SetAudioStreamGain(sdl_backend->stream, vol);
    }
}
