#include "tx_ser_full.h"
#include "../common/gve.h"
#include "../common/macros_ext.h"
#include <ledger_assert.h>
#include <string.h>

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return res_error(_ctx, ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE)

#define CHECK_CALL_RESULT_OK(_ctx, _call)                                                          \
    do {                                                                                           \
        ergo_tx_serializer_full_result_e res = _call;                                              \
        if (res != ERGO_TX_SERIALIZER_FULL_RES_OK && res != ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA) \
            return res_error(_ctx, res);                                                           \
    } while (0)

#define CHECK_CALL_DATA_IS_FINISHED(_ctx, _call, _block) \
    do {                                                 \
        ergo_tx_serializer_full_result_e res = _call;    \
        switch (res) {                                   \
            case ERGO_TX_SERIALIZER_FULL_RES_OK: {       \
                _block;                                  \
            }                                            \
            case ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA:  \
                return res;                              \
            default:                                     \
                return res_error(_ctx, res);             \
        }                                                \
    } while (0)

static inline ergo_tx_serializer_full_result_e res_error(ergo_tx_serializer_full_context_t* context,
                                                         ergo_tx_serializer_full_result_e err) {
    context->state = ERGO_TX_SERIALIZER_FULL_STATE_ERROR;
    return err;
}

static inline ergo_tx_serializer_full_result_e map_table_result(
    ergo_tx_serializer_table_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_TABLE_RES_OK:
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        case ERGO_TX_SERIALIZER_TABLE_RES_MORE_DATA:
            return ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_BUFFER:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER;
    }
    LEDGER_ASSERT(false, "Unknown table response: %d", (int) res);
}

static inline ergo_tx_serializer_full_result_e map_input_result(
    ergo_tx_serializer_input_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_INPUT_RES_OK:
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        case ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA:
            return ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_INPUT_ID:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_INPUT_ID;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_FRAME_INDEX:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_FRAME_INDEX;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_VALUE:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_VALUE;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_INPUT_FRAMES:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_INPUT_FRAMES;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MUCH_PROOF_DATA:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MUCH_DATA;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_U64_OVERFLOW;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BUFFER:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE;
        case ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_TOKENS:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_TOKENS;
    }
    LEDGER_ASSERT(false, "Unknown input response: %d", (int) res);
}

static inline ergo_tx_serializer_full_result_e map_box_result(ergo_tx_serializer_box_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_INDEX;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_VALUE;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MUCH_DATA;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_U64_OVERFLOW;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_SMALL_CHUNK:
            return ERGO_TX_SERIALIZER_FULL_RES_ERR_SMALL_CHUNK;
    }
    LEDGER_ASSERT(false, "Unknown box response: %d", (int) res);
}

static inline bool hash_u16(cx_blake2b_t* hash, uint16_t u16) {
    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 4);
    if (gve_put_u16(&buffer, u16) != GVE_OK) return false;
    return blake2b_update(hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer));
}

static NOINLINE ergo_tx_serializer_full_result_e
data_inputs_finished(ergo_tx_serializer_full_context_t* context) {
    CHECK_CALL_RESULT_OK(
        context,
        map_table_result(ergo_tx_serializer_table_hash(&context->table_ctx, context->hash)));
    if (!hash_u16(context->hash, context->outputs_count)) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER);
    }
    context->state = ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED;
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

static NOINLINE ergo_tx_serializer_full_result_e
input_finished(ergo_tx_serializer_full_context_t* context) {
    context->inputs_count--;
    if (context->inputs_count == 0) {
        if (!hash_u16(context->hash, context->data_inputs_count)) {
            return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER);
        }
        context->state = ERGO_TX_SERIALIZER_FULL_STATE_DATA_INPUTS_STARTED;
        if (context->data_inputs_count == 0) {
            return data_inputs_finished(context);
        }
    }
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

static inline ergo_tx_serializer_full_result_e output_finished(
    ergo_tx_serializer_full_context_t* context) {
    context->outputs_count--;
    if (context->outputs_count == 0) {
        context->state = ERGO_TX_SERIALIZER_FULL_STATE_FINISHED;
    }
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_init(
    ergo_tx_serializer_full_context_t* context,
    uint16_t inputs_count,
    uint16_t data_inputs_count,
    uint16_t outputs_count,
    uint8_t tokens_count,
    cx_blake2b_t* hash,
    token_table_t* tokens_table) {
    memset(context, 0, sizeof(ergo_tx_serializer_full_context_t));
    if (inputs_count == 0) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_INPUT_COUNT);
    }
    if (outputs_count == 0) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_OUTPUT_COUNT);
    }
    if (!hash_u16(hash, inputs_count)) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER);
    }
    context->hash = hash;
    context->inputs_count = inputs_count;
    context->data_inputs_count = data_inputs_count;
    context->outputs_count = outputs_count;
    CHECK_CALL_RESULT_OK(
        context,
        map_table_result(
            ergo_tx_serializer_table_init(&context->table_ctx, tokens_count, tokens_table)));
    if (tokens_count != 0) {
        context->state = ERGO_TX_SERIALIZER_FULL_STATE_TOKENS_STARTED;
    } else {
        context->state = ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED;
        context->input_ctx.state = ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
    }
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_tokens(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* tokens) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_TOKENS_STARTED);

    CHECK_CALL_DATA_IS_FINISHED(
        context,
        map_table_result(ergo_tx_serializer_table_add(&context->table_ctx, tokens)),
        {
            context->state = ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED;
            context->input_ctx.state = ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        });
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_input(
    ergo_tx_serializer_full_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t frames_count,
    uint32_t context_extension_data_size) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED);

    if (!ergo_tx_serializer_input_is_finished(&context->input_ctx)) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE);
    }
    if (context->inputs_count == 0) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_INPUTS);
    }

    CHECK_CALL_RESULT_OK(
        context,
        map_input_result(ergo_tx_serializer_input_init(&context->input_ctx,
                                                       box_id,
                                                       frames_count,
                                                       context_extension_data_size,
                                                       context->table_ctx.tokens_table,
                                                       context->hash)));
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_input_tokens(
    ergo_tx_serializer_full_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t frame_index,
    buffer_t* tokens) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED);

    CHECK_CALL_DATA_IS_FINISHED(
        context,
        map_input_result(
            ergo_tx_serializer_input_add_tokens(&context->input_ctx, box_id, frame_index, tokens)),
        {
            if (ergo_tx_serializer_input_is_finished(&context->input_ctx)) {
                return input_finished(context);
            }
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        });
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_input_context_extension(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* echunk) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED);

    CHECK_CALL_DATA_IS_FINISHED(
        context,
        map_input_result(
            ergo_tx_serializer_input_add_context_extension(&context->input_ctx, echunk)),
        { return input_finished(context); });
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_data_inputs(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* inputs) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_DATA_INPUTS_STARTED);
    while (buffer_data_len(inputs) > 0) {
        if (context->data_inputs_count == 0) {
            return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_DATA_INPUTS);
        }
        uint8_t box_id[ERGO_ID_LEN];
        if (!buffer_read_bytes(inputs, box_id, ERGO_ID_LEN)) {
            return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_DATA_INPUT);
        }
        if (!blake2b_update(context->hash, box_id, ERGO_ID_LEN)) {
            return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER);
        }
        context->data_inputs_count--;
    }
    if (context->data_inputs_count == 0) {
        return data_inputs_finished(context);
    }
    return ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box(
    ergo_tx_serializer_full_context_t* context,
    uint64_t value,
    uint32_t ergo_tree_size,
    uint32_t creation_height,
    uint8_t tokens_count,
    uint32_t registers_size) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED);
    if (context->outputs_count == 0) {
        return res_error(context, ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_OUTPUTS);
    }
    CHECK_CALL_RESULT_OK(context,
                         map_box_result(ergo_tx_serializer_box_init(&context->box_ctx,
                                                                    value,
                                                                    ergo_tree_size,
                                                                    creation_height,
                                                                    tokens_count,
                                                                    registers_size,
                                                                    context->hash)));
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_ergo_tree(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* tree_chunk) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED);

    CHECK_CALL_DATA_IS_FINISHED(
        context,
        map_box_result(ergo_tx_serializer_box_add_tree(&context->box_ctx, tree_chunk)),
        {
            if (ergo_tx_serializer_box_is_finished(&context->box_ctx)) {
                return output_finished(context);
            }
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        });
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_change_tree(
    ergo_tx_serializer_full_context_t* context,
    const uint8_t raw_pub_key[static PUBLIC_KEY_LEN]) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED);

    CHECK_CALL_RESULT_OK(
        context,
        map_box_result(ergo_tx_serializer_box_add_change_tree(&context->box_ctx, raw_pub_key)));

    if (ergo_tx_serializer_box_is_finished(&context->box_ctx)) {
        return output_finished(context);
    }
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_miners_fee_tree(
    ergo_tx_serializer_full_context_t* context,
    bool is_mainnet) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED);

    CHECK_CALL_RESULT_OK(
        context,
        map_box_result(ergo_tx_serializer_box_add_miners_fee_tree(&context->box_ctx, is_mainnet)));

    if (ergo_tx_serializer_box_is_finished(&context->box_ctx)) {
        return output_finished(context);
    }
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_tokens(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* tokens) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED);

    CHECK_CALL_DATA_IS_FINISHED(
        context,
        map_box_result(
            ergo_tx_serializer_box_add_tokens(&context->box_ctx, tokens, &context->table_ctx)),
        {
            if (ergo_tx_serializer_box_is_finished(&context->box_ctx)) {
                return output_finished(context);
            }
            return ERGO_TX_SERIALIZER_FULL_RES_OK;
        });
}

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_registers(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* registers_chunk) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED);

    CHECK_CALL_DATA_IS_FINISHED(
        context,
        map_box_result(ergo_tx_serializer_box_add_registers(&context->box_ctx, registers_chunk)),
        { return output_finished(context); });
}
