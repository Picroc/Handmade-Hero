#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct win32_offscreen_buffer {
    BITMAPINFO info;

    void *bitmapMemory;

    int width;
    int height;

    int pitch;
    int bytesPerPixel;
};

struct win32_window_dimension {
    int width;
    int height;
};

struct win32_sound_output {
    // Sound test
    int samplesPerSecond;
    uint32 runningSampleIndex;
    int bytesPerSample;
    DWORD secondaryBufferSize;
    DWORD safetySampleBytes;
    real32 tSine;
    int latencySampleCount;
};

struct win32_debug_time_marker {
    DWORD outputPlayCursor;
    DWORD outputWriteCursor;
    DWORD outputLocation;
    DWORD outputByteCount;

    DWORD expectedFlipCursor;

    DWORD flipPlayCursor;
    DWORD flipWriteCursor;
};

#endif // WIN32_HANDMADE_H
