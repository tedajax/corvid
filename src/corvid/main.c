#include "corvid.h"
#include <SDL2/SDL.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void test_pattern(float t);

int main(int argc, char* argv[])
{
    SDL_Window* window =
        SDL_CreateWindow("corvid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 960, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* screen_tex = NULL;

    corvid_init(NULL);

    screen_tex = SDL_CreateTextureFromSurface(renderer, corvid_get_screen_surface());

    float t = 0.0f;
    double dt = 0.0;
    uint64_t last_counter = SDL_GetPerformanceCounter();
    int32_t fps = 0;
    int32_t frames_this_sec = 0;
    double second_timer = 1.0;

    static float ang = 0.0f;
    corvid_color clear_color = 0;

    bool is_paused = false;

    bool should_run = true;
    while (should_run) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                should_run = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    should_run = false;
                } else if (event.key.keysym.scancode == SDL_SCANCODE_P) {
                    is_paused = !is_paused;
                } else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                    t -= 1.0f / 144.0f;
                } else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                    t += 1.0f / 144.0f;
                }
                break;
            }
        }

        uint64_t counter = SDL_GetPerformanceCounter();
        uint64_t frequency = SDL_GetPerformanceFrequency();

        uint64_t delta_nano = (counter - last_counter) * 1000000000 / frequency;
        last_counter = counter;
        dt = (double)delta_nano / 1000000000.0;

        second_timer -= dt;
        if (second_timer <= 0.0) {
            second_timer += 1.0;
            fps = frames_this_sec;
            frames_this_sec = 0;
            char buf[64];
            snprintf(buf, 64, "corvid (FPS: %d)", fps);
            SDL_SetWindowTitle(window, buf);
        }

        ++frames_this_sec;

        if (!is_paused) t += (float)dt;

        test_pattern(t);

        SDL_Surface* screen = corvid_get_screen_surface();
        SDL_LockSurface(screen);
        SDL_UpdateTexture(screen_tex, NULL, screen->pixels, screen->pitch);
        SDL_UnlockSurface(screen);

        SDL_RenderCopy(renderer, screen_tex, NULL, NULL);

        SDL_RenderPresent(renderer);
    }

    corvid_term();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}

void test_pattern(float t)
{
    corvid_clear(0);

    for (int i = 0; i < 16; ++i) {
        int cx = i % 4;
        int cy = i / 4;
        corvid_fill_rect(cx * 80, cy * 60, (cx + 1) * 80, (cy + 1) * 60, i);
    }
    for (int32_t y = 0; y < 240; ++y) {
        float u = t + (y / 30.0f);
        float xx = sinf(u) * 40.0f;
        int32_t ix = (int32_t)xx;
        corvid_line(160 - ix, y, ix + 160, y, (y % 3) + 13);
    }
    for (int i = 0; i < 16; ++i) {
        int32_t xx = i * 8;
        int32_t yy = i * 6;
        corvid_line_rect(xx, yy, 320 - xx, 240 - yy, i);
    }
    for (uint8_t i = 0; i < 16; ++i) {
        float a = t + (i / 16.0f) * (float)M_PI * 2.0f;
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
}