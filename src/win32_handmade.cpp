// TODO: Implement sine
#include <cmath>
#include <cstdint>

#include "game/common.h"

#include <windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <cstdio>

#include "platform/win32_handmade.h"

// TODO: Global for now
global_variable bool running;
global_variable win32_offscreen_buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
global_variable int64 globalPerfCountFrequency;
global_variable bool32 globalPause;

// XInputGetState support
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)

typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState support
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)

typedef DIRECT_SOUND_CREATE(direct_sound_create);

void catString(size_t sourceACount, char *sourceA, size_t sourceBCount, char *sourceB, size_t destCount, char *dest) {
    for (int index = 0; index < sourceACount; ++index) {
        *dest++ = *sourceA++;
    }

    for (int index = 0; index < sourceBCount; ++index) {
        *dest++ = *sourceB++;
    }

    *dest++ = 0;
}

internal void win32GetEXEFileName(win32_state *state) {\
    DWORD sizeOfFilename = GetModuleFileNameA(0, state->exeFileName, sizeof(state->exeFileName));
    state->onePastLastEXEFileNameSlash = state->exeFileName;
    for (char *scan = state->exeFileName; *scan; scan++) {
        if (*scan == '\\') {
            state->onePastLastEXEFileNameSlash = scan + 1;
        }
    }
}

internal int stringLength(char *string) {
    int count = 0;
    while (*string++) {
        ++count;
    }
    return count;
}

internal void win32BuildEXEPathFileName(win32_state *state, char *fileName, int destCount, char *dest) {
    catString(state->onePastLastEXEFileNameSlash - state->exeFileName, state->exeFileName,
              stringLength(fileName), fileName,
              destCount, dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory) {
    VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile) {
    bool32 result = false;

    HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0)) {
            // File write successfully
            result = bytesWritten == memorySize;
        } else {
            // Log
        }

        CloseHandle(fileHandle);
    } else {
        // Log
    }


    return result;
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile) {
    debug_read_file_result result = {};

    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize)) {
            // TODO: Defines for max values
            uint32 fileSize32 = safeTruncateUInt64(fileSize.QuadPart);
            result.contents = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result.contents) {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead)) {
                    // File read successfully
                    result.contentsSize = fileSize32;
                } else {
                    DEBUGPlatformFreeFileMemory(result.contents);
                    result.contents = 0;
                }
            } else {
                // Logging
            }
        } else {
            // Logging
        }

        CloseHandle(fileHandle);
    } else {
        // Logging
    }

    return result;
}

inline FILETIME win32GetLastWriteTime(char *fileName) {
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(fileName, GetFileExInfoStandard, &data)) {
        lastWriteTime = data.ftLastWriteTime;
    }

    return lastWriteTime;
}

internal win32_game_code win32LoadGameCode(char *sourceFilePath, char *tempFilePath) {
    win32_game_code result = {};

    result.DLLLastWriteTime = win32GetLastWriteTime(sourceFilePath);

    CopyFile(sourceFilePath, tempFilePath, FALSE);
    result.gameCodeDLL = LoadLibraryA(tempFilePath);
    if (result.gameCodeDLL) {
        result.getSoundSamples = (game_get_sound_samples *) GetProcAddress(result.gameCodeDLL, "gameGetSoundSamples");
        result.getUpdateAndRender = (game_update_and_render *) GetProcAddress(
            result.gameCodeDLL, "gameUpdateAndRender");

        result.isValid = result.getSoundSamples && result.getUpdateAndRender;
    }

    if (!result.isValid) {
        result.getUpdateAndRender = 0;
        result.getSoundSamples = 0;
    }

    return result;
}

internal void win32UnloadGameCode(win32_game_code *gameCode) {
    if (gameCode->gameCodeDLL) {
        FreeLibrary(gameCode->gameCodeDLL);
        gameCode->gameCodeDLL = 0;
    }
    gameCode->isValid = false;
    gameCode->getSoundSamples = 0;
    gameCode->getUpdateAndRender = 0;
}

internal void win32LoadXInput() {
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!xInputLibrary) {
        HMODULE xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (!xInputLibrary) {
        HMODULE xInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if (xInputLibrary) {
        XInputGetState = (x_input_get_state *) GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *) GetProcAddress(xInputLibrary, "xInputSetState");
    }
}

internal void win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize) {
    // Load the lib
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

    if (dSoundLibrary) {
        direct_sound_create *directSoundCreate = (direct_sound_create *) GetProcAddress(
            dSoundLibrary, "DirectSoundCreate");

        // Get a DirectSound object
        LPDIRECTSOUND directSound;
        if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0))) {
            WAVEFORMATEX waveFormat = {};

            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.nBlockAlign = (WORD) (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // Create a primary buffer
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) {
                    HRESULT error = primaryBuffer->SetFormat(&waveFormat);
                    if (SUCCEEDED(error)) {
                        // Buffer format set
                        OutputDebugStringA("Primary buffer format was set.\n");
                    } else {
                        // Diagnostic
                    }
                } else {
                    // Diagnostic
                }
            } else {
                // Diagnostic
            }

            // Create a secondary buffer
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0);
            if (SUCCEEDED(error)) {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
        } else {
            // Diagnostic
        }
    } else {
        // DIganostic
    }
}

internal win32_window_dimension win32GetWindowDimesion(HWND window) {
    RECT clientRect;
    GetClientRect(window, &clientRect);

    win32_window_dimension result;
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;

    return result;
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height) {
    // Bulletproof this, don't free first, free after

    if (buffer->bitmapMemory) {
        VirtualFree(buffer->bitmapMemory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    int bytesPerPixel = 4;
    buffer->bytesPerPixel = bytesPerPixel;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // Top-Down buffer, growing down
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (buffer->width * buffer->height) * bytesPerPixel;
    buffer->bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = width * bytesPerPixel;

    // TODO: Probably clear to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *buffer, HDC deviceContext, int windowWidth,
                                         int windowHeight) {
    // TODO: Correct aspect ratio

    StretchDIBits(
        deviceContext,
        0, 0, buffer->width, buffer->height,
        0, 0, buffer->width, buffer->height,
        buffer->bitmapMemory,
        &buffer->info,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE: {
        }
        break;

        case WM_DESTROY: {
            // TODO: Recreate window
            running = false;
        }
        break;

        case WM_CLOSE: {
            // TODO: Handle this with a message to the user
            running = false;
        }
        break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        }
        break;

        case WM_ACTIVATEAPP: {
            // Set the window transparent when inactive
#if 0
            SetLayeredWindowAttributes(window, RGB(0, 0, 0), wParam ? 255 : 64, LWA_ALPHA);
#endif
        }
        break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);

            win32_window_dimension dimension = win32GetWindowDimesion(window);

            Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);

            EndPaint(window, &paint);
        }
        break;

        default: {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;
    }

    return result;
}

internal void win32ClearBuffer(win32_sound_output *soundOutput) {
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;
    if (SUCCEEDED(
        globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize, &region1, &region1Size, &region2, &region2Size,
            0))) {
        uint8 *destSample = (uint8 *) region1;

        for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
            *destSample++ = 0;
        }
        destSample = (uint8 *) region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
            *destSample++ = 0;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD bytesToLock, DWORD bytesToWrite,
                                   game_sound_output_buffer *sourceBuffer) {
    // More test
    // Switch to sin wave
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    if (SUCCEEDED(
        globalSecondaryBuffer->Lock(bytesToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0))) {
        // assert that region1/2Size is valid
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        int16 *destSample = (int16 *) region1;
        int16 *sourceSample = sourceBuffer->samples;

        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;

            ++soundOutput->runningSampleIndex;
        }

        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        destSample = (int16 *) region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;

            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void win32ProcessKeyboardMessage(game_button_state *newState, bool32 isDown) {
    Assert(newState->endedDown != isDown);
    newState->endedDown = isDown;
    ++newState->halfTransitionCount;
}

internal void win32ProcessXInputDigitalButton(DWORD xInputButtonState, game_button_state *newState,
                                              game_button_state *oldState, DWORD buttonBit) {
    newState->endedDown = (xInputButtonState & buttonBit) == buttonBit;
    newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void win32GetInputFileLocation(win32_state *state, int slotIndex, int destCount, char *dest) {
    Assert(slotIndex == 1);
    win32BuildEXEPathFileName(state, "loop_edit.hmi", destCount, dest);
}

internal void win32BeginRecordingInput(win32_state *state, int inputRecordingIndex) {
    state->inputRecordingIndex = inputRecordingIndex;

    char filename[WIN32_STATE_FILE_NAME_COUNT];
    win32GetInputFileLocation(state, inputRecordingIndex, sizeof(filename), filename);

    state->recordingHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    DWORD bytesToWrite = (DWORD) state->totalSize;
    Assert(state->totalSize == bytesToWrite);
    DWORD bytesWritten;
    WriteFile(state->recordingHandle, state->gameMemoryBlock, bytesToWrite, &bytesWritten, 0);
}

internal void win32EndRecordingInput(win32_state *state) {
    CloseHandle(state->recordingHandle);
    state->inputRecordingIndex = 0;
}

internal void win32BeginInputPlayback(win32_state *state, int inputPlayingIndex) {
    state->inputPlayingIndex = inputPlayingIndex;

    char filename[WIN32_STATE_FILE_NAME_COUNT];
    win32GetInputFileLocation(state, inputPlayingIndex, sizeof(filename), filename);

    state->playbackHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD bytesToRead = (DWORD) state->totalSize;
    Assert(state->totalSize == bytesToRead);
    DWORD bytesRead;
    ReadFile(state->playbackHandle, state->gameMemoryBlock, bytesToRead, &bytesRead, 0);
}

internal void win32EndInputPlayback(win32_state *state) {
    CloseHandle(state->playbackHandle);
    state->inputPlayingIndex = 0;
}

internal void win32RecordInput(win32_state *state, game_input *input) {
    DWORD bytesWritten;
    WriteFile(state->recordingHandle, input, sizeof(*input), &bytesWritten, 0);
}

internal void win32PlayBackInput(win32_state *state, game_input *input) {
    DWORD bytesRead = 0;
    if (ReadFile(state->playbackHandle, input, sizeof(*input), &bytesRead, 0)) {
        if (bytesRead == 0) {
            // Go back to the beginning
            int playingIndex = state->inputPlayingIndex;
            win32EndInputPlayback(state);
            win32BeginInputPlayback(state, playingIndex);
            ReadFile(state->playbackHandle, input, sizeof(*input), &bytesRead, 0);
        }
    }
}

internal void win32ProcessPendingMessages(win32_state *state, game_controller_input *keyboardController) {
    MSG message;
    // Fetching and handling all Windows events for the window
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            running = false;
        }

        switch (message.message) {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                uint32 vKCode = (uint32) message.wParam;
                bool wasDown = (message.lParam & (1 << 30)) != 0;
                bool isDown = (message.lParam & (1 << 31)) == 0;

                // Eat repeated presses
                if (isDown != wasDown) {
                    if (vKCode == 'W') {
                        win32ProcessKeyboardMessage(
                            &keyboardController->moveUp, isDown);
                    } else if (vKCode == 'A') {
                        win32ProcessKeyboardMessage(
                            &keyboardController->moveLeft, isDown);
                    } else if (vKCode == 'S') {
                        win32ProcessKeyboardMessage(
                            &keyboardController->moveDown, isDown);
                    } else if (vKCode == 'D') {
                        win32ProcessKeyboardMessage(
                            &keyboardController->moveRight, isDown);
                    } else if (vKCode == 'Q') {
                        win32ProcessKeyboardMessage(
                            &keyboardController->rightShoulder, isDown);
                        win32ProcessKeyboardMessage(
                            &keyboardController->leftShoulder, isDown);
                    } else if (vKCode == 'E') {
                        win32ProcessKeyboardMessage(
                            &keyboardController->rightShoulder, isDown);
                    } else if (vKCode == VK_UP) {
                        win32ProcessKeyboardMessage(&keyboardController->actionUp, isDown
                        );
                    } else if (vKCode == VK_DOWN) {
                        win32ProcessKeyboardMessage(&keyboardController->actionDown,
                                                    isDown);
                    } else if (vKCode == VK_LEFT) {
                        win32ProcessKeyboardMessage(&keyboardController->actionLeft,
                                                    isDown);
                    } else if (vKCode == VK_RIGHT) {
                        win32ProcessKeyboardMessage(&keyboardController->actionRight,
                                                    isDown);
                    } else if (vKCode == VK_ESCAPE) {
                        win32ProcessKeyboardMessage(&keyboardController->start,
                                                    isDown);
                    } else if (vKCode == VK_SPACE) {
                        win32ProcessKeyboardMessage(&keyboardController->back,
                                                    isDown);
#if HANDMADE_INTERNAL
                    } else if (vKCode == 'L' && isDown) {
                        if (state->inputRecordingIndex == 0) {
                            win32BeginRecordingInput(state, 1);
                        } else {
                            win32EndRecordingInput(state);
                            win32BeginInputPlayback(state, 1);
                        }
                    } else if (vKCode == 'P' && isDown) {
                        globalPause = !globalPause;
#endif
                    }


                    bool32 altKeyWasDown = (message.lParam & (1 << 29));
                    if ((vKCode == VK_F4) && altKeyWasDown) {
                        running = false;
                    }
                }
            }
            break;
            default: {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
            break;
        }
    }
}

// Returns normalized value between -1 and 1 for the axis, accounting for deadzone
internal real32 win32ProcessXInputStickValue(SHORT stickValue, SHORT deadzone) {
    real32 result = 0;
    if (stickValue < -deadzone) {
        result = ((real32) stickValue + deadzone) / (32768.0f - deadzone);
    } else if (stickValue > deadzone) {
        result = ((real32) stickValue - deadzone) / (32767.0f - deadzone);
    }
    return result;
}

inline LARGE_INTEGER win32getWallClock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);

    return result;
}

inline real32 win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    real32 result = (((real32) (end.QuadPart - start.QuadPart)) / (real32) globalPerfCountFrequency);
    return result;
}

internal void win32DebugDrawVertical(win32_offscreen_buffer *backBuffer, int x, int top, int bottom,
                                     uint32 color) {
    if (top <= 0) {
        top = 0;
    }

    if (bottom > backBuffer->height) {
        bottom = backBuffer->height;
    }

    if (x >= 0 && x < backBuffer->width) {
        uint8 *pixel = (uint8 *) backBuffer->bitmapMemory + x * backBuffer->bytesPerPixel + top *
                       backBuffer
                       ->pitch;
        for (int y = top; y < bottom; y++) {
            *(uint32 *) pixel = color;
            pixel += backBuffer->pitch;
        }
    }
}

internal void win32DrawSoundBufferMarker(win32_offscreen_buffer *backBuffer,
                                         win32_sound_output *soundOutput, real32 c, int padX, int top, int bottom,
                                         DWORD value, uint32 color) {
    real32 xReal32 = c * (real32) value;
    int x = padX + (int) xReal32;
    win32DebugDrawVertical(backBuffer, x, top, bottom, color);
}

internal void win32DebugSyncDisplay(win32_offscreen_buffer *backBuffer, int markerCount,
                                    win32_debug_time_marker *markers,
                                    int currentMarkerIndex,
                                    win32_sound_output *soundOutput, real32 targetSecondsPerFrame) {
    int padX = 16;
    int padY = 16;

    int lineHeight = 64;

    real32 c = (real32) (backBuffer->width - 2 * padX) / (real32) soundOutput->secondaryBufferSize;
    for (int markerIndex = 0; markerIndex < (markerCount); ++markerIndex) {
        win32_debug_time_marker *thisMarker = &markers[markerIndex];
        Assert(thisMarker->outputPlayCursor < soundOutput->secondaryBufferSize);
        Assert(thisMarker->outputWriteCursor < soundOutput->secondaryBufferSize);
        Assert(thisMarker->outputLocation < soundOutput->secondaryBufferSize);
        Assert(thisMarker->flipPlayCursor < soundOutput->secondaryBufferSize);
        Assert(thisMarker->flipWriteCursor < soundOutput->secondaryBufferSize);


        DWORD playColor = 0xFFFFFFFF;
        DWORD writeColor = 0xFFFF0000;
        DWORD expectedFlipColor = 0xFFFFFF00;
        DWORD playWindow = 0xFFFF00FF;


        int top = padY;
        int bottom = padY + lineHeight;
        if (markerIndex == currentMarkerIndex) {
            top += lineHeight + padY;
            bottom += lineHeight + padY;

            int firstTop = top;

            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thisMarker->outputPlayCursor,
                                       playColor);
            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thisMarker->outputWriteCursor,
                                       writeColor);


            top += lineHeight + padY;
            bottom += lineHeight + padY;

            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thisMarker->outputLocation,
                                       playColor);
            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom,
                                       thisMarker->outputLocation + thisMarker->outputByteCount,
                                       writeColor);

            top += lineHeight + padY;
            bottom += lineHeight + padY;

            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, firstTop, bottom,
                                       thisMarker->expectedFlipCursor,
                                       expectedFlipColor);
        }

        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thisMarker->flipPlayCursor,
                                   playColor);
        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom,
                                   thisMarker->flipPlayCursor + 480 * soundOutput->bytesPerSample,
                                   playWindow);
        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thisMarker->flipWriteCursor,
                                   writeColor);
    }
}

internal DWORD getDistanceBetweenCursors(DWORD forward, DWORD back, int bufferLength) {
    if (forward < back) {
        return (bufferLength - back) + forward;
    }
    return forward - back;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE preInstance,
    LPSTR commandLine,
    int showCode) {
    win32_state win32State = {};

    win32GetEXEFileName(&win32State);

    char sourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    win32BuildEXEPathFileName(&win32State, "libhandmade.dll", sizeof(sourceGameCodeDLLFullPath),
                              sourceGameCodeDLLFullPath);

    char tempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    win32BuildEXEPathFileName(&win32State, "handmade_temp.dll", sizeof(tempGameCodeDLLFullPath),
                              tempGameCodeDLLFullPath);


    // Frequency to translate performance counts to time
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    globalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

    // OS scheduler granularity for more precise sleep
    UINT desiredSchedulerMs = 1;
    bool32 sleepIsGranular = timeBeginPeriod(desiredSchedulerMs) == TIMERR_NOERROR;

    // XInput dll loading
    win32LoadXInput();
    // Window class init
    WNDCLASSA WindowClass = {};

    // Allocate the buffer for image
    Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback; // handler for Windows events
    WindowClass.hInstance = instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

# define monitorRefreshHz 60
# define gameUpdateHz  (monitorRefreshHz / 2)
    real32 targetSecondsPerFrame = 1.0f / (real32) gameUpdateHz;

    // TODO: Maybe think about non-frame locked audio output
    /// TODO: Use write cursor delta from the play cursor to adjust audio latency

    // Window registered
    if (RegisterClassA(&WindowClass)) {
        // Window created
        HWND window = CreateWindowExA(
            0, // WS_EX_TOPMOST | WS_EX_LAYERED,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);

        if (window) {
            running = true;

            // Sound test vars
            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.bytesPerSample = sizeof(int16) * 2; // Stereo 2ch
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = 3 * (soundOutput.samplesPerSecond / gameUpdateHz);
            soundOutput.safetySampleBytes = (soundOutput.samplesPerSecond * soundOutput.bytesPerSample) / gameUpdateHz /
                                            3;

            // 2 frames ahead

            // Sound buffer init
            win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            win32ClearBuffer(&soundOutput);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16 *samples = (int16 *) VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT,
                                                    PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID) Terabytes(2);
#else
            LPVOID baseAddress = 0;
#endif

            game_memory gameMemory = {};
            gameMemory.permanentStorageSize = Megabytes(64);
            gameMemory.transientStorageSize = Gigabytes(1);
            gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            win32State.totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
            win32State.gameMemoryBlock = VirtualAlloc(baseAddress, (size_t) win32State.totalSize,
                                                      MEM_RESERVE | MEM_COMMIT,
                                                      PAGE_READWRITE);
            gameMemory.permanentStorage = win32State.gameMemoryBlock;
            gameMemory.transientStorage = (uint8 *) gameMemory.permanentStorage + gameMemory.permanentStorageSize;

            if (samples && gameMemory.permanentStorage && gameMemory.transientStorage) {
                game_input input[2] = {};
                game_input *newInput = &input[0];
                game_input *oldInput = &input[1];

                // Counter for timings
                LARGE_INTEGER lastCounter = win32getWallClock();
                LARGE_INTEGER flipWallClock = win32getWallClock();

                int debugTimeMarkerIndex = 0;
                win32_debug_time_marker debugTimeMarkers[gameUpdateHz / 2] = {};

                bool32 soundIsValid = false;
                DWORD audioLatencyBytes = 0;
                real32 audioLatencySeconds = 0;


                uint64 lastCycleCount = __rdtsc();

                win32_game_code gameCode = win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);

                while (running) {
                    FILETIME newDLLWriteTime = win32GetLastWriteTime(sourceGameCodeDLLFullPath);
                    if (CompareFileTime(&newDLLWriteTime, &gameCode.DLLLastWriteTime) > 0) {
                        win32UnloadGameCode(&gameCode);
                        gameCode = win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);
                    }

                    game_controller_input *oldKeyboardController = getController(oldInput, 0);
                    game_controller_input *newKeyboardController = getController(newInput, 0);
                    // TODO: Zero macro
                    // TODO: Zeroing messes up the up/down state
                    *newKeyboardController = {};
                    newKeyboardController->isConnected = true;

                    for (int buttonIndex = 0; buttonIndex < ArrayCount(newKeyboardController->buttons); buttonIndex++) {
                        newKeyboardController->buttons[buttonIndex].endedDown = oldKeyboardController->buttons[
                            buttonIndex].endedDown;
                    }

                    win32ProcessPendingMessages(&win32State, newKeyboardController);


                    // Controller input handling
                    // TODO: Stop polling disconnected controllers to avoid XInput frame rate hit
                    // TODO: Should we pull it more frequently?
                    DWORD maxControllerCount = XUSER_MAX_COUNT;
                    if (maxControllerCount > (ArrayCount(newInput->controllers) - 1)) {
                        maxControllerCount = (ArrayCount(newInput->controllers) - 1);
                    }

                    for (DWORD controllerIndex = 0; controllerIndex < maxControllerCount; controllerIndex++) {
                        DWORD ourControllerIndex = controllerIndex + 1;
                        game_controller_input *oldController = getController(oldInput, ourControllerIndex);
                        game_controller_input *newController = getController(newInput, ourControllerIndex);

                        XINPUT_STATE controllerState;
                        if (XInputGetState_(controllerIndex, &controllerState) == ERROR_SUCCESS) {
                            // Controller plugged in
                            XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
                            newController->isConnected = true;
                            newController->isAnalog = oldController->isAnalog;

                            // TODO: Square deadzone, check if it's returned as round
                            newController->stickAverageX = win32ProcessXInputStickValue(
                                pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            newController->stickAverageY = win32ProcessXInputStickValue(
                                pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                            if (newController->stickAverageY || newController->stickAverageX) {
                                newController->isAnalog = true;
                            }

                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
                                newController->stickAverageY = 1.0f;
                                newController->isAnalog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
                                newController->stickAverageY = -1.0f;
                                newController->isAnalog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
                                newController->stickAverageX = -1.0f;
                                newController->isAnalog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
                                newController->stickAverageX = 1.0f;
                                newController->isAnalog = false;
                            }

                            // Fake mapping of sticks to buttons
                            real32 threshold = 0.5f;
                            win32ProcessXInputDigitalButton(((newController->stickAverageX < -threshold) ? 1 : 0),
                                                            &oldController->moveLeft,
                                                            &newController->moveLeft, 1);
                            win32ProcessXInputDigitalButton(((newController->stickAverageX > threshold) ? 1 : 0),
                                                            &oldController->moveRight,
                                                            &newController->moveRight, 1);
                            win32ProcessXInputDigitalButton(((newController->stickAverageY < -threshold) ? 1 : 0),
                                                            &oldController->moveDown,
                                                            &newController->moveDown, 1);
                            win32ProcessXInputDigitalButton(((newController->stickAverageY > threshold) ? 1 : 0),
                                                            &oldController->moveUp,
                                                            &newController->moveUp, 1);


                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->actionLeft,
                                                            &newController->actionLeft,
                                                            XINPUT_GAMEPAD_X);
                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->actionUp,
                                                            &newController->actionUp,
                                                            XINPUT_GAMEPAD_Y);
                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->actionDown,
                                                            &newController->actionDown,
                                                            XINPUT_GAMEPAD_A);
                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->actionRight,
                                                            &newController->actionRight,
                                                            XINPUT_GAMEPAD_B);


                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->leftShoulder,
                                                            &newController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->rightShoulder,
                                                            &newController->rightShoulder,
                                                            XINPUT_GAMEPAD_RIGHT_SHOULDER);

                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->start,
                                                            &newController->start,
                                                            XINPUT_GAMEPAD_START);
                            win32ProcessXInputDigitalButton(pad->wButtons, &oldController->back,
                                                            &newController->back,
                                                            XINPUT_GAMEPAD_BACK);
                        } else {
                            // Controller not available
                            newController->isConnected = false;
                        }
                    }

                    if (globalPause) {
                        continue;
                    }

                    game_offscreen_buffer buffer = {};
                    buffer.memory = globalBackBuffer.bitmapMemory;
                    buffer.width = globalBackBuffer.width;
                    buffer.height = globalBackBuffer.height;
                    buffer.pitch = globalBackBuffer.pitch;
                    buffer.bytesPerPixel = globalBackBuffer.bytesPerPixel;

                    if (win32State.inputRecordingIndex) {
                        win32RecordInput(&win32State, newInput);
                    }

                    if (win32State.inputPlayingIndex) {
                        win32PlayBackInput(&win32State, newInput);
                    }

                    if (gameCode.getUpdateAndRender) {
                        gameCode.getUpdateAndRender(&gameMemory, newInput, &buffer);
                    }

                    LARGE_INTEGER audioWallClock = win32getWallClock();
                    real64 fromBeginToAudioSeconds = win32GetSecondsElapsed(flipWallClock, audioWallClock);

                    // DirectSound output test
                    DWORD playCursor;
                    DWORD writeCursor;
                    if ((globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)) == DS_OK) {
                        // Sound computation:
                        //
                        // When we wake up to write audio, we will check the position of play cursor
                        // and forecast where it will be on the next frame. Then we check if write cursor is before that.
                        //
                        // If Yes, the target fill position is frame boundary plus one frame. Perfect sync
                        // If no, we do not sync perfectly, and write one frame's worth of audio plus some guard samples to
                        // catch up to the write cursor
                        //
                        // CASE I:
                        // |----|----|----|----|
                        //  | | |====|
                        // PC WC  1F
                        //
                        // CASE II:
                        // |----|----|----|----|
                        //   |    |====|=|
                        //  PC   WC 1F  G
                        // Filling image buffer with test gradient
                        if (!soundIsValid) {
                            soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
                            soundIsValid = true;
                        }

                        DWORD bytesToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.
                                            secondaryBufferSize;

                        DWORD expectedBytesPerFrame =
                                (soundOutput.samplesPerSecond * soundOutput.bytesPerSample) / gameUpdateHz;
                        real32 secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);
                        DWORD expectedBytesUntilFlip = (DWORD) (
                            (secondsLeftUntilFlip / targetSecondsPerFrame) * (real32) expectedBytesPerFrame
                        );
                        DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;

                        DWORD safeWriteCursor = writeCursor;

                        if (safeWriteCursor < playCursor) {
                            safeWriteCursor += soundOutput.secondaryBufferSize;
                        }
                        Assert(safeWriteCursor >= playCursor);

                        safeWriteCursor += soundOutput.safetySampleBytes;
                        bool32 audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;

                        DWORD targetCursor = 0;
                        if (audioCardIsLowLatency) {
                            targetCursor = (expectedFrameBoundaryByte + expectedBytesPerFrame);
                        } else {
                            targetCursor = (writeCursor + expectedBytesPerFrame + soundOutput.safetySampleBytes);
                        }
                        targetCursor = targetCursor % soundOutput.secondaryBufferSize;

                        DWORD bytesToWrite = 0;
                        if (bytesToLock > targetCursor) {
                            bytesToWrite = soundOutput.secondaryBufferSize - bytesToLock;
                            bytesToWrite += targetCursor;
                        } else {
                            bytesToWrite = targetCursor - bytesToLock;
                        }

                        game_sound_output_buffer soundBuffer = {};
                        soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                        soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                        soundBuffer.samples = samples;

                        if (gameCode.getSoundSamples) {
                            gameCode.getSoundSamples(&gameMemory, &soundBuffer);
                        }
#if HANDMADE_INTERNAL
                        win32_debug_time_marker *marker = &debugTimeMarkers[debugTimeMarkerIndex];
                        marker->outputPlayCursor = playCursor;
                        marker->outputWriteCursor = writeCursor;
                        marker->outputLocation = bytesToLock;
                        marker->outputByteCount = bytesToWrite;
                        marker->expectedFlipCursor = expectedFrameBoundaryByte;

                        audioLatencyBytes = getDistanceBetweenCursors(writeCursor, playCursor,
                                                                      soundOutput.secondaryBufferSize);
                        audioLatencySeconds =
                                (real32) audioLatencyBytes / (real32) (soundOutput.bytesPerSample * soundOutput.
                                                                       samplesPerSecond);

                        char FPSbuffer[256];
                        snprintf(FPSbuffer, sizeof(FPSbuffer),
                                 "BTL:%d TC:%d BTW:%d -- PC:%u WC:%u DELTA:%u (%.2fms)\n",
                                 bytesToLock, targetCursor, bytesToWrite, playCursor, writeCursor,
                                 audioLatencyBytes, audioLatencySeconds * 1000.0);
                        OutputDebugStringA(FPSbuffer);
#endif
                        win32FillSoundBuffer(&soundOutput, bytesToLock, bytesToWrite, &soundBuffer);
                    } else {
                        soundIsValid = false;
                    }

                    LARGE_INTEGER workCounter = win32getWallClock();
                    real32 workSecondsElapsed = win32GetSecondsElapsed(lastCounter, workCounter);

                    // TODO: Not tested yet, prob buggy
                    real32 secondsElapsedForFrame = workSecondsElapsed;
                    if (secondsElapsedForFrame < targetSecondsPerFrame) {
                        if (sleepIsGranular) {
                            DWORD sleepMs = (DWORD) (1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
                            if (sleepMs > 0) {
                                // Arbitrary number not to overshoot
                                Sleep(sleepMs);
                            }
                        }
                        real32 testSecondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32getWallClock());
                        if (testSecondsElapsedForFrame < targetSecondsPerFrame) {
                            // LOG MISSED SLEEP TIMINGS
                        }

                        while (secondsElapsedForFrame < targetSecondsPerFrame) {
                            secondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32getWallClock());
                        }
                    } else {
                        // Missed frame rate
                        // TODO: Logging
                    }

                    LARGE_INTEGER endCounter = win32getWallClock();
                    real64 msPerFrame = 1000.0f * win32GetSecondsElapsed(lastCounter, endCounter);
                    lastCounter = endCounter;

                    // Display frame AFTER sync delay
                    win32_window_dimension dimension = win32GetWindowDimesion(window);
#if HANDMADE_INTERNAL
                    win32DebugSyncDisplay(&globalBackBuffer, ArrayCount(debugTimeMarkers), debugTimeMarkers,
                                          debugTimeMarkerIndex - 1,
                                          &soundOutput, targetSecondsPerFrame);
#endif

                    HDC deviceContext = GetDC(window);
                    Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);
                    ReleaseDC(window, deviceContext);

                    flipWallClock = win32getWallClock();
#if HANDMADE_INTERNAL
                    // Sound debug code
                    {
                        DWORD playCursor;
                        DWORD writeCursor;
                        if (globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK) {
                            Assert(debugTimeMarkerIndex < ArrayCount(debugTimeMarkers));
                            win32_debug_time_marker *marker = &debugTimeMarkers[debugTimeMarkerIndex];
                            marker->flipPlayCursor = playCursor;
                            marker->flipWriteCursor = writeCursor;
                        }
                    }
#endif

                    game_input *temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;

                    uint64 endCycleCount = __rdtsc();
                    uint64 cyclesElapsed = endCycleCount - lastCycleCount;
                    lastCycleCount = endCycleCount;


                    real64 FPS = 1.0f / msPerFrame;
                    real64 MCPF = ((real64) cyclesElapsed / (1000.0f * 1000.0f));
#if HANDMADE_INTERNAL
                    char FPSbuffer[256];
                    snprintf(FPSbuffer, sizeof(FPSbuffer), "ms/frame: %.02f, FPS: %.02f, mcycles/frame: %.02f\n",
                             msPerFrame, FPS, MCPF);
                    OutputDebugStringA(FPSbuffer);

                    debugTimeMarkerIndex++;
                    if (debugTimeMarkerIndex == ArrayCount(debugTimeMarkers)) {
                        debugTimeMarkerIndex = 0;
                    }
#endif
                }
            } else {
                // Logging
            }
        } else {
            // TODO:
        }
    } else {
        // TODO:
    }

    return (0);
}
