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

#if HANDMADE_INTERNAL
// IMPORTANT: Not for a shipping game
struct debug_read_file_result {
    void *contents;
    uint32 contentsSize;
};

debug_read_file_result DEBUGPlatformReadEntireFile(char *filename);

void DEBUGPlatformFreeFileMemory(void *memory);

bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);
#endif

#if HANDMADE_SLOW
#define Assert(expression)  if(!(expression)) { *(int *)0 = 0; }
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
// TODO: Swap, min, max, macros

inline uint32 safeTruncateUInt64(uint64 value) {
    Assert(value < 0xFFFFFFFF);
    uint32 result = (uint32) value;
    return result;
}

// ###### Services that the platform layer provides to the game ######

// TODO: Add something here


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
    bool32 isConnected;
    bool32 isAnalog;

    real32 stickAverageX;
    real32 stickAverageY;

    union {
        game_button_state buttons[12];

        struct {
            game_button_state moveUp;
            game_button_state moveDown;
            game_button_state moveLeft;
            game_button_state moveRight;

            game_button_state actionUp;
            game_button_state actionDown;
            game_button_state actionLeft;
            game_button_state actionRight;

            game_button_state leftShoulder;
            game_button_state rightShoulder;

            game_button_state start;
            game_button_state back;


            // DO NOT WRITE BELOW
            game_button_state terminator;
        };
    };
};

struct game_input {
    // TODO: Insert clock value here
    game_controller_input controllers[5];
};

internal game_controller_input *getController(game_input *input, int controllerIndex) {
    Assert(controllerIndex < ArrayCount(input->controllers));
    return &input->controllers[controllerIndex];
}

struct game_memory {
    bool32 isInitialised;
    uint64 permanentStorageSize;
    void *permanentStorage; // Required to be cleared to 0 at startup

    uint64 transientStorageSize;
    void *transientStorage;
};

// Required: controller/keyboard input, bitmap buffer, sound buffer, timing
internal void gameUpdateAndRender(game_memory *memory, game_input *input, game_offscreen_buffer *buffer,
                                  game_sound_output_buffer *soundBuffer);


// NOT PLATFORM LAYER, MOVE EVENTUALLY

struct game_state {
    int blueOffset;
    int greenOffset;
    int toneHz;
};

#define HANDMADE_H
#endif
