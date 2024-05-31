#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <buffer.h>

#include "../constants.h"
#include "../helpers/blake2b.h"
#include "tx_ser_table.h"

typedef enum {
    ERGO_TX_SERIALIZER_BOX_RES_OK = 0x7F,
    ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA = 0x7E,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX = 0x01,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID = 0x02,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE = 0x03,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS = 0x04,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA = 0x05,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER = 0x06,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER = 0x07,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE = 0x08,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW = 0x09,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_SMALL_CHUNK = 0x0A
} ergo_tx_serializer_box_result_e;

typedef enum {
    ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED,
    ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED,
    ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED,
    ERGO_TX_SERIALIZER_BOX_STATE_FINISHED,
    ERGO_TX_SERIALIZER_BOX_STATE_HASH_FINALIZED,
    ERGO_TX_SERIALIZER_BOX_STATE_ERROR
} ergo_tx_serializer_box_state_e;

typedef enum {
    ERGO_TX_SERIALIZER_BOX_TYPE_FEE,
    ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE,
    ERGO_TX_SERIALIZER_BOX_TYPE_TREE
} ergo_tx_serializer_box_type_e;

typedef ergo_tx_serializer_box_result_e (
    *ergo_tx_serializer_box_type_cb)(ergo_tx_serializer_box_type_e, uint64_t, void*);
typedef ergo_tx_serializer_box_result_e (*ergo_tx_serializer_box_token_cb)(
    ergo_tx_serializer_box_type_e,
    const uint8_t[static ERGO_ID_LEN],
    uint64_t,
    void*);
typedef ergo_tx_serializer_box_result_e (
    *ergo_tx_serializer_box_finished_cb)(ergo_tx_serializer_box_type_e, void*);

typedef struct {
    void* context;
    ergo_tx_serializer_box_type_cb on_type;
    ergo_tx_serializer_box_token_cb on_token;
    ergo_tx_serializer_box_finished_cb on_finished;
} ergo_tx_serializer_box_callbacks_t;

typedef struct {
    ergo_tx_serializer_box_callbacks_t callbacks;
    uint64_t value;
    uint32_t ergo_tree_size;
    uint32_t creation_height;
    uint32_t registers_size;
    uint8_t tokens_count;
    ergo_tx_serializer_box_state_e state;
    ergo_tx_serializer_box_type_e type;
    cx_blake2b_t* hash;
} ergo_tx_serializer_box_context_t;

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_init(
    ergo_tx_serializer_box_context_t* context,
    uint64_t value,
    uint32_t ergo_tree_size,
    uint32_t creation_height,
    uint8_t tokens_count,
    uint32_t registers_size,
    cx_blake2b_t* hash);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tree(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* tree_chunk);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_miners_fee_tree(
    ergo_tx_serializer_box_context_t* context,
    bool is_mainnet);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_change_tree(
    ergo_tx_serializer_box_context_t* context,
    const uint8_t raw_public_key[static PUBLIC_KEY_LEN]);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tokens(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* tokens,
    const ergo_tx_serializer_table_context_t* table);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_registers(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* registers_chunk);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_id_hash(
    ergo_tx_serializer_box_context_t* context,
    const uint8_t tx_id[static ERGO_ID_LEN],
    uint16_t box_index,
    uint8_t box_id[static ERGO_ID_LEN]);

bool ergo_tx_serializer_box_id_hash_init(cx_blake2b_t* hash);

static inline bool ergo_tx_serializer_box_is_finished(
    const ergo_tx_serializer_box_context_t* context) {
    return context->state == ERGO_TX_SERIALIZER_BOX_STATE_FINISHED;
}

static inline void ergo_tx_serializer_box_set_callbacks(
    ergo_tx_serializer_box_context_t* context,
    ergo_tx_serializer_box_type_cb on_type,
    ergo_tx_serializer_box_token_cb on_token,
    ergo_tx_serializer_box_finished_cb on_finished,
    void* cb_context) {
    context->callbacks.on_type = on_type;
    context->callbacks.on_token = on_token;
    context->callbacks.on_finished = on_finished;
    context->callbacks.context = cb_context;
}
