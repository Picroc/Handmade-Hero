//
// Created by Alexey Logachev on 23.09.2025.
//

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

internal void game_update_and_render(game_offscreen_buffer *buffer, int x_offset, int y_offset) {
    render_weird_gradient(buffer, x_offset, y_offset);
}