#include "SDL.h"
#include "SDL_mixer.h"
#include <stdio.h>
#include <stdlib.h>
#include <kos.h> 
#define ADX_FILE "/rd/gs-16b-2c-44100hz.adx" //44100 2channels 16bit
// #define ADX_FILE "/rd/starwars60.adx"   //22050 1channel 16bit 
// #define ADX_FILE "/rd/001_hazard.adx"   //11025 2channel 16bit 

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_mixer with ADX support for Dreamcast
    if (Mix_Init(MIX_INIT_ADX) != MIX_INIT_ADX) {
        fprintf(stderr, "Could not initialize mixer: %s\n", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 1024) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        SDL_Quit();
        return 1;
    }

    Mix_Music *music = Mix_LoadMUS(ADX_FILE);
    if (!music) {
        fprintf(stderr, "Failed to load ADX file: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    if (Mix_PlayMusic(music, -1) < 0) {
        fprintf(stderr, "Mix_PlayMusic failed: %s\n", Mix_GetError());
        Mix_FreeMusic(music);
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    printf("Playing %s... Press Enter to stop.\n", ADX_FILE);
    getchar();

    Mix_HaltMusic();
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_Quit();
    return 0;
}
