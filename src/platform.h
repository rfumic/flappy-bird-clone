#ifndef PLATFORM_H_
#define PLATFORM_H_

typedef struct {
    void   *memory;
    usize  size;
} LoadedFile;

static inline i32 PlatformGetRandomI32(i32 min_value, i32 max_value);
static LoadedFile PlatformLoadEntireFile(char *file_path, Arena *arena);
static inline f32 PlatformSineF32(f32 x);

#define PlatformDebugPrint(format_str, ...) SDL_Log(format_str, ##__VA_ARGS__)

#define ASSET_ARENA_SIZE MB(2)

#endif /* PLATFORM_H_ */
