#include "sprites.h"
#include "stb_image.h"
#include "tx_rand.h"
#include <SDL2/SDL.h>

enum {
    k_max_sprites = 128,
};

const sprite_handle k_invalid = {0};

struct sprite {
    SDL_Surface* surface;
    SDL_Rect rect;
    stbi_uc* data;
};

struct sprite sprites[k_max_sprites + 1] = {0};
sprite_handle handles[k_max_sprites + 1] = {0};

sprite_handle _alloc_sprite()
{
    for (int i = 1; i <= k_max_sprites; ++i) {
        if (!sprite_handle_is_valid(handles[i])) {
            handles[i].handle = (uint32_t)i;
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

    uint32_t masks[4] = {
        0x000000FF,
        0x0000FF00,
        0x00FF0000,
        0xFF000000,
    };

    SDL_Surface* surface = NULL;
    int safety = 1000;

    while (!surface && safety > 0) {
        // --safety;
        int idx[4] = {0, 1, 2, 3};
        // for (int i = 0; i < 3; ++i) {
        //     int j = (int)(txrng_next() * (4 - i));
        //     int t = idx[i];
        //     idx[i] = idx[j];
        //     idx[j] = t;
        // }

        // for i from 0 to n−2 do
        //  j ← random integer such that i ≤ j < n
        //  exchange a[i] and a[j]

        uint32_t rmask = masks[idx[0]];
        uint32_t gmask = masks[idx[1]];
        uint32_t bmask = masks[idx[2]];
        uint32_t amask = masks[idx[3]];

        surface =
            SDL_CreateRGBSurfaceFrom(data, w, h, chan * 8, chan * w, rmask, gmask, bmask, amask);
    }

    if (!surface) {
        stbi_image_free(data);
        return k_invalid;
    }

    SDL_Surface* screen = corvid_get_screen_surface();
    // SDL_Surface* optimized = SDL_ConvertSurface(surface, screen->format, 0);

    // if (optimized) {
    //     SDL_FreeSurface(surface);
    //     surface = optimized;
    // }

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