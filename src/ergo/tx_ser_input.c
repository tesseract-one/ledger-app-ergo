#include "tx_ser_input.h"
#include <string.h>
#include "../common/buffer_ext.h"

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return res_error(_ctx, ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE)

static inline ergo_tx_serializer_input_result_e res_error(
    ergo_tx_serializer_input_context_t* context,
    ergo_tx_serializer_input_result_e err) {
    context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
    return err;
}

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_init(
    ergo_tx_serializer_input_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t token_frames_count,
    uint32_t context_extension_data_size,
    token_table_t* tokens_table,
    cx_blake2b_t* hash) {
    memset(context, 0, sizeof(ergo_tx_serializer_input_context_t));

    if (context_extension_data_size == 1 || context_extension_data_size > MAX_TX_DATA_PART_LEN) {
        return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE);
    }

    memcpy(context->box_id, box_id, ERGO_ID_LEN);
    context->frames_count = token_frames_count;
    context->frames_processed = 0;
    context->context_extension_data_size = context_extension_data_size;
    context->tokens_table = tokens_table;
    context->hash = hash;

    context->state = ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED;

    return ERGO_TX_SERIALIZER_INPUT_RES_OK;
}

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_tokens(
    ergo_tx_serializer_input_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t token_frame_index,
    buffer_t* tokens) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED);
    if (memcmp(context->box_id, box_id, ERGO_ID_LEN) != 0) {
        return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_INPUT_ID);
    }
    if (token_frame_index >= context->frames_count) {
        return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_INPUT_FRAMES);
    }
    if (token_frame_index != context->frames_processed) {
        return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_FRAME_INDEX);
    }
    while (buffer_data_len(tokens) > 0) {
        uint8_t token_id[ERGO_ID_LEN];
        uint64_t token_value;
        if (!buffer_read_bytes(tokens, token_id, ERGO_ID_LEN)) {
            return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID);
        }
        if (!buffer_read_u64(tokens, &token_value, BE)) {
            return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_VALUE);
        }
        if (context->on_token_cb != NULL) {
            ergo_tx_serializer_input_result_e res = context->on_token_cb(context->box_id,
                                                                         token_id,
                                                                         token_value,
                                                                         context->callback_context);
            if (res != ERGO_TX_SERIALIZER_INPUT_RES_OK) {
                return res_error(context, res);
            }
        }
    }
    context->frames_processed++;
    if (context->frames_processed == context->frames_count) {  // finished. serializing input
        if (!blake2b_update(context->hash, box_id, ERGO_ID_LEN)) {
            return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER);
        }
        // adding empty input proof (array with len 0).
        uint8_t empty_byte = 0;
        if (!blake2b_update(context->hash, &empty_byte, 1)) {
            return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER);
        }
        if (context->context_extension_data_size == 0) {
            // adding empty extension
            if (!blake2b_update(context->hash, &empty_byte, 1)) {
                return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER);
            }
            context->state = ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
        } else {
            context->state = ERGO_TX_SERIALIZER_INPUT_STATE_EXTENSION_STARTED;
        }
        return ERGO_TX_SERIALIZER_INPUT_RES_OK;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA;
}

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_context_extension(
    ergo_tx_serializer_input_context_t* context,
    buffer_t* chunk) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_INPUT_STATE_EXTENSION_STARTED);
    size_t len = buffer_data_len(chunk);
    if (context->context_extension_data_size < len) {
        return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MUCH_PROOF_DATA);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(chunk), len)) {
        return res_error(context, ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER);
    }
    context->context_extension_data_size -= len;
    if (context->context_extension_data_size == 0) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
        return ERGO_TX_SERIALIZER_INPUT_RES_OK;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA;
}
