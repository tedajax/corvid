#pragma once

#include "corvid_system.h"
#include "sprites.h"

typedef uint32_t corvid_color;

typedef struct corvid_rect {
    int32_t x0, y0, x1, y1;
} corvid_rect;

typedef struct corvid_window_desc {
    const char* title;
    float scale;
} corvid_window_desc;

typedef enum corvid_color_mode {
    CorvidColorMode_Palette,
    CorvidColorMode_RGB,
} corvid_color_mode;

typedef struct corvid_video_desc {
    void* window_ptr;
    struct {
        uint32_t width;
        uint32_t height;
    } buffer;
    corvid_color_mode color_mode;
    struct {
        corvid_color colors[256];
        uint8_t count;
    } palette;
} corvid_video_desc;

typedef struct corvid_init_desc {
    corvid_video_desc video;
} corvid_init_desc;
corvid_init_desc get_default_config();

void corvid_init(const corvid_init_desc* desc);
void corvid_term();

typedef struct SDL_Surface SDL_Surface;
SDL_Surface* corvid_get_screen_surface();

/*
 *
 * Drawing API
 *
 *
 */
void corvid_clear(corvid_color color);
void corvid_point(int32_t x, int32_t y, corvid_color color);
void corvid_points(int32_t* p, int32_t count, corvid_color color);
void corvid_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, corvid_color color);
void corvid_line_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, corvid_color color);
void corvid_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, corvid_color color);
void corvid_line_circ(int32_t x, int32_t y, int32_t r, corvid_color color);
void corvid_fill_circ(int32_t x, int32_t y, int32_t r, corvid_color color);
void corvid_draw_sprite(sprite_handle sprite_h, int32_t x, int32_t y);