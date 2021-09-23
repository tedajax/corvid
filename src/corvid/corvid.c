#include "corvid.h"
#include <SDL2/SDL.h>
#include <assert.h>

corvid_init_desc get_default_config()
{
    return (corvid_init_desc){
        //.window = {.scale = 3.0f, .title = "corvid"},
        .video =
            {
                .buffer =
                    {
                        .width = 320,
                        .height = 240,
                    },
                .color_mode = CorvidColorMode_Palette,
                .palette =
                    {
                        .count = 32,
                        .colors =
                            {
                                0x000000, 0x493c2b, 0xbe2633, 0xe06f8b, 0xa46422, 0xeb8931,
                                0xf7e26b, 0xffffff, 0x9d9d9d, 0x2f484e, 0x1b2632, 0x44891a,
                                0xa3ce27, 0x005784, 0x31a2f2, 0xb2dcef, 0x342a97, 0x656d71,
                                0xcccccc, 0x732930, 0xcb43a7, 0x524f40, 0xad9d33, 0xec4700,
                                0xfab40b, 0x115e33, 0x14807e, 0x15c2a5, 0x225af6, 0x9964f9,
                                0xf78ed6, 0xf4b990,
                            },
                    },
            },
    };
}

// TODO: make dynamic based on palette size
#define CORVID_COLOR_MASK 0x1F

// corvid runtime structure
struct corvid {
    uint32_t magic;
    corvid_init_desc config;
    SDL_Surface* screen;
    SDL_Palette* palette;
    corvid_rect vbuf_rect;
    corvid_rect clip_rect;
};

struct corvid corvid;

corvid_color _corvid_get_u32color(corvid_color color);

bool _clip_point(const corvid_rect* r, int32_t x, int32_t y);
uint32_t _calc_clip_line_region_code(const corvid_rect* r, int32_t x, int32_t y);
bool _clip_line(const corvid_rect* r, int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1);
bool _clip_rect(const corvid_rect* src, corvid_rect* dst);

// vbuf refers to the video buffer
// currently stored as an SDL_Surface (struct corvid -> screen)

// These vbuf calls mirror the public drawing api but accept raw uint32_t colors.
// The idea is that these functions perform the actual work of updating the video buffer directly
// whereas the public functions may use different color modes/structures or refer to alternative
// draw states.
// _vbuf clip functions clip against internal video buffer rectangle (0,0 -> width,height)
bool _vbuf_clip_point(int32_t x, int32_t y);
bool _vbuf_clip_line(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1);
bool _vbuf_clip_rect(corvid_rect* rect);

void _vbuf_clear(uint32_t u32color);
void _vbuf_point(int32_t x, int32_t y, uint32_t u32color);
void _vbuf_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t u32color);
void _vbuf_line_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t u32color);
void _vbuf_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t u32color);
void _vbuf_line_circ(int32_t x, int32_t y, int32_t r, uint32_t u32color);
void _vbuf_fill_circ(int32_t x, int32_t y, int32_t r, uint32_t u32color);

void _vbuf_scanline(int32_t y, int32_t x0, int32_t x1, uint32_t u32color);

// unsafe functions assume all inputs point to valid pixel coordinates
uint32_t* _vbuf_get_pixel_addr_unsafe(int32_t x, int32_t y);
void _vbuf_point_unsafe(int32_t x, int32_t y, uint32_t u32color);
void _vbuf_scanline_unsafe(int32_t y, int32_t x0, int32_t x1, uint32_t u32color);

// Public API Implementation
void corvid_init(const corvid_init_desc* desc)
{
    // corvid has already been initialized
    assert(corvid.magic == 0);

    const corvid_init_desc config = (desc) ? *desc : get_default_config();
    memcpy(&corvid.config, &config, sizeof(corvid_init_desc));

    corvid.magic = 0xB12DF00D;

    corvid.screen = SDL_CreateRGBSurface(
        0,
        config.video.buffer.width,
        config.video.buffer.height,
        32,
        0xff000000,
        0x00ff0000,
        0x0000ff00,
        0x000000ff);
    // SDL_SetSurfaceBlendMode(corvid.screen, SDL_BLENDMODE_NONE);

    SDL_Color colors[256] = {0};
    uint8_t color_count = config.video.palette.count;
    for (uint8_t i = 0; i < color_count; ++i) {
        uint32_t col = config.video.palette.colors[i];
        colors[i] = (SDL_Color){
            .r = (col & 0x00FF0000) >> 16,
            .g = (col & 0x0000FF00) >> 8,
            .b = (col & 0x000000FF),
            .a = 0xFF,
        };
    }

    corvid.vbuf_rect = (corvid_rect){
        .x1 = config.video.buffer.width - 1,
        .y1 = config.video.buffer.height - 1,
    };

    corvid.clip_rect = corvid.vbuf_rect;

    SDL_SetPaletteColors(corvid.screen->format->palette, colors, 0, color_count);
}

void corvid_term()
{
    // corvid has not yet been initialized
    assert(corvid.magic == 0xB12DF00d);

    SDL_FreeSurface(corvid.screen);

    memset(&corvid, 0, sizeof(struct corvid));
}

SDL_Surface* corvid_get_screen_surface()
{
    // corvid has not yet been initialized
    assert(corvid.magic == 0xB12DF00d);

    return corvid.screen;
}

void corvid_clear(corvid_color color)
{
    uint32_t u32color = _corvid_get_u32color(color);
    _vbuf_clear(_corvid_get_u32color(color));
}

void corvid_point(int32_t x, int32_t y, corvid_color color)
{
    _vbuf_point(x, y, _corvid_get_u32color(color));
}

// n is number of ints, expected to be a multiple of 2 (2 ints per point)
void corvid_points(int32_t* p, int32_t n, corvid_color color)
{
    CV_ASSERT(p && n > 0 && n % 2 == 0, "Invalid Argument");

    uint32_t u32color = _corvid_get_u32color(color);
    for (int32_t i = 0; i < n; i += 2) {
        int32_t x = p[i];
        int32_t y = p[i + 1];
        _vbuf_point(x, y, u32color);
    }
}

void corvid_line_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, corvid_color color)
{
    _vbuf_line_rect(x0, y0, x1, y1, _corvid_get_u32color(color));
}

void corvid_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, corvid_color color)
{
    _vbuf_fill_rect(x0, y0, x1, y1, _corvid_get_u32color(color));
}

void corvid_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, corvid_color color)
{
    _vbuf_line(x0, y0, x1, y1, _corvid_get_u32color(color));
}

void corvid_line_circ(int32_t x, int32_t y, int32_t r, corvid_color color)
{
    _vbuf_line_circ(x, y, r, _corvid_get_u32color(color));
}

void corvid_fill_circ(int32_t x, int32_t y, int32_t r, corvid_color color)
{
    _vbuf_fill_circ(x, y, r, _corvid_get_u32color(color));
}

void corvid_draw_sprite(sprite_handle sprite_h, int32_t x, int32_t y)
{
    SDL_Surface* surface = get_sprite_surface(sprite_h);

    SDL_Rect* rect = get_sprite_rect(sprite_h);

    SDL_Rect dst_rect = (SDL_Rect){
        .x = x,
        .y = y,
        .w = rect->w,
        .h = rect->h,
    };

    SDL_BlitSurface(surface, rect, corvid.screen, &dst_rect);
}

// Private API Implementation
corvid_color _corvid_get_u32color(corvid_color color)
{
    if (corvid.config.video.color_mode == CorvidColorMode_RGB) {
        return color;
    } else {
        color &= CORVID_COLOR_MASK; // FIX ME
        return corvid.config.video.palette.colors[color];
    }
}

// video buffer clipping
bool _vbuf_clip_point(int32_t x, int32_t y)
{
    return _clip_point(&corvid.vbuf_rect, x, y);
}

bool _vbuf_clip_line(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1)
{
    return _clip_line(&corvid.vbuf_rect, x0, y0, x1, y1);
}

bool _vbuf_clip_rect(corvid_rect* rect)
{
    return _clip_rect(&corvid.vbuf_rect, rect);
}

// video buffer draw functions
void _vbuf_clear(uint32_t u32color)
{
    uint32_t len = corvid.screen->w * corvid.screen->h;
    for (uint32_t i = 0; i < len; ++i) {
        *((uint32_t*)corvid.screen->pixels + i) = u32color;
    }
}

void _vbuf_point(int32_t x, int32_t y, uint32_t u32color)
{
    if (_vbuf_clip_point(x, y)) {
        _vbuf_point_unsafe(x, y, u32color);
    }
}

void _vbuf_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t u32color)
{
    if (!_vbuf_clip_line(&x0, &y0, &x1, &y1)) {
        return;
    }

    int32_t dx = abs(x1 - x0);
    int32_t dy = abs(y1 - y0);

    if (dy == 0) {
        _vbuf_scanline_unsafe(y0, min(x0, x1), max(x0, x1), u32color);
        return;
    }

    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = (dx > dy ? dx : -dy) / 2, e2;

    int32_t x = x0, y = y0;

    while (true) {
        _vbuf_point_unsafe(x, y, u32color);

        if (x == x1 && y == y1) {
            break;
        }
        e2 = err * 2;
        if (e2 > -dx) {
            err -= dy;
            x += sx;
        }
        if (e2 < dy) {
            err += dx;
            y += sy;
        }
    }
}

void _vbuf_line_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t u32color)
{
    corvid_rect r = {x0, y0, x1, y1};
    if (_vbuf_clip_rect(&r)) {
        int32_t left = min(r.x0, r.x1);
        int32_t right = max(r.x0, r.x1);
        int32_t top = min(r.y0, r.y1);
        int32_t bottom = max(r.y0, r.y1);

        _vbuf_scanline_unsafe(top, left, right, u32color);
        _vbuf_scanline_unsafe(bottom, left, right, u32color);

        if (bottom - top > 1) {
            for (int32_t y = top + 1; y <= bottom - 1; ++y) {
                *_vbuf_get_pixel_addr_unsafe(left, y) = u32color;
                *_vbuf_get_pixel_addr_unsafe(right, y) = u32color;
            }
        }
    }
}

void _vbuf_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t u32color)
{
    corvid_rect r = {min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)};
    if (_vbuf_clip_rect(&r)) {
        for (int32_t y = r.y0; y <= r.y1; ++y) {
            _vbuf_scanline_unsafe(y, r.x0, r.x1, u32color);
        }
    }
}

void _vbuf_line_circ(int32_t x, int32_t y, int32_t r, uint32_t u32color)
{
    corvid_rect rect = {x - r, y - r, x + r, y + r};
    if (!_vbuf_clip_rect(&rect)) {
        return;
    }

    int32_t dx = r;
    int32_t dy = 0;
    int32_t err = 0;

    while (dx >= dy) {
        _vbuf_point(x + dx, y + dy, u32color);
        _vbuf_point(x + dy, y + dx, u32color);
        _vbuf_point(x + dx, y - dy, u32color);
        _vbuf_point(x + dy, y - dx, u32color);
        _vbuf_point(x - dx, y + dy, u32color);
        _vbuf_point(x - dy, y + dx, u32color);
        _vbuf_point(x - dx, y - dy, u32color);
        _vbuf_point(x - dy, y - dx, u32color);

        if (err <= 0) {
            ++dy;
            err += 2 * dy + 1;
        }
        if (err > 0) {
            --dx;
            err -= 2 * dx + 1;
        }
    }
}

void _vbuf_fill_circ(int32_t x, int32_t y, int32_t r, uint32_t u32color)
{
    corvid_rect rect = {x - r, y - r, x + r, y + r};
    if (!_vbuf_clip_rect(&rect)) {
        return;
    }

    int32_t dx = r;
    int32_t dy = 0;
    int32_t err = 0;

    while (dx >= dy) {
        int32_t yy[4] = {y - dy, y - dx, y + dy, y + dx};
        _vbuf_scanline(y - dy, x - dx, x + dx, u32color);
        _vbuf_scanline(y - dx, x - dy, x + dy, u32color);
        _vbuf_scanline(y + dy, x - dx, x + dx, u32color);
        _vbuf_scanline(y + dx, x - dy, x + dy, u32color);

        if (err <= 0) {
            ++dy;
            err += 2 * dy + 1;
        }

        if (err > 0) {
            --dx;
            err -= 2 * dx + 1;
        }
    }
}

uint32_t* _vbuf_get_pixel_addr_unsafe(int32_t x, int32_t y)
{
    int bpp = corvid.screen->format->BytesPerPixel;
    return (uint32_t*)(((uint8_t*)corvid.screen->pixels) + y * corvid.screen->pitch + x * bpp);
}

void _vbuf_point_unsafe(int32_t x, int32_t y, uint32_t u32color)
{
    *_vbuf_get_pixel_addr_unsafe(x, y) = u32color;
}

void _vbuf_scanline(int32_t y, int32_t x0, int32_t x1, uint32_t u32color)
{
    if (y < corvid.vbuf_rect.y0 || y > corvid.vbuf_rect.y1) {
        return;
    }

    if (x0 > x1) {
        int32_t t = x0;
        x0 = x1;
        x1 = t;
    }

    x0 = max(x0, corvid.vbuf_rect.x0);
    x1 = min(x1, corvid.vbuf_rect.x1);

    _vbuf_scanline_unsafe(y, x0, x1, u32color);
}

void _vbuf_scanline_unsafe(int32_t y, int32_t x0, int32_t x1, uint32_t u32color)
{
    uint32_t* start = _vbuf_get_pixel_addr_unsafe(x0, y);
    uint32_t* end = _vbuf_get_pixel_addr_unsafe(x1, y);
    for (uint32_t* pixel = start; pixel <= end; pixel++) {
        *pixel = u32color;
    }
}

// Clipping Utilities
bool _clip_point(const corvid_rect* r, int32_t x, int32_t y)
{
    return x >= r->x0 && x <= r->x1 && y >= r->y0 && y <= r->y1;
}

uint32_t _calc_clip_line_region_code(const corvid_rect* r, int32_t x, int32_t y)
{
    uint32_t code = 0;
    if (x < r->x0) code += 1;
    if (x > r->x1) code += 2;
    if (y < r->y0) code += 4;
    if (y > r->y1) code += 8;
    return code;
}

bool _clip_line(const corvid_rect* r, int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1)
{
    struct point {
        int32_t x, y;
    };

    struct point a = {*x0, *y0};
    struct point b = {*x1, *y1};

    while (true) {
        uint32_t c0 = _calc_clip_line_region_code(r, a.x, a.y);
        uint32_t c1 = _calc_clip_line_region_code(r, b.x, b.y);

        if (c0 == 0 && c1 == 0) {
            *x0 = a.x;
            *y0 = a.y;
            *x1 = b.x;
            *y1 = b.y;
            return true;
        } else {
            if ((c0 & c1) != 0) {
                return false;
            } else {
                struct point* out = (c0 == 0) ? &b : &a;
                uint32_t code = (c0 == 0) ? c1 : c0;

                if ((code & 4) != 0) {
                    // top
                    out->x = a.x + (b.x - a.x) * (r->y0 - a.y) / (b.y - a.y);
                    out->y = r->y0;
                } else if ((code & 8) != 0) {
                    // bottom
                    out->x = a.x + (b.x - a.x) * (r->y1 - a.y) / (b.y - a.y);
                    out->y = r->y1;
                } else if ((code & 2) != 0) {
                    // right
                    out->y = a.y + (b.y - a.y) * (r->x1 - a.x) / (b.x - a.x);
                    out->x = r->x1;
                } else if ((code & 1) != 0) {
                    // left
                    out->y = a.y + (b.y - a.y) * (r->x0 - a.x) / (b.x - a.x);
                    out->x = r->x0;
                }
            }
        }
    }
}

bool _rect_intersect(const corvid_rect* a, const corvid_rect* b)
{
    return a->x0 <= b->x1 && a->x1 >= b->x0 && a->y0 <= b->y1 && a->y1 >= b->y0;
}

bool _clip_rect(const corvid_rect* src, corvid_rect* dst)
{
    if (!_rect_intersect(src, dst)) {
        return false;
    } else {
        dst->x0 = max(dst->x0, src->x0);
        dst->y0 = max(dst->y0, src->y0);
        dst->x1 = min(dst->x1, src->x1);
        dst->y1 = min(dst->y1, src->y1);
        return true;
    }
}
