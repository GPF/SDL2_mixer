#include "SDL.h"
#include "SDL_mixer.h"
#include <stdio.h>
#include <stdlib.h>
#include <kos.h> 
#define ADX_FILE "/rd/gs-16b-2c-44100hz.adx" //44100 2channels 16bit
// #define ADX_FILE "/rd/starwars60.adx"   //22050 1channel 16bit 
#define PAN_SWITCH_INTERVAL 10000  // 10 seconds in milliseconds

// Define this to switch between music and chunk playback
// #define PLAY_AS_MUSIC

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

#ifdef PLAY_AS_MUSIC
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

    // Optional: Gradually increase volume
    // for (int vol = 0; vol <= 128; vol += 5) {
    //     printf("Setting volume to %d\n", vol);
    //     Mix_VolumeMusic(vol);
    //     SDL_Delay(1000);
    // }

    printf("Playing %s as music... Press Enter to stop.\n", ADX_FILE);

    getchar();

    Mix_HaltMusic();
    Mix_FreeMusic(music);

#else
    Mix_Chunk* chunk = Mix_LoadWAV(ADX_FILE);
    if (!chunk) {
        fprintf(stderr, "Failed to load ADX file: %s\n", Mix_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    int channel = Mix_PlayChannel(-1, chunk, -1); // Loop indefinitely
    if (channel == -1) {
        fprintf(stderr, "Mix_PlayChannel failed: %s\n", Mix_GetError());
        Mix_FreeChunk(chunk);
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    printf("Playing %s as a chunk... Press Enter to stop. Panning will switch every 10 seconds.\n", ADX_FILE);

    // Panning control (only for chunks, music doesn't support panning in the same way)
    // Uint32 start_time = SDL_GetTicks();
    // int current_pan = 128; // Start with center panning
    // int pan_direction = -1; // -1 for moving left, 1 for moving right

    // while (true) {
    //     Uint32 current_time = SDL_GetTicks();
        
    //     if (current_time - start_time >= PAN_SWITCH_INTERVAL) {
    //         // Switch panning direction
    //         current_pan += pan_direction * 256; // 256 because we want to go from 0 to 255
    //         if (current_pan <= 0) {
    //             current_pan = 0;
    //             pan_direction = 1; // Switch to moving right
    //         } else if (current_pan >= 255) {
    //             current_pan = 255;
    //             pan_direction = -1; // Switch to moving left
    //         }
            
    //         Mix_SetPanning(channel, current_pan, 255 - current_pan);
    //         printf("Pan set to: %d\n", current_pan);
    //         start_time = current_time; // Reset the timer
    //     }

    //     SDL_Delay(100); // Small delay to prevent busy-waiting

    //     if (SDL_PollEvent(NULL) && SDL_QuitRequested()) {
    //         break;
    //     }
    // }
    getchar();
    Mix_HaltChannel(channel);
    Mix_FreeChunk(chunk);

#endif

    Mix_CloseAudio();
    Mix_Quit(); // Don't forget to quit the mixer subsystem
    SDL_Quit();
    return 0;
}