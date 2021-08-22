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

#define CHECK_COMMAND(_cmd) \
    if (_cmd != G_context.current_command) return handler_err(&G_context.input_ctx, SW_BAD_STATE)

#define CHECK_SESSION(session_id)                  \
    if (session_id != G_context.input_ctx.session) \
    return handler_err(&G_context.input_ctx, SW_BAD_SESSION_ID)

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return handler_err(_ctx, SW_BAD_STATE)

#define CHECK_READ_PARAM(_ctx, _call) \
    if (!_call) return handler_err(_ctx, SW_NOT_ENOUGH_DATA)

#define CHECK_PARAMS_FINISHED(_ctx, _buffer) \
    if (buffer_can_read(_buffer, 1)) return handler_err(_ctx, SW_TOO_MUCH_DATA)

#define CHECK_TX_CALL_RESULT_OK(_ctx, _call)                \
    {                                                       \
        ergo_tx_serializer_simple_result_e res = _call;     \
        if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK &&      \
            res != ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA) \
            return handler_err(_ctx, sw_from_tx_res(res));  \
    }

#define CHECK_BOX_CALL_RESULT_OK(_ctx, _call)                                                    \
    {                                                                                            \
        ergo_tx_serializer_box_result_e res = _call;                                             \
        if (res != ERGO_TX_SERIALIZER_BOX_RES_OK && res != ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA) \
            return handler_err(_ctx, sw_from_box_res(res));                                      \
    }

#define CHECK_BOX_CALL_TX_FINISHED(_ctx, _call)                \
    {                                                          \
        ergo_tx_serializer_box_result_e res = _call;           \
        switch (res) {                                         \
            case ERGO_TX_SERIALIZER_BOX_RES_OK:                \
                return check_box_finished(ctx);                \
            case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:         \
                break;                                         \
            default:                                           \
                return handler_err(ctx, sw_from_box_res(res)); \
        }                                                      \
    }

static inline int handler_err(attest_input_ctx_t *ctx, uint16_t err) {
    ctx->state = ATTEST_INPUT_STATE_ERROR;
    return res_error(err);
}

static NOINLINE int check_box_finished(attest_input_ctx_t *ctx) {
    if (!ergo_tx_serializer_box_is_registers_finished(&ctx->box.ctx)) {
        return res_ok();
    }
    CHECK_BOX_CALL_RESULT_OK(
        ctx,
        ergo_tx_serializer_box_add_tx_id_and_index(&ctx->box.ctx, ctx->tx_id, ctx->box.box_index));

    if (!ergo_tx_serializer_box_id_hash(&ctx->box.ctx, ctx->box_id)) {
        return handler_err(ctx, SW_HASHER_ERROR);
    }
    ctx->state = ATTEST_INPUT_STATE_BOX_FINISHED;
    return send_response_attested_input_frame_count(ctx->box.tokens_count);
}

static NOINLINE ergo_tx_serializer_box_result_e box_token_cb(ergo_tx_serializer_box_type_e type,
                                                             uint8_t index,
                                                             uint64_t value,
                                                             void *context) {
    (void) (type);
    attest_input_ctx_t *ctx = (attest_input_ctx_t *) context;
    if (!checked_add_u64(ctx->token_amounts[index], value, &ctx->token_amounts[index])) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline int handle_init(attest_input_ctx_t *ctx,
                              buffer_t *cdata,
                              bool has_token,
                              uint32_t app_session_id) {
    uint32_t prefix_size, suffix_size;
    uint8_t tokens_count;
    uint32_t app_session_id_in = 0;

    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &prefix_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &suffix_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, !(has_token && !buffer_read_u32(cdata, &app_session_id_in, BE)));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    ctx->tokens_table.count = 0;

    CHECK_TX_CALL_RESULT_OK(ctx,
                            ergo_tx_serializer_simple_init(&ctx->tx,
                                                           prefix_size,
                                                           suffix_size,
                                                           tokens_count,
                                                           &ctx->tokens_table));

    ctx->state = ATTEST_INPUT_STATE_INITIALIZED;
    ctx->session = session_id_new_random(ctx->session);

    if (is_known_application(app_session_id, app_session_id_in)) {
        ctx->state = ATTEST_INPUT_STATE_APPROVED;
        return send_response_attested_input_session_id(ctx->session);
    }

    return ui_display_access_token(app_session_id_in);
}

static inline int handle_tx_prefix_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_APPROVED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_simple_add_prefix(&ctx->tx, cdata));
    ctx->state = ATTEST_INPUT_STATE_TX_STARTED;
    return res_ok();
}

static inline int handle_tx_tokens(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_TX_STARTED);
    CHECK_TX_CALL_RESULT_OK(ctx, ergo_tx_serializer_simple_add_tokens(&ctx->tx, cdata));
    return res_ok();
}

static inline int handle_tx_suffix_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_TX_STARTED);
    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_suffix(&ctx->tx, cdata);
    switch (res) {
        case ERGO_TX_SERIALIZER_SIMPLE_RES_OK:
            if (!ergo_tx_serializer_simple_hash(&ctx->tx, ctx->tx_id)) {
                return handler_err(ctx, SW_HASHER_ERROR);
            }
            ctx->state = ATTEST_INPUT_STATE_TX_FINISHED;
            return res_ok();
        case ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA:
            return res_ok();
        default:
            return handler_err(ctx, sw_from_tx_res(res));
    }
}

static inline int handle_box_init(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_TX_FINISHED);

    uint16_t box_index;
    uint64_t value;
    uint32_t ergo_tree_size, creation_height;
    uint8_t tokens_count, registers_count;

    CHECK_READ_PARAM(ctx, buffer_read_u16(cdata, &box_index, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u64(cdata, &value, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &ergo_tree_size, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &creation_height, BE));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &tokens_count));
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &registers_count));
    CHECK_PARAMS_FINISHED(ctx, cdata);

    memset(&ctx->box, 0, sizeof(_attest_input_box_ctx_t));

    if (!ergo_tx_serializer_box_id_hash_init(&ctx->box.hash)) {
        return handler_err(ctx, SW_HASHER_ERROR);
    }

    CHECK_BOX_CALL_RESULT_OK(ctx,
                             ergo_tx_serializer_box_init(&ctx->box.ctx,
                                                         value,
                                                         ergo_tree_size,
                                                         creation_height,
                                                         tokens_count,
                                                         registers_count,
                                                         true,
                                                         &ctx->tokens_table,
                                                         &ctx->box.hash));

    ergo_tx_serializer_box_set_callbacks(&ctx->box.ctx, NULL, &box_token_cb, (void *) ctx);

    ctx->box.box_index = box_index;
    ctx->box.tokens_count = tokens_count;

    ctx->state = ATTEST_INPUT_STATE_BOX_STARTED;
    return res_ok();
}

static inline int handle_box_tree_chunk(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_BOX_STARTED);
    CHECK_BOX_CALL_TX_FINISHED(ctx, ergo_tx_serializer_box_add_tree(&ctx->box.ctx, cdata));
    return res_ok();
}

static inline int handle_box_tokens(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_BOX_STARTED);
    CHECK_BOX_CALL_TX_FINISHED(ctx, ergo_tx_serializer_box_add_tokens(&ctx->box.ctx, cdata));
    return res_ok();
}

static inline int handle_box_register(attest_input_ctx_t *ctx, buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_BOX_STARTED);
    CHECK_BOX_CALL_TX_FINISHED(ctx, ergo_tx_serializer_box_add_register(&ctx->box.ctx, cdata));
    return res_ok();
}

static inline int handle_box_get_frame(attest_input_ctx_t *ctx,
                                       uint8_t session_key[static SESSION_KEY_LEN],
                                       buffer_t *cdata) {
    CHECK_PROPER_STATE(ctx, ATTEST_INPUT_STATE_BOX_FINISHED);
    uint8_t index;
    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &index));
    CHECK_PARAMS_FINISHED(ctx, cdata);
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
                return res_error(SW_WRONG_P1P2);
            }
            clear_context(&G_context, CMD_ATTEST_INPUT_BOX);
            return handle_init(&G_context.input_ctx,
                               cdata,
                               session_or_token == 0x02,
                               G_context.app_session_id);
        case ATTEST_INPUT_SUBCOMMAND_PREFIX_CHUNK:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_tx_prefix_chunk(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_TOKEN_IDS:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_tx_tokens(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_SUFFIX_CHUNK:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_tx_suffix_chunk(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_box_init(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_TREE_CHUNK:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_box_tree_chunk(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_TOKENS:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_box_tokens(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_REGISTER:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_box_register(&G_context.input_ctx, cdata);
        case ATTEST_INPUT_SUBCOMMAND_GET_RESPONSE_FRAME:
            CHECK_COMMAND(CMD_ATTEST_INPUT_BOX);
            CHECK_SESSION(session_or_token);
            return handle_box_get_frame(&G_context.input_ctx, G_context.session_key, cdata);
        default:
            return res_error(SW_WRONG_SUBCOMMAND);
    }
}
