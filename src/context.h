#pragma once

#include "types.h"
#include "commands/extpubkey/epk_context.h"
#include "commands/deriveaddress/da_context.h"
#include "commands/attestinput/ainpt_context.h"

/**
 * Structure for global UI storage
 */
typedef struct {
    bool is_busy;
    union {
        derive_address_ui_ctx_t derive_address;
        extended_public_key_ui_ctx_t ext_pub_key;
        attest_input_ui_ctx_t attest_inpt;
    };
} global_ui_ctx_t;

/**
 * Structure for global context.
 */
typedef struct {
    command_e current_command;  /// current command
    uint8_t session_key[SESSION_KEY_LEN];
    uint32_t app_session_id;
    global_ui_ctx_t ui;
    union {
        attest_input_ctx_t input_ctx;
        sign_transaction_ctx_t sign_tx_ctx;
    };
} global_ctx_t;

/**
 * Clear context saving app token and session key
 */
void clear_context(global_ctx_t* context, command_e current_command);