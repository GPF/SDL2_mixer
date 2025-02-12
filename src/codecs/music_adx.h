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

/* This file supports streaming Dreamcast ADX files */

#include "music.h"

/* Function prototypes */
void *ADX_CreateFromRW(SDL_RWops *src, int freesrc);
static void ADX_Delete(void *context);
static int ADX_Play(void *context, int play_count);
static int ADX_Seek(void *context, double position);
static double ADX_Tell(void *context);
static double ADX_Duration(void *context);
static void ADX_Stop(void *context);
static SDL_bool ADX_IsPlaying(void *context);
static int ADX_GetAudio(void *context, void *data, int bytes);
static void ADX_SetVolume(void *context, int volume);
static void ADX_Pause(void *context);
static void ADX_Resume(void *context);
// static Mix_Chunk* ADX_LoadFromFile(const char* file);

extern Mix_MusicInterface Mix_MusicInterface_ADX;

/* vi: set ts=4 sw=4 expandtab: */
