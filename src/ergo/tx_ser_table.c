#include "tx_ser_table.h"
#include "../common/gve.h"

static inline ergo_tx_serializer_table_result_e parse_token(buffer_t* tokens,
                                                            token_table_t* table,
                                                            uint8_t tokens_max) {
    if (table->count >= tokens_max) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS;
    }
    if (!buffer_read_bytes(tokens, table->tokens[table->count++], ERGO_ID_LEN)) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID;
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_init(
    ergo_tx_serializer_table_context_t* context,
    uint8_t tokens_count,
    token_table_t* tokens_table) {
    // tokens table should be empty.
    // we add distinct tokens to the start of the table
    if (tokens_table->count != 0 || tokens_count > TOKEN_MAX_COUNT) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS;
    }
    // clean tokens area (we will search)
    memset(&tokens_table->tokens, 0, MEMBER_SIZE(token_table_t, tokens));
    // save in context
    context->distinct_tokens_count = tokens_count;
    context->tokens_table = tokens_table;
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_add(
    ergo_tx_serializer_table_context_t* context,
    buffer_t* tokens) {
    while (buffer_data_len(tokens) > 0) {
        ergo_tx_serializer_table_result_e res =
            parse_token(tokens, context->tokens_table, context->distinct_tokens_count);
        if (res != ERGO_TX_SERIALIZER_TABLE_RES_OK) {
            return res;
        }
    }
    if (ergo_tx_serializer_table_is_finished(context)) {
        return ERGO_TX_SERIALIZER_TABLE_RES_OK;
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_MORE_DATA;
}

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_hash(
    const ergo_tx_serializer_table_context_t* context,
    cx_blake2b_t* hash) {
    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    if (gve_put_u32(&buffer, context->distinct_tokens_count) != GVE_OK) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_BUFFER;
    }
    if (!blake2b_update(hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER;
    }
    for (uint8_t i = 0; i < context->distinct_tokens_count; i++) {
        if (!blake2b_update(hash, context->tokens_table->tokens[i], ERGO_ID_LEN)) {
            return ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER;
        }
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}
