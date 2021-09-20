#include "corvid.h"
#include <SDL2/SDL.h>
#include <assert.h>

// corvid runtime structure
struct corvid {
    uint32_t magic;
    corvid_init_desc config;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* screen;
    SDL_Texture* screen_tex;
    SDL_Palette* palette;
    corvid_rect vbuf_rect;
    corvid_rect clip_rect;
};

struct corvid corvid;

// unsafe functions assume all inputs point to valid pixel coordinates
uint32_t* _corvid_get_pixel_unsafe(int32_t x, int32_t y);
uint32_t* _corvid_get_pixel(int32_t x, int32_t y);
void _corvid_set_pixel(int32_t x, int32_t y, uint32_t col);
void _corvid_scanline_unsafe(int32_t y, int32_t x0, int32_t x1, uint32_t col);

corvid_init_desc get_default_config()
{
    return (corvid_init_desc){
        .window = {.scale = 3.0f, .title = "corvid"},
        .video =
            {
                .buffer =
                    {
                        .width = 320,
                        .height = 240,
                    },
                .palette =
                    {
                        .count = 16,
                        .colors =
                            {
                                0x000000,
                                0x493c2b,
                                0xbe2633,
                                0xe06f8b,
                                0x9d9d9d,
                                0xa46422,
                                0xeb8931,
                                0xf7e26b,
                                0xffffff,
                                0x1b2632,
                                0x2f484e,
                                0x44891a,
                                0xa3ce27,
                                0x005784,
                                0x31a2f2,
                                0xb2dcef,
                            },
                    },
            },
    };
}

void corvid_init(const corvid_init_desc* desc)
{
    // corvid has already been initialized
    assert(corvid.magic == 0);

    const corvid_init_desc config = (desc) ? *desc : get_default_config();
    memcpy(&corvid.config, &config, sizeof(corvid_init_desc));

    corvid.magic = 0xB12DF00D;

    corvid.window = SDL_CreateWindow(
        "corvid",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        (int)(config.video.buffer.width * config.window.scale),
        (int)(config.video.buffer.height * config.window.scale),
        0);

    corvid.renderer = SDL_CreateRenderer(corvid.window, -1, SDL_RENDERER_ACCELERATED);

    corvid.screen = SDL_CreateRGBSurface(
        0, config.video.buffer.width, config.video.buffer.height, 32, 0, 0, 0, 0);
    SDL_SetSurfaceBlendMode(corvid.screen, SDL_BLENDMODE_NONE);

    SDL_Color colors[256] = {0};
    uint8_t color_count = config.video.palette.count;
    for (uint8_t i = 0; i < color_count; ++i) {
        uint32_t col = config.video.palette.colors[i];
        colors[i] = (SDL_Color){
            .r = (col & 0x00FF0000) >> 16,
            .g = (col & 0x0000FF00) >> 8,
            .b = (col & 0x000000FF),
            .a = 255,
        };
    }

    corvid.vbuf_rect = (corvid_rect){
        .x1 = config.video.buffer.width - 1,
        .y1 = config.video.buffer.height - 1,
    };

    corvid.clip_rect = corvid.vbuf_rect;

    SDL_SetPaletteColors(corvid.screen->format->palette, colors, 0, color_count);

    corvid.screen_tex = SDL_CreateTextureFromSurface(corvid.renderer, corvid.screen);
    SDL_SetTextureScaleMode(corvid.screen_tex, SDL_ScaleModeNearest);
}

void corvid_term()
{
    // corvid has not yet been initialized
    assert(corvid.magic == 0xB12DF00d);

    SDL_DestroyTexture(corvid.screen_tex);
    SDL_FreeSurface(corvid.screen);
    SDL_DestroyRenderer(corvid.renderer);
    SDL_DestroyWindow(corvid.window);

    memset(&corvid, 0, sizeof(struct corvid));
}

void corvid_present()
{
    // corvid has not yet been initialized
    assert(corvid.magic == 0xB12DF00d);

    SDL_LockSurface(corvid.screen);
    SDL_UpdateTexture(corvid.screen_tex, NULL, corvid.screen->pixels, corvid.screen->pitch);
    SDL_UnlockSurface(corvid.screen);

    SDL_RenderCopy(corvid.renderer, corvid.screen_tex, NULL, NULL);

    SDL_RenderPresent(corvid.renderer);
}

uint32_t _corvid_get_u32color(uint8_t color)
{
    color &= 0xF; // FIX ME
    return corvid.config.video.palette.colors[color];
}

// clipping
corvid_rect corvid_set_clip(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    return corvid.vbuf_rect;
}

corvid_rect corvid_reset_clip()
{
    return corvid.vbuf_rect;
}

corvid_rect corvid_get_clip()
{
    return corvid.vbuf_rect;
}

bool corvid_clip_point(int32_t x, int32_t y)
{
    corvid_rect* r = &corvid.vbuf_rect;
    return x >= r->x0 && x <= r->x1 && y >= r->y0 && y <= r->y1;
}

uint32_t _corvid_clip_line_code(const corvid_rect* r, int32_t x, int32_t y)
{
    uint32_t code = 0;
    if (x < r->x0) code += 1;
    if (x > r->x1) code += 2;
    if (y < r->y0) code += 4;
    if (y > r->y1) code += 8;
    return code;
}

bool _corvid_clip_line(const corvid_rect* r, int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1)
{
    struct point {
        int32_t x, y;
    };

    struct point a = {*x0, *y0};
    struct point b = {*x1, *y1};

    while (true) {
        uint32_t c0 = _corvid_clip_line_code(r, a.x, a.y);
        uint32_t c1 = _corvid_clip_line_code(r, b.x, b.y);

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

bool corvid_clip_line(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1)
{
    CV_ASSERT(x0 && y0 && x1 && y1, "");

    return _corvid_clip_line(&corvid.vbuf_rect, x0, y0, x1, y1);
}

bool _corvid_rect_intersect(const corvid_rect* a, const corvid_rect* b)
{
    return a->x0 <= b->x1 && a->x1 >= b->x0 && a->y0 <= b->y1 && a->y1 >= b->y0;
}

bool _corvid_clip_rect(const corvid_rect* src, corvid_rect* dst)
{
    if (!_corvid_rect_intersect(src, dst)) {
        return false;
    } else {
        dst->x0 = max(dst->x0, src->x0);
        dst->y0 = max(dst->y0, src->y0);
        dst->x1 = min(dst->x1, src->x1);
        dst->y1 = min(dst->y1, src->y1);
        return true;
    }
}

bool corvid_clip_rect(corvid_rect* rect)
{
    return _corvid_clip_rect(&corvid.vbuf_rect, rect);
}

void corvid_clear(uint8_t color)
{
    uint32_t u32color = _corvid_get_u32color(color);
    uint32_t len = corvid.screen->w * corvid.screen->h;
    for (uint32_t i = 0; i < len; ++i) {
        *((uint32_t*)corvid.screen->pixels + i) = u32color;
    }
}

void corvid_point(int32_t x, int32_t y, corvid_color color)
{
    if (corvid_clip_point(x, y)) {
        *_corvid_get_pixel_unsafe(x, y) = _corvid_get_u32color(color);
    }
}

// n is number of ints, expected to be a multiple of 2 (2 ints per point)
void corvid_points(int32_t* p, int32_t n, corvid_color color)
{
    CV_ASSERT(p && n > 0 && n % 2 == 0, "Invalid Argument");

    for (int32_t i = 0; i < n; i += 2) {
        int32_t x = p[i];
        int32_t y = p[i + 1];
        if (corvid_clip_point(x, y)) {
            *_corvid_get_pixel_unsafe(x, y) = _corvid_get_u32color(color);
        }
    }
}

void corvid_line_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color)
{
    corvid_rect r = {x0, y0, x1, y1};
    if (corvid_clip_rect(&r)) {
        uint32_t u32color = _corvid_get_u32color(color);

        int32_t left = min(r.x0, r.x1);
        int32_t right = max(r.x0, r.x1);
        int32_t top = min(r.y0, r.y1);
        int32_t bottom = max(r.y0, r.y1);

        _corvid_scanline_unsafe(top, left, right, u32color);
        _corvid_scanline_unsafe(bottom, left, right, u32color);

        if (bottom - top > 1) {
            for (int32_t y = top + 1; y <= bottom - 1; ++y) {
                *_corvid_get_pixel_unsafe(left, y) = u32color;
                *_corvid_get_pixel_unsafe(right, y) = u32color;
            }
        }
    }
}

void corvid_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color)
{
    corvid_rect r = {min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)};
    if (corvid_clip_rect(&r)) {
        int32_t bpp = corvid.screen->format->BytesPerPixel;
        uint32_t u32color = _corvid_get_u32color(color);
        for (int32_t y = r.y0; y <= r.y1; ++y) {
            _corvid_scanline_unsafe(y, r.x0, r.x1, u32color);
        }
    }
}

void corvid_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color)
{
    if (!corvid_clip_line(&x0, &y0, &x1, &y1)) {
        return;
    }

    uint32_t surfcol = _corvid_get_u32color(color);

    int32_t dx = abs(x1 - x0);
    int32_t dy = abs(y1 - y0);

    if (dy == 0) {
        _corvid_scanline_unsafe(y0, min(x0, x1), max(x0, x1), surfcol);
        return;
    }

    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = (dx > dy ? dx : -dy) / 2, e2;

    int32_t x = x0, y = y0;

    while (true) {
        // TODO: replace with branchless version when clipping work is done
        _corvid_set_pixel(x, y, surfcol);
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

void corvid_line_circ(int32_t x, int32_t y, int32_t r, uint8_t color)
{
    int32_t cx = x;
    int32_t cy = y;
    int32_t dx = r;
    int32_t dy = 0;
    int32_t err = 0;
    uint32_t u32color = _corvid_get_u32color(color);

    while (dx >= dy) {
        _corvid_set_pixel(cx + dx, cy + dy, u32color);
        _corvid_set_pixel(cx + dy, cy + dx, u32color);
        _corvid_set_pixel(cx + dx, cy - dy, u32color);
        _corvid_set_pixel(cx + dy, cy - dx, u32color);
        _corvid_set_pixel(cx - dx, cy + dy, u32color);
        _corvid_set_pixel(cx - dy, cy + dx, u32color);
        _corvid_set_pixel(cx - dx, cy - dy, u32color);
        _corvid_set_pixel(cx - dy, cy - dx, u32color);

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

uint32_t* _corvid_get_pixel(int32_t x, int32_t y)
{
    int bpp = corvid.screen->format->BytesPerPixel;
    return (uint32_t*)((uint8_t*)corvid.screen->pixels + y * corvid.screen->pitch + x * bpp);
}

void _corvid_set_pixel(int32_t x, int32_t y, uint32_t col)
{
    // *_corvid_get_pixel(x, y) = col;
    SDL_FillRect(corvid.screen, &(SDL_Rect){.x = x, .y = y, .w = 1, .h = 1}, col);
}

uint32_t* _corvid_get_pixel_unsafe(int32_t x, int32_t y)
{
    int bpp = corvid.screen->format->BytesPerPixel;
    return (uint32_t*)(((uint8_t*)corvid.screen->pixels) + y * corvid.screen->pitch + x * bpp);
}

void _corvid_scanline_unsafe(int32_t y, int32_t x0, int32_t x1, uint32_t col)
{
    uint32_t* start = _corvid_get_pixel_unsafe(x0, y);
    uint32_t* end = _corvid_get_pixel_unsafe(x1, y);
    for (uint32_t* pixel = start; pixel <= end; pixel++) {
        *pixel = col;
    }
}