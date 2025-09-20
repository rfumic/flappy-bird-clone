#include <stdint.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t b32;

#define PercentOf(percentage, total) ((percentage/100.0f) * total)

/* TODO: think about this  */
/*  game itself is 9:16, but should i put some bars on left and right? */
#define WINDOW_HEIGHT 1024
/* #define WINDOW_WIDTH WINDOW_HEIGHT */
#define WINDOW_WIDTH 576
