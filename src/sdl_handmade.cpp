#include "../include/common.h"
#include <SDL2/SDL.h>

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
        }
    }

    return should_quit;
}


int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        // TODO: SDL_Init didn't work
    }

    SDL_Window *window = SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
    if (window) {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
        if (renderer) {
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

    SDL_Quit();

    return 0;
}