// TODO: Implement sine
#include <math.h>
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

#define pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

typedef int32 bool32;

#include "handmade.cpp"

#include <windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_handmade.h"


// TODO: Global for now
global_variable bool running;
global_variable win32_offscreen_buffer globalBackBuffer;    
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

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

internal void win32LoadXInput() {
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!xInputLibrary) {
        HMODULE xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (!xInputLibrary) {
        HMODULE xInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if (xInputLibrary) {
        XInputGetState = (x_input_get_state*) GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*) GetProcAddress(xInputLibrary, "xInputSetState");
    }
}

internal void win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize) {
    // Load the lib
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

    if (dSoundLibrary) {
        direct_sound_create *directSoundCreate = (direct_sound_create *)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        // Get a DirectSound object
        LPDIRECTSOUND directSound;
        if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0))) {
            WAVEFORMATEX waveFormat = {};

            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
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

    if(buffer->bitmapMemory) {
        VirtualFree(buffer->bitmapMemory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // Top-Down buffer, growing down
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;
    int bitmapMemorySize =  (buffer->width * buffer->height) * bytesPerPixel;
    buffer->bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = width*bytesPerPixel;

    // TODO: Probably clear to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *buffer, HDC deviceContext, int windowWidth, int windowHeight) {
    // TODO: Correct aspect ratio

    StretchDIBits(
        deviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer->width, buffer->height,
        buffer->bitmapMemory,
        &buffer->info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK Win32MainWindowCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE:
        {
        } break;
        
        case WM_DESTROY:
        {
            // TODO: Recreate window
            running = false;
        } break;

        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user
            running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 vKCode = wParam;
            bool wasDown = (lParam & (1 << 30)) != 0;
            bool isDown = (lParam & (1 << 31)) == 0;

            // Eat repeated presses
            if (isDown != wasDown) {
                if (vKCode == 'W') {

                } else if (vKCode == 'A') {

                } else if (vKCode == 'S') {
                    
                } else if (vKCode == 'D') {
                    
                } else if (vKCode == 'Q') {
                    
                } else if (vKCode == 'E') {
                    
                } else if (vKCode == VK_UP) {
                    
                } else if (vKCode == VK_DOWN) {
                    
                } else if (vKCode == VK_LEFT) {
                    
                } else if (vKCode == VK_RIGHT) {
                    
                } else if (vKCode == VK_ESCAPE) {
                    
                } else if (vKCode == VK_SPACE) {
                    
                }


                bool32 altKeyWasDown = (lParam & (1 << 29));
                if ((vKCode == VK_F4) && altKeyWasDown) {
                    running = false;
                }
            }
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);

            win32_window_dimension dimension = win32GetWindowDimesion(window);

            Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);

            EndPaint(window, &paint);
        } break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

internal void win32ClearBuffer(win32_sound_output *soundOutput) {
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;
    if (SUCCEEDED(globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize, &region1, &region1Size, &region2, &region2Size, 0))) {
        uint8 *destSample = (uint8 *)region1;

        for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
            *destSample++ = 0;
        }
        destSample = (uint8 *)region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
            *destSample++ = 0;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD bytesToLock, DWORD bytesToWrite, game_sound_output_buffer *sourceBuffer) {
    // More test
    // Switch to sin wave
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    if (SUCCEEDED(globalSecondaryBuffer->Lock(bytesToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0))) {
        // assert that region1/2Size is valid
        DWORD region1SampleCount = region1Size/soundOutput->bytesPerSample;
        int16 *destSample = (int16 *)region1;
        int16 *sourceSample = sourceBuffer->samples;

        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;

            ++soundOutput->runningSampleIndex;
        }
        
        DWORD region2SampleCount = region2Size/soundOutput->bytesPerSample;
        destSample = (int16 *)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;

            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void win32ProcessXInputDigitalButton(DWORD xInputButtonState, game_button_state *newState, game_button_state *oldState, DWORD buttonBit) {
    newState->endedDown = (xInputButtonState & buttonBit) == buttonBit;
    newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE preInstance,
    LPSTR commandLine,
    int showCode
) {
    // Frequency to translate performance counts to time
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    int64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

    // XInput dll loading
    win32LoadXInput();
    // Window class init
    WNDCLASSA WindowClass = {};

    // Allocate the buffer for image
    Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback; // handler for Windows events
    WindowClass.hInstance = instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

    // Window registered
    if (RegisterClassA(&WindowClass)) {
        // Window created
        HWND window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0
        );

        if (window) {
            // Locking device context, it's exclusive for our window
            // due to CS_OWNDC style flag set on WindowClass
            HDC deviceContext = GetDC(window);

            // Sound test vars
            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.bytesPerSample = sizeof(int16)*2; // Stereo 2ch
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15; // write to buffer 15 times per second, latency
            
            // Sound buffer init and filling with sine
            win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            win32ClearBuffer(&soundOutput);

            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            // Start playing
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            game_input input[2];
            game_input *newInput = &input[0];
            game_input *oldInput = &input[1];


            // Counter for timings
            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);

            uint64 lastCycleCount = __rdtsc();

            running = true;
            while (running) {
                MSG message;

                // Fetching and handling all Windows events for the window
                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                // Controller input handling
                // TODO: Should we pull it more frequently?
                int maxContollerCount = XUSER_MAX_COUNT;
                if (maxContollerCount > ArrayCount(newInput->controllers)) {
                    maxContollerCount = ArrayCount(newInput->controllers);
                }

                for (DWORD controllerIndex=0; controllerIndex < maxContollerCount; controllerIndex++) {
                    game_controller_input *oldController = &oldInput->controllers[controllerIndex];
                    game_controller_input *newController = &newInput->controllers[controllerIndex];

                    XINPUT_STATE controllerState;
                    if(XInputGetState_(controllerIndex, &controllerState) == ERROR_SUCCESS) {
                        // Controller plugged in
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        // TODO: DPad
                        bool up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

                        real32 x;
                        if (pad->sThumbLX < 0) {
                            x = (real32)pad->sThumbLX / 32768.0f;
                        } else {
                            x = (real32)pad->sThumbLX / 32767.0f;
                        }

                        real32 y;
                        if (pad->sThumbLY < 0) {
                            y = (real32)pad->sThumbLY / 32768.0f;
                        } else {
                            y = (real32)pad->sThumbLY / 32767.0f;
                        }

                        newController->isAnalog = true;
                        newController->startX = oldController->endX;
                        newController->startY = oldController->endY;

                        newController->minX = newController->maxX = newController->endX = x;
                        newController->minY = newController->maxY = newController->endY = y;

                        // TODO: Deadzone processing

                        win32ProcessXInputDigitalButton(pad->wButtons, &oldController->left, &newController->left, XINPUT_GAMEPAD_X);
                        win32ProcessXInputDigitalButton(pad->wButtons, &oldController->up, &newController->up, XINPUT_GAMEPAD_Y);
                        win32ProcessXInputDigitalButton(pad->wButtons, &oldController->down, &newController->down, XINPUT_GAMEPAD_A);
                        win32ProcessXInputDigitalButton(pad->wButtons, &oldController->right, &newController->right, XINPUT_GAMEPAD_B);
                        
                        // win32ProcessXInputDigitalButton(pad->wButtons, &oldController->state, &newController->state, XINPUT_GAMEPAD_START);
                        // win32ProcessXInputDigitalButton(pad->wButtons, &oldController->state, &newController->state, XINPUT_GAMEPAD_BACK);
                        win32ProcessXInputDigitalButton(pad->wButtons, &oldController->leftShoulder, &newController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
                        win32ProcessXInputDigitalButton(pad->wButtons, &oldController->rightShoulder, &newController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                    } else {
                        // Controller not available
                    }
                }

                // DirectSound output test
                DWORD bytesToLock;
                DWORD targetCursor;
                DWORD bytesToWrite;

                DWORD playCursor;
                DWORD writeCursor;
                bool32 soundIsValid = false;
                // Tighten up sound logic so we know where we should be writing to
                if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
                    bytesToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;

                    targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
                    if (bytesToLock > targetCursor) {
                        bytesToWrite = soundOutput.secondaryBufferSize - bytesToLock;
                        bytesToWrite += targetCursor;
                    } else {
                        bytesToWrite = targetCursor - bytesToLock;
                    }

                    soundIsValid = true;
                }

                game_sound_output_buffer soundBuffer = {};
                soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                soundBuffer.samples = samples;

                // Filling image buffer with test gradient
                game_offscreen_buffer buffer = {};
                buffer.memory = globalBackBuffer.bitmapMemory;
                buffer.width = globalBackBuffer.width;
                buffer.height = globalBackBuffer.height;
                buffer.pitch = globalBackBuffer.pitch;

                gameUpdateAndRender(newInput, &buffer, &soundBuffer);


                if (soundIsValid) {
                    win32FillSoundBuffer(&soundOutput, bytesToLock, bytesToWrite, &soundBuffer);
                }

                win32_window_dimension dimension = win32GetWindowDimesion(window);
                Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);

                uint64 endCycleCount = __rdtsc();

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);

                int64 cyclesElapsed = endCycleCount - lastCycleCount;
                int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                real64 msPerFrame = (real64)((1000.0f * (real64)counterElapsed) / (real64)perfCountFrequency);
                real64 FPS = (real64)perfCountFrequency / (real64)counterElapsed;
                real64 MCPF = ((real64)cyclesElapsed / (1000.0f * 1000.0f));
# if 0
                char buffer[256];
                sprintf(buffer, "ms/frame: %.02f, FPS: %.02f, mcycles/frame: %.02f\n", msPerFrame, FPS, MCPF);
                OutputDebugStringA(buffer);
# endif

                lastCounter = endCounter;
                lastCycleCount = endCycleCount;

                game_input *temp = newInput;
                newInput = oldInput;
                oldInput = temp;
            }
        } else {
            // TODO:
        }
    } else {
        // TODO:
    }

    return(0);
}