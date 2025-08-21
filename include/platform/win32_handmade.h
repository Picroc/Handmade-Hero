#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

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

struct win32_sound_output {
    // Sound test
    int samplesPerSecond;
    uint32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tSine;
    int latencySampleCount;
};

#endif // WIN32_HANDMADE_H
