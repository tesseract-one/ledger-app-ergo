#pragma once

#include <stdint.h>

#include <io.h>
#include "../sw.h"
#include "../common/rwbuffer.h"

static inline int res_ok() {
    return io_send_sw(SW_OK);
}

static inline int res_ok_data(const rw_buffer_t* data) {
    return io_send_response_buffer(&data->read, SW_OK);
}

static inline int res_ui_busy() {
    return io_send_sw(SW_BUSY);
}

static inline int res_deny() {
    return io_send_sw(SW_DENY);
}

static inline int res_error(uint16_t code) {
    return io_send_sw(code);
}