#include "../include/common.h"
#include <SDL2/SDL.h>

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

global_variable bool running;
global_variable sdl_offscreen_buffer global_back_buffer;

void renderWeirdGradient(sdl_offscreen_buffer buffer, int xOffset, int yOffset) {
    uint8* row = (uint8*)buffer.memory;
    for (int y = 0; y < buffer.height; ++y) {
        uint32* pixel = (uint32*)row;
        for (int x = 0; x < buffer.width; ++x) {
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset);
            *pixel++ = (green << 8) | blue;
        }
        row += buffer.pitch;
    }
}

sdl_window_dimension SDLGetWindowDimension(SDL_Window *window) {
    sdl_window_dimension result;
    SDL_GetWindowSize(window, &result.width, &result.height);

    return result;
}

void SDLResizeTexture(sdl_offscreen_buffer *buffer, SDL_Renderer *renderer, int width, int height) {
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

void SDLUpdateWindow(sdl_offscreen_buffer buffer, SDL_Renderer *renderer) {
    if (SDL_UpdateTexture(buffer.texture, 0, buffer.memory, buffer.pitch) != 0) {
        // TODO: Error
        return;
    }

    SDL_RenderCopy(renderer, buffer.texture, 0, 0);
    SDL_RenderPresent(renderer);
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

            bool wasDown = false;
            bool isDown = event->key.state == SDL_PRESSED ;
            if (event->key.state == SDL_RELEASED) {
                wasDown = true;
            }
            if (event->key.repeat != 0) {
                wasDown = true;
            }

            if (wasDown != isDown) {
                if (keycode == SDLK_w) {
                    printf("SDLK_w\n");
                }
            }
        } break;
        }
    }

    return should_quit;
}


int main() {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC) != 0) {
        // TODO: SDL_Init didn't work
    }

    SDL_Window *window = SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
    if (window) {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
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

            while (running) {
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
                    } else {
                        // Controller is not plugged in
                    }
                }

                renderWeirdGradient(global_back_buffer, xOffset, yOffset);

                SDLUpdateWindow(global_back_buffer, renderer);

                xOffset++;
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

    SDL_Quit();

    return 0;
}