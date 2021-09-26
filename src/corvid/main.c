#include "corvid.h"
#include "corvid_rec.h"
#include "corvid_time.h"
#include "corvid_timers.h"
#include "tx_rand.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define _USE_MATH_DEFINES
#include <math.h>

void test_pattern(sprite_handle spr);

struct fps_display {
    int32_t frames_this_sec;
    SDL_Window* window;
};
void update_fps_display_action(void* param);

int main(int argc, char* argv[])
{
    SDL_Window* window =
        SDL_CreateWindow("corvid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 1200, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* screen_tex = NULL;

    corvid_init(NULL);
    corvid_time_init();
    corvid_rec_init(NULL);
    sprite_handle test_sprite_h = load_image_as_sprite("assets/test00.png");

    txrng_seed((uint32_t)time(NULL));

    screen_tex = SDL_CreateTextureFromSurface(renderer, corvid_get_screen_surface());

    struct fps_display fps_display = (struct fps_display){.window = window};

    corvid_timer_create(&(corvid_timer_desc){
        .action = update_fps_display_action,
        .delay_sec = 0.25f,
        .interval_sec = 0.25f,
        .parameter = &fps_display,
        .repeat_count = -1,
    });

    corvid_color clear_color = 0;

    bool should_run = true;
    while (should_run) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                should_run = false;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    should_run = false;
                    break;
                case SDL_SCANCODE_F9:
                    corvid_rec_write_video("test.mpg");
                    break;
                }
                break;
            }
        }

        corvid_time_new_frame();
        corvid_update_timers();

        const corvid_time* time = corvid_get_time();

        fps_display.frames_this_sec++;

        test_pattern(test_sprite_h);

        corvid_rec_update((float)time->delta_time_unscaled);

        SDL_Surface* screen = corvid_get_screen_surface();
        SDL_LockSurface(screen);
        SDL_UpdateTexture(screen_tex, NULL, screen->pixels, screen->pitch);
        SDL_UnlockSurface(screen);

        SDL_RenderCopy(renderer, screen_tex, NULL, NULL);

        SDL_RenderPresent(renderer);
    }

    corvid_rec_term();
    corvid_term();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}

void update_fps_display_action(void* param)
{
    struct fps_display* p = (struct fps_display*)param;
    int32_t frames_this_sec = p->frames_this_sec * 4;
    p->frames_this_sec = 0;
    char buf[64];
    snprintf(buf, 64, "corvid (FPS: %d)", frames_this_sec);
    SDL_SetWindowTitle(p->window, buf);
}

void test_pattern(sprite_handle spr)
{
    float t = (float)corvid_get_time()->elapsed_time;

    corvid_clear(0);
#if 1
    for (int i = 0; i < 32; ++i) {
        int cx = i % 8;
        int cy = i / 8;
        corvid_fill_rect(cx * 40, cy * 60, (cx + 1) * 40, (cy + 1) * 60, i);
    }

    for (int32_t y = 0; y < 240; ++y) {
        float u = t + (y / 30.0f);
        float xx = sinf(u) * 40.0f;
        int32_t ix = (int32_t)xx;
        corvid_line(160 - ix, y, ix + 160, y, (y % 3) + 13);
    }

    for (int i = 0; i < 32; ++i) {
        int32_t xx = i * 4;
        int32_t yy = i * 3;
        corvid_line_rect(xx, yy, 320 - xx, 240 - yy, i);
    }

    for (uint8_t i = 0; i < 32; ++i) {
        float a = t + (i / 32.0f) * (float)M_PI * 2.0f;
        int32_t x = 160;
        int32_t y = 120;
        corvid_line(x, y, x + (int32_t)(cosf(a) * 200.0f), y + (int32_t)(sinf(a) * 200.0f), i);
    }

    float u = sinf(t);
    float v = cosf(t / 2.0f);
    corvid_line_rect(160, 120, (int32_t)(u * 200) + 160, (int32_t)(160 * v) + 120, (int32_t)t);
    corvid_fill_rect(160, 120, (int32_t)(u * 200) + 160, (int32_t)(160 * v) + 120, (int32_t)t);

    corvid_fill_rect(10, 20, 100, 20, 10);

    corvid_color col = 5;
    int32_t cnum = 0;
    for (int32_t x = 80; x <= 240; x += 160) {
        for (int32_t y = 60; y <= 180; y += 60) {
            float u = t + (float)M_PI / 3.0f * cnum;
            int32_t rad = (int32_t)(sinf(u) * 32.0f) + 40;
            if (x < 160) {
                corvid_fill_circ(x, y, rad, cnum + 5);
                corvid_line_circ(x, y, rad, cnum + 10);
            } else {
                corvid_fill_circ(x, y, rad, cnum + 5);
            }
            cnum++;
        }
    }
#endif

    for (int32_t i = 0; i < 16; ++i) {
        int32_t sx = (i % 4) * 16;
        int32_t sy = (i / 4) * 16;
        corvid_draw_sprite(&(sprite_draw_desc){
            .sprite_h = spr,
            .pos_x = sx,
            .pos_y = sy,
        });
    }

    for (int32_t i = 0; i < SpriteDrawOrigin_Count; ++i) {
        int32_t sx = 160 + (i % 3) * 32;
        int32_t sy = 120 + (i / 3) * 32;
        corvid_line_rect(sx - 16, sy - 16, sx + 16, sy + 16, 7);
        corvid_draw_sprite(&(sprite_draw_desc){
            .sprite_h = spr,
            .pos_x = sx,
            .pos_y = sy,
            .origin = i,
        });
    }
}