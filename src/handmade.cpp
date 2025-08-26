#include <cmath>

#include "game/common.h"
#include "game/handmade.h"

internal void gameOutputSound(game_sound_output_buffer *soundBuffer, game_state *gameState) {
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->samplesPerSecond / gameState->toneHz;

    int16 *sampleOut = soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex) {
        real32 sineValue = sinf(gameState->tSine);

        int16 sampleValue = (int16) (sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        gameState->tSine += 2.0f * pi32 * 1.0f / (real32) wavePeriod;
        if (gameState->tSine > 2.0f * pi32) {
            gameState->tSine -= 2.0f * pi32;
        }
    }
}

internal void renderWeirdGradient(game_offscreen_buffer *buffer, int xOffset, int yOffset) {
    uint8 *row = (uint8 *) buffer->memory;
    for (int y = 0; y < buffer->height; ++y) {
        uint32 *pixel = (uint32 *) row;
        for (int x = 0; x < buffer->width; ++x) {
            uint8 blue = (uint8) (x + xOffset);
            uint8 green = (uint8) (y + yOffset);

            *pixel++ = (green << 8) | blue;
        }

        row += buffer->pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender) {
    Assert(
        (&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == ((ArrayCount(input->controllers[0].
                buttons))
        ));
    Assert(sizeof(game_state) <= memory->permanentStorageSize);


    game_state *gameState = (game_state *) memory->permanentStorage;
    if (!memory->isInitialised) {
        char *filename = __FILE__;

        debug_read_file_result file = memory->DEBUGPlatformReadEntireFile(filename);
        if (file.contents) {
            memory->DEBUGPlatformWriteEntireFile("test.out", file.contentsSize,
                                                 file.contents);
            memory->DEBUGPlatformFreeFileMemory(memory);
        }

        gameState->toneHz = 256;
        gameState->tSine = 0.0f;

        // TODO: This may be more appropriate to do in the platform layer
        memory->isInitialised = true;
    }

    // TODO: Change for input.controllers array count, getting some weird input
    for (int controllerIndex = 0; controllerIndex < ArrayCount(input->controllers); ++controllerIndex) {
        game_controller_input *controllerInput = getController(input, controllerIndex);
        if (controllerInput->isAnalog) {
            // use anaglog movement
            gameState->toneHz = 256 + (int) (128.0f * (controllerInput->stickAverageY));
            gameState->blueOffset += (int) (4.0f * (controllerInput->stickAverageX));
        } else {
            // use digital movement
            if (controllerInput->moveLeft.endedDown) {
                gameState->blueOffset -= 1;
            }
            if (controllerInput->moveRight.endedDown) {
                gameState->blueOffset += 1;
            }
        }

        if (controllerInput->actionDown.endedDown) {
            gameState->greenOffset += 1;
        }
    }

    // TODO: Allow sample offsets here for more robust platform options
    renderWeirdGradient(buffer, gameState->blueOffset, gameState->greenOffset);
}

extern "C" GAME_GET_SOUND_SAMPLES(gameGetSoundSamples) {
    game_state *gameState = (game_state *) memory->permanentStorage;
    gameOutputSound(soundBuffer, gameState);
}

