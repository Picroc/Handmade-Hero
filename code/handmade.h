#if !defined(HANDMADE_H)

// ###### Services that the platform layer provides to the game ######

// TODO: Add


// ###### Services that the game provides to the platform layer ######

struct game_offscreen_buffer {
    // Pixels are 32-bit wide, mem order BB GG RR XX
    void *memory;

    int width;
    int height;

    int pitch;
};

// Required: controller/keyboard input, bitmap buffer, sound buffer, timing
internal void gameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset);


#define HANDMADE_H
#endif