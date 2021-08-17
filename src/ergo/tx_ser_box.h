//
//  tx_ser_box.h
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

typedef enum {
    ERGO_TX_SERIALIZER_BOX_RES_OK = 0xFF,
    ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA = 0xFE,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX = 0x01,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE = 0x02,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS = 0x03,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_REGISTERS = 0x04,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA = 0x05,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER = 0x06,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER = 0x07,
    ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE = 0x08
} ergo_tx_serializer_box_result_e;

typedef struct {
    uint64_t value;
    uint32_t ergo_tree_size;
    uint32_t creation_height;
    uint8_t tokens_count;
    uint8_t registers_count;
    bool is_input_box;
    token_amount_table_t* tokens_table;
    cx_blake2b_t* hash;
} ergo_tx_serializer_box_context_t;

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_init(
    ergo_tx_serializer_box_context_t* context,
    uint64_t value,
    uint32_t ergo_tree_size,
    uint32_t creation_height,
    uint8_t tokens_count,
    uint8_t registers_count,
    bool is_input_box,
    token_amount_table_t* tokens_table,
    cx_blake2b_t* hash
);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tree_chunk(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input
);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tokens(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input
);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_register(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input
);

bool ergo_tx_serializer_box_id_hash_init(
    cx_blake2b_t* hash
);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_id_finalize(
    cx_blake2b_t* hash,
    uint8_t tx_id[static TRANSACTION_HASH_LEN],
    uint16_t box_index,
    uint8_t box_id[static BOX_ID_LEN]
);
