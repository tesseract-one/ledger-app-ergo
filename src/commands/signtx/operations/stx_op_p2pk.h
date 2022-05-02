#pragma once

#include "../stx_types.h"
#include "../stx_amounts.h"
#include "../../../common/bip32.h"
#include "../../../ui/ui_application_id.h"
#include "../../../ui/ui_bip32_path.h"

typedef struct {
    sign_transaction_ui_aprove_ctx_t ui_approve;
    char bip32_path[60];  // Bip32 path string
} sign_transaction_operation_p2pk_ui_approve_data_ctx_t;

typedef enum {
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_INITIALIZED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED,
    SIGN_TRANSACTION_OPERATION_P2PK_STATE_ERROR
} sign_transaction_operation_p2pk_state_e;

typedef struct {
    uint8_t schnorr_key[PRIVATE_KEY_LEN];
    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];
    sign_transaction_operation_p2pk_state_e state;
    cx_blake2b_t hash;
    sign_transaction_amounts_ctx_t amounts;
    union {
        ergo_tx_serializer_full_context_t tx;
        sign_transaction_operation_p2pk_ui_approve_data_ctx_t ui_approve;
        sign_transaction_ui_confirm_ctx_t ui_confirm;
    };
} sign_transaction_operation_p2pk_ctx_t;

//****************** OPERATION CALLS ****************

uint16_t stx_operation_p2pk_init(sign_transaction_operation_p2pk_ctx_t *ctx,
                                 const uint32_t *bip32_path,
                                 uint8_t bip32_path_len);

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
                                                   const uint8_t pub_key[static PUBLIC_KEY_LEN]);

uint16_t stx_operation_p2pk_add_output_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                              buffer_t *data);

uint16_t stx_operation_p2pk_add_output_registers(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                 buffer_t *data);

//****************** OPERATION UI *******************
/**
 * Display bip44 path and application access_token on the device
 * and ask confirmation to proceed.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_stx_operation_p2pk_show_token_and_path(sign_transaction_operation_p2pk_ctx_t *ctx,
                                              uint32_t app_access_token);

int ui_stx_operation_p2pk_show_confirm_screen(sign_transaction_operation_p2pk_ctx_t *ctx);
