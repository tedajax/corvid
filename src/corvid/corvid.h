#pragma once

#include "corvid_system.h"

typedef uint8_t corvid_color;

typedef struct corvid_rect {
    int32_t x0, y0, x1, y1;
} corvid_rect;

typedef struct corvid_window_desc {
    const char* title;
    float scale;
} corvid_window_desc;

typedef struct corvid_video_desc {
    struct {
        uint32_t width;
        uint32_t height;
    } buffer;
    struct {
        uint32_t colors[256];
        uint8_t count;
    } palette;
} corvid_video_desc;

typedef struct corvid_init_desc {
    corvid_window_desc window;
    corvid_video_desc video;
} corvid_init_desc;

void corvid_init(const corvid_init_desc* desc);
void corvid_term();
void corvid_present();

/*
 * Draw State
 */

// clipping
corvid_rect corvid_set_clip(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
corvid_rect corvid_reset_clip();
corvid_rect corvid_get_clip();

bool corvid_clip_point(int32_t x, int32_t y);
bool corvid_clip_line(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1);
bool corvid_clip_rect(corvid_rect* rect);

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