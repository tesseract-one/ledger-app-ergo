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

#include "./operations/stx_op_p2pk.h"

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

#define CHECK_CALL_RESULT_OK(_ctx, _call)                \
    do {                                                 \
        uint16_t res = _call;                            \
        if (res != SW_OK) return handler_err(_ctx, res); \
    } while (0)

static inline int handler_err(sign_transaction_ctx_t *ctx, uint16_t err) {
    ctx->state = SIGN_TRANSACTION_STATE_ERROR;
    return res_error(err);
}

static inline uint16_t read_bip32_path(buffer_t *input,
                                       uint32_t path[MAX_BIP32_PATH],
                                       uint8_t *path_len) {
    if (!buffer_read_u8(input, path_len)) return SW_BUFFER_ERROR;
    if (!buffer_read_bip32_path(input, path, *path_len)) return SW_BUFFER_ERROR;
    return SW_OK;
}

static inline uint16_t bip32_public_key(uint32_t path[MAX_BIP32_PATH],
                                        uint8_t path_len,
                                        uint8_t pub_key[static PUBLIC_KEY_LEN]) {
    if (!bip32_path_validate(path,
                             path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return SW_BIP32_BAD_PATH;
    }
    if (crypto_generate_public_key(path, path_len, pub_key, NULL) != 0) {
        return SW_INTERNAL_CRYPTO_ERROR;
    }
    return SW_OK;
}

static inline int show_output_screen_if_needed(sign_transaction_ctx_t *ctx) {
    // Should have switch for more ops
    // Check if we have to show screen
    if (stx_operation_p2pk_should_show_output_confirm_screen(&ctx->p2pk)) {
        // Show it
        CHECK_CALL_RESULT_OK(ctx, ui_stx_operation_p2pk_show_output_confirm_screen(&ctx->p2pk));
        return 0;
    }
    // We don't need to show confirm screen
    return res_ok();
}

static inline int handle_init_p2pk(sign_transaction_ctx_t *ctx,
                                   buffer_t *cdata,
                                   bool has_token,
                                   uint32_t app_session_id) {
    uint32_t app_session_id_in = 0;
    uint8_t network_id = 0;
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &network_id));
    CHECK_CALL_RESULT_OK(ctx, read_bip32_path(cdata, ctx->p2pk.bip32.path, &ctx->p2pk.bip32.len));
    CHECK_READ_PARAM(ctx, !(has_token && !buffer_read_u32(cdata, &app_session_id_in, BE)));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    CHECK_CALL_RESULT_OK(
        ctx,
        stx_operation_p2pk_init(&ctx->p2pk, ctx->p2pk.bip32.path, ctx->p2pk.bip32.len, network_id));

    ctx->operation = SIGN_TRANSACTION_OPERATION_P2PK;
    ctx->state = SIGN_TRANSACTION_STATE_INITIALIZED;
    ctx->session = session_id_new_random(ctx->session);

    // switch beetwen operations if more will be added
    CHECK_CALL_RESULT_OK(ctx,
                         ui_stx_operation_p2pk_show_token_and_path(
                             &ctx->p2pk,
                             app_session_id_in,
                             is_known_application(app_session_id, app_session_id_in),
                             ctx));
    return 0;
}

static inline int handle_tx_start(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);

    uint16_t inputs_count, data_inputs_count, outputs_count;
    uint8_t tokens_count;
    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &inputs_count, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &data_inputs_count, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &outputs_count, BE));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    // switch beetwen operations if more will be added
    CHECK_CALL_RESULT_OK(ctx,
                         stx_operation_p2pk_start_tx(&ctx->p2pk,
                                                     inputs_count,
                                                     data_inputs_count,
                                                     outputs_count,
                                                     tokens_count));
    return res_ok();
}

static inline int handle_tokens(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // switch beetwen operations if more will be added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_tokens(&ctx->p2pk, cdata));
    return res_ok();
}

static inline int handle_input_frame(sign_transaction_ctx_t *ctx,
                                     uint8_t session_key[static SESSION_KEY_LEN],
                                     buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    uint8_t id_buffer[ERGO_ID_LEN];

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
                   id_buffer,
                   CX_SHA256_SIZE);
    // Compare signature with frame signature
    if (memcmp(id_buffer, input_frame_signature_ptr(cdata), INPUT_FRAME_SIGNATURE_LEN) != 0) {
        return handler_err(ctx, SW_BAD_FRAME_SIGNATURE);
    }

    // Reading frame
    uint64_t value;
    buffer_t tokens;
    uint8_t frames_count, frame_index, tokens_count;

    CHECK_READ_PARAM(ctx, buffer_read_bytes(cdata, id_buffer, ERGO_ID_LEN));
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
        // get extension length
        uint32_t extension_len;
        CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &extension_len, BE));

        // Add new input. Should be switch if more ops added
        CHECK_CALL_RESULT_OK(ctx,
                             stx_operation_p2pk_add_input(&ctx->p2pk,
                                                          id_buffer,
                                                          value,
                                                          frames_count,
                                                          extension_len));
    }
    // Add tokens to the input. Swould be switch if more ops added
    CHECK_CALL_RESULT_OK(
        ctx,
        stx_operation_p2pk_add_input_tokens(&ctx->p2pk, id_buffer, frame_index, &tokens));

    return res_ok();
}

static inline int handle_input_context_extension(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_input_context_extension(&ctx->p2pk, cdata));
    return res_ok();
}

static inline int handle_data_inputs(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_data_inputs(&ctx->p2pk, cdata));
    return res_ok();
}

static inline int handle_output_init(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    uint64_t value;
    uint32_t ergo_tree_size, creation_height, registers_size;
    uint8_t tokens_count;

    CHECK_READ_PARAM(ctx, buffer_read_u64(cdata, &value, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &ergo_tree_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &creation_height, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &registers_size, BE));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    // Add box. Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx,
                         stx_operation_p2pk_add_output(&ctx->p2pk,
                                                       value,
                                                       ergo_tree_size,
                                                       creation_height,
                                                       tokens_count,
                                                       registers_size));
    return res_ok();
}

static inline int handle_output_tree_chunk(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_output_tree_chunk(&ctx->p2pk, cdata));
    return show_output_screen_if_needed(ctx);
}

static inline int handle_output_tree_fee(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    CHECK_PARAMS_FINISHED(ctx, cdata);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_output_tree_fee(&ctx->p2pk));
    return show_output_screen_if_needed(ctx);
}

static inline int handle_output_tree_change(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    uint8_t public_key[PUBLIC_KEY_LEN];
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx,
                         read_bip32_path(cdata,
                                         ctx->p2pk.transaction.ui.output.bip32_path.path,
                                         &ctx->p2pk.transaction.ui.output.bip32_path.len));
    CHECK_CALL_RESULT_OK(ctx,
                         bip32_public_key(ctx->p2pk.transaction.ui.output.bip32_path.path,
                                          ctx->p2pk.transaction.ui.output.bip32_path.len,
                                          public_key));
    CHECK_CALL_RESULT_OK(
        ctx,
        stx_operation_p2pk_add_output_tree_change(&ctx->p2pk,
                                                  ctx->p2pk.transaction.ui.output.bip32_path.path,
                                                  ctx->p2pk.transaction.ui.output.bip32_path.len,
                                                  public_key));
    return show_output_screen_if_needed(ctx);
}

static inline int handle_output_tokens(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_output_tokens(&ctx->p2pk, cdata));
    return show_output_screen_if_needed(ctx);
}

static inline int handle_output_registers(sign_transaction_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, stx_operation_p2pk_add_output_registers(&ctx->p2pk, cdata));
    return show_output_screen_if_needed(ctx);
}

static inline int handle_sign_confirm(sign_transaction_ctx_t *ctx) {
    CHECK_PROPER_STATE(ctx, SIGN_TRANSACTION_STATE_APPROVED);
    // Should be switch if more ops added
    CHECK_CALL_RESULT_OK(ctx, ui_stx_operation_p2pk_show_confirm_screen(&ctx->p2pk));
    return 0;
}

int handler_sign_transaction(buffer_t *cdata,
                             sign_transaction_subcommand_e subcommand,
                             uint8_t session_or_token) {
    if (G_context.is_ui_busy) {
        return res_ui_busy();
    }
    switch (subcommand) {
        case SIGN_TRANSACTION_SUBCOMMAND_SIGN_PK:
            if (session_or_token != 0x01 && session_or_token != 0x02) {
                return res_error(SW_WRONG_P1P2);
            }
            clear_context(&G_context, CMD_SIGN_TRANSACTION);
            return handle_init_p2pk(&CONTEXT(G_context),
                                    cdata,
                                    session_or_token == 0x02,
                                    G_context.app_session_id);
        case SIGN_TRANSACTION_SUBCOMMAND_START_TX:
            CHECK_COMMAND(CMD_SIGN_TRANSACTION);
            CHECK_SESSION(session_or_token);
            return handle_tx_start(&CONTEXT(G_context), cdata);
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
        default:
            return res_error(SW_WRONG_SUBCOMMAND);
    }
}
