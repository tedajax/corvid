#pragma once

#include "corvid_system.h"

typedef void (*corvid_timer_action_fn)(void*);

typedef struct corvid_timer_desc {
    corvid_timer_action_fn action; // action callback
    void* parameter; // will be passed into action when it's called. If parameter_size is provided
                     // (i.e. non-zero) then the data the parameter points to will be copied to
                     // internal opaque storage for stability. This requires that the parameter
                     // structure be trivially copyable. If the parameter is not stored internally
                     // the caller of corvid_timer_create is responsible for their own parameter
                     // lifecycle.
    size_t parameter_size; // size of the parameter structure that parameter points to, if 0 the
                           // parameter will not be copied into internal storage and the caller will
                           // be responsible for the lifecycle.
    float delay_sec; // initial delay in seconds before action is called, 0 to be called next frame
    float interval_sec;   // seconds between repeat calls to action, 0 means it will be called every
                          // frame.
    int32_t repeat_count; // negative for infinite, 0 for no repeats, positive for limited number
} corvid_timer_desc;

typedef struct timer_handle {
    uint32_t handle;
} timer_handle;

timer_handle corvid_timer_create(const corvid_timer_desc* desc);
void corvid_timer_destroy(timer_handle timer_h);
void corvid_update_timers();
bool timer_handle_is_valid(timer_handle timer_h);