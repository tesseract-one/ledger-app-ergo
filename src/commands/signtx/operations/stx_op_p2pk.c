#include "stx_op_p2pk.h"
#include "../../../common/macros.h"
#include "../../../helpers/crypto.h"
#include "../../../helpers/response.h"
#include "../../../ergo/schnorr.h"
#include "../stx_sw.h"
#include "../stx_ui.h"

#include <string.h>

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_PROPER_STATES(_ctx, _state1, _state2) \
    if (_ctx->state != _state1 && _ctx->state != _state2) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_SW_CALL_RESULT_OK(_ctx, _call)             \
    do {                                                 \
        uint16_t res = _call;                            \
        if (res != SW_OK) return handler_err(_ctx, res); \
    } while (0)

#define CHECK_TX_CALL_RESULT_OK(_tctx, _tcall) \
    CHECK_SW_CALL_RESULT_OK(_tctx, sw_from_ser_res(_tcall))

#define CHECK_TX_FINISHED(ctx)                                          \
    if (ergo_tx_serializer_full_is_finished(&ctx->tx)) {                \
        ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED; \
    }

static inline uint16_t handler_err(sign_transaction_operation_p2pk_ctx_t *ctx, uint16_t err) {
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_ERROR;
    return err;
}

uint16_t stx_operation_p2pk_init(sign_transaction_operation_p2pk_ctx_t *ctx,
                                 const uint32_t *bip32_path,
                                 uint8_t bip32_path_len) {
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return SW_BIP32_BAD_PATH;
    }

    uint8_t secret[PRIVATE_KEY_LEN];
    if (crypto_generate_private_key(bip32_path, bip32_path_len, secret) != 0) {
        explicit_bzero(secret, PRIVATE_KEY_LEN);
        return SW_INTERNAL_CRYPTO_ERROR;
    }
    bool inited = ergo_secp256k1_schnorr_p2pk_sign_init(&ctx->hash, ctx->schnorr_key, secret);
    explicit_bzero(secret, PRIVATE_KEY_LEN);

    if (!inited) {
        explicit_bzero(ctx->schnorr_key, PRIVATE_KEY_LEN);
        return SW_INTERNAL_CRYPTO_ERROR;
    }

    memmove(ctx->bip32_path, bip32_path, sizeof(uint32_t) * bip32_path_len);
    ctx->bip32_path_len = bip32_path_len;
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_INITIALIZED;

    return SW_OK;
}

uint16_t stx_operation_p2pk_start_tx(sign_transaction_operation_p2pk_ctx_t *ctx,
                                     uint16_t inputs_count,
                                     uint16_t data_inputs_count,
                                     uint16_t outputs_count,
                                     uint8_t tokens_count) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INITIALIZED);
    stx_amounts_init(&ctx->amounts);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_init(&ctx->tx,
                                                         inputs_count,
                                                         data_inputs_count,
                                                         outputs_count,
                                                         tokens_count,
                                                         &ctx->hash,
                                                         &ctx->amounts.tokens_table));
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED;
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                       buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_tokens(&ctx->tx, cdata));
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_input(sign_transaction_operation_p2pk_ctx_t *ctx,
                                      const uint8_t box_id[static ERGO_ID_LEN],
                                      uint64_t erg_amount,
                                      uint8_t frames_count,
                                      uint32_t extension_length) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED,
                        SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    // Add new input
    CHECK_TX_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_input(&ctx->tx, box_id, frames_count, extension_length));
    // Add input value to the amounts
    CHECK_SW_CALL_RESULT_OK(ctx, stx_amounts_add_input(&ctx->amounts, erg_amount));
    // Add token callbacks (add_input recreated context)
    CHECK_SW_CALL_RESULT_OK(ctx,
                            stx_amounts_register_input_token_callback(&ctx->amounts, &ctx->tx));
    // Switch state
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED;
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_input_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                             const uint8_t box_id[static ERGO_ID_LEN],
                                             uint8_t frame_index,
                                             buffer_t *tokens) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_input_tokens(&ctx->tx, box_id, frame_index, tokens));
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_input_context_extension(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                        buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_input_context_extension(&ctx->tx, data));
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_data_inputs(sign_transaction_operation_p2pk_ctx_t *ctx,
                                            buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_data_inputs(&ctx->tx, data));
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output(sign_transaction_operation_p2pk_ctx_t *ctx,
                                       uint64_t value,
                                       uint32_t ergo_tree_size,
                                       uint32_t creation_height,
                                       uint8_t tokens_count,
                                       uint32_t registers_size) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED,
                        SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    // Create new box header
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_box(&ctx->tx,
                                                            value,
                                                            ergo_tree_size,
                                                            creation_height,
                                                            tokens_count,
                                                            registers_size));

    // Setup callbacks(add_box recreated context)
    CHECK_SW_CALL_RESULT_OK(ctx, stx_amounts_register_output_callbacks(&ctx->amounts, &ctx->tx));
    // Switch state
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED;
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tree_chunk(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                  buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_ergo_tree(&ctx->tx, data));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tree_fee(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                bool is_mainnet) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_box_miners_fee_tree(&ctx->tx, is_mainnet));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tree_change(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                   const uint8_t pub_key[static PUBLIC_KEY_LEN]) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_change_tree(&ctx->tx, pub_key));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                              buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_tokens(&ctx->tx, data));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_registers(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                 buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_registers(&ctx->tx, data));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

// ===========================
// UI

// Step with icon and text
UX_STEP_NOCB(ux_stx_stx_operation_p2pk_display_confirm_step,
             pn,
             {&C_icon_processing, "Start P2PK signing"});

uint16_t ui_stx_operation_p2pk_show_token_and_path(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                   uint32_t app_access_token) {
    uint8_t screen = 0;
    G_ux_flow[screen++] = &ux_stx_stx_operation_p2pk_display_confirm_step;

    const ux_flow_step_t *b32_step = ui_bip32_path_screen(
        ctx->bip32_path,
        ctx->bip32_path_len,
        ctx->ui_approve.bip32_path,
        MEMBER_SIZE(sign_transaction_operation_p2pk_ui_approve_data_ctx_t, bip32_path));
    if (b32_step == NULL) {
        return SW_BIP32_FORMATTING_FAILED;
    }
    G_ux_flow[screen++] = b32_step;

    if (!ui_stx_add_access_token_screens(app_access_token, &screen, &ctx->ui_approve.ui_approve)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    if (!ui_stx_display_screens(screen, (void *) &ctx->ui_approve.ui_approve)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    return SW_OK;
}

static NOINLINE void ui_stx_operation_p2pk_send_response(void *cb_context) {
    sign_transaction_operation_p2pk_ctx_t *ctx =
        (sign_transaction_operation_p2pk_ctx_t *) cb_context;

    uint8_t secret[PRIVATE_KEY_LEN];
    uint8_t signature[ERGO_SIGNATURE_LEN];
    BUFFER_FROM_ARRAY_FULL(res_sig, signature, ERGO_SIGNATURE_LEN);

    if (ctx->state != SIGN_TRANSACTION_OPERATION_P2PK_STATE_FINALIZED) {
        res_error(SW_BAD_STATE);
        return;
    }

    if (crypto_generate_private_key(ctx->bip32_path, ctx->bip32_path_len, secret) != 0) {
        explicit_bzero(ctx->schnorr_key, PRIVATE_KEY_LEN);
        res_error(SW_INTERNAL_CRYPTO_ERROR);
        return;
    }

    bool finished =
        ergo_secp256k1_schnorr_p2pk_sign_finish(signature, &ctx->hash, secret, ctx->schnorr_key);
    explicit_bzero(secret, PRIVATE_KEY_LEN);
    explicit_bzero(ctx->schnorr_key, PRIVATE_KEY_LEN);

    if (finished) {
        res_ok_data(&res_sig);
    } else {
        res_error(SW_SCHNORR_SIGNING_FAILED);
    }
}

static NOINLINE uint16_t ui_stx_operation_p2pk_show_tx_screen(uint8_t index,
                                                              char *title,
                                                              size_t title_len,
                                                              char *text,
                                                              size_t text_len,
                                                              void *cb_ctx) {
    sign_transaction_operation_p2pk_ctx_t *ctx = (sign_transaction_operation_p2pk_ctx_t *) cb_ctx;
    if (index != 0) return SW_BAD_STATE;
    strncpy(title, "P2PK Path", title_len);
    if (!bip32_path_format(ctx->bip32_path, ctx->bip32_path_len, text, text_len)) {
        return SW_BIP32_FORMATTING_FAILED;
    }
    return SW_OK;
}

uint16_t ui_stx_operation_p2pk_show_confirm_screen(sign_transaction_operation_p2pk_ctx_t *ctx) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED);
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_FINALIZED;
    uint8_t screen = 0;
    // Removing tokens sent to the change.
    stx_amounts_remove_unused_tokens(&ctx->amounts);
    if (!ui_stx_add_transaction_screens(&ctx->ui_confirm,
                                        &screen,
                                        &ctx->amounts,
                                        1,
                                        ui_stx_operation_p2pk_show_tx_screen,
                                        ui_stx_operation_p2pk_send_response,
                                        (void *) ctx)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    if (!ui_stx_display_screens(screen, (void *) &ctx->ui_confirm)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    return SW_OK;
}
