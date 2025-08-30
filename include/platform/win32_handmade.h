#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

#include "handmade.h"

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

struct win32_game_code {
    HMODULE gameCodeDLL;
    FILETIME DLLLastWriteTime;

    // CAUTION: Can be 0, check before using
    game_get_sound_samples *getSoundSamples;
    // CAUTION: Can be 0, check before using
    game_update_and_render *getUpdateAndRender;

    bool32 isValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

struct win32_state {
    HANDLE recordingHandle;
    int inputRecordingIndex;

    HANDLE playbackHandle;
    int inputPlayingIndex;

    uint64 totalSize;
    void *gameMemoryBlock;

    char exeFileName[WIN32_STATE_FILE_NAME_COUNT];
    char *onePastLastEXEFileNameSlash;
};

#endif // WIN32_HANDMADE_H
