#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

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

static inline void PlatformShowErrorWindow(char *message)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Boomislav Runtime Error", message, NULL);
}

static PlatformLoadedFile PlatformLoadEntireFile(char *file_path, Arena *arena)
{
    PlatformLoadedFile result = {0};

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
            result = (PlatformLoadedFile){0};
        }
        SDL_CloseIO(sdl_stream);

        result.memory = file_memory;
    }

    return result;
}

// NOTE: 1 is ok
static b32 PlatformWriteEntireFile(PlatformDataToWrite file_data, char *file_path)
{
    b32 result = false;

    SDL_IOStream *sdl_stream = SDL_IOFromFile(file_path, "wb");

    usize bytes_written = SDL_WriteIO(sdl_stream, file_data.data, file_data.size);

    if(bytes_written == file_data.size) {
        result = true;
    }

    SDL_CloseIO(sdl_stream);

#ifdef __EMSCRIPTEN__
    EM_ASM( 
        FS.syncfs(false, function (err) {
        if (err) console.error('Sync error:', err);
        else console.log('Data synced to IndexedDB');
        });
    );
#endif

    return result;
}

global SDL_Window *window;
global SDL_Renderer *renderer;
global SDL_Texture *texture;

global GameState game_state;
global GameScreenBuffer screen_buffer;

global u32 *main_buffer;
global b32 game_running = true;

global u64 fps_current_tick;
global u64 fps_start_time = 0;
global u32 fps_frame_count = 0;
global u64 d_now;
global u64 d_last;
global f32 d_freq;

static int initialize()
{
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        APP_NAME,
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

    renderer = SDL_CreateRenderer(window, NULL);
#if PLATFORM_USE_VSYNC
    SDL_SetRenderVSync(renderer, 1);
#endif

    texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_RGBA8888, 
                                SDL_TEXTUREACCESS_STREAMING, 
                                WINDOW_WIDTH, WINDOW_HEIGHT);

    main_buffer = malloc(WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(u32));
    if(main_buffer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Failed to allocate main buffer\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    u32 total_memory_size = ASSET_ARENA_SIZE + TEMPORARY_ARENA_SIZE + GAME_ARENA_SIZE;
    u8 *usable_memory = calloc(total_memory_size, sizeof(u8));
    if(usable_memory == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Failed to allocate game memory\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    u8 *sdl_base_path = (u8 *)SDL_GetBasePath();
    if(sdl_base_path == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Failed to get application base path\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

#ifdef __EMSCRIPTEN__
    // IMPORTANT: if this changes `SAVE_DATA_DIR` in `emscripten_template.html` needs to be updated
    u8 *save_data_dir = "/data_dir";
#else
    u8 *save_data_dir = (u8 *)SDL_GetPrefPath(ORG_NAME, APP_NAME);
    if(save_data_dir == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, 
                     "Failed to get application data path\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
#endif // __EMSCRIPTEN__


    GameSetupResult setup_result = GameSetup((void *) main_buffer, 
                                             (void *) usable_memory, 
                                             sdl_base_path,
                                             save_data_dir);

    screen_buffer = setup_result.game_screen_buffer;
    game_state = setup_result.game_state;

    
    return 0;
}

static void main_loop() {
    if(!game_running) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#else
        exit(0);
#endif // __EMSCRIPTEN__
    }

    fps_current_tick = SDL_GetTicks();

    d_last = d_now;
    d_now = SDL_GetPerformanceCounter();
    f32 diff = (d_now - d_last);
    diff = diff <= 0 ? 1.0f : diff;
    game_state.delta_time_sec = (f32) diff / d_freq;

    if(fps_current_tick - fps_start_time >= 1000) {
        game_state.current_fps = (u32) (fps_frame_count / ((fps_current_tick - fps_start_time) / 1000.0f));
        fps_start_time = fps_current_tick;
        fps_frame_count = 0;
    }

    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_QUIT: {
                game_running = false;
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP: {
                game_state.keys_up |= KEY_MOUSE;
                break;
            }

            case SDL_EVENT_KEY_DOWN: {
                break;
            }

            case SDL_EVENT_KEY_UP: {
                if(event.key.key == SDLK_UP) {
                    game_state.keys_up |= KEY_UP_ARROW;
                    break;
                }

                if(event.key.key == SDLK_SPACE) {
                    game_state.keys_up |= KEY_SPACE;
                    break;
                }


                if(event.key.key == SDLK_ESCAPE) {
                    // NOTE: "p" is for pausing in debug mode
#if FLAPPY_DEBUG
                    game_running = false;
#else 
                    game_state.keys_up |= KEY_ESCAPE;
#endif // FLAPPY_DEBUG
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

                if(event.key.key == SDLK_3) {
                    game_state.game_debug_flags ^= GDF_SHOW_FPS;
                    break;
                }

                if(event.key.key == SDLK_K) {
                    game_state.debug_score_increment_pressed = true;
                    break;
                }

                if(event.key.key == SDLK_R) {
                    game_state.current_mode = CM_GET_READY;
                    break;
                }

                if(event.key.key == SDLK_P) {
                    game_state.keys_up |= KEY_ESCAPE;
                    break;
                }
#endif // FLAPPY_DEBUG

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

    fps_frame_count++;
#if PLATFORM_USE_VSYNC != 1
    SDL_Delay(16);
    /* SDL_Delay(32); */
#endif
}

int main() {
    if(initialize()) {
        return 1;
    }

    fps_current_tick = SDL_GetTicks();

    fps_start_time = SDL_GetTicks();
    fps_frame_count = 0;

    d_now = SDL_GetPerformanceCounter();
    d_last = d_now;
    d_freq = (f32)SDL_GetPerformanceFrequency();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while(true) {
        main_loop();
    }
#endif // __EMSCRIPTEN__

    return 0;
}
