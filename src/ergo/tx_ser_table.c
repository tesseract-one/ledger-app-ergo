//
//  tx_ser_table.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "tx_ser_table.h"
#include <os.h>
#include "../common/varint.h"

static inline ergo_tx_serializer_table_result_e parse_token(
    buffer_t *input,
    token_amount_table_t* table,
    uint8_t tokens_max
) {
    if (table->count >= tokens_max) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS;
    }
    if (!buffer_read_bytes(input, table->tokens[table->count++].id, TOKEN_ID_LEN)) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID;
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}

void ergo_tx_serializer_table_init(
    ergo_tx_serializer_table_context_t* context,
    uint8_t tokens_count,
    token_amount_table_t* tokens_table
) {
    context->tokens_count = tokens_count;
    context->tokens_table = tokens_table;
    context->tokens_table->count = 0;
}

ergo_tx_serializer_table_result_e ergo_tx_serializer_table_add(
    ergo_tx_serializer_table_context_t* context,
    buffer_t* input
) {
    while (buffer_data_len(input) > 0) {
        ergo_tx_serializer_table_result_e res = parse_token(
            input,
            context->tokens_table,
            context->tokens_count
        );
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
    cx_blake2b_t* hash
) {
    uint8_t ser_buf[10];
    buffer_t buffer = {0};
    buffer_init(&buffer, ser_buf, sizeof(ser_buf), 0);
    if (gve_put_u32(&buffer, context->tokens_table->count) != GVE_OK) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_BUFFER;
    }
    if (!blake2b_update(hash, buffer.ptr, buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER;
    }
    for (uint8_t i = 0; i < context->tokens_table->count; i++) {
        if (!blake2b_update(hash, context->tokens_table->tokens[i].id, TOKEN_ID_LEN)) {
            return ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER;
        }
    }
    return ERGO_TX_SERIALIZER_TABLE_RES_OK;
}
