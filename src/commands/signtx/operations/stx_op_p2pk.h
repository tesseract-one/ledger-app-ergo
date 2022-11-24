#pragma once

#include "../stx_types.h"
#include "../stx_amounts.h"
#include "../stx_output.h"
#include "../../../common/bip32.h"
#include "../../../ui/ui_application_id.h"
#include "../../../ui/ui_bip32_path.h"

typedef struct {
    sign_transaction_ui_aprove_ctx_t ui_approve;
    char bip32_path[MAX_BIP32_STRING_LEN];  // Bip32 path string
} sign_transaction_operation_p2pk_ui_approve_data_ctx_t;

typedef struct {
    sign_transaction_output_info_ctx_t output;
    sign_transaction_ui_output_confirm_ctx_t ui;
} sign_transaction_operation_p2pk_ui_output_info_ctx_t;

typedef enum {
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_INITIALIZED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_FINALIZED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_ERROR
} sign_transaction_operation_p2pk_state_e;

typedef struct {
    ergo_tx_serializer_full_context_t tx;
    sign_transaction_operation_p2pk_ui_output_info_ctx_t ui;
} sign_transaction_operation_p2pk_transaction_ctx_t;

typedef struct {
    sign_transaction_operation_p2pk_state_e state;
    uint8_t schnorr_key[PRIVATE_KEY_LEN];
    sign_transaction_bip32_path_t bip32;
    cx_blake2b_t tx_hash;
    uint8_t network_id;
    sign_transaction_amounts_ctx_t amounts;
    union {
        sign_transaction_operation_p2pk_transaction_ctx_t transaction;
        sign_transaction_operation_p2pk_ui_approve_data_ctx_t ui_approve;
        sign_transaction_ui_transaction_confirm_ctx_t ui_confirm;
    };
} sign_transaction_operation_p2pk_ctx_t;

//****************** OPERATION CALLS ****************

uint16_t stx_operation_p2pk_init(sign_transaction_operation_p2pk_ctx_t *ctx,
                                 const uint32_t *bip32_path,
                                 uint8_t bip32_path_len,
                                 uint8_t network_id);

uint16_t stx_operation_p2pk_start_tx(sign_transaction_operation_p2pk_ctx_t *ctx,
                                     uint16_t inputs_count,
                                     uint16_t data_inputs_count,
                                     uint16_t outputs_count,
                                     uint8_t tokens_count);

uint16_t stx_operation_p2pk_add_tokens(sign_transaction_operation_p2pk_ctx_t *ctx, buffer_t *cdata);

uint16_t stx_operation_p2pk_add_input(sign_transaction_operation_p2pk_ctx_t *ctx,
                                      const uint8_t box_id[static ERGO_ID_LEN],
                                      uint64_t erg_amount,
                                      uint8_t frames_count,
                                      uint32_t extension_length);

uint16_t stx_operation_p2pk_add_input_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                             const uint8_t box_id[static ERGO_ID_LEN],
                                             uint8_t frame_index,
                                             buffer_t *tokens);

uint16_t stx_operation_p2pk_add_input_context_extension(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                        buffer_t *data);

uint16_t stx_operation_p2pk_add_data_inputs(sign_transaction_operation_p2pk_ctx_t *ctx,
                                            buffer_t *data);

uint16_t stx_operation_p2pk_add_output(sign_transaction_operation_p2pk_ctx_t *ctx,
                                       uint64_t value,
                                       uint32_t ergo_tree_size,
                                       uint32_t creation_height,
                                       uint8_t tokens_count,
                                       uint32_t registers_size);

uint16_t stx_operation_p2pk_add_output_tree_chunk(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                  buffer_t *data);

uint16_t stx_operation_p2pk_add_output_tree_fee(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                bool is_mainnet);

uint16_t stx_operation_p2pk_add_output_tree_change(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                   const uint32_t path[static MAX_BIP32_PATH],
                                                   uint8_t path_len,
                                                   const uint8_t pub_key[static PUBLIC_KEY_LEN]);

uint16_t stx_operation_p2pk_add_output_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                              buffer_t *data);

uint16_t stx_operation_p2pk_add_output_registers(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                 buffer_t *data);

bool stx_operation_p2pk_should_show_output_confirm_screen(
    sign_transaction_operation_p2pk_ctx_t *ctx);

static inline bool stx_operation_p2pk_is_tx_finished(sign_transaction_operation_p2pk_ctx_t *ctx) {
    return ctx->state == SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED;
}

//****************** OPERATION UI *******************
/**
 * Display bip44 path and application access_token on the device
 * and ask confirmation to proceed.
 *
 * @return SW_OK if success, error code otherwise.
 *
 */
uint16_t ui_stx_operation_p2pk_show_token_and_path(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                   uint32_t app_access_token,
                                                   void *sign_tx_ctx);

/**
 * Display output confirmation screen
 *
 * @return SW_OK if success, error code otherwise.
 *
 */
uint16_t ui_stx_operation_p2pk_show_output_confirm_screen(
    sign_transaction_operation_p2pk_ctx_t *ctx);

/**
 * Display transaction confirmation screen
 *
 * @return SW_OK if success, error code otherwise.
 *
 */
uint16_t ui_stx_operation_p2pk_show_confirm_screen(sign_transaction_operation_p2pk_ctx_t *ctx);
