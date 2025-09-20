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

typedef struct {
    i32 x;
    i32 bottom_pipe_y;
} PipePair;

static void DrawPipePair(PipePair *pipe_pair, GameScreenBuffer *buffer)
{
    /* TODO: this currently doesnt take into account the "ground" */
    /*       everything is calculated relative to the  whole game screen */

    /* TODO: global variables? */
    u32 pipe_color = 0x00FF00FF;
    i32 pipe_width = PercentOf(17, buffer->width);

    i32 y_between_pipes = PercentOf(30, buffer->height);
    
    // Draw top pipe
    DrawRectangle(buffer, 
                  pipe_pair->x, 0, 
                  pipe_pair->x + pipe_width, 
                  (pipe_pair->bottom_pipe_y - y_between_pipes), 
                  /* pipe_color); */
                  0xFF0000FF);

    // Draw bottom pipe
    DrawRectangle(buffer, 
                  pipe_pair->x, 
                  pipe_pair->bottom_pipe_y, 
                  pipe_pair->x + pipe_width, 
                  buffer->height, 
                  pipe_color);

}

/* static Pipe[3] pipes; */
static PipePair pipe_pair = { WINDOW_WIDTH / 2 , 550};

static void GameUpdateAndRender(GameScreenBuffer *game_screen_buffer) 
{
    for(i32 i = 0; 
        i < game_screen_buffer->width * game_screen_buffer->height; 
        i++) {
        ((u32 *)game_screen_buffer->memory)[i] = 0xFFFF00FF;
    }

    DrawPipePair(&pipe_pair, game_screen_buffer);

}
