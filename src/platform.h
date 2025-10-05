#ifndef PLATFORM_H_
#define PLATFORM_H_

typedef struct {
    void   *memory;
    usize  size;
} LoadedFile;

static inline i32 PlatformGetRandomI32(i32 min_value, i32 max_value);
static LoadedFile PlatformLoadEntireFile(char *file_path, Arena *arena);
static inline f32 PlatformSineF32(f32 x);

// TODO: this shouldnt be here
#define PlatformDebugPrint(format_str, ...) SDL_Log(format_str, ##__VA_ARGS__)

#define ASSET_ARENA_SIZE     MB(4)
#define TEMPORARY_ARENA_SIZE MB(1)

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
