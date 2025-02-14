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

#include <SDL.h>
#include <SDL_audio.h>
#include <stdlib.h>
#include <string.h>

#include "Resamplers/resamplers.hpp"
#include "circular_buffer.hpp"
#include "main.hpp"

#include <RMG-Core/m64p/api/m64p_types.h>

#include <RMG-Core/Settings.hpp>
#include <RMG-Core/Netplay.hpp>

/* number of bytes per sample */
#define N64_SAMPLE_BYTES 4
#define SDL_SAMPLE_BYTES 4

#define SDL_LockAudio() SDL_LockAudioDevice(sdl_backend->device)
#define SDL_UnlockAudio() SDL_UnlockAudioDevice(sdl_backend->device)
#define SDL_PauseAudio(A) SDL_PauseAudioDevice(sdl_backend->device, A)
#define SDL_CloseAudio() SDL_CloseAudioDevice(sdl_backend->device)
#define SDL_OpenAudio(A, B) ((sdl_backend->device = SDL_OpenAudioDevice(nullptr, 0, A, B, 0)) - 1)
struct sdl_backend
{
    /* Audio Device */
    SDL_AudioDeviceID device;

    /* Primary Buffer */
    struct circular_buffer primary_buffer;

    /* Primary buffer size (in output samples) */
    size_t primary_buffer_size;

    /* Primary buffer fullness target (in output samples) */
    size_t target;

    /* Secondary buffer size (in output samples) */
    size_t secondary_buffer_size;

    /* Mixing buffer used for volume control */
    unsigned char* mix_buffer;

    unsigned int last_cb_time;
    unsigned int input_frequency;
    unsigned int output_frequency;
    unsigned int speed_factor;

    unsigned int swap_channels;

    unsigned int audio_sync;

    unsigned int paused_for_sync;

    unsigned int underrun_count;

    unsigned int error;

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


static void my_audio_callback(void* userdata, unsigned char* stream, int len)
{
    struct sdl_backend* sdl_backend = (struct sdl_backend*)userdata;

    /* mark the time, for synchronization on the input side */
    sdl_backend->last_cb_time = SDL_GetTicks();

    unsigned int newsamplerate = sdl_backend->output_frequency * 100 / sdl_backend->speed_factor;
    unsigned int oldsamplerate = sdl_backend->input_frequency;
    size_t needed = (len * oldsamplerate) / newsamplerate;
    size_t available;
    size_t consumed;

    const void* src = cbuff_tail(&sdl_backend->primary_buffer, &available);
    if ((available > 0) && (available >= needed))
    {
        consumed = ResampleAndMix(sdl_backend->resampler, sdl_backend->iresampler,
                sdl_backend->mix_buffer,
                src, available, oldsamplerate,
                stream, len, newsamplerate);

        consume_cbuff_data(&sdl_backend->primary_buffer, consumed);
    }
    else
    {
        ++sdl_backend->underrun_count;
        memset(stream, 0, len);
    }
}

static size_t new_primary_buffer_size(const struct sdl_backend* sdl_backend)
{
    return N64_SAMPLE_BYTES * ((uint64_t)sdl_backend->primary_buffer_size * sdl_backend->input_frequency * sdl_backend->speed_factor) /
        (sdl_backend->output_frequency * 100);
}

static void resize_primary_buffer(struct sdl_backend* sdl_backend, size_t new_size)
{
    /* only grows the buffer */
    if (new_size > sdl_backend->primary_buffer.size) {
        SDL_LockAudio();
        sdl_backend->primary_buffer.data = realloc(sdl_backend->primary_buffer.data, new_size);
        memset((unsigned char*)sdl_backend->primary_buffer.data + sdl_backend->primary_buffer.size, 0, new_size - sdl_backend->primary_buffer.size);
        sdl_backend->primary_buffer.size = new_size;
        SDL_UnlockAudio();
    }
}

static unsigned int select_output_frequency(unsigned int input_frequency)
{
    if (input_frequency <= 11025) { return 11025; }
    else if (input_frequency <= 22050) { return 22050; }
    else { return 44100; }
}

static void sdl_init_audio_device(struct sdl_backend* sdl_backend)
{
    SDL_AudioSpec desired, obtained;

    sdl_backend->error = 0;

    if (SDL_WasInit(SDL_INIT_AUDIO|SDL_INIT_TIMER) == (SDL_INIT_AUDIO|SDL_INIT_TIMER) )
    {
        DebugMessage(M64MSG_VERBOSE, "sdl_init_audio_device(): SDL Audio sub-system already initialized.");

        SDL_PauseAudio(1);
        SDL_CloseAudio();
    }
    else
    {
        if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
        {
            DebugMessage(M64MSG_ERROR, "Failed to initialize SDL audio subsystem.");
            sdl_backend->error = 1;
            return;
        }
    }

    sdl_backend->paused_for_sync = 1;

    /* reload these because they gets re-assigned from SDL data below, and sdl_init_audio_device can be called more than once */
    sdl_backend->primary_buffer_size = CoreSettingsGetIntValue(SettingsID::Audio_PrimaryBufferSize);
    sdl_backend->target = CoreSettingsGetIntValue(SettingsID::Audio_PrimaryBufferTarget);
    sdl_backend->secondary_buffer_size = CoreSettingsGetIntValue(SettingsID::Audio_SecondaryBufferSize);

    DebugMessage(M64MSG_INFO,    "Initializing SDL audio subsystem...");
    DebugMessage(M64MSG_VERBOSE, "Primary buffer: %i output samples.", (uint32_t) sdl_backend->primary_buffer_size);
    DebugMessage(M64MSG_VERBOSE, "Primary target fullness: %i output samples.", (uint32_t) sdl_backend->target);
    DebugMessage(M64MSG_VERBOSE, "Secondary buffer: %i output samples.", (uint32_t) sdl_backend->secondary_buffer_size);

    memset(&desired, 0, sizeof(desired));
    desired.freq = select_output_frequency(sdl_backend->input_frequency);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = sdl_backend->secondary_buffer_size;
    desired.callback = my_audio_callback;
    desired.userdata = sdl_backend;

    DebugMessage(M64MSG_VERBOSE, "Requesting frequency: %iHz.", desired.freq);
    DebugMessage(M64MSG_VERBOSE, "Requesting format: " AFMT_FMTSPEC ".", AFMT_ARGS(desired.format));

    /* Open the audio device */
    if (SDL_OpenAudio(&desired, &obtained) < 0)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't open audio: %s", SDL_GetError());
        sdl_backend->error = 1;
        return;
    }
    if (desired.format != obtained.format)
    {
        DebugMessage(M64MSG_WARNING, "Obtained audio format (" AFMT_FMTSPEC ") differs from requested (" AFMT_FMTSPEC ").", AFMT_ARGS(obtained.format), AFMT_ARGS(desired.format));
    }
    if (desired.freq != obtained.freq)
    {
        DebugMessage(M64MSG_WARNING, "Obtained frequency (%i) differs from requested (%i).", obtained.freq, desired.freq);
    }

    /* adjust some variables given the obtained audio spec */
    sdl_backend->output_frequency = obtained.freq;
    sdl_backend->secondary_buffer_size = obtained.samples;

    if (sdl_backend->target < sdl_backend->secondary_buffer_size)
        sdl_backend->target = sdl_backend->secondary_buffer_size;

    if (sdl_backend->primary_buffer_size < sdl_backend->target)
        sdl_backend->primary_buffer_size = sdl_backend->target;
    if (sdl_backend->primary_buffer_size < sdl_backend->secondary_buffer_size * 2)
        sdl_backend->primary_buffer_size = sdl_backend->secondary_buffer_size * 2;

    /* allocate memory for audio buffers */
    resize_primary_buffer(sdl_backend, new_primary_buffer_size(sdl_backend));
    sdl_backend->mix_buffer = (unsigned char*)realloc(sdl_backend->mix_buffer, sdl_backend->secondary_buffer_size * SDL_SAMPLE_BYTES);

    /* preset the last callback time */
    if (sdl_backend->last_cb_time == 0) {
        sdl_backend->last_cb_time = SDL_GetTicks();
    }

    DebugMessage(M64MSG_VERBOSE, "Frequency: %i", obtained.freq);
    DebugMessage(M64MSG_VERBOSE, "Format: " AFMT_FMTSPEC, AFMT_ARGS(obtained.format));
    DebugMessage(M64MSG_VERBOSE, "Channels: %i", obtained.channels);
    DebugMessage(M64MSG_VERBOSE, "Silence: %i", obtained.silence);
    DebugMessage(M64MSG_VERBOSE, "Samples: %i", obtained.samples);
    DebugMessage(M64MSG_VERBOSE, "Size: %i", obtained.size);
}

static void release_audio_device(struct sdl_backend* sdl_backend)
{
    if (SDL_WasInit(SDL_INIT_AUDIO) != 0) {
        SDL_PauseAudio(1);
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    if (SDL_WasInit(SDL_INIT_TIMER) != 0) {
        SDL_QuitSubSystem(SDL_INIT_TIMER);
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

    sdl_backend->input_frequency = CoreSettingsGetIntValue(SettingsID::Audio_DefaultFrequency);
    sdl_backend->swap_channels = CoreSettingsGetBoolValue(SettingsID::Audio_SwapChannels);
    sdl_backend->audio_sync = !CoreHasInitNetplay() && CoreSettingsGetBoolValue(SettingsID::Audio_Synchronize);
    sdl_backend->paused_for_sync = 1;
    sdl_backend->speed_factor = 100;
    sdl_backend->resampler = resampler;
    sdl_backend->iresampler = iresampler;

    sdl_init_audio_device(sdl_backend);

    return sdl_backend;
}

void sdl_apply_settings(struct sdl_backend* sdl_backend)
{
    sdl_backend->input_frequency = CoreSettingsGetIntValue(SettingsID::Audio_DefaultFrequency);
    sdl_backend->swap_channels = CoreSettingsGetBoolValue(SettingsID::Audio_SwapChannels);
    sdl_backend->audio_sync = CoreSettingsGetBoolValue(SettingsID::Audio_Synchronize);
    sdl_backend->primary_buffer_size = CoreSettingsGetIntValue(SettingsID::Audio_PrimaryBufferSize);
    sdl_backend->target = CoreSettingsGetIntValue(SettingsID::Audio_PrimaryBufferTarget);
    sdl_backend->secondary_buffer_size = CoreSettingsGetIntValue(SettingsID::Audio_SecondaryBufferSize);
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
    release_cbuff(&sdl_backend->primary_buffer);

    /* release mix buffer */
    free(sdl_backend->mix_buffer);

    /* release resampler */
    sdl_backend->iresampler->release(sdl_backend->resampler);

    /* release sdl backend */
    free(sdl_backend);
}

void sdl_set_frequency(struct sdl_backend* sdl_backend, unsigned int frequency)
{
    if (sdl_backend->error != 0)
        return;

    sdl_backend->input_frequency = frequency;
    sdl_init_audio_device(sdl_backend);
}


void sdl_push_samples(struct sdl_backend* sdl_backend, const void* src, size_t size)
{
    size_t available;

    if (sdl_backend->error != 0)
        return;

    /* truncate to full samples */
    if (size & 0x3) {
        DebugMessage(M64MSG_VERBOSE, "sdl_push_samples: pushing non full samples: %zu bytes !", size);
    }
    size = (size / 4) * 4;

    /* We need to lock audio before accessing cbuff */
    SDL_LockAudio();
    unsigned char* dst = (unsigned char*)cbuff_head(&sdl_backend->primary_buffer, &available);
    if (size <= available)
    {
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
            memcpy(dst, src, size);
        }
        else {
            size_t i;
            for (i = 0 ; i < size ; i += 4 )
            {
                memcpy(dst + i + 0, (const unsigned char*)src + i + 2, 2); /* Left */
                memcpy(dst + i + 2, (const unsigned char*)src + i + 0, 2); /* Right */
            }
        }

        produce_cbuff_data(&sdl_backend->primary_buffer, size);
    }
    SDL_UnlockAudio();

    if (size > available)
    {
        DebugMessage(M64MSG_VERBOSE, "sdl_push_samples: pushing %zu bytes, but only %zu available !", size, available);
    }
}


static size_t estimate_level_at_next_audio_cb(struct sdl_backend* sdl_backend)
{
    size_t available;
    unsigned int now = SDL_GetTicks();

    /* NOTE: given that we only access "available" counter from cbuff, we don't need to protect it's access with LockAudio/UnlockAudio */
    cbuff_tail(&sdl_backend->primary_buffer, &available);

    /* Start by calculating the current Primary buffer fullness in terms of output samples */
    size_t expected_level = (size_t)(((int64_t)(available/N64_SAMPLE_BYTES) * sdl_backend->output_frequency * 100) / (sdl_backend->input_frequency * sdl_backend->speed_factor));

    /* Next, extrapolate to the buffer level at the expected time of the next audio callback, assuming that the
       buffer is filled at the same rate as the output frequency */
    unsigned int expected_next_cb_time = sdl_backend->last_cb_time + ((1000 * sdl_backend->secondary_buffer_size) / sdl_backend->output_frequency);

    if (now < expected_next_cb_time) {
        expected_level += (expected_next_cb_time - now) * sdl_backend->output_frequency / 1000;
    }

    return expected_level;
}

void sdl_synchronize_audio(struct sdl_backend* sdl_backend)
{
    enum { TOLERANCE_MS = 10 };

    size_t expected_level = estimate_level_at_next_audio_cb(sdl_backend);

    /* If the expected value of the Primary Buffer Fullness at the time of the next audio callback is more than 10
       milliseconds ahead of our target buffer fullness level, then insert a delay now */
    if (sdl_backend->audio_sync && expected_level >= sdl_backend->target + sdl_backend->output_frequency * TOLERANCE_MS / 1000)
    {
        /* Core is ahead of SDL audio thread,
         * delay emulation to allow the SDL audio thread to catch up */
        unsigned int wait_time = (expected_level - sdl_backend->target) * 1000 / sdl_backend->output_frequency;

        if (sdl_backend->paused_for_sync) { SDL_PauseAudio(0); }
        sdl_backend->paused_for_sync = 0;

        SDL_Delay(wait_time);
    }
    else if (expected_level < sdl_backend->secondary_buffer_size)
    {
        /* Core is behind SDL audio thread (predicting an underflow),
         * pause the audio to let the Core catch up */
        if (!sdl_backend->paused_for_sync) { SDL_PauseAudio(1); }
        sdl_backend->paused_for_sync = 1;
    }
    else
    {
        /* Expected fullness is within tolerance,
         * audio thread is running */
        if (sdl_backend->paused_for_sync) { SDL_PauseAudio(0); }
        sdl_backend->paused_for_sync = 0;
    }
}

void sdl_set_speed_factor(struct sdl_backend* sdl_backend, unsigned int speed_factor)
{
    if (speed_factor < 10 || speed_factor > 300)
        return;

    sdl_backend->speed_factor = speed_factor;

    /* we need a different size primary buffer to store the N64 samples when the speed changes */
    resize_primary_buffer(sdl_backend, new_primary_buffer_size(sdl_backend));
}
