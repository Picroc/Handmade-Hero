#include <windows.h>
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// TODO: Global for now
global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;

global_variable int bitmapWidth;
global_variable int bitmapHeight;

global_variable int bytesPerPixel = 4;

internal void renderWeirdGradient(int xOffset, int yOffset) {
    int pitch = bitmapWidth * bytesPerPixel;
    uint8* row = (uint8*)bitmapMemory;
    for (int y = 0; y < bitmapHeight; ++y) {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < bitmapWidth; ++x) {
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset); 

            *pixel++ = (green << 8) | blue;
        }

        row += pitch;
    }
}

internal void Win32ResizeDIBSection(int width, int height) {
    // Bulletproof this, don't free first, free after

    if(bitmapMemory) {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize =  (bitmapWidth * bitmapHeight) * bytesPerPixel;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO: Probably clear to black
}

internal void Win32UpdateWindow(HDC deviceContext, RECT *clientRect, int x, int y, int width, int height) {
    int windowWidth = clientRect->right - clientRect->left;
    int windowHeight = clientRect->bottom - clientRect->top;

    StretchDIBits(deviceContext, 0, 0, bitmapWidth, bitmapHeight, 0, 0, windowWidth, windowHeight, bitmapMemory, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
            RECT clientRect;
            GetClientRect(window, &clientRect);

            int height = clientRect.bottom - clientRect.top;
            int width = clientRect.right - clientRect.left;

            Win32ResizeDIBSection(width, height);
            OutputDebugStringA("WM_SIZE\n");
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

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);

            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;

            RECT clientRect;
            GetClientRect(window, &clientRect);

            Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);

            EndPaint(window, &paint);
        } break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return result;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE preInstance,
    LPSTR commandLine,
    int showCode
) {
    WNDCLASSA WindowClass = {};

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
            running = true;
            int xOffset = 0;
            int yOffset = 0;

            while (running) {
                MSG message;

                while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        running = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                renderWeirdGradient(xOffset, yOffset);

                HDC deviceContext = GetDC(window);

                RECT clientRect;
                GetClientRect(window, &clientRect);
                int windowWidth = clientRect.right - clientRect.left;
                int windowHeight = clientRect.bottom - clientRect.top;

                Win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
                ReleaseDC(window, deviceContext);

                ++xOffset;
            }
        } else {
            // TODO:
        }
    } else {
        // TODO:
    }

    return(0);
}