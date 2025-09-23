#include "../include/common.h"
#include "handmade.cpp"
#include <SDL2/SDL.h>

#include <math.h>

#define pi32 3.14159265359f

#define MAX_CONTROLLERS 4
SDL_GameController *controller_handles[MAX_CONTROLLERS];
SDL_Haptic *haptic_handles[MAX_CONTROLLERS];

struct sdl_offscreen_buffer {
    SDL_Texture *texture;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct sdl_window_dimension {
    int width;
    int height;
};

struct sdl_audio_ring_buffer {
    int size;
    int write_cursor;
    int play_cursor;
    void *data;
};

struct sdl_sound_output {
    int samples_per_second;
    int tone_hz;
    int16 tone_volume;
    int32 running_sample_index;
    int wave_period;
    int bytes_per_sample;
    int buffer_size;
    real32 sin_t;
    int latency_sample_count;
};

global_variable bool running;
global_variable sdl_offscreen_buffer global_back_buffer;
global_variable sdl_audio_ring_buffer audio_ring_buffer;

internal sdl_window_dimension SDLGetWindowDimension(SDL_Window *window) {
    sdl_window_dimension result;
    SDL_GetWindowSize(window, &result.width, &result.height);

    return result;
}

internal void SDLResizeTexture(sdl_offscreen_buffer *buffer, SDL_Renderer *renderer, int width, int height) {
    if (buffer->texture) {
        SDL_DestroyTexture(buffer->texture);
    }

    buffer->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (buffer->memory) {
        free(buffer->memory);
    }

    int bytesPerPixel = 4;
    buffer->memory = malloc(width * height * bytesPerPixel);

    buffer->width = width;
    buffer->height = height;

    buffer->pitch = width * bytesPerPixel;
}

internal void SDLUpdateWindow(sdl_offscreen_buffer buffer, SDL_Renderer *renderer) {
    if (SDL_UpdateTexture(buffer.texture, 0, buffer.memory, buffer.pitch) != 0) {
        // TODO: Error
        return;
    }

    SDL_RenderCopy(renderer, buffer.texture, 0, 0);
    SDL_RenderPresent(renderer);
}

internal void SDLAudioCallback(void *user_data, uint8 *audio_data, int length) {
    sdl_audio_ring_buffer *ring_buffer = (sdl_audio_ring_buffer *)user_data;

    int region_1_size = length;
    int region_2_size = 0;
    if (ring_buffer->play_cursor + length > ring_buffer->size) {
        region_1_size = ring_buffer->size - ring_buffer->play_cursor;
        region_2_size = length - region_1_size;
    }

    memcpy(audio_data, (uint8*)(ring_buffer->data) + ring_buffer->play_cursor, region_1_size);
    memcpy(&audio_data[region_1_size], ring_buffer->data, region_2_size);

    ring_buffer->play_cursor = (ring_buffer->play_cursor + length) % ring_buffer->size;
    ring_buffer->write_cursor = (ring_buffer->play_cursor + length) % ring_buffer->size;
}

void SDLInitAudio(int32 samples_per_second, int32 buffer_size) {
    SDL_AudioSpec audio_settings = {};

    audio_settings.freq = samples_per_second;
    audio_settings.format = AUDIO_S16LSB;
    audio_settings.channels = 2;
    audio_settings.samples = 512;
    audio_settings.callback = &SDLAudioCallback;
    audio_settings.userdata = &audio_ring_buffer;

    audio_ring_buffer.size = buffer_size;
    audio_ring_buffer.data = malloc(buffer_size);
    audio_ring_buffer.play_cursor = audio_ring_buffer.write_cursor = 0;

    SDL_OpenAudio(&audio_settings, 0);
}

void SDLFillSoundBuffer(sdl_sound_output *sound_output, int byte_to_lock, int bytes_to_write) {
    void* region_1 = (uint8*) audio_ring_buffer.data + byte_to_lock;
    int region_1_size = bytes_to_write;
    if (region_1_size + byte_to_lock > sound_output->buffer_size) {
        region_1_size = sound_output->buffer_size - byte_to_lock;
    }
    void *region_2 = (uint8*) audio_ring_buffer.data;
    int region_2_size = bytes_to_write - region_1_size;

    int region_1_sample_count = region_1_size / sound_output->bytes_per_sample;
    int16 *sample_out = (int16 *) region_1;

    for (int sample_index = 0; sample_index < region_1_sample_count; ++sample_index) {
        real32 sine_value = sinf(sound_output->sin_t);
        int16 sample_value = (int16) (sine_value * sound_output->tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        sound_output->sin_t += 2.0f * pi32 * 1.0f / (real32) sound_output->wave_period;
        ++sound_output->running_sample_index;
    }

    int region_2_sample_count = region_2_size / sound_output->bytes_per_sample;
    sample_out = (int16 *) region_2;
    for (int sample_index = 0; sample_index < region_2_sample_count; ++sample_index) {
        real32 sine_value = sinf(sound_output->sin_t);
        int16 sample_value = (int16) (sine_value * sound_output->tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        sound_output->sin_t += 2.0f * pi32 * 1.0f / (real32) sound_output->wave_period;
        ++sound_output->running_sample_index;
    }
}

bool handle_event(SDL_Event *event) {
    bool should_quit = false;

    switch (event->type) {
        case SDL_QUIT: {
            printf("SDL_QUIT\n");
            should_quit = true;
        } break;
        case SDL_WINDOWEVENT: {
            switch (event->window.event) {
                case SDL_WINDOWEVENT_RESIZED: {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);

                    SDLResizeTexture(&global_back_buffer, renderer, event->window.data1, event->window.data2);
                } break;
                case SDL_WINDOWEVENT_EXPOSED: {
                    SDL_Window *window =  SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);

                    SDLUpdateWindow(global_back_buffer, renderer);
                } break;

            } break;
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            SDL_Keycode keycode = event->key.keysym.sym;

            bool alt_key_was_down = event->key.keysym.mod & KMOD_ALT;

            bool was_down = false;
            bool is_down = event->key.state == SDL_PRESSED ;
            if (event->key.state == SDL_RELEASED) {
                was_down = true;
            }
            if (event->key.repeat != 0) {
                was_down = true;
            }

            if (was_down != is_down) {
                if (keycode == SDLK_w) {
                    printf("SDLK_w\n");
                } else if (keycode == SDLK_F4 && alt_key_was_down) {
                    should_quit = true;
                }
            }
        } break;
        }
    }

    return should_quit;
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC|SDL_INIT_AUDIO) != 0) {
        // TODO: SDL_Init didn't work
    }

    uint64 perf_count_frequency = SDL_GetPerformanceFrequency();

    SDL_Window *window = SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
    if (window) {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
        if (renderer) {
            int max_joysticks = SDL_NumJoysticks();
            int controller_index = 0;

            for (int joystick_index = 0; joystick_index < max_joysticks; ++joystick_index) {
                if (!SDL_IsGameController(joystick_index)) {
                    continue;
                }

                if (controller_index >= MAX_CONTROLLERS) {
                    break;
                }

                controller_handles[controller_index] = SDL_GameControllerOpen(joystick_index);
                SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller_handles[controller_index]);
                haptic_handles[controller_index] = SDL_HapticOpenFromJoystick(joystick);

                if (SDL_HapticRumbleInit(haptic_handles[controller_index]) != 0) {
                    printf("SDL_HapticRumbleInit failed\n");
                    SDL_HapticClose(haptic_handles[controller_index]);
                    haptic_handles[controller_index] = 0;
                }

                controller_index++;
            }

            sdl_window_dimension window_dimension = SDLGetWindowDimension(window);
            SDLResizeTexture(&global_back_buffer, renderer, window_dimension.width, window_dimension.height);

            running = true;

            int xOffset = 0;
            int yOffset = 0;

            sdl_sound_output sound_output = {};

            sound_output.samples_per_second = 48000;
            sound_output.tone_hz = 256;
            sound_output.tone_volume = 3000;
            sound_output.running_sample_index = 0;
            sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
            sound_output.bytes_per_sample = sizeof(int16) * 2;
            sound_output.latency_sample_count = sound_output.samples_per_second / 15;
            sound_output.sin_t = 0.0f;

            sound_output.buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
            SDLInitAudio(48000, sound_output.buffer_size);
            SDLFillSoundBuffer(&sound_output, 0, sound_output.latency_sample_count * sound_output.bytes_per_sample);

            SDL_PauseAudio(0);

            while (running) {
                uint64 last_counter = SDL_GetPerformanceCounter();

                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (handle_event(&event)) {
                        running = false;
                    }
                }

                for (int controller_index = 0; controller_index < MAX_CONTROLLERS; ++controller_index) {
                    if (controller_handles[controller_index] != 0 && SDL_GameControllerGetAttached(controller_handles[controller_index])) {
                        bool up = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_UP);
                        bool down = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                        bool left = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                        bool right = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

                        bool start = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_START);
                        bool back = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_BACK);

                        bool left_shoulder = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                        bool right_shoulder = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

                        bool a_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_A);
                        bool b_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_B);
                        bool x_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_X);
                        bool y_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_Y);

                        int16 stick_x = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTX);
                        int16 stick_y = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTY);

                        if (a_button) {
                            yOffset += 2;
                        }

                        if (b_button) {
                            if (haptic_handles[controller_index]) {
                                SDL_HapticRumblePlay(haptic_handles[controller_index], 0.5f, 2000);
                            }
                        }

                        xOffset += stick_x / 4096;
                        yOffset += stick_y / 4096;

                        sound_output.tone_hz = 512 + (int)(256 * ((real32)stick_y / 30000.0f));
                        sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
                    } else {
                        // Controller is not plugged in
                    }
                }

                SDL_LockAudio();

                int byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.buffer_size;
                int bytes_to_write;
                int target_cursor = ((audio_ring_buffer.play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.buffer_size);

                if (byte_to_lock > target_cursor) {
                    bytes_to_write = (sound_output.buffer_size - byte_to_lock);
                    bytes_to_write += target_cursor;
                } else {
                    bytes_to_write = target_cursor - byte_to_lock;
                }

                SDL_UnlockAudio();

                SDLFillSoundBuffer(&sound_output, byte_to_lock, bytes_to_write);

                game_offscreen_buffer buffer = {};
                buffer.memory = global_back_buffer.memory;
                buffer.width = global_back_buffer.width;
                buffer.height = global_back_buffer.height;
                buffer.pitch = global_back_buffer.pitch;

                game_update_and_render(&buffer, xOffset, yOffset);

                SDLUpdateWindow(global_back_buffer, renderer);

                xOffset++;

                uint64 end_counter = SDL_GetPerformanceCounter();
                uint64 counter_elapsed = end_counter - last_counter;

                real64 ms_per_frame = 1000.0f * (real64) counter_elapsed / (real64) perf_count_frequency;
                real64 FPS = (real64) perf_count_frequency / (real64) counter_elapsed;

                printf("%.02f ms/f, %.02ff/s\n", ms_per_frame, FPS);
                last_counter = end_counter;
            }
        } else {
            // TODO: Renderer failed
        }
    } else {
        // TODO: Window not created
    }

    for (int controller_index = 0; controller_index < MAX_CONTROLLERS; ++controller_index) {
        if (controller_handles[controller_index]) {
            SDL_GameControllerClose(controller_handles[controller_index]);
        }
        if (haptic_handles[controller_index]) {
            SDL_HapticClose(haptic_handles[controller_index]);
        }
    }

    SDL_CloseAudio();

    SDL_Quit();

    return 0;
}
