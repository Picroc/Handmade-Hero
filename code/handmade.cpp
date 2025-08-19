#include "handmade.h"

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset) {
    uint8* row = (uint8*)buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer->width; ++x) {
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset); 

            *pixel++ = (green << 8) | blue;
        }

        row += buffer->pitch;
    }
}

internal void gameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset) {
    renderWeirdGradient(buffer, blueOffset, greenOffset);
}