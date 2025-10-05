#include <SDL3/SDL.h>
#include <stdlib.h>


#include "common.c"
#include "platform.h"
#include "game.c"

#define PLATFORM_USE_VSYNC 1

static inline f32 PlatformSineF32(f32 x)
{
    return SDL_sinf(x);
}

/* TODO: custom random generator */
static inline i32 PlatformGetRandomI32(i32 min_value, i32 max_value)
{
    i32 result = min_value + SDL_rand(max_value - min_value);

    return result;
}

static LoadedFile PlatformLoadEntireFile(char *file_path, Arena *arena)
{
    LoadedFile result = {0};

    SDL_PathInfo sdl_path_info;
    b32 info_loaded = SDL_GetPathInfo(file_path, &sdl_path_info);

    if(info_loaded) {
        result.size = sdl_path_info.size;

        SDL_IOStream *sdl_stream = 
            SDL_IOFromFile(file_path, "rb");

        void *file_memory = ArenaAllocArray(arena, byte, result.size);

        usize bytes_read = 
            SDL_ReadIO(sdl_stream, file_memory, result.size);

        SDL_IOStatus status = SDL_GetIOStatus(sdl_stream);
        if(bytes_read == 0 && status != SDL_IO_STATUS_EOF) {
            result = (LoadedFile){0};
        }
        SDL_CloseIO(sdl_stream);

        result.memory = file_memory;
    }

    return result;
}

/* TODO: The game crashes when moving window between monitors */
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
#if PLATFORM_USE_VSYNC
    SDL_SetRenderVSync(renderer, 1);
#endif

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


    u8 *usable_memory = calloc(ASSET_ARENA_SIZE + TEMPORARY_ARENA_SIZE, sizeof(u8));
    if(usable_memory == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Failed to allocate game memory\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GameSetupResult setup_result = GameSetup((void *) main_buffer, 
                                             (void *) usable_memory, 
                                             (u8 *)SDL_GetBasePath());

    GameScreenBuffer screen_buffer = setup_result.game_screen_buffer;
    GameState game_state = setup_result.game_state;


    u64 current_tick = SDL_GetTicks();
    u64 last_tick;

    b32 game_running = true;
    while(game_running) {
        last_tick = current_tick;
        current_tick = SDL_GetTicks();

        game_state.delta_time_ms = current_tick - last_tick;

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_EVENT_QUIT: {
                    game_running = false;
                    break;
                }
                case SDL_EVENT_KEY_UP: {
                    if(event.key.key == SDLK_UP) {
                        game_state.jump_key_pressed = true;
                        break;
                    }

#if FLAPPY_DEBUG
                    if(event.key.key == SDLK_1) {
                        game_state.game_debug_flags ^= GDF_ALWAYS_SCORE;
                        break;
                    }

                    if(event.key.key == SDLK_2) {
                        game_state.game_debug_flags ^= GDF_PRIMITIVE_RENDER;
                        break;
                    }

                    if(event.key.key == SDLK_K) {
                        game_state.debug_score_increment_pressed = true;
                        break;
                    }

#endif

                }
                case SDL_EVENT_KEY_DOWN: {

                    if(event.key.key == SDLK_ESCAPE) {
                        game_running = false;
                        break;
                    }

                    if(event.key.key == SDLK_R) {
                        game_state.current_mode = CM_GET_READY;
                        break;
                    }

                }
            }
        }

        GameUpdateAndRender(&screen_buffer, &game_state);

        void *pixels;
        int pitch;
        SDL_LockTexture(texture, 0, &pixels, &pitch);
        memcpy(pixels, main_buffer, WINDOW_HEIGHT * pitch);
        SDL_UnlockTexture(texture);

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, 0, 0);
        SDL_RenderPresent(renderer);

#if PLATFORM_USE_VSYNC != 1
        SDL_Delay(16);
        /* SDL_Delay(32); */
#endif
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
