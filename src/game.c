#define COLOR_BLACK 0x0F1B26FF
#define COLOR_WHITE 0xF5E8D1FF
#define COLOR_BLUE  0x20A5A6FF
#define COLOR_RED   0xDD5639FF

/* NOTE: 5x7 is assumed for each digit in font */
/* TODO: should this be dynamic? */
#define FONT_DIGIT_WIDTH  5
#define FONT_DIGIT_HEIGHT 7

typedef enum {
    GDF_ALWAYS_SCORE     = (1 << 0), // Shortcut: 1
    GDF_PRIMITIVE_RENDER = (1 << 1), // Shortcut: 2
    GDF_SHOW_FPS         = (1 << 2), // Shortcut: 3
} GameDebugFlags;

/* TODO: why isn't this in GameState? */
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
    u32  scale_factor;
} BitmapAsset;

typedef struct {
    i32 y;
    i32 x;
    i32 height;
    i32 width;
    f32 velocity;
    b32 is_falling;
} Bird;

typedef enum {
    CM_NONE = 0,
    CM_PLAYING,
    CM_GET_READY,
    CM_COUNT,
} CurrentMode;

// NOTE: this should get passed by pointer
typedef struct {
    Arena asset_file_arena;
    Arena temp_arena;
    String executable_base_path;
    GameDebugFlags game_debug_flags;
    u32 current_fps;

    b32 jump_key_pressed;
    b32 debug_score_increment_pressed;
    CurrentMode current_mode;
    u64 delta_time_ms;
    i32 current_score;
    b32 can_score;
    

    Bird bird;
    i32 pipe_width;
    i32 y_between_pipes;

    BitmapAsset pipe_bitmap;
    BitmapAsset digits_font_bitmap;
    BitmapAsset boomislav_bitmap;
    BitmapAsset test_bitmap;
} GameState;

static inline b32 IsBitmapAvailable(BitmapAsset bitmap)
{
    b32 result = bitmap.memory != NULL && 
                 bitmap.width > 0 && 
                 bitmap.height > 0;

    return result;
}

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

static inline void DrawBitmapEx(BitmapAsset *bitmap, GameScreenBuffer *buffer, 
                                i32 dest_x, i32 dest_y,
                                i32 src_x, i32 src_y,
                                i32 sprite_w, i32 sprite_h)
{
    u32 *src = (u32 *)bitmap->memory;
    u32 *dest = (u32 *)buffer->memory;

    i32 src_w = bitmap->width;

    i32 start_x = 0;
    i32 start_y = 0;
    i32 end_x   = sprite_w;
    i32 end_y   = sprite_h;

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
            u32 src_pixel = src[(src_y + y) * src_w + (src_x + x)];

            u32 *dest_pixel_ptr =
                &dest[(dest_y + (y - start_y)) * buffer->width +
                (dest_x + (x - start_x))];

            *dest_pixel_ptr = ComputePixel(src_pixel, *dest_pixel_ptr);
        }
    }
}

// NOTE: draws entire bitmap. DrawBitmapEx draws sections
static inline void DrawBitmap(BitmapAsset *bitmap, GameScreenBuffer *buffer, 
                              i32 dest_x, i32 dest_y)
{
    DrawBitmapEx(bitmap, buffer, dest_x, dest_y, 0, 0,
                 bitmap->width, bitmap->height);
}

static inline void DrawDigit(u32 digit, GameState *game_state,
                             GameScreenBuffer *game_screen_buffer,
                             i32 start_x, i32 start_y)
{
    Assert(digit >= 0 && digit < 10);

    const u32 scale_factor = game_state->digits_font_bitmap.scale_factor;

    i32 x_offset = digit * (FONT_DIGIT_WIDTH + 1) * scale_factor;

    DrawBitmapEx(&game_state->digits_font_bitmap, game_screen_buffer, 
                 start_x, start_y, x_offset, 0, 
                 FONT_DIGIT_WIDTH * scale_factor, 
                 FONT_DIGIT_HEIGHT * scale_factor);
}

static inline void DrawPipePair(GameState *game_state, PipePair *pipe_pair, 
                                GameScreenBuffer *buffer)
{
    if(!(game_state->game_debug_flags & GDF_PRIMITIVE_RENDER) && 
       IsBitmapAvailable(game_state->pipe_bitmap)) {
        BitmapAsset bitmap = game_state->pipe_bitmap;

        // draw top pipe
        i32 top_pipe_y = 
            pipe_pair->bottom_pipe_y - game_state->y_between_pipes - bitmap.height;
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
                      pipe_pair->x + game_state->pipe_width, 
                      (pipe_pair->bottom_pipe_y - game_state->y_between_pipes), 
                      0xFF00FFFF);

        // Draw bottom pipe
        DrawRectangle(buffer, 
                pipe_pair->x, 
                pipe_pair->bottom_pipe_y, 
                pipe_pair->x + game_state->pipe_width, 
                buffer->playable_height, 
                pipe_color);
    }
}

static inline i32 GetRandomPipeY(GameState *game_state, i32 game_screen_height) {
    i32 result;

    i32 y_margin_top    = PercentOf(10, game_screen_height) + 
                          game_state->y_between_pipes;
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

static inline PipePair *GetAvailablePipe(GameScreenBuffer *screen_buffer,
                                         GameState *game_state) 
{
    Assert(pipe_queue.count >= 0);

    PipePair *result = NULL;

    if(pipe_queue.count > 0) {
        result = pipe_queue.pipes[0];
        result->x = screen_buffer->width;
        result->bottom_pipe_y = GetRandomPipeY(game_state, screen_buffer->playable_height);

#if FLAPPY_DEBUG
        if(game_state->game_debug_flags & GDF_ALWAYS_SCORE) {
            result->bottom_pipe_y = (screen_buffer->playable_height / 2) + (game_state->y_between_pipes / 2);
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

#define GetPipeMovementSpeed(window_width) (0.0004340277778 * window_width / 2)

static void DrawBird(GameState *game_state, GameScreenBuffer *game_screen_buffer) 
{
    u32 bird_color  = COLOR_WHITE; 
    Bird bird = game_state->bird;
    b32 is_falling = bird.is_falling == true ? 1 : 0;
    BitmapAsset bird_bitmap = game_state->boomislav_bitmap;

    if((game_state->game_debug_flags & GDF_PRIMITIVE_RENDER) ||
        !IsBitmapAvailable(bird_bitmap)) {

        DrawRectangle(game_screen_buffer, bird.x, bird.y, 
                bird.x + bird.width, bird.y + bird.height,
                bird_color);

    } else {

        u32 scale = bird_bitmap.scale_factor;
        i32 x_offset = is_falling * (bird.width) * scale;

        DrawBitmapEx(&bird_bitmap, game_screen_buffer,
                     bird.x, bird.y, x_offset, 0,
                     bird.width * scale,
                     bird.height * scale);
    }
}

static inline b32 BirdCollidesWithCurrentPipe(GameState *game_state)
{
    b32 result = false;
    Bird bird = game_state->bird;

    if(current_pipe) {
        i32 bird_x_end = bird.x + bird.width;

        b32 intersect_horizontally = 
            (bird_x_end >= current_pipe->x && 
                bird_x_end <= current_pipe->x + game_state->pipe_width) ||
            (bird.x >= current_pipe->x && 
                bird.x <= current_pipe->x + game_state->pipe_width);
        
        b32 intersect_vertically_bottom = 
            bird.y + bird.height >= current_pipe->bottom_pipe_y;

        b32 intersect_vertically_top = 
            bird.y <= current_pipe->bottom_pipe_y - game_state->y_between_pipes;

        b32 intersect_vertically = intersect_vertically_bottom 
                                   || intersect_vertically_top;

        result = intersect_horizontally && intersect_vertically;
    }

    return result;
}

static inline void DrawBackground(GameScreenBuffer *game_screen_buffer)
{
    for(i32 i = 0; 
        i < game_screen_buffer->width * game_screen_buffer->actual_height; 
        i++) {
        ((u32 *)game_screen_buffer->memory)[i] = COLOR_BLACK;
    }

}

static void MoveBird(GameScreenBuffer *game_screen_buffer,
                     GameState *game_state)
{
    /* TODO: Figure out how to get these speeds just right */
    if(game_state->jump_key_pressed) {
        game_state->bird.velocity = -(game_screen_buffer->playable_height * 0.6f);
        game_state->jump_key_pressed = false;
    }

    f32 delta_time = game_state->delta_time_ms / 1000.0f;

    // NOTE: this is gravity
    game_state->bird.velocity += (game_screen_buffer->playable_height 
                                  * 0.002f * 30.0f * 30.0f) 
                                  * delta_time;

    i32 delta = game_state->bird.velocity * delta_time;
    game_state->bird.y += delta;
    game_state->bird.is_falling = delta >= 0;

#if FLAPPY_DEBUG
    if(game_state->game_debug_flags & GDF_ALWAYS_SCORE) {
        game_state->bird.y = game_screen_buffer->playable_height / 2;
    }
#endif
    
    f32 max_fall = game_screen_buffer->playable_height * 0.03f * 30.0f;
    if(game_state->bird.velocity > max_fall) {
        game_state->bird.velocity = max_fall;
    }
}

static inline void DrawNumber(u32 number, i32 x, i32 y,
                             GameState *game_state, 
                             GameScreenBuffer *game_screen_buffer)
{
    /* NOTE: max is 9999 */
    u32 *digits = ArenaAllocArray(&game_state->temp_arena, u32, 4);

    u32 num_of_digits = 0;
    if(number == 0) {
        digits[num_of_digits++] = 0;
    } else {
        while(number)
        {
            digits[num_of_digits++] = number % 10;
            number /= 10;
        }
    }


    i32 scale_factor = game_state->digits_font_bitmap.scale_factor;
    i32 digit_width = (FONT_DIGIT_WIDTH * scale_factor);
    i32 total_width = num_of_digits * digit_width;
    i32 start_x = x - total_width / 2;

    for(i32 i = num_of_digits - 1, j = 0; i >= 0; i--, j++) {
        u32 digit = digits[i];
        DrawDigit(digit, game_state, game_screen_buffer,
                start_x + j * digit_width, y);
    }

}
static inline void DrawScore(u32 score, GameState *game_state, 
                             GameScreenBuffer *game_screen_buffer)
{
    i32 x = game_screen_buffer->width / 2;
    i32 y = PercentOf(game_screen_buffer->playable_height, 10);
    DrawNumber(score, x, y, game_state, game_screen_buffer);
}

/* TODO: change stupid name */
static void PlayGame(GameScreenBuffer *game_screen_buffer,
                     GameState *game_state,
                     b32 new_game)
{
    if(new_game) {
        game_state->current_score = 0;
        game_state->can_score = true;

        pipes[0] = (PipePair){0, 550};
        pipes[1] = (PipePair){0, 550};
        pipes[2] = (PipePair){0, 550};

        pipe_queue.pipes[0] = &pipes[0];
        pipe_queue.pipes[1] = &pipes[1];
        pipe_queue.pipes[2] = &pipes[2];
        pipe_queue.count = 3;


        oldest_pipe  = NULL;
        current_pipe = NULL;
        newest_pipe  = GetAvailablePipe(game_screen_buffer, game_state);

        game_state->jump_key_pressed = false;
        game_state->current_mode = CM_PLAYING;
    }

#if FLAPPY_DEBUG
    if(game_state->debug_score_increment_pressed) {
        game_state->current_score++;
        game_state->debug_score_increment_pressed = false;
    }
#endif
    
    if(game_state->bird.y + game_state->bird.height >= game_screen_buffer->playable_height ||
       BirdCollidesWithCurrentPipe(game_state)) {
        game_state->current_mode = CM_GET_READY;
        return;
    }
    
    if ((newest_pipe->x + game_state->pipe_width / 2) 
         <= game_screen_buffer->width / 2) {
        oldest_pipe = current_pipe;

        current_pipe = newest_pipe;
        game_state->can_score = true;

        newest_pipe = GetAvailablePipe(game_screen_buffer, game_state);
    }

    if(oldest_pipe) {
        if (oldest_pipe->x + game_state->pipe_width <= 0) {
            AddAvailablePipe(oldest_pipe);
        }

        DrawPipePair(game_state, oldest_pipe, game_screen_buffer);
        /* TODO: pipe movement still not FPS independent */
        oldest_pipe->x -= GetPipeMovementSpeed(game_screen_buffer->width) * game_state->delta_time_ms;
    }

    if(newest_pipe) {
        DrawPipePair(game_state, newest_pipe, game_screen_buffer);
        newest_pipe->x -= GetPipeMovementSpeed(game_screen_buffer->width) * game_state->delta_time_ms;
    }

    if(current_pipe) {
        if(game_state->bird.x > current_pipe->x && game_state->can_score) {
            game_state->current_score++;
            game_state->can_score = false;
        }

        DrawPipePair(game_state, current_pipe, game_screen_buffer);
        current_pipe->x -= GetPipeMovementSpeed(game_screen_buffer->width) * game_state->delta_time_ms;
    }

    MoveBird(game_screen_buffer, game_state);
    DrawScore(game_state->current_score, game_state, game_screen_buffer);
}

static inline void ResetBird(GameScreenBuffer *game_screen_buffer,
                             GameState *game_state)
{
    game_state->bird.velocity = 0;
    game_state->bird.y = game_screen_buffer->playable_height / 2;
    game_state->bird.x = (PercentOf(28.67, game_screen_buffer->width) 
                          - (game_state->bird.width / 2));
}

static void GameUpdateAndRender(GameScreenBuffer *game_screen_buffer, 
                                GameState *game_state) 
{
#if FLAPPY_DEBUG
    static u32 debug_frame_counter = 0;
#endif
    DrawBackground(game_screen_buffer);
    
    b32 new_game = false;
    if(game_state->current_mode == CM_GET_READY) {
        ResetBird(game_screen_buffer, game_state);
        if(game_state->jump_key_pressed) {
            MoveBird(game_screen_buffer, game_state);
            game_state->current_mode = CM_PLAYING;
            new_game = true;
        }
    }


    if(game_state->current_mode == CM_PLAYING) {
        PlayGame(game_screen_buffer, game_state, new_game);
    }


    // NOTE: DrawGround
    {
        u32 color = COLOR_WHITE;
        i32 start_y = game_screen_buffer->playable_height;
        i32 end_y = game_screen_buffer->actual_height;

        DrawRectangle(game_screen_buffer,
                      0, start_y,
                      game_screen_buffer->width, end_y,
                      color);
    }

    DrawBird(game_state, game_screen_buffer);

#if FLAPPY_DEBUG
    if(game_state->game_debug_flags & GDF_SHOW_FPS) {
        DrawRectangle(game_screen_buffer,
                0, game_screen_buffer->playable_height - 40,
                75, game_screen_buffer->playable_height,
                COLOR_BLUE);
        DrawNumber(game_state->current_fps, 
                   20, game_screen_buffer->playable_height - 30, 
                   game_state, game_screen_buffer);

    }

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
/* NOTE: this only loads BMPs created with aesprite, not generic
 *       that have been exported after a flatten
 *
 * NOTE: if replacement color is 0, uses original
 *       else replaces all pixels that have alpha FF
 */
static BitmapAsset LoadBitmapAsset(GameState *game_state,
                                   String asset_file_name,
                                   u32 scale_factor,
                                   u32 replacement_color)
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
        PlatformLoadEntireFile((char *)full_file_path, &game_state->temp_arena);

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

        // scaling
        i32 new_width = header->width * scale_factor;
        i32 new_height = header->height * scale_factor;
        u32 scaled_bitmap_size = new_height * new_width;
        u32 *scaled_pixels = ArenaAllocArray(&game_state->asset_file_arena, u32, 
                                             scaled_bitmap_size); 
        
        for(i32 row = 0; row < new_height;  row++) {
            u32 original_row = (u32)(row / scale_factor);
            u32 original_row_start_idx = original_row * header->width;
            u32 scaled_row_start_idx = row * new_width;

            for(i32 col = 0; col < new_width;  col++) {
                u32 original_col = (u32)(col / scale_factor);
                u32 original_idx = original_row_start_idx + original_col;

                u32 pixel = pixels[original_idx];

                if(replacement_color > 0 && pixel & 0x000000FF) {
                    pixel = replacement_color;
                }
                scaled_pixels[scaled_row_start_idx + col] = pixel;
            }
        }


        result.memory = scaled_pixels;
        result.width = new_width;
        result.height = new_height;
        result.scale_factor = scale_factor;
    }

    return result;
}

static inline void LoadAllAssets(GameState *game_state)
{
    game_state->pipe_bitmap = 
        LoadBitmapAsset(game_state, S("pipe_2.bmp"), 1, 0);

    game_state->boomislav_bitmap = 
        LoadBitmapAsset(game_state, S("boomislav.bmp"), 1, COLOR_WHITE);

    game_state->digits_font_bitmap = 
        LoadBitmapAsset(game_state, S("digits_font.bmp"), 3, COLOR_RED);

    game_state->test_bitmap = 
        LoadBitmapAsset(game_state, S("test_bitmap.bmp"), 1, 0);
}

typedef struct {
    GameScreenBuffer game_screen_buffer;
    GameState game_state;
} GameSetupResult;

static GameSetupResult GameSetup(void *main_window_buffer, void *usable_memory, u8 *game_base_path)
{
    GameSetupResult result = {0};

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

    result.game_state.can_score = true;
    result.game_state.current_mode = CM_GET_READY;
    result.game_state.delta_time_ms = 0;
    result.game_state.asset_file_arena = asset_file_arena;
    result.game_state.temp_arena = temp_arena;
    result.game_state.executable_base_path = executable_base_path;
    result.game_state.y_between_pipes = PercentOf(30, result.game_screen_buffer.playable_height);
    result.game_state.pipe_width = PercentOf(17, result.game_screen_buffer.width);

    /* result.game_state.bird.width = PercentOf(11, result.game_screen_buffer.width); */
    result.game_state.bird.width = 40;
    /* result.game_state.bird.height = PercentOf(7, result.game_screen_buffer.playable_height); */
    result.game_state.bird.height = 40;

    result.game_state.bird.y = result.game_screen_buffer.playable_height / 2;
    result.game_state.bird.x = (PercentOf(28.67, result.game_screen_buffer.width) 
                                - (result.game_state.bird.width / 2));
    result.game_state.bird.velocity = 0;


    LoadAllAssets(&result.game_state);

    return result;
}
