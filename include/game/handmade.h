#if !defined(HANDMADE_H)

#include "game/common.h"

/*
 * HANDMADE_INTERNAL:
 * 0 - Build for public release
 * 1 - Build for developer only
 *
 * HANDMADE_SLOW
 * 0 - No slow code allowed
 * 1 - Slow code welcome
 */

#if HANDMADE_SLOW
#define Assert(expression)  if(!(expression)) { *(int *)0 = 0; }
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value)*1024)
#define Megabytes(value) (Kilobytes(value)*1024)
#define Gigabytes(value) (Megabytes(value)*1024)
#define Terabytes(value) (Gigabytes(value)*1024)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
// TODO: Swap, min, max, macros

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

struct game_button_state {
    int halfTransitionCount;
    bool32 endedDown;
};

struct game_controller_input {
    bool32 isAnalog;

    real32 startX;
    real32 startY;

    real32 minX;
    real32 minY;

    real32 maxX;
    real32 maxY;
    
    real32 endX;
    real32 endY;

    union {
        game_button_state buttons[6];
        struct {
            game_button_state up;
            game_button_state down;
            game_button_state left;
            game_button_state right;
            game_button_state leftShoulder;
            game_button_state rightShoulder;
        };
    };
};

struct game_input {
    // TODO: Insert clock value here
    game_controller_input controllers[4];
};

struct game_memory {
    bool32 isInitialised;
    uint64 permanentStorageSize;
    void *permanentStorage; // Required to be cleared to 0 at startup

    uint64 transientStorageSize;
    void *transientStorage;
};

// Required: controller/keyboard input, bitmap buffer, sound buffer, timing
internal void gameUpdateAndRender(game_memory *memory, game_input *input, game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer);


// NOT PLATFORM LAYER, MOVE EVENTUALLY

struct game_state {
    int blueOffset;
    int greenOffset;
    int toneHz;
};

#define HANDMADE_H
#endif