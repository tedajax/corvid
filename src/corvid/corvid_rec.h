#pragma once

#include "corvid_system.h"

// recording corvid for gameplay video output

typedef struct corvid_rec_init_desc {
    uint32_t seconds; // 0 seconds to disable recording
    uint32_t frames_per_second;
} corvid_rec_init_desc;

void corvid_rec_init(const corvid_rec_init_desc* desc);
void corvid_rec_term();

void corvid_rec_update(float dt);
void corvid_rec_reset();
void corvid_rec_write_video(const char* filename);
