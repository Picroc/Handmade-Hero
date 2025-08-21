#include <cmath>

#include "game/common.h"
#include "game/handmade.h"

internal void gameOutputSound(game_sound_output_buffer *soundBuffer, int toneHz) {
    local_persist real32 tSine;
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->samplesPerSecond / toneHz;

    int16 *sampleOut = soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex) {
        real32 sineValue = sinf(tSine);

        int16 sampleValue = (int16) (sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        tSine += 2.0f * pi32 * 1.0f / (real32) wavePeriod;
    }
}

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset) {
    uint8 *row = (uint8 *) buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        uint32 *pixel = (uint32 *) row;
        for (int x = 0; x < buffer->width; ++x) {
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset);

            *pixel++ = (green << 8) | blue;
        }

        row += buffer->pitch;
    }
}

internal void gameUpdateAndRender(game_memory *memory, game_input *input, game_offscreen_buffer *buffer,
                                  game_sound_output_buffer *soundBuffer) {
    Assert(sizeof(game_state) <= memory->permanentStorageSize);

    game_state *gameState = (game_state *) memory->permanentStorage;
    if (!memory->isInitialised) {
        char *filename = __FILE__;

        debug_read_file_result file = DEBUGPlatformReadEntireFile(filename);
        if (file.contents) {
            DEBUGPlatformWriteEntireFile("test.out", file.contentsSize,
                                         file.contents);
            DEBUGPlatformFreeFileMemory(memory);
        }

        gameState->toneHz = 256;

        // TODO: This may be more appropriate to do in the platform layer
        memory->isInitialised = true;
    }

    game_controller_input *input0 = &input->controllers[0];
    if (input0->isAnalog) {
        // use anaglog movement
        gameState->toneHz = 256 + (int) (128.0f * (input0->endY));
        gameState->blueOffset += (int) 4.0f * (input0->endX);
    } else {
        // use digital movement
    }

    if (input0->down.endedDown) {
        gameState->greenOffset += 1;
    }

    // TODO: Allow sample offsets here for more robust platform options
    gameOutputSound(soundBuffer, gameState->toneHz);
    renderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}
