//
//  autxo_handler.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "ainpt_handler.h"
#include "ainpt_sw.h"
#include "ainpt_response.h"
#include "ainpt_ui.h"
#include "../../globals.h"
#include "../../helpers/session_id.h"
#include "../../helpers/response.h"

#include <string.h>

#define CHECK_SESSION(session_id)                  \
    if (session_id != G_context.input_ctx.session) \
    return handler_err(&G_context.input_ctx, SW_ATTEST_UTXO_BAD_SESSION)

static inline int handler_err(attest_input_ctx_t *ctx, uint16_t err) {
    ctx->state = ATTEST_INPUT_STATE_ERROR;
    return res_error(err);
}

static int check_box_finished(attest_input_ctx_t *ctx) {
    if (!ergo_tx_serializer_box_is_registers_finished(&ctx->box.ctx)) {
        return res_ok();
    }
    ergo_tx_serializer_box_result_e res =
        ergo_tx_serializer_box_add_tx_id_and_index(&ctx->box.ctx, ctx->tx_id, ctx->box.box_index);

    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
        return handler_err(ctx, SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t) res);
    }

    if (!ergo_tx_serializer_box_id_hash(&ctx->box.ctx, ctx->box_id)) {
        return handler_err(ctx, SW_ATTEST_UTXO_HASHER_ERROR);
    }

    ctx->state = ATTEST_INPUT_STATE_BOX_FINISHED;

    return send_response_attested_input_frame_count(ctx->box.tokens_count);
}

static inline int handle_init(attest_input_ctx_t *ctx,
                              buffer_t *cdata,
                              bool has_token,
                              uint32_t app_session_id) {
    uint32_t prefix_size;
    uint32_t suffix_size;
    uint8_t tokens_count;
    uint32_t app_session_id_in = 0;

    if (!buffer_read_u32(cdata, &prefix_size, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u32(cdata, &suffix_size, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u8(cdata, &tokens_count)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (has_token && !buffer_read_u32(cdata, &app_session_id_in, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (buffer_can_read(cdata, 1)) {
        return handler_err(ctx, SW_ATTEST_UTXO_MORE_DATA_THAN_NEEDED);
    }

    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_init(&ctx->tx,
                                                                            prefix_size,
                                                                            suffix_size,
                                                                            tokens_count,
                                                                            &ctx->tokens_table);

    if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK) {
        return handler_err(ctx, SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t) res);
    }
    ctx->state = ATTEST_INPUT_STATE_INITIALIZED;
    ctx->session = session_id_new_random(ctx->session);

    if (has_token && app_session_id == app_session_id_in) {
        ctx->state = ATTEST_INPUT_STATE_APPROVED;
        return send_response_attested_input_session_id(ctx->session);
    }

    return ui_display_access_token(app_session_id_in);
}

static inline int handle_tx_prefix_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_APPROVED) {
        ctx->state = ATTEST_INPUT_STATE_ERROR;
        return res_deny();
    }
    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_prefix(&ctx->tx, cdata);

    if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK && res != ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA) {
        return handler_err(ctx, SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t) res);
    }

    ctx->state = ATTEST_INPUT_STATE_TX_STARTED;

    return res_ok();
}

static inline int handle_tx_tokens(attest_input_ctx_t *ctx, buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_TX_STARTED) {
        return handler_err(ctx, SW_BAD_STATE);
    }

    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_tokens(&ctx->tx, cdata);

    if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK && res != ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA) {
        return handler_err(ctx, SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t) res);
    }
    return res_ok();
}

static inline int handle_tx_suffix_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_TX_STARTED) {
        return handler_err(ctx, SW_BAD_STATE);
    }

    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_suffix(&ctx->tx, cdata);

    switch (res) {
        case ERGO_TX_SERIALIZER_SIMPLE_RES_OK:
            if (!ergo_tx_serializer_simple_hash(&ctx->tx, ctx->tx_id)) {
                return handler_err(ctx, SW_ATTEST_UTXO_HASHER_ERROR);
            }
            ctx->state = ATTEST_INPUT_STATE_TX_FINISHED;
            return res_ok();
        case ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA:
            return res_ok();
        default:
            return handler_err(ctx, SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t) res);
    }
}

static inline int handle_box_init(attest_input_ctx_t *ctx, buffer_t *cdata) {
    uint16_t box_index;
    uint64_t value;
    uint32_t ergo_tree_size;
    uint32_t creation_height;
    uint8_t tokens_count;
    uint8_t registers_count;

    if (ctx->state != ATTEST_INPUT_STATE_TX_FINISHED) {
        return handler_err(ctx, SW_BAD_STATE);
    }

    if (!buffer_read_u16(cdata, &box_index, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u64(cdata, &value, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u32(cdata, &ergo_tree_size, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u32(cdata, &creation_height, BE)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u8(cdata, &tokens_count)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u8(cdata, &registers_count)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (buffer_can_read(cdata, 1)) {
        return handler_err(ctx, SW_ATTEST_UTXO_MORE_DATA_THAN_NEEDED);
    }

    memset(&ctx->box, 0, sizeof(_attest_input_box_ctx_t));

    if (!ergo_tx_serializer_box_id_hash_init(&ctx->box.hash)) {
        return handler_err(ctx, SW_ATTEST_UTXO_HASHER_ERROR);
    }

    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_init(&ctx->box.ctx,
                                                                      value,
                                                                      ergo_tree_size,
                                                                      creation_height,
                                                                      tokens_count,
                                                                      registers_count,
                                                                      true,
                                                                      &ctx->tokens_table,
                                                                      &ctx->box.hash);

    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
        return handler_err(ctx, SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t) res);
    }

    ctx->box.box_index = box_index;
    ctx->box.tokens_count = tokens_count;

    ctx->state = ATTEST_INPUT_STATE_BOX_STARTED;

    return res_ok();
}

static inline int handle_box_tree_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_BOX_STARTED) {
        return handler_err(ctx, SW_BAD_STATE);
    }
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_add_tree(&ctx->box.ctx, cdata);
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return check_box_finished(ctx);
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return res_ok();
        default:
            return handler_err(ctx, SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t) res);
    }
}

static inline int handle_box_tokens(attest_input_ctx_t *ctx, buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_BOX_STARTED) {
        return handler_err(ctx, SW_BAD_STATE);
    }
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_add_tokens(&ctx->box.ctx, cdata);
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return check_box_finished(ctx);
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return res_ok();
        default:
            return handler_err(ctx, SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t) res);
    }
}

static inline int handle_box_register(attest_input_ctx_t *ctx, buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_BOX_STARTED) {
        return handler_err(ctx, SW_BAD_STATE);
    }
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_add_register(&ctx->box.ctx, cdata);
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return check_box_finished(ctx);
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return res_ok();
        default:
            return handler_err(ctx, SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t) res);
    }
}

static inline int handle_box_get_frame(attest_input_ctx_t *ctx,
                                       uint8_t session_key[static SESSION_KEY_LEN],
                                       buffer_t *cdata) {
    if (ctx->state != ATTEST_INPUT_STATE_BOX_FINISHED) {
        return handler_err(ctx, SW_BAD_STATE);
    }
    uint8_t index;
    if (!buffer_read_u8(cdata, &index)) {
        return handler_err(ctx, SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    return send_response_attested_input_frame(ctx, session_key, index);
}

int handler_attest_input(buffer_t *cdata,
                         attest_input_subcommand_e subcommand,
                         uint8_t session_or_token) {
    if (G_context.ui.is_busy) {
        return res_ui_busy();
    }
    switch (subcommand) {
        case ATTEST_INPUT_SUBCOMMAND_INIT:
            if (session_or_token != 0x01 && session_or_token != 0x02) {
                return res_error(SW_ATTEST_UTXO_BAD_P2);
            }
            clear_context(&G_context, CMD_ATTEST_INPUT_BOX);
            return handle_init(&G_context.input_ctx,
                               cdata,
                               session_or_token == 0x02,
                               G_context.app_session_id);
        case ATTEST_INPUT_SUBCOMMAND_PREFIX_CHUNK:
            CHECK_SESSION(session_or_token);
            return handle_tx_prefix_chunk(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_TOKEN_IDS:
            CHECK_SESSION(session_or_token);
            return handle_tx_tokens(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_SUFFIX_CHUNK:
            CHECK_SESSION(session_or_token);
            return handle_tx_suffix_chunk(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX:
            CHECK_SESSION(session_or_token);
            return handle_box_init(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_TREE_CHUNK:
            CHECK_SESSION(session_or_token);
            return handle_box_tree_chunk(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_TOKENS:
            CHECK_SESSION(session_or_token);
            return handle_box_tokens(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_REGISTER:
            CHECK_SESSION(session_or_token);
            return handle_box_register(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_GET_RESPONSE_FRAME:
            CHECK_SESSION(session_or_token);
            return handle_box_get_frame(&G_context.input_ctx, G_context.session_key, cdata);
        default:
            return res_error(SW_ATTEST_UTXO_BAD_COMMAND);
    }
}
