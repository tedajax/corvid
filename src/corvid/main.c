#include "corvid.h"
#include <SDL2/SDL.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    corvid_init(NULL);

    float t = 0.0f;
    double dt = 0.0;
    uint64_t last_counter = SDL_GetPerformanceCounter();
    int32_t fps = 0;
    int32_t frames_this_sec = 0;
    double second_timer = 1.0;

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
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    should_run = false;
                } else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                    clear_color -= 1;
                } else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                    clear_color += 1;
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
            printf("FPS: %d     \r", fps);
        }

        ++frames_this_sec;

        t += (float)dt;

        corvid_clear(clear_color);
        for (int i = 0; i < 16; ++i) {
            int cx = i % 4;
            int cy = i / 4;
            corvid_fill_rect(cx * 80, cy * 60, (cx + 1) * 80, (cy + 1) * 60, i);
        }
        for (int i = 0; i < 16; ++i) {
            int32_t xx = i * 8;
            int32_t yy = i * 6;
            corvid_line_rect(xx, yy, 320 - xx, 240 - yy, i);
        }
        corvid_line_circ(160, 120, 32, 8);
        for (uint8_t i = 0; i < 16; ++i) {
            float a = t + (i / 16.0f) * (float)M_PI * 2.0f;
            int32_t x = 160;
            int32_t y = 120;
            corvid_line(x, y, x + (int32_t)(cosf(a) * 32.0f), y + (int32_t)(sinf(a) * 32.0f), i);
        }

        corvid_present();
    }

    corvid_term();

    return 0;
}