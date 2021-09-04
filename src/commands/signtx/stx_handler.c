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
#include "../../ergo/schnorr.h"

#include <string.h>

#define CONTEXT(gctx) gctx.ctx.sign_tx

#define CHECK_COMMAND(_cmd) \
    if (_cmd != G_context.current_command) return handler_err(&CONTEXT(G_context), SW_BAD_STATE)

#define CHECK_SESSION(_session)                 \
    if (_session != CONTEXT(G_context).session) \
    return handler_err(&CONTEXT(G_context), SW_BAD_SESSION_ID)

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_PROPER_STATES(_ctx, _state1, _state2) \
    if (_ctx->state != _state1 && _ctx->state != _state2) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_READ_PARAM(_ctx, _call) \
    if (!_call) return handler_err(_ctx, SW_NOT_ENOUGH_DATA)

#define CHECK_PARAMS_FINISHED(_ctx, _buffer) \
    if (buffer_can_read(_buffer, 1)) return handler_err(_ctx, SW_TOO_MUCH_DATA)

#define CHECK_CALL_RESULT_OK(_ctx, _call)                                                          \
    do {                                                                                           \
        ergo_tx_serializer_full_result_e res = _call;                                              \
        if (res != ERGO_TX_SERIALIZER_FULL_RES_OK && res != ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA) \
            return handler_err(_ctx, sw_from_ser_res(res));                                        \
    } while (0)

static inline int handler_err(sign_transaction_ctx_t *ctx, uint16_t err) {
    ctx->state = SIGN_TRANSACTION_STATE_ERROR;
    return res_error(err);
}

static inline uint8_t find_token_index(const token_table_t *table,
                                       const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return 0xFF;
}

static NOINLINE ergo_tx_serializer_input_result_e
input_token_cb(const uint8_t box_id[static ERGO_ID_LEN],
               const uint8_t tn_id[static ERGO_ID_LEN],
               uint64_t value,
               void *context) {
    (void) (box_id);
    _sign_transaction_amounts_ctx_t *ctx = (_sign_transaction_amounts_ctx_t *) context;
    // searching for token in table
    uint8_t index = 0;
    if ((index = find_token_index(&ctx->tokens_table, tn_id)) == 0xFF) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID;
    }
    // calculating proper token sum
    if (!checked_add_u64(ctx->tokens[index].input, value, &ctx->tokens[index].input)) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e output_type_cb(ergo_tx_serializer_box_type_e type,
                                                               uint64_t value,
                                                               void *context) {
    uint64_t *sum;
    _sign_transaction_amounts_ctx_t *ctx = (_sign_transaction_amounts_ctx_t *) context;
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            sum = &ctx->change;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE:
            sum = &ctx->fee;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE:
            // we don't need to calculate outputs. We will calc sum from other values
            return ERGO_TX_SERIALIZER_BOX_RES_OK;
    }
    if (!checked_add_u64(*sum, value, sum)) {  // calculating proper sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e
output_token_cb(ergo_tx_serializer_box_type_e type,
                const uint8_t id[static ERGO_ID_LEN],
                uint64_t value,
                void *context) {
    uint64_t *sum;
    _sign_transaction_amounts_ctx_t *ctx = (_sign_transaction_amounts_ctx_t *) context;
    uint8_t index = 0;
    if ((index = find_token_index(&ctx->tokens_table, id)) == 0xFF) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID;
    }
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE:
            sum = &ctx->tokens[index].output;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            sum = &ctx->tokens[index].change;
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

static inline uint16_t read_bip32_path(buffer_t *input,
                                       uint32_t path[MAX_BIP32_PATH],
                                       uint8_t *path_len) {
    if (!buffer_read_u8(input, path_len)) return SW_BUFFER_ERROR;
    if (!buffer_read_bip32_path(input, path, *path_len)) return SW_BUFFER_ERROR;
    return 0;
}

static NOINLINE uint16_t bip32_public_key(buffer_t *input, uint8_t pub_key[static PUBLIC_KEY_LEN]) {
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t bip32_path_len;
    uint16_t res = read_bip32_path(input, bip32_path, &bip32_path_len);
    if (res != 0) return res;
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return SW_BIP32_BAD_PATH;
    }
    if (crypto_generate_public_key(bip32_path, bip32_path_len, pub_key, NULL) != 0) {
        return SW_INTERNAL_CRYPTO_ERROR;
    }
    return 0;
}

static NOINLINE uint16_t bip32_private_key(buffer_t *input,
                                           uint8_t priv_key[static PRIVATE_KEY_LEN]) {
    uint32_t b32_path[MAX_BIP32_PATH];
    uint8_t path_len;
    uint16_t res = read_bip32_path(input, b32_path, &path_len);
    if (res != 0) return res;
    if (!bip32_path_validate(b32_path,
                             path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return SW_BIP32_BAD_PATH;
    }
    if (crypto_generate_private_key(b32_path, path_len, priv_key) != 0) {
        explicit_bzero(priv_key, PRIVATE_KEY_LEN);
        return SW_INTERNAL_CRYPTO_ERROR;
    }
    return 0;
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
                                                      tokens_count,
                                                      &ctx->amounts.tokens_table));

    ctx->state = SIGN_TRANSACTION_STATE_INITIALIZED;
    ctx->session = session_id_new_random(ctx->session);

    if (is_known_application(app_session_id, app_session_id_in)) {
        ctx->state = SIGN_TRANSACTION_STATE_DATA_APPROVED;
        return send_response_sign_transaction_session_id(ctx->session);
    }

    return ui_display_sign_tx_access_token(app_session_id_in);
}

static inline int handle_tokens(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_DATA_APPROVED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_tokens(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_input_frame(sign_transaction_ctx_t *ctx,
                                     uint8_t session_key[static SESSION_KEY_LEN],
                                     buffer_t *cdata) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_STATE_DATA_APPROVED,
                        SIGN_TRANSACTION_STATE_INPUTS_STARTED);

    // Check that frame all needed fields
    uint8_t frame_data_len = input_frame_data_length(cdata);
    if (frame_data_len == 0) {
        return handler_err(ctx, SW_NOT_ENOUGH_DATA);
    }
    // Calculate signature. Will be stored in tx_id field in context (we don't need it now).
    cx_hmac_sha256(session_key,
                   SESSION_KEY_LEN,
                   buffer_read_ptr(cdata),
                   frame_data_len,
                   ctx->tx_id,
                   CX_SHA256_SIZE);
    // Compare signature with frame signature
    if (memcmp(ctx->tx_id, input_frame_signature_ptr(cdata), INPUT_FRAME_SIGNATURE_LEN) != 0) {
        return handler_err(ctx, SW_BAD_FRAME_SIGNATURE);
    }

    // Reading frame
    uint64_t value;
    buffer_t tokens;
    uint8_t frames_count, frame_index, tokens_count;

    CHECK_READ_PARAM(ctx, buffer_read_bytes(cdata, ctx->tx_id, ERGO_ID_LEN));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &frames_count));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &frame_index));
    CHECK_READ_PARAM(ctx, buffer_read_u64(cdata, &value, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));

    // Tokens sub buffer
    uint8_t tokens_len = tokens_count * FRAME_TOKEN_VALUE_PAIR_SIZE;
    buffer_init(&tokens, buffer_read_ptr(cdata), tokens_len, tokens_len);
    // Seek to the frame end
    if (!buffer_seek_read_cur(cdata, tokens_len + INPUT_FRAME_SIGNATURE_LEN)) {
        return handler_err(ctx, SW_NOT_ENOUGH_DATA);
    }
    // New input
    if (frame_index == 0) {
        uint32_t extension_len;
        CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &extension_len, BE));
        CHECK_CALL_RESULT_OK(
            ctx,
            ergo_tx_serializer_full_add_input(&ctx->tx, ctx->tx_id, frames_count, extension_len));

        if (!checked_add_u64(ctx->amounts.inputs, value, &ctx->amounts.inputs)) {
            return handler_err(ctx, SW_U64_OVERFLOW);
        }
    }
    // Setup callbacks
    if (ctx->state == SIGN_TRANSACTION_STATE_DATA_APPROVED) {
        CHECK_CALL_RESULT_OK(ctx,
                             ergo_tx_serializer_full_set_input_callback(&ctx->tx,
                                                                        &input_token_cb,
                                                                        (void *) &ctx->amounts));
        ctx->state = SIGN_TRANSACTION_STATE_INPUTS_STARTED;
    }
    // Add tokens to the input
    CHECK_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_full_add_input_tokens(&ctx->tx, ctx->tx_id, frame_index, &tokens));

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

static inline int handle_output_init(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATES(ctx,
                        SIGN_TRANSACTION_STATE_INPUTS_STARTED,
                        SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    uint64_t value;
    uint32_t ergo_tree_size, creation_height, registers_size;
    uint8_t tokens_count;

    CHECK_READ_PARAM(ctx, buffer_read_u64(cdata, &value, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &ergo_tree_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &creation_height, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &registers_size, BE));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    CHECK_CALL_RESULT_OK(ctx,
                         ergo_tx_serializer_full_add_box(&ctx->tx,
                                                         value,
                                                         ergo_tree_size,
                                                         creation_height,
                                                         tokens_count,
                                                         registers_size));

    if (ctx->state == SIGN_TRANSACTION_STATE_INPUTS_STARTED) {
        CHECK_CALL_RESULT_OK(ctx,
                             ergo_tx_serializer_full_set_box_callbacks(&ctx->tx,
                                                                       &output_type_cb,
                                                                       &output_token_cb,
                                                                       (void *) &ctx->amounts));
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
    uint8_t public_key[PUBLIC_KEY_LEN];
    uint16_t res = bip32_public_key(cdata, public_key);
    if (res != 0) return handler_err(ctx, res);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_change_tree(&ctx->tx, public_key));
    return res_ok();
}

static inline int handle_output_tokens(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_tokens(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_output_registers(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_add_box_registers(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_sign_confirm(sign_transaction_ctx_t *ctx) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_OUTPUTS_STARTED);
    CHECK_CALL_RESULT_OK(ctx, ergo_tx_serializer_full_hash(&ctx->tx, ctx->tx_id));
    ctx->state = SIGN_TRANSACTION_STATE_TX_FINISHED;
    return ui_display_sign_tx_transaction();
}

static inline int handle_sign_pk(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_CONFIRMED);
    uint8_t secret[PRIVATE_KEY_LEN];
    uint16_t res = bip32_private_key(cdata, secret);
    if (res != 0) return handler_err(ctx, res);
    bool is_ok = ergo_secp256k1_schnorr_sign(G_io_apdu_buffer, ctx->tx_id, secret);
    explicit_bzero(secret, PRIVATE_KEY_LEN);
    if (!is_ok) return handler_err(ctx, SW_SCHNORR_SIGNING_FAILED);
    BUFFER_FROM_ARRAY_FULL(out, G_io_apdu_buffer, ERGO_SIGNATURE_LEN);
    return res_ok_data(&out);
}

int handler_sign_transaction(buffer_t *cdata,
                             sign_transaction_subcommand_e subcommand,
                             uint8_t session_or_token) {
    if (G_context.is_ui_busy) {
        return res_ui_busy();
    }
    switch (subcommand) {
        case SIGN_TRANSACTION_SUBCOMMAND_INIT:
            if (session_or_token != 0x01 && session_or_token != 0x02) {
                return res_error(SW_WRONG_P1P2);
            }
            clear_context(&G_context, CMD_SIGN_TRANSACTION);
            return handle_init(&CONTEXT(G_context),
                               cdata,
                               session_or_token == 0x02,
                               G_context.app_session_id);
        case SIGN_TRANSACTION_SUBCOMMAND_TOKEN_IDS:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_tokens(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_INPUT_FRAME:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_input_frame(&CONTEXT(G_context), G_context.session_key, cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_INPUT_CONTEXT_EXTENSION:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_input_context_extension(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_DATA_INPUT:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_data_inputs(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_init(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TREE_CHUNK:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tree_chunk(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_MINERS_FEE_TREE:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tree_fee(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_CHANGE_TREE:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tree_change(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TOKENS:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_tokens(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_REGISTERS:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_output_registers(&CONTEXT(G_context), cdata);
        case SIGN_TRANSACTION_SUBCOMMAND_CONFIRM:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_sign_confirm(&CONTEXT(G_context));
        case SIGN_TRANSACTION_SUBCOMMAND_SIGN_PK:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_sign_pk(&CONTEXT(G_context), cdata);
        default:
            return res_error(SW_WRONG_SUBCOMMAND);
    }
}
