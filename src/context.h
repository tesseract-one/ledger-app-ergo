#pragma once

#include "apdu_dispatcher.h"
#include "commands/extpubkey/epk_context.h"
#include "commands/deriveaddress/da_context.h"
#include "commands/attestinput/ainpt_context.h"
#include "commands/signtx/stx_context.h"

/**
 * Structure for application context.
 */
typedef struct {
    uint32_t connected_app_id;
    uint8_t session_key[SESSION_KEY_LEN];
    command_e current_command;  /// current command
    bool is_ui_busy;
    union {
        attest_input_ctx_t attest_input;
        sign_transaction_ctx_t sign_tx;
        derive_address_ctx_t derive_address;
        extended_public_key_ctx_t ext_pub_key;
    } commands_ctx;
} app_ctx_t;

/**
 * Global application context
 */
extern app_ctx_t G_app_context;

/**
 * Check is ui busy
 */
static inline bool app_is_ui_busy() {
    return G_app_context.is_ui_busy;
}

/**
 * Set UI busy
 */
static inline void app_set_ui_busy(bool is_busy) {
    G_app_context.is_ui_busy = is_busy;
}

/**
 * Get connected application id.
 */
static inline uint32_t app_connected_app_id(void) {
    return G_app_context.connected_app_id;
}

/**
 * Set connected application id.
 */
static inline void app_set_connected_app_id(uint32_t id) {
    G_app_context.connected_app_id = id;
}

/**
 * Get session key.
 */
static inline const uint8_t* app_session_key(void) {
    return G_app_context.session_key;
}

/**
 * Get current command key.
 */
static inline command_e app_current_command(void) {
    return G_app_context.current_command;
}

/**
 * Attest Input command context
 */
static inline attest_input_ctx_t* app_attest_input_context(void) {
    return &G_app_context.commands_ctx.attest_input;
}

/**
 * Sign Transaction command context
 */
static inline sign_transaction_ctx_t* app_sign_transaction_context(void) {
    return &G_app_context.commands_ctx.sign_tx;
}

/**
 * Derive Address command context
 */
static inline derive_address_ctx_t* app_derive_address_context(void) {
    return &G_app_context.commands_ctx.derive_address;
}

/**
 * Extended Public Key command context
 */
static inline extended_public_key_ctx_t* app_extended_public_key_context(void) {
    return &G_app_context.commands_ctx.ext_pub_key;
}

/**
 * Init application context
 */
void app_init(void);

/**
 * Switch context state to the command
 */
void app_set_current_command(command_e current_command);