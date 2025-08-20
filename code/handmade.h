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

struct game_sound_output_buffer {
    int16 *samples;
    int sampleCount;
    int samplesPerSecond;
};

// Required: controller/keyboard input, bitmap buffer, sound buffer, timing
internal void gameUpdateAndRender(game_offscreen_buffer *buffer, int blueOffset, int greenOffset, game_sound_output_buffer *soundBuffer, int toneHz);


#define HANDMADE_H
#endif