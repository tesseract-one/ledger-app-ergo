#include "stx_handler.h"
#include "stx_sw.h"
#include "stx_response.h"
#include "stx_ui.h"
#include "../../globals.h"
#include "../../helpers/session_id.h"
#include "../../helpers/response.h"
#include "../../common/int_ops.h"
#include "../../common/macros.h"
#include "../../helpers/crypto.h"
#include "../../helpers/input_frame.h"

#include <string.h>

#define CHECK_COMMAND(_cmd) \
    if (_cmd != G_context.current_command) return handler_err(&G_context.sign_tx_ctx, SW_BAD_STATE)

#define CHECK_SESSION(_session)                    \
    if (_session != G_context.sign_tx_ctx.session) \
    return handler_err(&G_context.sign_tx_ctx, SW_BAD_SESSION_ID)

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_PROPER_STATES(_ctx, _state1, _state2) \
    if (_ctx->state != _state1 && _ctx->state != _state2) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_READ_PARAM(_ctx, _call) \
    if (!_call) return handler_err(_ctx, SW_NOT_ENOUGH_DATA)

#define CHECK_PARAMS_FINISHED(_ctx, _buffer) \
    if (buffer_can_read(_buffer, 1)) return handler_err(_ctx, SW_TOO_MUCH_DATA)

#define CHECK_CALL_RESULT_OK(_ctx, _call)                                                          \
    {                                                                                              \
        ergo_tx_serializer_full_result_e res = _call;                                              \
        if (res != ERGO_TX_SERIALIZER_FULL_RES_OK && res != ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA) \
            return handler_err(_ctx, sw_from_ser_res(res));                                        \
    }

static inline int handler_err(sign_transaction_ctx_t *ctx, uint16_t err) {
    ctx->state = SIGN_TRANSACTION_STATE_ERROR;
    return res_error(err);
}

static NOINLINE ergo_tx_serializer_input_result_e input_token_cb(uint8_t *box_id,
                                                                 uint8_t index,
                                                                 uint64_t value,
                                                                 void *context) {
    (void) (box_id);
    sign_transaction_ui_ctx_t *ctx = (sign_transaction_ui_ctx_t *) context;
    // calculating proper token sum
    if (!checked_add_u64(ctx->token_amounts[index].input,
                         value,
                         &ctx->token_amounts[index].input)) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e output_type_cb(ergo_tx_serializer_box_type_e type,
                                                               uint64_t value,
                                                               void *context) {
    uint64_t *sum;
    sign_transaction_ui_ctx_t *ctx = (sign_transaction_ui_ctx_t *) context;
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            sum = &ctx->change_value;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE:
            sum = &ctx->fee_value;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_OUTPUT:
            // we don't need to calculate outputs. We will calc sum from other values
            return ERGO_TX_SERIALIZER_BOX_RES_OK;
        default:
            // shouldn't be input box
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    if (!checked_add_u64(*sum, value, sum)) {  // calculating proper sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e output_token_cb(ergo_tx_serializer_box_type_e type,
                                                                uint8_t index,
                                                                uint64_t value,
                                                                void *context) {
    uint64_t *sum;
    sign_transaction_ui_ctx_t *ctx = (sign_transaction_ui_ctx_t *) context;
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_OUTPUT:
            sum = &ctx->token_amounts[index].output;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            sum = &ctx->token_amounts[index].change;
            break;
        default:
            // we shouldn't send tokens to miners fee
            // also, can't be input box too (we have only output boxes in sign tx).
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    if (!checked_add_u64(*sum, value, sum)) {  // calculating proper token sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_full_result_e read_bip32_path(buffer_t *input,
                                                                 uint32_t path[MAX_BIP32_PATH],
                                                                 uint8_t *path_len) {
    if (!buffer_read_u8(input, path_len)) return ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER;
    if (!buffer_read_bip32_path(input, path, *path_len))
        return ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER;
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

static inline int handle_init(sign_transaction_ctx_t *ctx,
                              buffer_t *cdata,
                              bool has_token,
                              uint32_t app_session_id) {
    uint16_t inputs_count, data_inputs_count, outputs_count;
    uint8_t tokens_count;
    uint32_t app_session_id_in = 0;

    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &inputs_count, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &data_inputs_count, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &outputs_count, BE));
    CHECK_READ_PARAM(ctx, !(has_token && !buffer_read_u32(cdata, &app_session_id_in, BE)));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    CHECK_CALL_RESULT_OK(ctx,
                         ergo_tx_serializer_full_init(&ctx->tx,
                                                      inputs_count,
                                                      data_inputs_count,
                                                      outputs_count,
                                                      tokens_count));

    ctx->state = SIGN_TRANSACTION_STATE_INITIALIZED;
    ctx->session = session_id_new_random(ctx->session);

    if (is_known_application(app_session_id, app_session_id_in)) {
        ctx->state = SIGN_TRANSACTION_STATE_DATA_APPROVED;
        return send_response_sign_transaction_session_id(ctx->session);
    }

    return ui_display_sing_tx_access_token(app_session_id_in);
}

static inline int handle_tokens(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_DATA_APPROVED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_tokens(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_input_frame(sign_transaction_ctx_t *ctx,
                                     sign_transaction_ui_ctx_t *ui_ctx,
                                     uint8_t session_key[static SESSION_KEY_LEN],
                                     buffer_t *cdata) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_STATE_DATA_APPROVED,
                        SIGN_TRANSACTION_STATE_INPUTS_STARTED);

    uint8_t input_id[BOX_ID_LEN];
    uint8_t frames_count, frame_index, tokens_count;
    uint64_t value;
    buffer_t tokens;

    input_frame_read_result_e res = input_frame_read(cdata,
                                                     input_id,
                                                     &frames_count,
                                                     &frame_index,
                                                     &tokens_count,
                                                     &value,
                                                     &tokens,
                                                     session_key);
    switch (res) {
        case INPUT_FRAME_READ_RES_ERR_BUFFER:
            return handler_err(ctx, SW_NOT_ENOUGH_DATA);
        case INPUT_FRAME_READ_RES_ERR_HMAC:
            return handler_err(ctx, SW_HMAC_ERROR);
        case INPUT_FRAME_READ_RES_ERR_BAD_SIGNATURE:
            return handler_err(ctx, SW_BAD_FRAME_SIGNATURE);
        case INPUT_FRAME_READ_RES_OK:
            break;
    }

    if (frame_index == 0) {
        uint32_t extension_len;
        CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &extension_len, BE));
        CHECK_CALL_RESULT_OK(
            ctx,
            ergo_tx_serializer_full_add_input(&ctx->tx, input_id, frames_count, extension_len));

        if (!checked_add_u64(ui_ctx->inputs_value, value, &ui_ctx->inputs_value)) {
            return handler_err(ctx, SW_U64_OVERFLOW);
        }
        if (ctx->state == SIGN_TRANSACTION_STATE_DATA_APPROVED) {
            CHECK_CALL_RESULT_OK(ctx,
                                 ergo_tx_serializer_full_set_input_callback(&ctx->tx,
                                                                            &input_token_cb,
                                                                            (void *) ui_ctx));
            ctx->state = SIGN_TRANSACTION_STATE_INPUTS_STARTED;
        }
    }

    CHECK_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_input_tokens(&ctx->tx, input_id, frame_index, &tokens));

    return res_ok();
}

static inline int handle_input_context_extension(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_INPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_input_context_extension(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_data_inputs(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_INPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_data_inputs(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_output_init(sign_transaction_ctx_t *ctx,
                                     sign_transaction_ui_ctx_t *ui_ctx,
                                     buffer_t *cdata) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_STATE_INPUTS_STARTED,
                        SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    uint64_t value;
    uint32_t ergo_tree_size, creation_height;
    uint8_t tokens_count, registers_count;

    CHECK_READ_PARAM(ctx, buffer_read_u64(cdata, &value, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &ergo_tree_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &creation_height, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &registers_count));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    CHECK_CALL_RESULT_OK(ctx,
                         ergo_tx_serializer_full_add_box(&ctx->tx,
                                                         value,
                                                         ergo_tree_size,
                                                         creation_height,
                                                         tokens_count,
                                                         registers_count));

    if (ctx->state == SIGN_TRANSACTION_STATE_INPUTS_STARTED) {
        CHECK_CALL_RESULT_OK(ctx,
                             ergo_tx_serializer_full_set_box_callbacks(&ctx->tx,
                                                                       &output_type_cb,
                                                                       &output_token_cb,
                                                                       (void *) ui_ctx));
        ctx->state = SIGN_TRANSACTION_STATE_OUTPUTS_STARTED;
    }

    return res_ok();
}

static inline int handle_output_tree_chunk(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_ergo_tree(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_output_tree_fee(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    if (buffer_data_len(cdata) > 0) return handler_err(ctx, SW_TOO_MUCH_DATA);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_miners_fee_tree(&ctx->tx));
    return res_ok();
}

static inline int handle_output_tree_change(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t bip32_path_len;
    CHECK_CALL_RESULT_OK(ctx, read_bip32_path(cdata, bip32_path, &bip32_path_len));
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return res_error(SW_BIP32_BAD_PATH);
    }
    uint8_t public_key[PUBLIC_KEY_LEN];
    if (crypto_generate_public_key(bip32_path, bip32_path_len, public_key, NULL) != 0) {
        return res_error(SW_INTERNAL_CRYPTO_ERROR);
    }
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_change_tree(&ctx->tx, public_key));
    return res_ok();
}

static inline int handle_output_tokens(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_tokens(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_output_register(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_register(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_sign_confirm(sign_transaction_ctx_t *ctx,
                                      sign_transaction_ui_ctx_t *ui_ctx) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_hash(&ctx->tx, ctx->tx_id));
    ctx->state = SIGN_TRANSACTION_STATE_TX_FINISHED;
    BUFFER_FROM_ARRAY_FULL(out, ctx->tx_id, TRANSACTION_HASH_LEN);
    return res_ok_data(&out);
}

int handler_sign_transaction(buffer_t *cdata,
                             sign_transaction_subcommand_e subcommand,
                             uint8_t session_or_token) {
    if (G_context.ui.is_busy) {
        return res_ui_busy();
    }
    switch (subcommand) {
        case SIGN_TRANSACTION_SUBCOMMAND_INIT:
            if (session_or_token != 0x01 && session_or_token != 0x02) {
                return res_error(SW_WRONG_P1P2);
            }
            clear_context(&G_context, CMD_SIGN_TRANSACTION);
            return handle_init(&G_context.sign_tx_ctx,
                               cdata,
                               session_or_token == 0x02,
                               G_context.app_session_id);
        case SIGN_TRANSACTION_SUBCOMMAND_TOKEN_IDS:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_tokens(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_INPUT_FRAME:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_input_frame(&G_context.sign_tx_ctx,
                                      &G_context.ui.sign_tx,
                                      G_context.session_key,
                                      cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_INPUT_CONTEXT_EXTENSION:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_input_context_extension(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_DATA_INPUT:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_data_inputs(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_init(&G_context.sign_tx_ctx, &G_context.ui.sign_tx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TREE_CHUNK:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tree_chunk(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_MINERS_FEE_TREE:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tree_fee(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_CHANGE_TREE:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tree_change(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TOKENS:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tokens(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_REGISTER:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_register(&G_context.sign_tx_ctx, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_CONFIRM:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_sign_confirm(&G_context.sign_tx_ctx, &G_context.ui.sign_tx);
        default:
            return res_error(SW_WRONG_SUBCOMMAND);
    }
}
