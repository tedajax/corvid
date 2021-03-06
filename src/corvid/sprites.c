#include "sprites.h"
#include "stb_image.h"
#include "tx_rand.h"
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

enum {
    k_max_sprites = 128,
};

static const sprite_handle k_invalid = {0};

struct sprite {
    SDL_Surface* surface;
    SDL_Rect rect;
    stbi_uc* data;
};

static struct sprite sprites[k_max_sprites + 1] = {0};
static sprite_handle handles[k_max_sprites + 1] = {0};

sprite_handle _alloc_sprite()
{
    for (uint32_t i = 1; i <= k_max_sprites; ++i) {
        if (!sprite_handle_is_valid(handles[i])) {
            handles[i].handle = i;
            return handles[i];
        }
    }
    return k_invalid;
}

struct sprite* _get_sprite(sprite_handle sprite_h)
{
    CV_ASSERT(sprite_handle_is_valid(sprite_h), "Invalid sprite");

    return &sprites[sprite_h.handle];
}

bool sprite_handle_is_valid(sprite_handle sprite_h)
{
    return sprite_h.handle > 0 && sprite_h.handle <= k_max_sprites;
}

SDL_Surface* corvid_get_screen_surface();

sprite_handle load_image_as_sprite(const char* filename)
{
    sprite_handle sprite_h = _alloc_sprite();

    if (!sprite_handle_is_valid(sprite_h)) {
        return k_invalid;
    }

    struct sprite* sprite = _get_sprite(sprite_h);

    int w, h, chan;
    stbi_uc* data = stbi_load(filename, &w, &h, &chan, STBI_rgb_alpha);
    if (!data) {
        return k_invalid;
    }

    uint32_t rmask = 0x000000FF;
    uint32_t gmask = 0x0000FF00;
    uint32_t bmask = 0x00FF0000;
    uint32_t amask = 0xFF000000;

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        data, w, h, chan * 8, chan * w, SDL_PIXELFORMAT_ARGB8888);

    if (!surface) {
        stbi_image_free(data);
        return k_invalid;
    }

    SDL_Surface* screen = corvid_get_screen_surface();

    const bool optimize = true;
    if (optimize) {
        SDL_Surface* optimized = SDL_ConvertSurface(surface, screen->format, 0);

        if (optimized) {
            SDL_FreeSurface(surface);
            surface = optimized;
        }
    }

    sprite->surface = surface;
    sprite->rect = (SDL_Rect){.x = 0, .y = 0, .w = w, .h = h};
    sprite->data = data;

    return sprite_h;
}

SDL_Surface* get_sprite_surface(sprite_handle sprite_h)
{
    CV_ASSERT(sprite_handle_is_valid(sprite_h), "Invalid sprite handle");
    return _get_sprite(sprite_h)->surface;
}

SDL_Rect* get_sprite_rect(sprite_handle sprite_h)
{
    CV_ASSERT(sprite_handle_is_valid(sprite_h), "Invalid sprite handle");
    return &_get_sprite(sprite_h)->rect;
}