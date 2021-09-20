#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define CV_DEBUG_BREAK_ON_ASSERT 1

#if CV_DEBUG_BREAK_ON_ASSERT
#if _MSC_VER
#define CV_DEBUG_BREAK __debugbreak();
#else
#include <signal.h>
#define CV_DEBUG_BREAK raise(SIGTRAP);
#endif
#else
#define CV_DEBUG_BREAK
#endif

void _corvid_internal_print_assert(const char* exp, const char* msg);
#define CV_ASSERT_ALWAYS(exp, msg)                                                                 \
    do {                                                                                           \
        if (!(exp)) {                                                                              \
            _corvid_internal_print_assert(#exp, msg);                                              \
            CV_DEBUG_BREAK                                                                         \
            assert(exp);                                                                           \
        }                                                                                          \
    } while (0);

#if _DEBUG
#define CV_ASSERT(exp, msg) CV_ASSERT_ALWAYS(exp, msg)
#else
#define CV_ASSERT(exp, msg)
#endif