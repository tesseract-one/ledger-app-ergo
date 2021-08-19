//
//  tx_ser_simple.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "tx_ser_simple.h"
#include <os.h>
#include <string.h>

static inline ergo_tx_serializer_simple_result_e map_table_result(
    ergo_tx_serializer_table_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_TABLE_RES_OK:
            return ERGO_TX_SERIALIZER_SIMPLE_RES_OK;
        case ERGO_TX_SERIALIZER_TABLE_RES_MORE_DATA:
            return ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID:
            return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS:
            return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER:
            return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_HASHER;
        case ERGO_TX_SERIALIZER_TABLE_RES_ERR_BUFFER:
            return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BUFFER;
    }
}

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_init(
    ergo_tx_serializer_simple_context_t* context,
    uint32_t prefix_data_size,
    uint32_t suffix_data_size,
    uint8_t tokens_count,
    token_amount_table_t* tokens_table) {
    memset(context, 0, sizeof(ergo_tx_serializer_simple_context_t));
    if (prefix_data_size == 0) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_PREFIX_LEN;
    }

    if (suffix_data_size == 0) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_SUFFIX_LEN;
    }

    if (!blake2b_256_init(&context->hash)) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_HASHER;
    }

    context->prefix_data_size = prefix_data_size;
    context->suffix_data_size = suffix_data_size;

    return map_table_result(
        ergo_tx_serializer_table_init(&context->table_ctx, tokens_count, tokens_table));
}

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_add_prefix(
    ergo_tx_serializer_simple_context_t* context,
    buffer_t* chunk) {
    size_t len = buffer_data_len(chunk);
    if (context->prefix_data_size < len) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MUCH_DATA;
    }
    if (!blake2b_update(&context->hash, buffer_read_ptr(chunk), len)) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_HASHER;
    }
    if (!buffer_seek_read_cur(chunk, len)) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BUFFER;
    }
    context->prefix_data_size -= len;
    if (context->prefix_data_size == 0) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_OK;
    }
    return ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA;
}

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_add_tokens(
    ergo_tx_serializer_simple_context_t* context,
    buffer_t* tokens) {
    if (context->prefix_data_size > 0) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_STATE;
    }
    ergo_tx_serializer_simple_result_e res =
        map_table_result(ergo_tx_serializer_table_add(&context->table_ctx, tokens));
    if (res == ERGO_TX_SERIALIZER_SIMPLE_RES_OK) {
        res = map_table_result(ergo_tx_serializer_table_hash(&context->table_ctx, &context->hash));
    }
    return res;
}

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_add_suffix(
    ergo_tx_serializer_simple_context_t* context,
    buffer_t* chunk) {
    if (!ergo_tx_serializer_table_is_finished(&context->table_ctx)) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_STATE;
    }
    size_t len = buffer_data_len(chunk);
    if (context->suffix_data_size < len) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MUCH_DATA;
    }
    if (!blake2b_update(&context->hash, buffer_read_ptr(chunk), len)) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_HASHER;
    }
    if (!buffer_seek_read_cur(chunk, len)) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BUFFER;
    }
    context->suffix_data_size -= len;
    if (context->suffix_data_size == 0) {
        return ERGO_TX_SERIALIZER_SIMPLE_RES_OK;
    }
    return ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA;
}

bool ergo_tx_serializer_simple_finalize(ergo_tx_serializer_simple_context_t* context,
                                        uint8_t tx_id[static TRANSACTION_HASH_LEN]) {
    if (context->suffix_data_size > 0) {
        return false;
    }
    return blake2b_256_finalize(&context->hash, tx_id);
}
