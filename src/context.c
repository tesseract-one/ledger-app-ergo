#include <stdint.h>  // uint*_t
#include <string.h>  // memset, explicit_bzero
#include <cx.h>

#include "context.h"
#include "./common/macros_ext.h"

// Saved here to store it outside of the stack
app_ctx_t G_app_context;

void app_init(void) {
    // Clear context
    explicit_bzero(&G_app_context, sizeof(app_ctx_t));

    // Generate random key for session
    cx_rng(G_app_context.session_key, MEMBER_SIZE(app_ctx_t, session_key));

    // Reset context to default values
    app_set_current_command(CMD_NONE);
}

void app_set_current_command(command_e current_command) {
    explicit_bzero(&G_app_context.commands_ctx, MEMBER_SIZE(app_ctx_t, commands_ctx));
    app_set_ui_busy(false);
    G_app_context.current_command = current_command;
}