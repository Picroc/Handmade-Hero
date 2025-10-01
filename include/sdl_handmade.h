//
// Created by Alexey Logachev on 01.10.2025.
//

#ifndef HANDMADE_HERO_SDL_HANDMADE_H
#define HANDMADE_HERO_SDL_HANDMADE_H
#include <SDL2/SDL_render.h>

#include "common.h"

struct sdl_offscreen_buffer {
    SDL_Texture *texture;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct sdl_window_dimension {
    int width;
    int height;
};

struct sdl_audio_ring_buffer {
    int size;
    int write_cursor;
    int play_cursor;
    void *data;
};

struct sdl_sound_output {
    int samples_per_second;
    int tone_hz;
    int16 tone_volume;
    int32 running_sample_index;
    int wave_period;
    int bytes_per_sample;
    int buffer_size;
    real32 sin_t;
    int latency_sample_count;
};

#endif //HANDMADE_HERO_SDL_HANDMADE_H