#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <buffer.h>

#include "../constants.h"
#include "../helpers/blake2b.h"
#include "tx_ser_table.h"

typedef enum {
    ERGO_TX_SERIALIZER_INPUT_RES_OK = 0x7F,
    ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA = 0x7E,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE = 0x01,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_INPUT_ID = 0x02,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_FRAME_INDEX = 0x03,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID = 0x04,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_VALUE = 0x05,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_INPUT_FRAMES = 0x06,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MUCH_PROOF_DATA = 0x07,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_HASHER = 0x08,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BUFFER = 0x09,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE = 0x0A,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW = 0x0B,
    ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_TOKENS = 0x0C
} ergo_tx_serializer_input_result_e;

typedef enum {
    ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED,
    ERGO_TX_SERIALIZER_INPUT_STATE_EXTENSION_STARTED,
    ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED,
    ERGO_TX_SERIALIZER_INPUT_STATE_ERROR
} ergo_tx_serializer_input_state_e;

typedef ergo_tx_serializer_input_result_e (*ergo_tx_serializer_input_token_cb)(
    const uint8_t[static ERGO_ID_LEN],
    const uint8_t[static ERGO_ID_LEN],
    uint64_t,
    void*);

typedef struct {
    ergo_tx_serializer_input_state_e state;
    uint8_t box_id[ERGO_ID_LEN];
    uint8_t frames_count;
    uint8_t frames_processed;
    uint32_t context_extension_data_size;
    ergo_tx_serializer_input_token_cb on_token_cb;
    void* callback_context;
    token_table_t* tokens_table;
    cx_blake2b_t* hash;
} ergo_tx_serializer_input_context_t;

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_init(
    ergo_tx_serializer_input_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t token_frames_count,
    uint32_t proof_data_size,
    token_table_t* tokens_table,
    cx_blake2b_t* hash);

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_tokens(
    ergo_tx_serializer_input_context_t* context,
    const uint8_t box_id[ERGO_ID_LEN],
    uint8_t token_frame_index,
    buffer_t* tokens);

ergo_tx_serializer_input_result_e ergo_tx_serializer_input_add_context_extension(
    ergo_tx_serializer_input_context_t* context,
    buffer_t* chunk);

static inline void ergo_tx_serializer_input_set_callback(
    ergo_tx_serializer_input_context_t* context,
    ergo_tx_serializer_input_token_cb callback,
    void* cb_context) {
    context->on_token_cb = callback;
    context->callback_context = cb_context;
}

static inline bool ergo_tx_serializer_input_is_finished(
    ergo_tx_serializer_input_context_t* context) {
    return context->state == ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED;
}