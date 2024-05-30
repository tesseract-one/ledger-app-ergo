#include <string.h>

#include "stx_op_p2pk.h"
#include "../../../context.h"
#include "../../../common/macros_ext.h"
#include "../../../helpers/crypto.h"
#include "../../../helpers/response.h"
#include "../../../helpers/sw_result.h"
#include "../../../ergo/schnorr.h"
#include "../../../ergo/network_id.h"
#include "../stx_ui.h"
#include "../../../ui/ui_main.h"

#define COMMAND_ERROR_HANDLER handler_err
#include "../../../helpers/cmd_macros.h"

#define CHECK_TX_CALL_RESULT_OK(_tctx, _tcall) \
    CHECK_CALL_RESULT_SW_OK(_tctx, sw_from_tx_full_result(_tcall))

#define CHECK_TX_FINISHED(ctx)                                          \
    if (ergo_tx_serializer_full_is_finished(&ctx->transaction.tx)) {    \
        ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED; \
    }

static inline uint16_t handler_err(sign_transaction_operation_p2pk_ctx_t *ctx, uint16_t err) {
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_ERROR;
    app_set_current_command(CMD_NONE);
    return err;
}

static NOINLINE ergo_tx_serializer_input_result_e
p2pk_input_token_cb(const uint8_t box_id[static ERGO_ID_LEN],
                    const uint8_t tn_id[static ERGO_ID_LEN],
                    uint64_t value,
                    void *context) {
    sign_transaction_operation_p2pk_ctx_t *ctx = (sign_transaction_operation_p2pk_ctx_t *) context;
    return stx_amounts_add_input_token(&ctx->amounts, box_id, tn_id, value);
}

static NOINLINE ergo_tx_serializer_box_result_e
p2pk_output_type_cb(ergo_tx_serializer_box_type_e type, uint64_t value, void *context) {
    sign_transaction_operation_p2pk_ctx_t *ctx = (sign_transaction_operation_p2pk_ctx_t *) context;
    return stx_amounts_add_output(&ctx->amounts, type, value);
}

static NOINLINE ergo_tx_serializer_box_result_e
p2pk_output_token_cb(ergo_tx_serializer_box_type_e type,
                     const uint8_t token_id[static ERGO_ID_LEN],
                     uint64_t value,
                     void *context) {
    sign_transaction_operation_p2pk_ctx_t *ctx = (sign_transaction_operation_p2pk_ctx_t *) context;
    ergo_tx_serializer_box_result_e res =
        stx_output_info_add_token(&ctx->transaction.ui.output, token_id, value);
    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) return res;
    return stx_amounts_add_output_token(&ctx->amounts, type, token_id, value);
}

static NOINLINE ergo_tx_serializer_box_result_e
p2pk_output_finished_cb(ergo_tx_serializer_box_type_e type, void *context) {
    UNUSED(type);
    sign_transaction_operation_p2pk_ctx_t *ctx = (sign_transaction_operation_p2pk_ctx_t *) context;
    return stx_output_info_set_box_finished(&ctx->transaction.ui.output);
}

uint16_t stx_operation_p2pk_init(sign_transaction_operation_p2pk_ctx_t *ctx,
                                 const uint32_t *bip32_path,
                                 uint8_t bip32_path_len,
                                 uint8_t network_id) {
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return SW_BIP32_BAD_PATH;
    }
    if (!network_id_is_supported(network_id)) {
        return SW_BAD_NET_TYPE_VALUE;
    }

    uint8_t secret[PRIVATE_KEY_LEN];
    if (crypto_generate_private_key(bip32_path, bip32_path_len, secret) != 0) {
        explicit_bzero(secret, PRIVATE_KEY_LEN);
        return SW_INTERNAL_CRYPTO_ERROR;
    }
    bool inited = ergo_secp256k1_schnorr_p2pk_sign_init(&ctx->tx_hash, ctx->schnorr_key, secret);
    explicit_bzero(secret, PRIVATE_KEY_LEN);

    if (!inited) {
        explicit_bzero(ctx->schnorr_key, PRIVATE_KEY_LEN);
        return SW_INTERNAL_CRYPTO_ERROR;
    }

    memmove(ctx->bip32.path, bip32_path, sizeof(uint32_t) * bip32_path_len);
    ctx->bip32.len = bip32_path_len;
    ctx->network_id = network_id;
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_INITIALIZED;

    return SW_OK;
}

uint16_t stx_operation_p2pk_start_tx(sign_transaction_operation_p2pk_ctx_t *ctx,
                                     uint16_t inputs_count,
                                     uint16_t data_inputs_count,
                                     uint16_t outputs_count,
                                     uint8_t tokens_count) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INITIALIZED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_init(&ctx->transaction.tx,
                                                         inputs_count,
                                                         data_inputs_count,
                                                         outputs_count,
                                                         tokens_count,
                                                         &ctx->tx_hash,
                                                         &ctx->amounts.tokens_table));
    stx_amounts_init(&ctx->amounts);
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED;
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                       buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_tokens(&ctx->transaction.tx, cdata));
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
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_input(&ctx->transaction.tx,
                                                              box_id,
                                                              frames_count,
                                                              extension_length));
    // Add token callbacks (add_input recreated context)
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_set_input_callback(&ctx->transaction.tx,
                                                                       &p2pk_input_token_cb,
                                                                       (void *) ctx));
    // Add input value to the amounts
    CHECK_CALL_RESULT_SW_OK(ctx, stx_amounts_add_input(&ctx->amounts, erg_amount));
    // Switch state
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED;
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_input_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                             const uint8_t box_id[static ERGO_ID_LEN],
                                             uint8_t frame_index,
                                             buffer_t *tokens) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_input_tokens(&ctx->transaction.tx,
                                                                     box_id,
                                                                     frame_index,
                                                                     tokens));
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_input_context_extension(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                        buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_input_context_extension(&ctx->transaction.tx, data));
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_data_inputs(sign_transaction_operation_p2pk_ctx_t *ctx,
                                            buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_INPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_data_inputs(&ctx->transaction.tx, data));
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
                            ergo_tx_serializer_full_add_box(&ctx->transaction.tx,
                                                            value,
                                                            ergo_tree_size,
                                                            creation_height,
                                                            tokens_count,
                                                            registers_size));

    // Setup callbacks(add_box recreated context)
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_set_box_callbacks(&ctx->transaction.tx,
                                                                      &p2pk_output_type_cb,
                                                                      &p2pk_output_token_cb,
                                                                      &p2pk_output_finished_cb,
                                                                      (void *) ctx));
    // Init output info
    stx_output_info_init(&ctx->transaction.ui.output, value, &ctx->amounts.tokens_table);
    // Switch state
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED;
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tree_chunk(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                  buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    // Get buffer pointers for output info.
    const uint8_t *chunk = buffer_read_ptr(data);
    uint16_t chunk_len = buffer_data_len(data);
    // flag
    bool is_finished = false;
    // Add chunk to the serializer. Check is all tree added.
    ergo_tx_serializer_full_result_e res =
        ergo_tx_serializer_full_add_box_ergo_tree(&ctx->transaction.tx, data);
    switch (res) {
        case ERGO_TX_SERIALIZER_FULL_RES_OK: {
            is_finished = true;
            break;
        }
        case ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA: {
            is_finished = false;
            break;
        }
        default:  // ERROR happened.
            return handler_err(ctx, sw_from_tx_full_result(res));
    }
    // Add chunk to the output info. Uses copied pointers.
    CHECK_CALL_RESULT_SW_OK(
        ctx,
        stx_output_info_add_tree_chunk(&ctx->transaction.ui.output, chunk, chunk_len, is_finished));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tree_fee(sign_transaction_operation_p2pk_ctx_t *ctx) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_box_miners_fee_tree(&ctx->transaction.tx,
                                                        network_id_is_mainnet(ctx->network_id)));
    CHECK_CALL_RESULT_SW_OK(ctx, stx_output_info_set_fee(&ctx->transaction.ui.output));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tree_change(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                   const uint32_t path[static MAX_BIP32_PATH],
                                                   uint8_t path_len,
                                                   const uint8_t pub_key[static PUBLIC_KEY_LEN]) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_box_change_tree(&ctx->transaction.tx, pub_key));
    CHECK_CALL_RESULT_SW_OK(ctx,
                            stx_output_info_set_bip32(&ctx->transaction.ui.output, path, path_len));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_tokens(sign_transaction_operation_p2pk_ctx_t *ctx,
                                              buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_box_tokens(&ctx->transaction.tx, data));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

uint16_t stx_operation_p2pk_add_output_registers(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                 buffer_t *data) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_full_add_box_registers(&ctx->transaction.tx, data));
    CHECK_TX_FINISHED(ctx);
    return SW_OK;
}

bool stx_operation_p2pk_should_show_output_confirm_screen(
    sign_transaction_operation_p2pk_ctx_t *ctx) {
    if (ctx->state != SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED &&
        ctx->state != SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED)
        return false;
    if (!stx_output_info_is_finished(&ctx->transaction.ui.output)) return false;
    if (stx_output_info_type(&ctx->transaction.ui.output) ==
        SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE) {
        return stx_output_info_used_tokens_count(&ctx->transaction.ui.output) > 0;
    }
    // Change address
    if (stx_output_info_type(&ctx->transaction.ui.output) ==
        SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32) {
        // if path length is not 5 or wrong type then we should ask to confirm
        if (!bip32_path_validate(ctx->transaction.ui.output.bip32_path.path,
                                 ctx->transaction.ui.output.bip32_path.len,
                                 BIP32_HARDENED(44),
                                 BIP32_HARDENED(BIP32_ERGO_COIN),
                                 BIP32_PATH_VALIDATE_ADDRESS_E5))
            return true;
        // Check was it already approved then approve automatically
        if (stx_bip32_path_is_equal(&ctx->transaction.ui.output.bip32_path,
                                    &ctx->transaction.last_approved_change))
            return false;
        // if account is the same and change index is < 20 then we approve it automatically
        if (stx_bip32_path_same_account(&ctx->transaction.ui.output.bip32_path, &ctx->bip32) &&
            ctx->transaction.ui.output.bip32_path.path[4] < 20)
            return false;
    }
    return true;
}

// ===========================
// UI

static NOINLINE void ui_stx_operation_p2pk_approve_action(void *context) {
    sign_transaction_ui_aprove_ctx_t *ctx = (sign_transaction_ui_aprove_ctx_t *) context;
    ui_stx_operation_approve_reject(true, ctx);
}

uint16_t ui_stx_operation_p2pk_show_token_and_path(sign_transaction_operation_p2pk_ctx_t *ctx,
                                                   uint32_t app_access_token,
                                                   bool is_known_application,
                                                   void *sign_tx_ctx) {
    uint8_t screen = 0;
    const ux_flow_step_t *b32_step = ui_bip32_path_screen(
        ctx->bip32.path,
        ctx->bip32.len,
        "P2PK Signing",
        ctx->ui_approve.bip32_path,
        MEMBER_SIZE(sign_transaction_operation_p2pk_ui_approve_data_ctx_t, bip32_path),
        ui_stx_operation_p2pk_approve_action,
        &ctx->ui_approve.ui_approve);
    if (b32_step == NULL) {
        return SW_BIP32_FORMATTING_FAILED;
    }
    ui_add_screen(b32_step, &screen);

    if (!ui_stx_add_operation_approve_screens(&ctx->ui_approve.ui_approve,
                                              &screen,
                                              app_access_token,
                                              is_known_application,
                                              sign_tx_ctx)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    if (!ui_stx_display_screens(screen)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    return SW_OK;
}

uint16_t ui_stx_operation_p2pk_show_output_confirm_screen(
    sign_transaction_operation_p2pk_ctx_t *ctx) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_OPERATION_P2PK_STATE_OUTPUTS_STARTED,
                        SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED);
    uint8_t screen = 0;
    if (!ui_stx_add_output_screens(&ctx->transaction.ui.ui,
                                   &screen,
                                   &ctx->transaction.ui.output,
                                   &ctx->transaction.last_approved_change,
                                   ctx->network_id)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    if (!ui_stx_display_screens(screen)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    return SW_OK;
}

static NOINLINE void ui_stx_operation_p2pk_send_response(void *cb_context) {
    sign_transaction_operation_p2pk_ctx_t *ctx =
        (sign_transaction_operation_p2pk_ctx_t *) cb_context;

    uint8_t secret[PRIVATE_KEY_LEN];
    uint8_t signature[ERGO_SIGNATURE_LEN];
    RW_BUFFER_FROM_ARRAY_FULL(res_sig, signature, ERGO_SIGNATURE_LEN);

    if (ctx->state != SIGN_TRANSACTION_OPERATION_P2PK_STATE_FINALIZED) {
        app_set_current_command(CMD_NONE);
        res_error(SW_BAD_STATE);
        return;
    }

    if (crypto_generate_private_key(ctx->bip32.path, ctx->bip32.len, secret) != 0) {
        explicit_bzero(ctx->schnorr_key, PRIVATE_KEY_LEN);
        app_set_current_command(CMD_NONE);
        res_error(SW_INTERNAL_CRYPTO_ERROR);
        return;
    }

    bool finished =
        ergo_secp256k1_schnorr_p2pk_sign_finish(signature, &ctx->tx_hash, secret, ctx->schnorr_key);
    explicit_bzero(secret, PRIVATE_KEY_LEN);
    explicit_bzero(ctx->schnorr_key, PRIVATE_KEY_LEN);

    if (finished) {
        res_ok_data(&res_sig);
    } else {
        app_set_current_command(CMD_NONE);
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
    if (!bip32_path_format(ctx->bip32.path, ctx->bip32.len, text, text_len)) {
        return SW_BIP32_FORMATTING_FAILED;
    }
    return SW_OK;
}

uint16_t ui_stx_operation_p2pk_show_confirm_screen(sign_transaction_operation_p2pk_ctx_t *ctx) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_OPERATION_P2PK_STATE_TX_FINISHED);
    ctx->state = SIGN_TRANSACTION_OPERATION_P2PK_STATE_FINALIZED;
    uint8_t screen = 0;
    if (!ui_stx_add_transaction_screens(&ctx->ui_confirm,
                                        &screen,
                                        &ctx->amounts,
                                        1,
                                        ui_stx_operation_p2pk_show_tx_screen,
                                        ui_stx_operation_p2pk_send_response,
                                        (void *) ctx)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    if (!ui_stx_display_screens(screen)) {
        return SW_SCREENS_BUFFER_OVERFLOW;
    }
    return SW_OK;
}
