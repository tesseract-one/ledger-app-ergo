#pragma once

#include <stdint.h>
#include <stdbool.h>

static inline bool checked_add_u64(uint64_t l, uint64_t r, uint64_t* out) {
    if (UINT64_MAX - l < r) return false;
    *out = l + r;
    return true;
}

static inline bool checked_sub_u64(uint64_t l, uint64_t r, uint64_t* out) {
    if (r > l) return false;
    *out = l - r;
    return true;
}

static inline bool checked_add_i64(int64_t l, uint64_t r, int64_t* out) {
    if (l < 0) {
        // If r is so big that even without "l" it will overflow int64_t.
        if (r >= (uint64_t) -l && r + l > INT64_MAX) return false;
    } else {
        // if r is bigger than int64_t or r + l is bigger than int64_t.
        if (r > INT64_MAX || INT64_MAX - r < (uint64_t) l) return false;
    }
    *out = l + r;
    return true;
}

static inline bool checked_sub_i64(int64_t l, uint64_t r, int64_t* out) {
    if (l < 0) {
        // r is bigger than INT64_MAX or r + (-l) is greater than -INT64_MIN
        if (r > INT64_MAX || r - l > (INT64_MAX + (uint64_t) 1)) return false;
    } else {
        // r - l is greater than -INT64_MIN
        if ((uint64_t) l < r && r - l > (INT64_MAX + (uint64_t) 1)) return false;
    }
    *out = l - r;
    return true;
}