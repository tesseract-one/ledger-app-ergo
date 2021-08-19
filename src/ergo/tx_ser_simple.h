//
//  tx_ser_simple.h
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../constants.h"
#include "../common/buffer.h"
#include "../helpers/blake2b.h"
#include "tx_ser_table.h"

typedef struct {
    uint32_t prefix_data_size;
    uint32_t suffix_data_size;
    cx_blake2b_t hash;
    ergo_tx_serializer_table_context_t table_ctx;
} ergo_tx_serializer_simple_context_t;

typedef enum {
    ERGO_TX_SERIALIZER_SIMPLE_RES_OK = 0x7F,
    ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA = 0x7E,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_TOKEN_ID = 0x01,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_PREFIX_LEN = 0x02,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_SUFFIX_LEN = 0x03,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MANY_TOKENS = 0x04,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MUCH_DATA = 0x05,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_HASHER = 0x06,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BUFFER = 0x07,
    ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_STATE = 0x08
} ergo_tx_serializer_simple_result_e;

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_init(
    ergo_tx_serializer_simple_context_t* context,
    uint32_t prefix_data_size,
    uint32_t suffix_data_size,
    uint8_t tokens_count,
    token_amount_table_t* tokens_table);

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_add_prefix(
    ergo_tx_serializer_simple_context_t* context,
    buffer_t* chunk);

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_add_tokens(
    ergo_tx_serializer_simple_context_t* context,
    buffer_t* tokens);

ergo_tx_serializer_simple_result_e ergo_tx_serializer_simple_add_suffix(
    ergo_tx_serializer_simple_context_t* context,
    buffer_t* chunk);

bool ergo_tx_serializer_simple_hash(ergo_tx_serializer_simple_context_t* context,
                                    uint8_t tx_id[static TRANSACTION_HASH_LEN]);
