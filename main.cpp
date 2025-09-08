#include <SDL2/SDL.h>

static SDL_Texture *texture;
static void *pixels;
static int textureWidth;

void SDLResizeTexture(SDL_Renderer *renderer, int width, int height) {
    if (texture) {
        SDL_DestroyTexture(texture);
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (pixels) {
        free(pixels);
    }

    pixels = malloc(width * height * 4);

    textureWidth = width;
}

void SDLUpdateWindow(SDL_Renderer *renderer) {
    if (SDL_UpdateTexture(texture, 0, pixels, textureWidth * 4) != 0) {
        // TODO: Error
        return;
    }

    SDL_RenderCopy(renderer, texture, 0, 0);
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

                    SDLResizeTexture(renderer, event->window.data1, event->window.data2);
                } break;
                case SDL_WINDOWEVENT_EXPOSED: {
                    SDL_Window *window =  SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);

                    SDLUpdateWindow(renderer);
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
            for (;;) {
                SDL_Event event;
                SDL_WaitEvent(&event);

                if (handle_event(&event)) {
                    break;
                }

                int width, height;
                SDL_GetWindowSize(window, &width, &height);


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