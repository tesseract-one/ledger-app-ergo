#include <stdint.h>  // uint*_t
#include <string.h>  // memset, explicit_bzero

#include "context.h"

void clear_context(global_ctx_t* context, command_e current_command) {
    uint8_t session_key[SESSION_KEY_LEN] = {0};
    uint32_t app_session = context->app_session_id;
    memcpy(session_key, context->session_key, SESSION_KEY_LEN);
    explicit_bzero(context, sizeof(global_ctx_t));
    memcpy(context->session_key, session_key, SESSION_KEY_LEN);
    context->app_session_id = app_session;
    context->current_command = current_command;
    context->is_ui_busy = false;
}