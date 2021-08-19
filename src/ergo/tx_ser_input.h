#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../constants.h"
#include "../common/buffer.h"
#include "../helpers/blake2b.h"
#include "tx_ser_table.h"

typedef enum {
    ERGO_TX_SERIALIZER_INPUT_RES_OK = 0x7F,
    ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA = 0x7E,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_PROOF_SIZE = 0x01,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_INPUT_ID = 0x02,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_FRAME_INDEX = 0x03,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID = 0x04,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_VALUE = 0x05,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_INPUT_FRAMES = 0x06,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MUCH_PROOF_DATA = 0x07,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW = 0x08,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER = 0x09,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BUFFER = 0x10,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE = 0x11
} ergo_tx_serializer_input_result_e;

typedef enum {
    ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED,
    ERGO_TX_SERIALIZER_INPUT_STATE_PROOF_STARTED,
    ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED,
    ERGO_TX_SERIALIZER_INPUT_STATE_ERROR
} ergo_tx_serializer_input_state_e;

typedef struct {
    ergo_tx_serializer_input_state_e state;
    uint8_t box_id[BOX_ID_LEN];
    uint8_t frames_count;
    uint8_t frames_processed;
    uint32_t proof_data_size;
    token_amount_table_t* tokens_table;
    cx_blake2b_t* hash;
} ergo_tx_serializer_input_context_t;

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_init(
    ergo_tx_serializer_input_context_t* context,
    uint8_t box_id[BOX_ID_LEN],
    uint8_t token_frames_count,
    uint32_t proof_data_size,
    token_amount_table_t* tokens_table,
    cx_blake2b_t* hash);

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_tokens(
    ergo_tx_serializer_input_context_t* context,
    uint8_t box_id[BOX_ID_LEN],
    uint8_t token_frame_index,
    buffer_t* tokens);

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_proof(
    ergo_tx_serializer_input_context_t* context,
    buffer_t* chunk);

static inline bool ergo_tx_serializer_input_is_finished(
    ergo_tx_serializer_input_context_t* context) {
    return context->state == ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
}