#pragma once

#include "corvid_system.h"

typedef struct corvid_time {
    double elapsed_time;
    double delta_time;
    double elapsed_time_unscaled;
    double delta_time_unscaled;
    double timescale;
} corvid_time;

void corvid_time_init();
void corvid_time_new_frame();
void corvid_time_skip_time_since_last_update();
void corvid_time_set_timescale(double timescale);
const corvid_time* corvid_get_time();