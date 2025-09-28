#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t b32;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef uintptr_t uptr;
typedef char byte;
typedef ptrdiff_t size;
typedef size_t usize;

#define global static

#define PercentOf(percentage, total) ((percentage/100.0f) * total)

#ifdef FLAPPY_DEBUG

#define Assert(c)           \
    while (!(c))            \
    __builtin_unreachable() \

#else
#define Assert(c) 
#endif

#define ArrayCount(a) (size)(sizeof(a) / sizeof(*(a)))

#define Min(a,b) (((a)<(b))?(a):(b))

#define Max(a,b) (((a)>(b))?(a):(b))

#define KB(x) ((x) << 10)

#define MB(x) ((x) << 20)

static inline void swap_u32(u32 *a, u32 *b) 
{
    u32 temp = *a;
    *a = *b;
    *b = temp;
}

////////////////////
/// Arenas
////////////////////
typedef struct {
  u8 *base;
  usize size;
  usize used;
} Arena;

static inline void ArenaInit(Arena *arena, usize size, u8 *base)
{
  Assert(base != NULL);
  arena->base = base;
  arena->size = size;
  arena->used = 0;
}

static inline void *ArenaAlloc_(Arena *arena, usize size)
{
  Assert((arena->used + size) <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  return result;
}

#define ArenaAlloc(arena, type) (type *)ArenaAlloc_(arena, sizeof(type))
#define ArenaAllocArray(arena, type, count)                                  \
  ArenaAlloc_(arena, sizeof(type) * (count))

////////////////////
/// Strings
////////////////////
#define LengthOf(str) (ArrayCount(str) - 1)

#define S(str) (String){(u8 *) str, LengthOf(str)}

typedef struct {
    u8   *data;
    size length;
} String;

static inline size StringLength(u8 *input) 
{
    u8 *start = input;
    while (*input++ != '\0')
        ;

    return input - start - 1;
}

/* TODO: think about this
     consider going to even lower resolution, because
     i want to target browsers
*/

/*  game itself is 9:16, but should i put some bars on left and right? */
#define WINDOW_HEIGHT 1024
/* #define WINDOW_HEIGHT (1024/2) */
#define WINDOW_WIDTH 576
/* #define WINDOW_WIDTH (576/2) */
