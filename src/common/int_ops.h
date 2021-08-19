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