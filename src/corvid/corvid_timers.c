#include "corvid_timers.h"
#include "corvid_time.h"
#include "string.h"

enum {
    k_max_timers = 1024,
    k_max_param_size = 64,
};

struct timer {
    corvid_timer_desc config;
    uint8_t opaque_param_storage[k_max_param_size];
    float seconds_remaining;
    int32_t repeats_remaining;
};

static struct timer timers[k_max_timers + 1] = {0};
static timer_handle handles[k_max_timers + 1] = {0};

static const timer_handle k_invalid = {0};

timer_handle _alloc_timer()
{
    for (uint32_t i = 1; i <= k_max_timers; ++i) {
        if (!timer_handle_is_valid(handles[i])) {
            handles[i].handle = i;
            return handles[i];
        }
    }

    return k_invalid;
}

struct timer* _get_timer(timer_handle timer_h)
{
    CV_ASSERT(timer_handle_is_valid(timer_h), "Invalid timer");
    return &timers[timer_h.handle];
}

timer_handle corvid_timer_create(const corvid_timer_desc* desc)
{

    CV_ASSERT(desc->action, "Must provide action callback.");

    timer_handle handle = _alloc_timer();
    CV_ASSERT(timer_handle_is_valid(handle), "Unable to allocate timer handle.");

    struct timer* timer = _get_timer(handle);

    memcpy(&timer->config, desc, sizeof(corvid_timer_desc));

    if (desc->parameter && desc->parameter_size > 0) {
        CV_ASSERT(
            desc->parameter_size <= k_max_param_size,
            "Timer parameter size is restricted to k_max_param_size");

        memcpy(timer->opaque_param_storage, desc->parameter, desc->parameter_size);
        timer->config.parameter = timer->opaque_param_storage;
    }

    timer->seconds_remaining = timer->config.delay_sec;
    timer->repeats_remaining = timer->config.repeat_count;

    return handle;
}

void corvid_timer_destroy(timer_handle timer_h)
{
    CV_ASSERT(timer_handle_is_valid(timer_h), "Invalid timer");

    handles[timer_h.handle] = k_invalid;
    memset(&timers[timer_h.handle], 0, sizeof(struct timer));
}

void corvid_update_timers()
{
    float dt = (float)corvid_get_time()->delta_time;

    for (uint32_t i = 0; i < k_max_timers; ++i) {
        if (timer_handle_is_valid(handles[i])) {
            struct timer* timer = _get_timer(handles[i]);
            timer->seconds_remaining -= dt;

            if (timer->seconds_remaining <= 0.0f) {
                timer->config.action(timer->config.parameter);
                if (timer->repeats_remaining != 0) {
                    timer->seconds_remaining += timer->config.interval_sec;
                    if (timer->repeats_remaining > 0) --timer->repeats_remaining;
                } else {
                    corvid_timer_destroy(handles[i]);
                }
            }
        }
    }
}

bool timer_handle_is_valid(timer_handle timer_h)
{
    return timer_h.handle > 0 && timer_h.handle <= k_max_timers
           && handles[timer_h.handle].handle == timer_h.handle;
}