#ifndef PLATFORM_H_
#define PLATFORM_H_

#define ORG_NAME "rfumic"
#define APP_NAME "Boomislav"

#ifdef __EMSCRIPTEN__
#define ROOT_ASSET_FOLDER "/assets"

#else
#define ROOT_ASSET_FOLDER "./assets"

#endif // __EMSCRIPTEN__

#define GAME_SAVE_FILE_NAME "game_save_file.bin"

typedef struct {
    void   *memory;
    usize  size;
} PlatformLoadedFile;

typedef struct {
    void *data;
    usize size;
} PlatformDataToWrite;

static inline i32 PlatformGetRandomI32(i32 min_value, i32 max_value);
static PlatformLoadedFile PlatformLoadEntireFile(char *file_path, Arena *arena);
static b32 PlatformWriteEntireFile(PlatformDataToWrite file_data, char *file_path);
static inline void PlatformShowErrorWindow(char *message);
static inline f32 PlatformSineF32(f32 x);

#ifdef FLAPPY_DEBUG
// TODO: this shouldnt be here
#define PlatformDebugPrint(format_str, ...) SDL_Log(format_str, ##__VA_ARGS__)
#else
#define PlatformDebugPrint(format_str, ...) 
#endif // FLAPPY_DEBUG

#define ASSET_ARENA_SIZE     MB(4)
#define TEMPORARY_ARENA_SIZE MB(1)
#define GAME_ARENA_SIZE      KB(1)

/* TODO: think about this
     consider going to even lower resolution, because
     i want to target browsers
*/

/*  game itself is 9:16, but should i put some bars on left and right? */
/* #define WINDOW_HEIGHT 1024 */
#define WINDOW_HEIGHT 640
/* #define WINDOW_HEIGHT (1024/2) */
#define WINDOW_WIDTH 360
/* #define WINDOW_WIDTH (576/2) */

#endif /* PLATFORM_H_ */
