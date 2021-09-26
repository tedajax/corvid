#include "corvid_rec.h"
#include "corvid.h"
#include "corvid_time.h"
#include "jo_mpeg.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

struct frame {
    SDL_Surface* frame;
};

// runtime data
struct {
    uint32_t head; // current index the first frame of the recording
    uint32_t tail; // current index of the next frame that will be recorded to (if tail == head then
                   // no frames are recorded)
    uint32_t buf_size; // total number of frames available in the recording buffer
    struct frame* frames;
    float timer;    // time until next frame record
    float interval; // interval between frame recordings
    uint32_t frames_per_second;
} rec;

uint32_t next_frame(uint32_t frame)
{
    return (frame + 1) % rec.buf_size;
}

void corvid_rec_init(const corvid_rec_init_desc* desc)
{
    CV_ASSERT(corvid_get_screen_surface() != NULL, "Must init corvid first");

    corvid_rec_init_desc config = (desc) ? *desc
                                         : (corvid_rec_init_desc){
                                             .seconds = 8,
                                             .frames_per_second = 60,
                                         };

    if (config.seconds == 0) {
        memset(&rec, 0, sizeof(rec));
        return;
    }

    rec.buf_size = config.seconds * config.frames_per_second;
    rec.frames = (struct frame*)calloc(rec.buf_size, sizeof(struct frame));
    rec.head = 0;
    rec.tail = 0;
    rec.frames_per_second = config.frames_per_second;
    rec.interval = 1.0f / rec.frames_per_second;
    rec.timer = 0.0f;

    const SDL_Surface* screen = corvid_get_screen_surface();

    for (uint32_t i = 0; i < rec.buf_size; ++i) {
        rec.frames[i].frame =
            SDL_CreateRGBSurface(0, screen->w, screen->h, screen->format->BitsPerPixel, 0, 0, 0, 0);
    }
}

void corvid_rec_term()
{
    if (rec.frames) {
        for (uint32_t i = 0; i < rec.buf_size; ++i) {
            SDL_FreeSurface(rec.frames[i].frame);
        }
        free(rec.frames);
    }

    memset(&rec, 0, sizeof(rec));
}

void corvid_rec_update(float dt)
{
    if (rec.buf_size == 0) {
        return;
    }

    if (rec.timer <= 0.0f) {
        // record frame
        SDL_Surface* frame = rec.frames[rec.tail].frame;
        size_t size = frame->w * frame->h * frame->format->BytesPerPixel;
        const SDL_Surface* screen = corvid_get_screen_surface();
        uint32_t* src = (uint32_t*)screen->pixels;
        uint32_t* dst = (uint32_t*)frame->pixels;
        uint32_t* end = src + (screen->w * screen->h);
        while (src != end) {
            uint8_t* src_rgba = (uint8_t*)src;
            uint8_t* dst_rgba = (uint8_t*)dst;
            dst_rgba[0] = src_rgba[2];
            dst_rgba[1] = src_rgba[1];
            dst_rgba[2] = src_rgba[0];
            dst_rgba[3] = src_rgba[3];
            ++src;
            ++dst;
        }
        // memcpy(frame->pixels, screen->pixels, size);
        rec.tail = next_frame(rec.tail);
        if (rec.tail == rec.head) {
            rec.head = next_frame(rec.head);
        }
        rec.timer += rec.interval;
    } else {
        rec.timer -= dt;
    }
}

void corvid_rec_reset()
{
    rec.head = 0;
    rec.tail = 0;
    rec.timer = 0.0f;
}

uint32_t _rbuf_size()
{
    uint32_t idx = rec.head;
    uint32_t count = 0;
    while (idx != rec.tail) {
        idx = next_frame(idx);
        ++count;
    }
    return count;
}

void corvid_rec_write_video(const char* filename)
{
    if (rec.head == rec.tail) {
        return;
    }

    uint32_t _sz = _rbuf_size();
    float sec = (float)_sz / rec.frames_per_second;

    printf(
        "[corvid_rec] writing recording of %d frames (%0.2f seconds) to %s\n", _sz, sec, filename);

    FILE* file = fopen(filename, "wb");

    int width = rec.frames[rec.head].frame->w * 2;
    int height = rec.frames[rec.head].frame->h * 2;

    SDL_Surface* upscale = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

    uint32_t idx = rec.head;
    while (idx != rec.tail) {
        SDL_Surface* frame = rec.frames[idx].frame;
        SDL_BlitScaled(frame, NULL, upscale, NULL);
        jo_write_mpeg(file, upscale->pixels, upscale->w, upscale->h, rec.frames_per_second);
        idx = next_frame(idx);
    }

    SDL_FreeSurface(upscale);

    fclose(file);

    corvid_time_skip_time_since_last_update();
}