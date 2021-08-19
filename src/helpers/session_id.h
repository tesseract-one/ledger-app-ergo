
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cx.h>

static inline uint8_t session_id_new_random(uint8_t old_session_id) {
    uint8_t id = 0;
    do {
        id = cx_rng_u8();
    } while (id == 0 || id == old_session_id);
    return id;
}

static inline bool is_known_application(uint32_t saved, uint32_t app_id) {
    return saved != 0 && app_id != 0 && app_id == saved;
}