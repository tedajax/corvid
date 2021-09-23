#pragma once

#include "corvid_system.h"

typedef struct sprite_handle {
    uint32_t handle;
} sprite_handle;

typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Rect SDL_Rect;

bool sprite_handle_is_valid(sprite_handle sprite_h);
sprite_handle load_image_as_sprite(const char* filename);
SDL_Surface* get_sprite_surface(sprite_handle sprite_h);
SDL_Rect* get_sprite_rect(sprite_handle sprite_h);