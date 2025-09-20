typedef struct {
    void  *memory;
    i32    width;
    i32    height;
} GameScreenBuffer;

static void DrawRectangle(GameScreenBuffer *buffer, i32 min_x, i32 min_y, 
                          i32 max_x, i32 max_y, u32 color) 
{
    /* TODO: use min/max functions */
    if(min_x < 0) {
        min_x = 0;
    }

    if(min_y < 0) {
        min_y = 0;
    }

    if(max_x > buffer->width) {
        max_x = buffer->width;
    }

    if(max_y > buffer->height) {
        max_y = buffer->height;
    }

    /* TODO: consider adding these to GameScreenBuffer if used frequently */
    u32 bytes_per_pixel = sizeof(u32);
    u32 pitch = buffer->width * bytes_per_pixel;

    u8 *row = ((u8 *)buffer->memory + min_x * bytes_per_pixel + min_y * pitch);
    for(i32 y = min_y; y < max_y; y++) {
        u32 *pixel = (u32 *)row;

        for(i32 x = min_x; x < max_x; x++) {
            *pixel++ = color;
        }
        row += pitch;
    }
}

static void GameUpdateAndRender(GameScreenBuffer *game_screen_buffer) 
{
    for(i32 i = 0; 
        i < game_screen_buffer->width * game_screen_buffer->height; 
        i++) {
        ((u32 *)game_screen_buffer->memory)[i] = 0xFFFF00FF;
    }

    DrawRectangle(game_screen_buffer, 50, 50, 150, 200, 0x00FF00FF);
}
