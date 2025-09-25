//
// Created by Alexey Logachev on 23.09.2025.
//

#ifndef HANDMADE_HERO_HANDMADE_H
#define HANDMADE_HERO_HANDMADE_H

struct game_offscreen_buffer {
    void *memory;
    int width;
    int height;
    int pitch;
};

struct game_sound_output_buffer {
    int samples_per_second;
    int sample_count;
    int16 *samples;
};

internal void game_update_and_render(game_offscreen_buffer *buffer, int x_offset, int y_offset, game_sound_output_buffer *sound_buffer, int tone_hz);
internal void game_output_sound(game_sound_output_buffer *sound_buffer);


#endif //HANDMADE_HERO_HANDMADE_H