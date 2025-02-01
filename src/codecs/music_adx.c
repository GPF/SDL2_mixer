/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*
  This file includes contributions from LibADX for KallistiOS
  LibADX for KallistiOS 
  Copyright (C) 2011-2013 Josh 'PH3NOM' Pearson
  Copyright (C) 2024 The KOS Team and contributors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef MUSIC_ADX
/* This file supports streaming Dreamcast ADX files */

#include "SDL.h"
#include "SDL_mixer.h"
#include "music.h"

#include <kos.h>

#define ADX_CRI_SIZE 0x06
#define ADX_PAD_SIZE 0x0e
#define ADX_HDR_SIZE 0x2c
#define ADX_HDR_SIG  0x80
#define ADX_EXIT_SIG 0x8001

#define ADX_ADDR_START      0x02
#define ADX_ADDR_CHUNK      0x05
#define ADX_ADDR_CHAN       0x07
#define ADX_ADDR_RATE       0x08
#define ADX_ADDR_SAMP       0x0c
#define ADX_ADDR_TYPE       0x12
#define ADX_ADDR_LOOP       0x18
#define ADX_ADDR_SAMP_START 0x1c
#define ADX_ADDR_BYTE_START 0x20
#define ADX_ADDR_SAMP_END   0x24
#define ADX_ADDR_BYTE_END   0x28

#define PCM_BUF_SIZE 1024*1024 /* Adjust based on your needs */
#define BASEVOL      0x4000

typedef struct {
    int sample_offset;              
    int chunk_size;
    int channels;
    int rate;
    int samples;
    int loop_type;
    int loop;
    int loop_start;
    int loop_end;
    int loop_samp_start;
    int loop_samp_end;
    int loop_samples;
} ADX_INFO;

typedef struct {
    int s1, s2;
} PREV;

typedef struct {
    SDL_RWops *src;
    int freesrc;
    SDL_bool playing;
    SDL_bool paused;
    int loop;

    // ADX specific
    unsigned char adx_buffer[1024]; // Size should match ADX chunk size * channels
    short pcm_buffer[2048]; // Size should be enough for decoded PCM data from one chunk
    int pcm_size;
    int pcm_samples;
    
    int sample_rate;  // <-- Add this
    int channels;     // <-- Add this

    PREV prev[2];
    ADX_INFO adx_info;
} ADX_Music;


// Helper functions for reading big-endian numbers
static int read_be16(unsigned char *buf)
{
    return (buf[0]<<8)|buf[1];
}

static long read_be32(unsigned char *buf)
{
    return (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
}

/* Parse ADX file header */
static int adx_parse(ADX_Music *adx_music, unsigned char *buf)
{
    if (SDL_RWseek(adx_music->src, 0, RW_SEEK_SET) == -1) return -1;
    if (SDL_RWread(adx_music->src, buf, 1, ADX_HDR_SIZE) != ADX_HDR_SIZE) return -1;
    if (buf[0] != ADX_HDR_SIG) return -1;  // Check signature

    adx_music->adx_info.sample_offset = read_be16(buf + ADX_ADDR_START) - 2;
    adx_music->adx_info.chunk_size    = buf[ADX_ADDR_CHUNK];
    adx_music->adx_info.channels      = buf[ADX_ADDR_CHAN];
    adx_music->adx_info.rate          = read_be32(buf + ADX_ADDR_RATE);
    adx_music->adx_info.samples       = read_be32(buf + ADX_ADDR_SAMP);
    adx_music->adx_info.loop_type     = buf[ADX_ADDR_TYPE];

    if (adx_music->adx_info.loop_type == 3) {
        adx_music->adx_info.loop = read_be32(buf + ADX_ADDR_LOOP);
        adx_music->adx_info.loop_samp_start = read_be32(buf + ADX_ADDR_SAMP_START);
        adx_music->adx_info.loop_start      = read_be32(buf + ADX_ADDR_BYTE_START);
        adx_music->adx_info.loop_samp_end   = read_be32(buf + ADX_ADDR_SAMP_END);
        adx_music->adx_info.loop_end        = read_be32(buf + ADX_ADDR_BYTE_END);
    } else if (adx_music->adx_info.loop_type == 4) {
        adx_music->adx_info.loop = read_be32(buf + ADX_ADDR_LOOP + 0x0c);
        adx_music->adx_info.loop_samp_start = read_be32(buf + ADX_ADDR_SAMP_START + 0x0c);
        adx_music->adx_info.loop_start      = read_be32(buf + ADX_ADDR_BYTE_START + 0x0c);
        adx_music->adx_info.loop_samp_end   = read_be32(buf + ADX_ADDR_SAMP_END + 0x0c);
        adx_music->adx_info.loop_end        = read_be32(buf + ADX_ADDR_BYTE_END + 0x0c);
    }
    if (adx_music->adx_info.loop > 1 || adx_music->adx_info.loop < 0) {
        adx_music->adx_info.loop = 0;
    }
    if (adx_music->adx_info.loop) {
        adx_music->adx_info.loop_samples = adx_music->adx_info.loop_samp_end - adx_music->adx_info.loop_samp_start;
    }

    // Check for CRI identifier
    unsigned char cri[ADX_CRI_SIZE];
    if (SDL_RWseek(adx_music->src, adx_music->adx_info.sample_offset, RW_SEEK_SET) == -1) return -1;
    if (SDL_RWread(adx_music->src, cri, 1, ADX_CRI_SIZE) != ADX_CRI_SIZE) return -1;
    if (memcmp(cri, "(c)CRI", ADX_CRI_SIZE) != 0) {
        return -1;  // Invalid ADX header
    }

    // Debug output for audio information
    printf("ADX Header Information:\n");
    printf("  Sample Offset: %d\n", adx_music->adx_info.sample_offset);
    printf("  Chunk Size: %d\n", adx_music->adx_info.chunk_size);
    printf("  Channels: %d\n", adx_music->adx_info.channels);
    printf("  Sample Rate: %d Hz\n", adx_music->adx_info.rate);
    printf("  Total Samples: %d\n", adx_music->adx_info.samples);
    printf("  Loop Type: %d\n", adx_music->adx_info.loop_type);
    if (adx_music->adx_info.loop) {
        printf("  Loop Start Sample: %d\n", adx_music->adx_info.loop_samp_start);
        printf("  Loop End Sample: %d\n", adx_music->adx_info.loop_samp_end);
        printf("  Loop Samples: %d\n", adx_music->adx_info.loop_samples);
    }

    // Store ADX audio properties in `adx_music`
    adx_music->sample_rate = adx_music->adx_info.rate;
    adx_music->channels = adx_music->adx_info.channels;

    return 1;
}

/* Convert ADX samples to PCM samples */
static void adx_to_pcm(short *out, unsigned char *in, PREV *prev)
{
    int scale = ((in[0]<<8)|(in[1]));
    int i;
    int s0,s1,s2,d;

    in+=2;
    s1 = prev->s1;
    s2 = prev->s2;
    for(i=0;i<16;i++) {
        d = in[i]>>4;
        if (d&8) d-=16;
        s0 = (BASEVOL*d*scale + 0x7298*s1 - 0x3350*s2)>>14;
        s0 = (s0 > 32767) ? 32767 : ((s0 < -32768) ? -32768 : s0);
        *out++ = s0;
        s2 = s1;
        s1 = s0;

        d = in[i]&15;
        if (d&8) d-=16;
        s0 = (BASEVOL*d*scale + 0x7298*s1 - 0x3350*s2)>>14;
        s0 = (s0 > 32767) ? 32767 : ((s0 < -32768) ? -32768 : s0);
        *out++ = s0;
        s2 = s1;
        s1 = s0;
    }
    prev->s1 = s1;
    prev->s2 = s2;
}

/* Function prototypes */
static void *ADX_CreateFromRW(SDL_RWops *src, int freesrc);
static void ADX_Delete(void *context);
static int ADX_Play(void *context, int play_count);
static int ADX_Seek(void *context, double position);
static double ADX_Tell(void *context);
static double ADX_Duration(void *context);
static void ADX_Stop(void *context);
static int ADX_GetAudio(void *context, void *data, int bytes);
static void ADX_SetVolume(void *context, int volume);
static void ADX_Pause(void *context);
static void ADX_Resume(void *context);

/* ADX Music Interface */
Mix_MusicInterface Mix_MusicInterface_ADX = {
    "ADX",
    MIX_MUSIC_ADX,
    MUS_ADX,
    SDL_FALSE,
    SDL_FALSE,
    
    NULL,  // Load - not used for ADX
    NULL,  // Open - not used for ADX
    ADX_CreateFromRW,
    NULL,  // CreateFromFile - not implemented for ADX
    ADX_SetVolume,
    NULL,  // GetVolume - not implemented
    ADX_Play,
    NULL,  // IsPlaying - not implemented
    ADX_GetAudio,
    NULL,  // Jump - not applicable for ADX
    ADX_Seek,  // Seek - not applicable for ADX
    ADX_Tell,  // Tell - not applicable for ADX
    ADX_Duration,  // Duration - not implemented
    NULL,  // LoopStart - not applicable for ADX
    NULL,  // LoopEnd - not applicable for ADX
    NULL,  // LoopLength - not applicable for ADX
    NULL,  // GetMetaTag - not implemented
    NULL,  // GetNumTracks - not implemented
    NULL,  // StartTrack - not implemented
    ADX_Pause,
    ADX_Resume,
    ADX_Stop,
    ADX_Delete,
    NULL,  // Close - not implemented
    NULL   // Unload - not implemented
};

static void *ADX_CreateFromRW(SDL_RWops *src, int freesrc)
{
    ADX_Music *adx_music = (ADX_Music *)SDL_calloc(1, sizeof(*adx_music));
    if (!adx_music) {
        Mix_OutOfMemory();
        return NULL;
    }
    adx_music->src = src;
    adx_music->freesrc = freesrc;
    adx_music->playing = SDL_FALSE;
    adx_music->paused = SDL_FALSE;
    adx_music->loop = 0;
    adx_music->pcm_size = 0;
    unsigned char header[ADX_HDR_SIZE];

    if (adx_parse(adx_music, header) < 1) {
        Mix_SetError("Failed to parse ADX file");
        goto cleanup;
    }

    // Store ADX audio properties in `adx_music`
    adx_music->sample_rate = adx_music->adx_info.rate;
    adx_music->channels = adx_music->adx_info.channels;
    
    int freq, channels;
    Uint16 format;
    Mix_QuerySpec(&freq, &format, &channels);
    printf("SDL_mixer expects: %d Hz, %d channels\n", freq, channels);
    printf("ADX file reports: %d Hz, %d channels\n", adx_music->sample_rate, adx_music->channels);

    // Reset file position to start of ADX data
    SDL_RWseek(src, adx_music->adx_info.sample_offset + ADX_CRI_SIZE, RW_SEEK_SET);

    // // Debugging output to confirm the extracted values
    // printf("ADX Loaded: Sample Rate = %d Hz, Channels = %d\n", 
    //     adx_music->sample_rate, adx_music->channels);

    return adx_music;

cleanup:
    if (adx_music) {
        SDL_free(adx_music);
    }
    return NULL;
}

static void ADX_Delete(void *context)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    if (adx_music->freesrc && adx_music->src) {
        SDL_RWclose(adx_music->src);
    }
    SDL_free(adx_music);
}

static int ADX_Play(void *context, int play_count)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    adx_music->loop = (play_count == -1) ? 1 : 0; // 1 for looping, 0 for play once
    adx_music->playing = SDL_TRUE;
    adx_music->paused = SDL_FALSE;
    adx_music->pcm_samples = adx_music->adx_info.samples;
    return 0;
}

static void ADX_Stop(void *context)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    adx_music->playing = SDL_FALSE;
    adx_music->paused = SDL_FALSE;
}

static int ADX_GetAudio(void *context, void *data, int bytes)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    int bytes_filled = 0;

    // SDL_Log("ADX_GetAudio: Requesting %d bytes", bytes);

    if (!adx_music->playing || adx_music->paused) {
        SDL_memset(data, 0, bytes);
        SDL_Log("ADX_GetAudio: Music stopped or paused, filling with silence");
        return 0;
    }

    while (bytes_filled < bytes) {
        if (adx_music->pcm_size == 0) {
            int read_size = adx_music->adx_info.chunk_size * adx_music->adx_info.channels;
            unsigned char adx_buf[read_size]; 

            if (SDL_RWread(adx_music->src, adx_buf, 1, read_size) != read_size) {
                if (adx_music->loop) {
                    // Handle looping
                    if (adx_music->adx_info.loop) {
                        SDL_RWseek(adx_music->src, adx_music->adx_info.loop_start, RW_SEEK_SET);
                        adx_music->pcm_samples = adx_music->adx_info.loop_samples;
                        SDL_Log("ADX_GetAudio: Loop back to start at %d", adx_music->adx_info.loop_start);
                    } else {
                        SDL_RWseek(adx_music->src, adx_music->adx_info.sample_offset + ADX_CRI_SIZE, RW_SEEK_SET);
                        adx_music->pcm_samples = adx_music->adx_info.samples;
                        SDL_Log("ADX_GetAudio: Loop back to sample offset");
                    }
                } else {
                    adx_music->playing = SDL_FALSE;
                    SDL_memset((Uint8*)data + bytes_filled, 0, bytes - bytes_filled);
                    SDL_Log("ADX_GetAudio: End of stream reached, stopping playback");
                    return bytes_filled;
                }
            } else {
                // SDL_Log("ADX_GetAudio: Decoding new ADX chunk of size %d", read_size);

                // Decode ADX data to PCM
                int wsize = (adx_music->pcm_samples > 32) ? 32 : adx_music->pcm_samples;
                if (adx_music->adx_info.channels == 1) {
                    // Mono decoding
                    short outbuf[wsize * 2];
                    adx_to_pcm(outbuf, adx_buf, &adx_music->prev[0]);
                    memcpy(adx_music->pcm_buffer + adx_music->pcm_size, outbuf, wsize * 2);
                    adx_music->pcm_size += wsize * 2;
                    adx_music->pcm_samples -= wsize;
                    // SDL_Log("ADX_GetAudio: Mono decoding done, PCM size %d", adx_music->pcm_size);
                } else if (adx_music->adx_info.channels == 2) {
                    // Stereo decoding
                    short tmpbuf[32 * 2], outbuf[32 * 2];
                    adx_to_pcm(tmpbuf, adx_buf, &adx_music->prev[0]);
                    adx_to_pcm(tmpbuf + 32, adx_buf + adx_music->adx_info.chunk_size, &adx_music->prev[1]);

                    for (int i = 0; i < wsize; i++) {
                        outbuf[i * 2] = tmpbuf[i];
                        outbuf[i * 2 + 1] = tmpbuf[i + 32];
                    }

                    memcpy(adx_music->pcm_buffer + adx_music->pcm_size, outbuf, wsize * 4); // 2 channels
                    adx_music->pcm_size += wsize * 4;
                    adx_music->pcm_samples -= wsize;
                    // SDL_Log("ADX_GetAudio: Stereo decoding done, PCM size %d", adx_music->pcm_size);
                }
            }
        }

        // Copy decoded PCM data to the output buffer
        int copy_size = SDL_min(bytes - bytes_filled, adx_music->pcm_size);
        SDL_memcpy((Uint8*)data + bytes_filled, adx_music->pcm_buffer, copy_size);
        bytes_filled += copy_size;
        // SDL_Log("ADX_GetAudio: Copied %d bytes to output buffer", copy_size);

        // Shift remaining PCM data to the start of the buffer
        if (adx_music->pcm_size > copy_size) {
            SDL_memmove(adx_music->pcm_buffer, (Uint8*)adx_music->pcm_buffer + copy_size, adx_music->pcm_size - copy_size);
        }
        adx_music->pcm_size -= copy_size;
    }

    return 0;
}

static int ADX_Seek(void *context, double position)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    // Convert time to bytes. This is a simplified approach, adjust based on your specific ADX format:
    int bytes_per_second = adx_music->adx_info.rate * adx_music->adx_info.channels * 2; // Assuming 16-bit samples
    Sint64 byte_position = (Sint64)(position * bytes_per_second) + adx_music->adx_info.sample_offset + ADX_CRI_SIZE;
    
    if (SDL_RWseek(adx_music->src, byte_position, RW_SEEK_SET) == -1) {
        return -1; // Error seeking
    }
    
    // Reset decoding state if necessary
    adx_music->prev[0].s1 = adx_music->prev[0].s2 = 0;
    if (adx_music->adx_info.channels == 2) {
        adx_music->prev[1].s1 = adx_music->prev[1].s2 = 0;
    }
    adx_music->pcm_size = 0; // Clear PCM buffer
    return 0; // Success
}

static double ADX_Tell(void *context)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    Sint64 current_position = SDL_RWtell(adx_music->src);
    
    if (current_position == -1) {
        return -1.0; // Error
    }
    
    // Convert bytes to seconds
    int bytes_per_second = adx_music->adx_info.rate * adx_music->adx_info.channels * 2;
    double position = (double)(current_position - adx_music->adx_info.sample_offset - ADX_CRI_SIZE) / bytes_per_second;
    return (position < 0) ? 0 : position; // Ensure we don't return negative time
}

static double ADX_Duration(void *context)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    return (double)adx_music->adx_info.samples / adx_music->adx_info.rate;
}

static void ADX_SetVolume(void *context, int volume)
{
    // Placeholder for volume control
    (void)context;
    (void)volume;
    // Implement volume control if available
}

static void ADX_Pause(void *context)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    adx_music->paused = SDL_TRUE;
    SDL_Log("Paused ADX playback");
}

static void ADX_Resume(void *context)
{
    ADX_Music *adx_music = (ADX_Music *)context;
    adx_music->paused = SDL_FALSE;
}

#endif /* MUSIC_ADX */

/* vi: set ts=4 sw=4 expandtab: */