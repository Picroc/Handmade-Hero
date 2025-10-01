//
// Created by Alexey Logachev on 23.09.2025.
//

#include <math.h>

#include "../include/common.h"
#include "../include/handmade.h"

internal void render_weird_gradient(game_offscreen_buffer *buffer, int x_offset, int y_offset) {
    uint8* row = (uint8*)buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer->width; ++x) {
            uint8 blue = (x + x_offset);
            uint8 green = (y + y_offset);
            *pixel++ = (green << 8) | blue;
        }
        row += buffer->pitch;
    }
}

internal void game_output_sound(game_sound_output_buffer *sound_buffer, int tone_hz) {
    local_persist real32 t_sine;

    int16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second / tone_hz;

    int16 *sample_out = sound_buffer->samples;
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; ++sample_index) {
        real32 sine_value = sinf(t_sine);

        int16 sample_value = (int16)(sine_value * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        t_sine += 2.0f * pi32 *1.0f / (real32) wave_period;
    }
}

internal void game_update_and_render(game_input *input, game_offscreen_buffer *buffer, game_sound_output_buffer *sound_buffer) {
    local_persist int x_offset = 0;
    local_persist int y_offset = 0;
    local_persist int tone_hz = 256;

    game_controller_input *input_0 = &input->controllers[0];
    if (input_0->is_analog) {
        // use analog movement
        tone_hz = 256 + (int)(128.0f*(input_0->end_y));
        y_offset += (int)(4.0f*(input_0->end_x));
    } else {
        // use digital movement
    }

    if (input_0->down.ended_down) {
        x_offset += 1;
    }

    render_weird_gradient(buffer, x_offset, y_offset);
    game_output_sound(sound_buffer, tone_hz);
}