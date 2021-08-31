#pragma once

#include "types.h"
#include "commands/extpubkey/epk_context.h"
#include "commands/deriveaddress/da_context.h"
#include "commands/attestinput/ainpt_context.h"
#include "commands/signtx/stx_context.h"

/**
 * Structure for global context.
 */
typedef struct {
    uint32_t app_session_id;
    uint8_t session_key[SESSION_KEY_LEN];
    command_e current_command;  /// current command
    bool is_ui_busy;
    union {
        attest_input_ctx_t attest_input;
        sign_transaction_ctx_t sign_tx;
        derive_address_ctx_t derive_address;
        extended_public_key_ctx_t ext_pub_key;
    } ctx;
} global_ctx_t;

/**
 * Clear context saving app token and session key
 */
void clear_context(global_ctx_t* context, command_e current_command);