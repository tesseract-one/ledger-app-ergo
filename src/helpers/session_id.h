
#pragma once

#include <stdint.h>
#include <cx.h>

static inline uint8_t session_id_new_random(uint8_t old_session_id) {
    uint8_t id = 0;
    do {
        id = cx_rng_u8();
    } while (id == 0 || id == old_session_id);
    return id;
}