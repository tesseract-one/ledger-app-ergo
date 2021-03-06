#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../constants.h"
#include "../common/buffer.h"
#include "../helpers/blake2b.h"

typedef struct {
    uint8_t count;
    uint8_t tokens[TOKEN_MAX_COUNT][ERGO_ID_LEN];
} token_table_t;

typedef enum {
    ERGO_TX_SERIALIZER_TABLE_RES_OK = 0x7F,
    ERGO_TX_SERIALIZER_TABLE_RES_MORE_DATA = 0x7E,
    ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID = 0x01,
    ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS = 0x02,
    ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER = 0x03,
    ERGO_TX_SERIALIZER_TABLE_RES_ERR_BUFFER = 0x04
} ergo_tx_serializer_table_result_e;

typedef struct {
    token_table_t* tokens_table;
    uint8_t tokens_count;
} ergo_tx_serializer_table_context_t;

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_init(
    ergo_tx_serializer_table_context_t* context,
    uint8_t tokens_count,
    token_table_t* tokens_table);

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_add(
    ergo_tx_serializer_table_context_t* context,
    buffer_t* tokens);

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_hash(
    const ergo_tx_serializer_table_context_t* context,
    cx_blake2b_t* hash);

static inline bool ergo_tx_serializer_table_is_finished(
    const ergo_tx_serializer_table_context_t* context) {
    return context->tokens_table->count >= context->tokens_count;
}
