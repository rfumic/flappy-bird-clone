#include <SDL3/SDL.h>
#include <stdlib.h>


#include "common.c"

// Platform function signatures
static inline i32 PlatformGetRandomI32(i32 min_value, i32 max_value);

#include "game.c"

/* TODO: custom random generator */
static inline i32 PlatformGetRandomI32(i32 min_value, i32 max_value) {
    i32 result = min_value + SDL_rand(max_value - min_value);

    return result;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        argv[0],
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0 // SDL_WINDOW_OPENGL
    );

    if(window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Could not create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_RaiseWindow(window);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    SDL_Texture *texture = SDL_CreateTexture(renderer, 
                                             SDL_PIXELFORMAT_RGBA8888, 
                                             SDL_TEXTUREACCESS_STREAMING, 
                                             WINDOW_WIDTH, WINDOW_HEIGHT);

    u32 *main_buffer = malloc(WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(u32));
    if(main_buffer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Failed to allocate main buffer\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GameScreenBuffer screen_buffer = {
        .memory = (void *) main_buffer,
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
    };

    b32 game_running = 1;
    b32 new_game_started;
    while(game_running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_EVENT_QUIT: {
                    game_running = 0;
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    if(event.key.key == SDLK_ESCAPE) {
                        game_running = 0;
                        break;
                    }

                    if(event.key.key == SDLK_R) {
                        new_game_started = 1;
                        break;
                    }

                }
            }
        }

        GameUpdateAndRender(&screen_buffer, &new_game_started);

        void *pixels;
        int pitch;
        SDL_LockTexture(texture, 0, &pixels, &pitch);
        memcpy(pixels, main_buffer, WINDOW_HEIGHT * pitch);
        SDL_UnlockTexture(texture);

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, 0, 0);
        SDL_RenderPresent(renderer);

        /* TODO: figure out how to establish a frame rate correctly */
        SDL_Delay(32);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
