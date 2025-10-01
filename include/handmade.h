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

struct game_button_state {
    int half_transition_count;
    bool ended_down;
};

struct game_controller_input {
    bool is_analog;

    real32 start_x;
    real32 start_y;

    real32 min_x;
    real32 min_y;

    real32 max_x;
    real32 max_y;

    real32 end_x;
    real32 end_y;

    union {
        game_button_state buttons[6];
        struct {
            game_button_state up;
            game_button_state down;
            game_button_state left;
            game_button_state right;
            game_button_state left_shoulder;
            game_button_state right_shoulder;
        };
    };
};

struct game_input {
    game_controller_input controllers[4];
};

internal void game_update_and_render(game_input *input, game_offscreen_buffer *buffer, game_sound_output_buffer *sound_buffer);
internal void game_output_sound(game_sound_output_buffer *sound_buffer);


#endif //HANDMADE_HERO_HANDMADE_H