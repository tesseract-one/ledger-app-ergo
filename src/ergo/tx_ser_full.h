#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <buffer.h>

#include "../constants.h"
#include "tx_ser_table.h"
#include "tx_ser_box.h"
#include "tx_ser_input.h"
#include "../helpers/blake2b.h"

typedef enum {
    ERGO_TX_SERIALIZER_FULL_RES_OK = 0x7F,
    ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA = 0x7E,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_ID = 0x01,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_VALUE = 0x02,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE = 0x03,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_DATA_INPUT = 0x04,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_INPUT_ID = 0x05,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_INDEX = 0x06,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_FRAME_INDEX = 0x07,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_INPUT_COUNT = 0x08,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_OUTPUT_COUNT = 0x09,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_TOKENS = 0x0A,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_INPUTS = 0x0B,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_DATA_INPUTS = 0x0C,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_INPUT_FRAMES = 0x0D,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_OUTPUTS = 0x0E,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MUCH_DATA = 0x0F,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER = 0x10,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER = 0x11,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE = 0x12,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_U64_OVERFLOW = 0x13,
    ERGO_TX_SERIALIZER_FULL_RES_ERR_SMALL_CHUNK = 0x14
} ergo_tx_serializer_full_result_e;

typedef enum {
    ERGO_TX_SERIALIZER_FULL_STATE_TOKENS_STARTED,
    ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED,
    ERGO_TX_SERIALIZER_FULL_STATE_DATA_INPUTS_STARTED,
    ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED,
    ERGO_TX_SERIALIZER_FULL_STATE_FINISHED,
    ERGO_TX_SERIALIZER_FULL_STATE_ERROR
} ergo_tx_serializer_full_state_e;

typedef struct {
    ergo_tx_serializer_full_state_e state;
    uint16_t inputs_count;
    uint16_t data_inputs_count;
    uint16_t outputs_count;
    cx_blake2b_t* hash;
    ergo_tx_serializer_table_context_t table_ctx;
    union {
        ergo_tx_serializer_box_context_t box_ctx;
        ergo_tx_serializer_input_context_t input_ctx;
    };
} ergo_tx_serializer_full_context_t;

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_init(
    ergo_tx_serializer_full_context_t* context,
    uint16_t inputs_count,
    uint16_t data_inputs_count,
    uint16_t outputs_count,
    uint8_t tokens_count,
    cx_blake2b_t* hash,
    token_table_t* tokens_table);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_tokens(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* tokens);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_input(
    ergo_tx_serializer_full_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t token_frames_count,
    uint32_t context_extension_data_size);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_input_tokens(
    ergo_tx_serializer_full_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t token_frame_index,
    buffer_t* tokens);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_input_context_extension(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* extension_chunk);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_data_inputs(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* inputs);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box(
    ergo_tx_serializer_full_context_t* context,
    uint64_t value,
    uint32_t ergo_tree_size,
    uint32_t creation_height,
    uint8_t tokens_count,
    uint32_t registers_size);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_ergo_tree(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* tree_chunk);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_change_tree(
    ergo_tx_serializer_full_context_t* context,
    const uint8_t raw_pub_key[static PUBLIC_KEY_LEN]);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_miners_fee_tree(
    ergo_tx_serializer_full_context_t* context,
    bool is_mainnet);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_tokens(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* tokens);

ergo_tx_serializer_full_result_e ergo_tx_serializer_full_add_box_registers(
    ergo_tx_serializer_full_context_t* context,
    buffer_t* registers_chunk);

static inline bool ergo_tx_serializer_full_is_finished(ergo_tx_serializer_full_context_t* context) {
    return context->state == ERGO_TX_SERIALIZER_FULL_STATE_FINISHED;
}

static inline ergo_tx_serializer_full_result_e ergo_tx_serializer_full_set_input_callback(
    ergo_tx_serializer_full_context_t* context,
    ergo_tx_serializer_input_token_cb callback,
    void* cb_context) {
    if (context->state != ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED) {
        context->state = ERGO_TX_SERIALIZER_FULL_STATE_ERROR;
        return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE;
    }
    ergo_tx_serializer_input_set_callback(&context->input_ctx, callback, cb_context);
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}

static inline ergo_tx_serializer_full_result_e ergo_tx_serializer_full_set_box_callbacks(
    ergo_tx_serializer_full_context_t* context,
    ergo_tx_serializer_box_type_cb on_type,
    ergo_tx_serializer_box_token_cb on_token,
    ergo_tx_serializer_box_finished_cb on_finished,
    void* cb_context) {
    if (context->state != ERGO_TX_SERIALIZER_FULL_STATE_OUTPUTS_STARTED) {
        context->state = ERGO_TX_SERIALIZER_FULL_STATE_ERROR;
        return ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE;
    }
    ergo_tx_serializer_box_set_callbacks(&context->box_ctx,
                                         on_type,
                                         on_token,
                                         on_finished,
                                         cb_context);
    return ERGO_TX_SERIALIZER_FULL_RES_OK;
}
