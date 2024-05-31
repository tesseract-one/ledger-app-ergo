#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <buffer.h>

#include "../constants.h"
#include "../helpers/blake2b.h"
#include "../common/macros_ext.h"

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
    uint8_t distinct_tokens_count;
} ergo_tx_serializer_table_context_t;

static inline uint8_t token_table_find_token_index(const token_table_t* table,
                                                   const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return INDEX_NOT_EXIST;
}

static inline uint8_t token_table_add_token(token_table_t* table,
                                            const uint8_t id[static ERGO_ID_LEN]) {
    if (table->count >= TOKEN_MAX_COUNT) return INDEX_NOT_EXIST;
    uint8_t index = table->count++;
    memmove(table->tokens[index], id, ERGO_ID_LEN);
    return index;
}

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
    return context->tokens_table->count >= context->distinct_tokens_count;
}
