#include "ainpt_handler.h"
#include "ainpt_sw.h"
#include "ainpt_response.h"
#include "ainpt_ui.h"
#include "../../globals.h"
#include "../../helpers/session_id.h"
#include "../../helpers/response.h"
#include "../../common/int_ops.h"
#include "../../common/macros.h"

#include <string.h>

#define CONTEXT(gctx) gctx.ctx.attest_input

#define CHECK_COMMAND(_cmd) \
    if (_cmd != G_context.current_command) return handler_err(&CONTEXT(G_context), SW_BAD_STATE)

#define CHECK_SESSION(session_id)                 \
    if (session_id != CONTEXT(G_context).session) \
    return handler_err(&CONTEXT(G_context), SW_BAD_SESSION_ID)

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_READ_PARAM(_ctx, _call) \
    if (!_call) return handler_err(_ctx, SW_NOT_ENOUGH_DATA)

#define CHECK_PARAMS_FINISHED(_ctx, _buffer) \
    if (buffer_can_read(_buffer, 1)) return handler_err(_ctx, SW_TOO_MUCH_DATA)

#define CHECK_CALL_RESULT_OK(_ctx, _call)                                                        \
    do {                                                                                         \
        ergo_tx_serializer_box_result_e res = _call;                                             \
        if (res != ERGO_TX_SERIALIZER_BOX_RES_OK && res != ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA) \
            return handler_err(_ctx, sw_from_box_res(res));                                      \
    } while (0)

#define CHECK_CALL_BOX_FINISHED(_ctx, _call)                   \
    do {                                                       \
        ergo_tx_serializer_box_result_e res = _call;           \
        switch (res) {                                         \
            case ERGO_TX_SERIALIZER_BOX_RES_OK:                \
                return check_box_finished(ctx);                \
            case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:         \
                break;                                         \
            default:                                           \
                return handler_err(ctx, sw_from_box_res(res)); \
        }                                                      \
    } while (0)

static inline int handler_err(attest_input_ctx_t *ctx, uint16_t err) {
    ctx->state = ATTEST_INPUT_STATE_ERROR;
    return res_error(err);
}

static NOINLINE int check_box_finished(attest_input_ctx_t *ctx) {
    if (!ergo_tx_serializer_box_is_finished(&ctx->box)) {
        return res_ok();
    }
    CHECK_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_box_id_hash(&ctx->box, ctx->box_id, ctx->box_index, ctx->box_id));
    ctx->state = ATTEST_INPUT_STATE_FINISHED;
    return send_response_attested_input_frame_count(ctx->tokens_table.count);
}

static NOINLINE ergo_tx_serializer_box_result_e box_token_cb(ergo_tx_serializer_box_type_e type,
                                                             const uint8_t id[static ERGO_ID_LEN],
                                                             uint64_t value,
                                                             void *context) {
    (void) (type);
    attest_input_ctx_t *ctx = (attest_input_ctx_t *) context;
    if (ctx->tokens_table.count >= TOKEN_MAX_COUNT) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS;
    }
    for (uint8_t i = 0; i < ctx->tokens_table.count; i++) {
        if (memcmp(id, ctx->tokens_table.tokens[i], ERGO_ID_LEN) == 0) {
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID;
        }
    }
    memmove(ctx->tokens_table.tokens[ctx->tokens_table.count], id, ERGO_ID_LEN);
    ctx->token_amounts[ctx->tokens_table.count++] = value;
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline int handle_init(attest_input_ctx_t *ctx,
                              buffer_t *cdata,
                              bool has_token,
                              uint32_t app_session_id) {
    uint64_t value;
    uint32_t ergo_tree_size, creation_height, registers_size, app_session_id_in = 0;
    uint8_t tokens_count;

    CHECK_READ_PARAM(ctx, buffer_read_bytes(cdata, ctx->box_id, ERGO_ID_LEN));
    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &ctx->box_index, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u64(cdata, &value, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &ergo_tree_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &creation_height, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &registers_size, BE));
    CHECK_READ_PARAM(ctx, !(has_token && !buffer_read_u32(cdata, &app_session_id_in, BE)));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    ctx->tokens_table.count = 0;

    memset(&ctx->ui, 0, sizeof(_attest_input_ui_ctx_t));

    if (!ergo_tx_serializer_box_id_hash_init(&ctx->hash)) {
        return handler_err(ctx, SW_HASHER_ERROR);
    }

    CHECK_CALL_RESULT_OK(ctx,
                         ergo_tx_serializer_box_init(&ctx->box,
                                                     value,
                                                     ergo_tree_size,
                                                     creation_height,
                                                     tokens_count,
                                                     registers_size,
                                                     &ctx->hash));

    ergo_tx_serializer_box_set_callbacks(&ctx->box, NULL, &box_token_cb, NULL, (void *) ctx);

    ctx->state = ATTEST_INPUT_STATE_INITIALIZED;
    ctx->session = session_id_new_random(ctx->session);

    if (is_known_application(app_session_id, app_session_id_in)) {
        ctx->state = ATTEST_INPUT_STATE_APPROVED;
        return send_response_attested_input_session_id(ctx->session);
    }

    return ui_display_access_token(app_session_id_in, ctx);
}

static inline int handle_tree_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_APPROVED);
    CHECK_CALL_BOX_FINISHED(ctx, ergo_tx_serializer_box_add_tree(&ctx->box, cdata));
    return res_ok();
}

static inline int handle_tokens(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_APPROVED);
    CHECK_CALL_BOX_FINISHED(ctx, ergo_tx_serializer_box_add_tokens(&ctx->box, cdata, NULL));
    return res_ok();
}

static inline int handle_registers_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_APPROVED);
    CHECK_CALL_BOX_FINISHED(ctx, ergo_tx_serializer_box_add_registers(&ctx->box, cdata));
    return res_ok();
}

static inline int handle_get_frame(attest_input_ctx_t *ctx,
                                   uint8_t session_key[static SESSION_KEY_LEN],
                                   buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_FINISHED);
    uint8_t index;
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &index));
    CHECK_PARAMS_FINISHED(ctx, cdata);
    return send_response_attested_input_frame(ctx, session_key, index);
}

int handler_attest_input(buffer_t *cdata,
                         attest_input_subcommand_e subcommand,
                         uint8_t session_or_token) {
    if (G_context.is_ui_busy) {
        return res_ui_busy();
    }
    switch (subcommand) {
        case ATTEST_INPUT_SUBCOMMAND_INIT:
            if (session_or_token != 0x01 && session_or_token != 0x02) {
                return res_error(SW_WRONG_P1P2);
            }
            clear_context(&G_context, CMD_ATTEST_INPUT_BOX);
            return handle_init(&CONTEXT(G_context),
                               cdata,
                               session_or_token == 0x02,
                               G_context.app_session_id);
        case ATTEST_INPUT_SUBCOMMAND_TREE_CHUNK:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_tree_chunk(&CONTEXT(G_context), cdata);
        case ATTEST_INPUT_SUBCOMMAND_TOKENS:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_tokens(&CONTEXT(G_context), cdata);
        case ATTEST_INPUT_SUBCOMMAND_REGISTERS:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_registers_chunk(&CONTEXT(G_context), cdata);
        case ATTEST_INPUT_SUBCOMMAND_GET_RESPONSE_FRAME:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_get_frame(&CONTEXT(G_context), G_context.session_key, cdata);
        default:
            return res_error(SW_WRONG_SUBCOMMAND);
    }
}
