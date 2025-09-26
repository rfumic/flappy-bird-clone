typedef enum {
    GDF_ALWAYS_SCORE     = (1 << 0), // Shortcut: 1
    GDF_PRIMITIVE_RENDER = (1 << 1), // Shortcut: 2
} GameDebugFlags;

global GameDebugFlags game_debug_flags = 0;

typedef struct {
    void  *memory;
    i32    width;
    i32    actual_height;
    i32    playable_height;
    u32    bytes_per_pixel;
    u32    pitch;
} GameScreenBuffer;

typedef struct {
    void *memory;
    i32  width;
    i32  height;
} BitmapAsset;

// NOTE: this should get passed by pointer
typedef struct {
    Arena asset_file_arena;
    Arena temp_arena;
    String executable_base_path;

    b32 jump_key_pressed;
    b32 new_game_started;
    u64 delta_time_ms;

    BitmapAsset pipe_bitmap;
    BitmapAsset test_bitmap;
} GameState;

static void DrawRectangle(GameScreenBuffer *buffer, i32 min_x, i32 min_y, 
                          i32 max_x, i32 max_y, u32 color) 
{
    min_x = Max(0, min_x);
    min_y = Max(0, min_y);
    max_x = Min(max_x, buffer->width);
    max_y = Min(max_y, buffer->actual_height);

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

/* TODO: move these to a state struct */
global i32 pipe_width = 0;
global i32 y_between_pipes = 0;

global PipePair *oldest_pipe;
global PipePair *current_pipe;
global PipePair *newest_pipe;

#define ARGB_To_RGBA(u32_pixel) (((u32_pixel) >> 24) | ((u32_pixel) << 8))

/* NOTE: Calculates the correct pixel, with linear blending 
 *       Assumes RGBA
*/
static inline u32 ComputePixel(u32 src_pixel, u32 dest_pixel)
{
    u32 result = 0;

    // Extract color values
    f32 alpha = (f32)(src_pixel & 0xFF) / 255.0f;

    f32 src_red   = (f32)((src_pixel >> 24) & 0xFF);
    f32 src_green = (f32)((src_pixel >> 16) & 0xFF);
    f32 src_blue  = (f32)((src_pixel >>  8) & 0xFF);

    f32 dest_red   = (f32)((dest_pixel >> 24) & 0xFF);
    f32 dest_green = (f32)((dest_pixel >> 16) & 0xFF);
    f32 dest_blue  = (f32)((dest_pixel >>  8) & 0xFF);

    // Calculate alpha blend
    f32 red   = (1.0f - alpha) * dest_red + alpha * src_red;
    f32 green = (1.0f - alpha) * dest_green + alpha * src_green;
    f32 blue  = (1.0f - alpha) * dest_blue + alpha * src_blue;

    // Put back together
    /* NOTE: the + 0.5f is for correct rounding */
    result  = (((u32)(red   + 0.5f) << 24) | 
               ((u32)(green + 0.5f) << 16) | 
               ((u32)(blue  + 0.5f) <<  8) |
               0xFF);

    return result;
}

static inline void DrawBitmap(BitmapAsset *bitmap, GameScreenBuffer *buffer, 
                              i32 dest_x, i32 dest_y)
{
    u32 *src = (u32 *)bitmap->memory;
    u32 *dest = (u32 *)buffer->memory;

    i32 src_w = bitmap->width;
    i32 src_h = bitmap->height;

    i32 start_x = 0;
    i32 start_y = 0;
    i32 end_x   = src_w;
    i32 end_y   = src_h;

    if (dest_x < 0) {
        start_x = -dest_x;
        dest_x = 0;
    }

    if (dest_y < 0) {
        start_y = -dest_y;
        dest_y = 0;
    }

    if (dest_x + (end_x - start_x) > buffer->width) {
        end_x = buffer->width - dest_x + start_x;
    }

    if (dest_y + (end_y - start_y) > buffer->playable_height) {
        end_y = buffer->playable_height - dest_y + start_y;
    }

    for (i32 y = start_y; y < end_y; y++) {
        for (i32 x = start_x; x < end_x; x++) {
            u32 src_pixel = src[y * src_w + x];
            u32 *dest_pixel_ptr =
                &dest[(dest_y + (y - start_y)) * buffer->width +
                (dest_x + (x - start_x))];

            *dest_pixel_ptr = ComputePixel(src_pixel, *dest_pixel_ptr);
        }
    }
}

static inline void DrawPipePair(GameState *game_state, PipePair *pipe_pair, 
                                GameScreenBuffer *buffer)
{
    b32 bitmap_available = game_state->pipe_bitmap.memory != NULL && 
                           game_state->pipe_bitmap.width > 0 && 
                           game_state->pipe_bitmap.height > 0;

    if(!(game_debug_flags & GDF_PRIMITIVE_RENDER) && bitmap_available) {
        BitmapAsset bitmap = game_state->pipe_bitmap;

        // draw top pipe
        i32 top_pipe_y = 
            pipe_pair->bottom_pipe_y - y_between_pipes - bitmap.height;
        DrawBitmap(&bitmap, buffer, pipe_pair->x, top_pipe_y);

        // draw bottom pipe
        DrawBitmap(&bitmap, buffer, pipe_pair->x, pipe_pair->bottom_pipe_y);

    } else {
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
                      0xFF00FFFF);

        // Draw bottom pipe
        DrawRectangle(buffer, 
                pipe_pair->x, 
                pipe_pair->bottom_pipe_y, 
                pipe_pair->x + pipe_width, 
                buffer->playable_height, 
                pipe_color);
    }
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
        result->bottom_pipe_y = GetRandomPipeY(screen_buffer->playable_height);

#if FLAPPY_DEBUG
        if(game_debug_flags & GDF_ALWAYS_SCORE) {
            result->bottom_pipe_y = 700;
        }
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
            (bird_x_end >= current_pipe->x && 
                bird_x_end <= current_pipe->x + pipe_width) ||
            (bird.x >= current_pipe->x && 
                bird.x <= current_pipe->x + pipe_width);
        
        b32 intersect_vertically_bottom = 
            bird.y + bird.height >= current_pipe->bottom_pipe_y;

        b32 intersect_vertically_top = 
            bird.y <= current_pipe->bottom_pipe_y - y_between_pipes;

        b32 intersect_vertically = intersect_vertically_bottom 
                                   || intersect_vertically_top;

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
        y_between_pipes = PercentOf(30, game_screen_buffer->playable_height);
        pipe_width      = PercentOf(17, game_screen_buffer->width);

        bird.width  = PercentOf(11, game_screen_buffer->width);
        bird.height = PercentOf(7, game_screen_buffer->playable_height);
        bird.y      = game_screen_buffer->playable_height / 2;
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

        game_state->jump_key_pressed = 0;
        game_state->new_game_started = 0;
    }
    
    for(i32 i = 0; 
        i < game_screen_buffer->width * game_screen_buffer->actual_height; 
        i++) {
        ((u32 *)game_screen_buffer->memory)[i] = 0xFFFF00FF;
    }

    if ((newest_pipe->x + pipe_width / 2) <= game_screen_buffer->width / 2) {
        oldest_pipe = current_pipe;

        current_pipe = newest_pipe;
        can_score = 1;

        newest_pipe = GetAvailablePipe(game_screen_buffer);
    }

    if(bird.y + bird.height >= game_screen_buffer->playable_height ||
       BirdCollidesWithCurrentPipe()) {
        game_state->new_game_started = true;
        return;
    }

    if(oldest_pipe) {
        if (oldest_pipe->x + pipe_width <= 0) {
            AddAvailablePipe(oldest_pipe);
        }

        DrawPipePair(game_state, oldest_pipe, game_screen_buffer);
        /* TODO: make pipe movement NOT resolution dependent */
        oldest_pipe->x -= PIPE_MOVEMENT_SPEED * game_state->delta_time_ms;
    }

    if(newest_pipe) {
        DrawPipePair(game_state, newest_pipe, game_screen_buffer);
        newest_pipe->x -= PIPE_MOVEMENT_SPEED * game_state->delta_time_ms;
    }

    if(current_pipe) {
        if(bird.x > current_pipe->x && can_score) {
            current_score++;
            can_score = 0;
            PlatformDebugPrint("score: %d", current_score);
        }

        DrawPipePair(game_state, current_pipe, game_screen_buffer);
        current_pipe->x -= PIPE_MOVEMENT_SPEED * game_state->delta_time_ms;
    }

    // NOTE: DrawGround
    {
        u32 color = 0xCC00FFFF;
        i32 start_y = game_screen_buffer->playable_height;
        i32 end_y = game_screen_buffer->actual_height;

        DrawRectangle(game_screen_buffer,
                      0, start_y,
                      game_screen_buffer->width, end_y,
                      color);
    }

    DrawBird(game_screen_buffer);

    /* TODO: Figure out how to get these speeds just right */

    if(game_state->jump_key_pressed) {
        bird.velocity = -(game_screen_buffer->playable_height * 0.6f);
        game_state->jump_key_pressed = false;
    }

    f32 delta_time = game_state->delta_time_ms / 1000.0f;

    // NOTE: this is gravity
    bird.velocity += (game_screen_buffer->playable_height 
                        * 0.002f * 30.0f * 30.0f) 
                     * delta_time;
    bird.y += bird.velocity * delta_time;

#if FLAPPY_DEBUG
    if(game_debug_flags & GDF_ALWAYS_SCORE) {
        bird.y = game_screen_buffer->playable_height / 2;
    }
#endif
    
    f32 max_fall = game_screen_buffer->playable_height * 0.03f * 30.0f;
    if(bird.velocity > max_fall) {
        bird.velocity = max_fall;
    }
   

#if FLAPPY_DEBUG
    (void)debug_frame_counter;
    debug_frame_counter++;
#endif
}

typedef struct __attribute__((packed)) {
    u16 file_type;
    u32 file_size;
    u16 reserved_1;
    u16 reserved_2;
    u32 bitmap_offset;
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bits_per_pixel;
} BitmapFormatHeader;

/* TODO: move to asset_loader_file? */
/* NOTE: this only loads BMPs created with aesprite, not generic */
static BitmapAsset LoadBitmapAsset(GameState *game_state,
                                   String asset_file_name)
{
    BitmapAsset result = {0};

    String exe_path = game_state->executable_base_path;
    String assets_folder = S("../assets/");

    // create full asset file path
    u8 *full_file_path = ArenaAllocArray(&game_state->temp_arena, u8, 
                                         exe_path.length + 
                                         assets_folder.length + 
                                         asset_file_name.length);
    i32 curr = 0;

    for(i32 i = 0; i < exe_path.length; i++) {
        full_file_path[curr++] = exe_path.data[i];
    }

    for(i32 i = 0; i < assets_folder.length; i++) {
        full_file_path[curr++] = assets_folder.data[i];
    }

    for(i32 i = 0; i < asset_file_name.length; i++) {
        full_file_path[curr++] = asset_file_name.data[i];
    }

    LoadedFile bitmap_file = 
        PlatformLoadEntireFile((char *)full_file_path, &game_state->asset_file_arena);

    if(bitmap_file.size != 0) {
        BitmapFormatHeader *header = 
            (BitmapFormatHeader *) bitmap_file.memory;

        u32 *pixels = (u32 *)((u8 *)bitmap_file.memory + header->bitmap_offset);


        // flip rows
        for(i32 row = 0; row < header->height / 2; row++) {
            for(i32 col = 0; col < header->width; col++) {
                u32 *first = &pixels[row * header->width + col];
                *first = ARGB_To_RGBA(*first);

                u32 *second = &pixels[(header->height - 1 - row) * 
                                      header->width + col];
                *second = ARGB_To_RGBA(*second);

                swap_u32(first, second);
            }
        }

        result.memory = pixels;
        result.width = header->width;
        result.height = header->height;
    }

    return result;
}

static inline void LoadAllAssets(GameState *game_state)
{
    game_state->pipe_bitmap = 
        LoadBitmapAsset(game_state, S("pipe_1.bmp"));

    game_state->test_bitmap = 
        LoadBitmapAsset(game_state, S("test_bitmap.bmp"));
}

typedef struct {
    GameScreenBuffer game_screen_buffer;
    GameState game_state;
} GameSetupResult;

static GameSetupResult GameSetup(void *main_window_buffer, void *usable_memory, u8 *game_base_path)
{
    GameSetupResult result;

    result.game_screen_buffer.memory          = main_window_buffer;
    result.game_screen_buffer.actual_height   = WINDOW_HEIGHT;
    result.game_screen_buffer.playable_height = PercentOf(91, WINDOW_HEIGHT);
    result.game_screen_buffer.width           = WINDOW_WIDTH;

    Arena asset_file_arena;
    ArenaInit(&asset_file_arena, ASSET_ARENA_SIZE, (u8 *)usable_memory);

    Arena temp_arena;
    ArenaInit(&temp_arena, TEMPORARY_ARENA_SIZE, (u8 *)&usable_memory[ASSET_ARENA_SIZE]);

    String executable_base_path = {
        .data = game_base_path,
        .length = StringLength(game_base_path)
    };

    result.game_state.new_game_started = true;
    result.game_state.delta_time_ms = 0;
    result.game_state.asset_file_arena = asset_file_arena;
    result.game_state.temp_arena = temp_arena;
    result.game_state.executable_base_path = executable_base_path;

    LoadAllAssets(&result.game_state);

    return result;
}
