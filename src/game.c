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

static i32 pipe_width = 0;
static u32 pipe_color = 0x00FF00FF;
static i32 y_between_pipes = 0;

static inline void DrawPipePair(PipePair *pipe_pair, GameScreenBuffer *buffer)
{
    /* TODO: this currently doesnt take into account the "ground" */
    /*       everything is calculated relative to the  whole game screen */

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

static inline i32 GetRandomPipeY(i32 game_screen_height) {
    i32 result;

    i32 y_margin_top    = PercentOf(10, game_screen_height) + y_between_pipes;
    i32 y_margin_bottom = PercentOf(90, game_screen_height);

    result = PlatformGetRandomI32(y_margin_top, y_margin_bottom);
    
    return result;
}

typedef struct {
    PipePair *pipes[3];
    i32 count;
} PipeQueue;

static PipeQueue pipe_queue;

static inline void AddAvailablePipe(PipePair *new_pipe)
{
    if(new_pipe == NULL) {
        return;
    }
    Assert(pipe_queue.count >= 0 && pipe_queue.count < 3);

    i32 count = pipe_queue.count == 0 ? 1 : pipe_queue.count;

    for(i32 i = 0; i < count; i++) {
        PipePair *curr = pipe_queue.pipes[i];
        if(curr == NULL) {
            pipe_queue.pipes[i] = new_pipe;
            pipe_queue.count++;
            return;
        }
    }
}

static inline PipePair *GetAvailablePipe(GameScreenBuffer *screen_buffer) 
{
    Assert(pipe_queue.count >= 0);

    PipePair *result = NULL;

    if(pipe_queue.count > 0) {
        result = pipe_queue.pipes[0];
        result->x = screen_buffer->width;
        /* TODO: handle random y here */
        result->bottom_pipe_y = GetRandomPipeY(screen_buffer->height);

        pipe_queue.pipes[0] = pipe_queue.pipes[1];
        pipe_queue.pipes[1] = pipe_queue.pipes[2];
        pipe_queue.pipes[2] = NULL;
        pipe_queue.count--;
    }

    return result;
}

static PipePair *oldest_pipe;
static PipePair *current_pipe;
static PipePair *newest_pipe;

static PipePair pipes[3];

#define PIPE_MOVEMENT_SPEED 10

static void GameUpdateAndRender(GameScreenBuffer *game_screen_buffer, 
                                b32 new_game_started) 
{
#if FLAPPY_DEBUG
    static u32 debug_frame_counter = 0;
#endif

    if(new_game_started) {
        y_between_pipes = PercentOf(30, game_screen_buffer->height);
        pipe_width      = PercentOf(17, game_screen_buffer->width);

        pipes[0] = (PipePair){0, 550};
        pipes[1] = (PipePair){0, 550};
        pipes[2] = (PipePair){0, 550};

        pipe_queue.pipes[0] = &pipes[0];
        pipe_queue.pipes[1] = &pipes[1];
        pipe_queue.pipes[2] = &pipes[2];
        pipe_queue.count = 3;


        oldest_pipe  = NULL;
        current_pipe = NULL;
        newest_pipe  = GetAvailablePipe(game_screen_buffer);
    }
    
    for(i32 i = 0; 
        i < game_screen_buffer->width * game_screen_buffer->height; 
        i++) {
        ((u32 *)game_screen_buffer->memory)[i] = 0xFFFF00FF;
    }

    if ((newest_pipe->x + pipe_width / 2) <= game_screen_buffer->width / 2) {
        oldest_pipe = current_pipe;
        current_pipe = newest_pipe;
        newest_pipe = GetAvailablePipe(game_screen_buffer);
    }

    if(oldest_pipe) {
        if (oldest_pipe->x + pipe_width <= 0) {
            AddAvailablePipe(oldest_pipe);
        }

        DrawPipePair(oldest_pipe, game_screen_buffer);
        oldest_pipe->x -= PIPE_MOVEMENT_SPEED;
    }

    if(newest_pipe) {
        DrawPipePair(newest_pipe, game_screen_buffer);
        newest_pipe->x -= PIPE_MOVEMENT_SPEED;
    }

    if(current_pipe) {
        DrawPipePair(current_pipe, game_screen_buffer);
        current_pipe->x -= PIPE_MOVEMENT_SPEED;
    }

#if FLAPPY_DEBUG
    (void)debug_frame_counter;
    debug_frame_counter++;
#endif
}
