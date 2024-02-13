#pragma once

#include <buffer.h>
#include "../context.h"
#include "../sw.h"

#define CHECK_COMMAND(_ctx, _cmd) \
    if (_cmd != app_current_command()) return COMMAND_ERROR_HANDLER(_ctx, SW_BAD_STATE)

#define CHECK_SESSION(_ctx, _session_id) \
    if (_session_id != _ctx->session) return COMMAND_ERROR_HANDLER(_ctx, SW_BAD_SESSION_ID)

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return COMMAND_ERROR_HANDLER(_ctx, SW_BAD_STATE)

#define CHECK_PROPER_STATES(_ctx, _state1, _state2)       \
    if (_ctx->state != _state1 && _ctx->state != _state2) \
    return COMMAND_ERROR_HANDLER(_ctx, SW_BAD_STATE)

#define CHECK_READ_PARAM(_ctx, _call) \
    if (!_call) return COMMAND_ERROR_HANDLER(_ctx, SW_NOT_ENOUGH_DATA)

#define CHECK_PARAMS_FINISHED(_ctx, _buffer) \
    if (buffer_can_read(_buffer, 1)) return COMMAND_ERROR_HANDLER(_ctx, SW_TOO_MUCH_DATA)

#define CHECK_CALL_RESULT_SW_OK(_ctx, _call)                         \
    do {                                                             \
        uint16_t _res = _call;                                       \
        if (_res != SW_OK) return COMMAND_ERROR_HANDLER(_ctx, _res); \
    } while (0)

#define CHECK_WRITE_PARAM(_call) \
    if (!_call) return WRITE_ERROR_HANDLER(SW_BUFFER_ERROR)