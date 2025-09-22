// NOTE: toggles for gameplay debugging
#define GAME_DEBUG_ALWAYS_SCORE 0

typedef struct {
    void  *memory;
    i32    width;
    i32    height;
} GameScreenBuffer;

typedef struct {
    b32 jump_key_pressed;
    b32 new_game_started;
    u64 delta_time_ms;
} GameState;

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

global i32 pipe_width = 0;
global i32 y_between_pipes = 0;

global PipePair *oldest_pipe;
global PipePair *current_pipe;
global PipePair *newest_pipe;

static inline void DrawPipePair(PipePair *pipe_pair, GameScreenBuffer *buffer)
{
    /* TODO: this currently doesnt take into account the "ground" */
    /*       everything is calculated relative to the  whole game screen */

    u32 pipe_color = 0x00FF00FF;
#if FLAPPY_DEBUG
    if(pipe_pair == oldest_pipe) {
        pipe_color = 0xFF0000FF;
    }

    if(pipe_pair == current_pipe) {
        pipe_color = 0x00FF00FF;
    }

    if(pipe_pair == newest_pipe) {
        pipe_color = 0x0000FFFF;
    }
#endif

    // Draw top pipe
    DrawRectangle(buffer, 
                  pipe_pair->x, 0, 
                  pipe_pair->x + pipe_width, 
                  (pipe_pair->bottom_pipe_y - y_between_pipes), 
                  /* pipe_color); */
                  0xFF00FFFF);

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

global PipeQueue pipe_queue;

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
#if GAME_DEBUG_ALWAYS_SCORE
        result->bottom_pipe_y = 700;
#else
        result->bottom_pipe_y = GetRandomPipeY(screen_buffer->height);
#endif

        pipe_queue.pipes[0] = pipe_queue.pipes[1];
        pipe_queue.pipes[1] = pipe_queue.pipes[2];
        pipe_queue.pipes[2] = NULL;
        pipe_queue.count--;
    }

    return result;
}


global PipePair pipes[3];

#define PIPE_MOVEMENT_SPEED 0.25


typedef struct {
    i32 y;
    i32 x;
    i32 height;
    i32 width;
    f32 velocity;
} Bird;

#define BIRD_FALLING_RATE 10

global Bird bird;

static void DrawBird(GameScreenBuffer *game_screen_buffer) 
{

    u32 bird_color  = 0xFF9500FF; 

    DrawRectangle(game_screen_buffer, bird.x, bird.y, 
                  bird.x + bird.width, bird.y + bird.height,
                  bird_color);
}

static inline b32 BirdCollidesWithCurrentPipe()
{
    b32 result = false;
    if(current_pipe) {
        i32 bird_x_end = bird.x + bird.width;

        b32 intersect_horizontally = 
            (bird_x_end >= current_pipe->x && bird_x_end <= current_pipe->x + pipe_width) ||
            (bird.x >= current_pipe->x && bird.x <= current_pipe->x + pipe_width);
        
        b32 intersect_vertically_bottom = 
            bird.y + bird.height >= current_pipe->bottom_pipe_y;

        b32 intersect_vertically_top = 
            bird.y <= current_pipe->bottom_pipe_y - y_between_pipes;

        b32 intersect_vertically = intersect_vertically_bottom || intersect_vertically_top;

        result = intersect_horizontally && intersect_vertically;
    }

    return result;
}

global i32 current_score = 0;
global b32 can_score = true;

static void GameUpdateAndRender(GameScreenBuffer *game_screen_buffer, 
                                GameState *game_state) 
{
#if FLAPPY_DEBUG
    static u32 debug_frame_counter = 0;
#endif

    if(game_state->new_game_started) {
        y_between_pipes = PercentOf(30, game_screen_buffer->height);
        pipe_width      = PercentOf(17, game_screen_buffer->width);

        bird.width  = PercentOf(11, game_screen_buffer->width);
        bird.height = PercentOf(7, game_screen_buffer->height);
        bird.y      = game_screen_buffer->height / 2;
        bird.x      = (PercentOf(28.67, game_screen_buffer->width) 
                        - (bird.width / 2));
        bird.velocity = 0;

        current_score = 0;
        can_score = true;

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

        /* game_state->new_game_started = false; */
        *game_state = (GameState) {
            .jump_key_pressed = 0, 
            .new_game_started = 0
        };
    }
    
    for(i32 i = 0; 
        i < game_screen_buffer->width * game_screen_buffer->height; 
        i++) {
        ((u32 *)game_screen_buffer->memory)[i] = 0xFFFF00FF;
    }

    if ((newest_pipe->x + pipe_width / 2) <= game_screen_buffer->width / 2) {
        oldest_pipe = current_pipe;

        current_pipe = newest_pipe;
        can_score = 1;

        newest_pipe = GetAvailablePipe(game_screen_buffer);
    }

    if(bird.y + bird.height >= game_screen_buffer->height ||
       BirdCollidesWithCurrentPipe()) {
        game_state->new_game_started = true;
        return;
    }

    if(oldest_pipe) {
        if (oldest_pipe->x + pipe_width <= 0) {
            AddAvailablePipe(oldest_pipe);
        }

        DrawPipePair(oldest_pipe, game_screen_buffer);
        oldest_pipe->x -= PIPE_MOVEMENT_SPEED * game_state->delta_time_ms;
    }

    if(newest_pipe) {
        DrawPipePair(newest_pipe, game_screen_buffer);
        newest_pipe->x -= PIPE_MOVEMENT_SPEED * game_state->delta_time_ms;
    }

    if(current_pipe) {
        if(bird.x > current_pipe->x && can_score) {
            current_score++;
            can_score = 0;
            PlatformDebugPrint("score: %d", current_score);
        }

        DrawPipePair(current_pipe, game_screen_buffer);
        current_pipe->x -= PIPE_MOVEMENT_SPEED * game_state->delta_time_ms;
    }

    DrawBird(game_screen_buffer);

    /* TODO: Figure out how to get these speeds just right */

    if(game_state->jump_key_pressed) {
        bird.velocity = -(game_screen_buffer->height * 0.6f);
        game_state->jump_key_pressed = false;
    }

    f32 delta_time = game_state->delta_time_ms / 1000.0f;

    // NOTE: this is gravity
    bird.velocity += (game_screen_buffer->height * 0.002f * 30.0f * 30.0f) * delta_time;
    bird.y += bird.velocity * delta_time;
#if GAME_DEBUG_ALWAYS_SCORE
    bird.y = game_screen_buffer->height / 2;
#endif
    
    f32 max_fall = game_screen_buffer->height * 0.03f * 30.0f;
    if(bird.velocity > max_fall) {
        bird.velocity = max_fall;
    }
   

/* #if !GAME_DEBUG_ALWAYS_SCORE */
/*     bird.y += BIRD_FALLING_RATE; */
/* #endif */

#if FLAPPY_DEBUG
    (void)debug_frame_counter;
    debug_frame_counter++;
#endif
}
