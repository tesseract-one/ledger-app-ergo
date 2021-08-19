#include "tx_ser_input.h"
#include <string.h>

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_init(
    ergo_tx_serializer_input_context_t* context,
    uint8_t box_id[BOX_ID_LEN],
    uint8_t token_frames_count,
    uint32_t proof_data_size,
    token_amount_table_t* tokens_table,
    cx_blake2b_t* hash) {
    memset(context, 0, sizeof(ergo_tx_serializer_input_context_t));

    if (proof_data_size == 0) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_PROOF_SIZE;
    }

    memcpy(context->box_id, box_id, BOX_ID_LEN);
    context->frames_count = token_frames_count;
    context->frames_processed = 0;
    context->proof_data_size = proof_data_size;

    context->state = ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED;

    return ERGO_TX_SERIALIZER_INPUT_RES_OK;
}

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_tokens(
    ergo_tx_serializer_input_context_t* context,
    uint8_t box_id[BOX_ID_LEN],
    uint8_t token_frame_index,
    buffer_t* tokens) {
    if (context->state != ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE;
    }
    if (memcmp(context->box_id, box_id, BOX_ID_LEN) != 0) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_INPUT_ID;
    }
    if (token_frame_index >= context->frames_count) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_INPUT_FRAMES;
    }
    if (token_frame_index != context->frames_processed) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_FRAME_INDEX;
    }
    while (buffer_data_len(tokens) > 0) {
        uint8_t token_id[TOKEN_ID_LEN];
        uint64_t token_value;
        bool token_found = false;
        if (!buffer_read_bytes(tokens, token_id, TOKEN_ID_LEN)) {
            context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
            return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID;
        }
        if (!buffer_read_u64(tokens, &token_value, BE)) {
            context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
            return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_VALUE;
        }
        for (int i = 0; i < context->tokens_table->count; i++) {
            if (memcmp(token_id, context->tokens_table->tokens[i].id, TOKEN_ID_LEN) == 0) {
                token_found = true;
                if (!checked_add_u64(context->tokens_table->tokens[i].amount,
                                     token_value,
                                     &context->tokens_table->tokens[i].amount)) {
                    context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
                    return ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW;
                }
                break;
            }
        }
        if (!token_found) {
            context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
            return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID;
        }
    }
    context->frames_processed++;
    if (context->frames_processed == context->frames_count) {
        if (!blake2b_update(context->hash, box_id, BOX_ID_LEN)) {
            context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
            return ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER;
        }
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_PROOF_STARTED;
        return ERGO_TX_SERIALIZER_INPUT_RES_OK;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA;
}

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_proof(
    ergo_tx_serializer_input_context_t* context,
    buffer_t* chunk) {
    if (context->state != ERGO_TX_SERIALIZER_INPUT_STATE_PROOF_STARTED) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE;
    }
    size_t len = buffer_data_len(chunk);
    if (context->proof_data_size < len) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MUCH_PROOF_DATA;
    }
    if (!blake2b_update(&context->hash, buffer_read_ptr(chunk), len)) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_ERROR;
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER;
    }
    context->proof_data_size -= len;

    if (context->proof_data_size == 0) {
        context->state = ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
        return ERGO_TX_SERIALIZER_INPUT_RES_OK;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA;
}
