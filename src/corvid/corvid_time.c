#include "corvid_time.h"
#include <SDL2/SDL.h>

enum {
    k_nano_to_seconds = 1000000000,
};

static struct {
    uint64_t last_counter;
    bool should_skip_time;
    bool should_set_timescale;
    double new_timescale;
    corvid_time public_state;
} time;

void corvid_time_init()
{
    time.should_skip_time = true;
    time.public_state = (corvid_time){
        .timescale = 1.0f,
    };
}

void corvid_time_new_frame()
{
    uint64_t counter = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();

    if (time.should_set_timescale) {
        time.should_set_timescale = false;
        time.public_state.timescale = time.new_timescale;
    }

    if (time.should_skip_time) {
        time.should_skip_time = false;
        time.last_counter = counter;
    }

    uint64_t delta_nano = (counter - time.last_counter) * 1000000000 / frequency;
    time.last_counter = counter;

    time.public_state.delta_time_unscaled = (double)delta_nano / k_nano_to_seconds;
    time.public_state.elapsed_time_unscaled += time.public_state.delta_time_unscaled;
    time.public_state.delta_time =
        time.public_state.delta_time_unscaled * time.public_state.timescale;
    time.public_state.elapsed_time += time.public_state.delta_time;
}

void corvid_time_skip_time_since_last_update()
{
    time.should_skip_time = true;
}

void corvid_time_set_timescale(double timescale)
{
    time.should_set_timescale = true;
    time.new_timescale = timescale;
}

const corvid_time* corvid_get_time()
{
    return &time.public_state;
}
