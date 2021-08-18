#pragma once

#include <stdint.h>

#include "io.h"
#include "../sw.h"
#include "../types.h"
#include "../context.h"
#include "../globals.h"
#include "../common/buffer.h"

static inline int res_ok() {
    return io_send_sw(SW_OK);
}

static inline int res_ui_busy() {
    return io_send_sw(SW_BUSY);
}

static inline int res_deny() {
    clear_context(&G_context, CMD_NONE);
    return io_send_sw(SW_DENY);
}

static inline int res_ok_data(const buffer_t* data) {
    return io_send_response(data, SW_OK);
}

static inline int res_error(uint16_t code) {
    clear_context(&G_context, CMD_NONE);
    return io_send_sw(code);
}