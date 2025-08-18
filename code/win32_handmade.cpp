#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

// TODO: Implement sine
#include <math.h>

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

struct win32_offscreen_buffer {
    BITMAPINFO info;

    void *bitmapMemory;

    int width;
    int height;

    int pitch;
};

struct win32_window_dimension {
    int width;
    int height;
};

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

internal void win32LoadXInput(void) {
    // TODO: Test on Windows 8
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

            // Start it playing

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

internal void renderWeirdGradient(win32_offscreen_buffer *buffer, int xOffset, int yOffset) {
    uint8* row = (uint8*)buffer->bitmapMemory;
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

struct win32_sound_output {
    // Sound test
    int samplesPerSecond;
    int toneHz;
    int16 toneVolume;
    uint32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tSine;
    int latencySampleCount;
};

internal void win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD bytesToLock, DWORD bytesToWrite) {
    // More test
    // Switch to sin wave
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;


    if (SUCCEEDED(globalSecondaryBuffer->Lock(bytesToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0))) {
        // assert that region1/2Size is valid
        DWORD region1SampleCount = region1Size/soundOutput->bytesPerSample;
        int16 *sampleOut = (int16 *)region1;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
            real32 sineValue = sinf(soundOutput->tSine);

            int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            soundOutput->tSine += 2.0f * pi32 * 1.0f / (real32) soundOutput->wavePeriod;
            ++soundOutput->runningSampleIndex;
        }
        
        DWORD region2SampleCount = region2Size/soundOutput->bytesPerSample;
        sampleOut = (int16 *)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
            real32 sineValue = sinf(soundOutput->tSine);

            int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            soundOutput->tSine += 2.0f * pi32 * 1.0f / (real32) soundOutput->wavePeriod;
            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE preInstance,
    LPSTR commandLine,
    int showCode
) {
    win32LoadXInput();
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

    if (RegisterClassA(&WindowClass)) {
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
            HDC deviceContext = GetDC(window);

            // Graphics test
            int xOffset = 0;
            int yOffset = 0;

            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.toneHz = 256;
            soundOutput.toneVolume = 3000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            soundOutput.bytesPerSample = sizeof(int16)*2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond*soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
            
            win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            win32FillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            
            running = true;
            while (running) {
                MSG message;

                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                // TODO: Should we pull it more frequently?
                for (DWORD controllerIndex=0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
                    XINPUT_STATE controllerState;
                    if(XInputGetState_(controllerIndex, &controllerState) == ERROR_SUCCESS) {
                        // Controller plugged in
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        bool up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool start = pad->wButtons & XINPUT_GAMEPAD_START;
                        bool back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool xButton = pad->wButtons & XINPUT_GAMEPAD_X;
                        bool yButton = pad->wButtons & XINPUT_GAMEPAD_Y;
                        bool aButton = pad->wButtons & XINPUT_GAMEPAD_A;
                        bool bButton = pad->wButtons & XINPUT_GAMEPAD_B;

                        int16 stickX = pad->sThumbLX;
                        int16 stickY = pad->sThumbLY;

                        xOffset += stickX / 4096;
                        yOffset += stickY / 4096;

                        soundOutput.toneHz = 512 + (int)(256 * ((real32)stickY / 30000.0f));
                        soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
                    } else {
                        // Controller not available
                    }
                }

                renderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

                DWORD playCursor;
                DWORD writeCursor;
                // DirectSound output test
                if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
                    DWORD bytesToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;

                    DWORD targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
                    DWORD bytesToWrite;
                    if (bytesToLock > targetCursor) {
                        bytesToWrite = soundOutput.secondaryBufferSize - bytesToLock;
                        bytesToWrite += targetCursor;
                    } else {
                        bytesToWrite = targetCursor - bytesToLock;
                    }

                    win32FillSoundBuffer(&soundOutput, bytesToLock, bytesToWrite);
                }

                win32_window_dimension dimension = win32GetWindowDimesion(window);
                Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);
            }
        } else {
            // TODO:
        }
    } else {
        // TODO:
    }

    return(0);
}