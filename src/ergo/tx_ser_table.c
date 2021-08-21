//
//  tx_ser_table.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "tx_ser_table.h"
#include <os.h>
#include "../common/varint.h"

static inline ergo_tx_serializer_table_result_e parse_token(buffer_t* tokens,
                                                            token_table_t* table,
                                                            uint8_t tokens_max) {
    if (table->count >= tokens_max) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS;
    }
    if (!buffer_read_bytes(tokens, table->tokens[table->count++], TOKEN_ID_LEN)) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID;
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_init(
    ergo_tx_serializer_table_context_t* context,
    uint8_t tokens_count,
    token_table_t* tokens_table) {
    if (tokens_count + tokens_table->count > TOKEN_MAX_COUNT) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS;
    }
    context->tokens_count = tokens_count;
    context->tokens_table = tokens_table;
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_add(
    ergo_tx_serializer_table_context_t* context,
    buffer_t* tokens) {
    while (buffer_data_len(tokens) > 0) {
        ergo_tx_serializer_table_result_e res =
            parse_token(tokens, context->tokens_table, context->tokens_count);
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
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    if (gve_put_u32(&buffer, context->tokens_table->count) != GVE_OK) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_BUFFER;
    }
    if (!blake2b_update(hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER;
    }
    for (uint8_t i = 0; i < context->tokens_table->count; i++) {
        if (!blake2b_update(hash, context->tokens_table->tokens[i], TOKEN_ID_LEN)) {
            return ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER;
        }
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}
