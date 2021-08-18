//
//  tx_ser_box.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "tx_ser_box.h"
#include <os.h>
#include <string.h>
#include "../common/varint.h"

static inline ergo_tx_serializer_box_result_e parse_token(
    buffer_t *input,
    uint32_t* index,
    uint64_t* value
) {
    if (!buffer_read_u32(input, index, BE)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX;
    }
    if (!buffer_read_u64(input, value, BE)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline ergo_tx_serializer_box_result_e add_height_and_token_count(
    ergo_tx_serializer_box_context_t* context
) {
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    if (gve_put_u32(&buffer, context->creation_height) != GVE_OK) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer.ptr, buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }

    buffer_empty(&buffer);
    if (gve_put_u8(&buffer, context->tokens_count) != GVE_OK) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer.ptr, buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline ergo_tx_serializer_box_result_e add_registers_count(
    ergo_tx_serializer_box_context_t* context
) {
    BUFFER_NEW_LOCAL_EMPTY(buffer, 1);
    if (gve_put_u8(&buffer, context->registers_count) != GVE_OK) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer.ptr, buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

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
) {
    memset(context, 0, sizeof(ergo_tx_serializer_box_context_t));
    
    if (tokens_count > tokens_table->count) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS;
    }
    
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    if (gve_put_u64(&buffer, value) != GVE_OK) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(hash, buffer.ptr, buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    
    context->ergo_tree_size = ergo_tree_size;
    context->creation_height = creation_height;
    context->tokens_count = tokens_count;
    context->registers_count = registers_count;
    context->tokens_table = tokens_table;
    context->hash = hash;
    context->is_input_box = is_input_box;
    context->value = value;
    
    if (is_input_box) {
        for (uint8_t i = 0; i < tokens_table->count; i++) {
            tokens_table->tokens[i].amount = 0;
        }
    }
    
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tree_chunk(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input
) {
    size_t len = buffer_data_len(input);
    if (context->ergo_tree_size < len) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA;
    }
    bool is_ok = blake2b_update(context->hash,
                                input->ptr + input->read_offset,
                                len);
    if (!is_ok) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    if (!buffer_seek_read_cur(input, len)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    context->ergo_tree_size -= len;
    if (context->ergo_tree_size == 0) {
        return add_height_and_token_count(context);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tokens(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input
) {
    if (context->ergo_tree_size > 0) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    ergo_tx_serializer_box_result_e res = ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
    while (buffer_data_len(input) > 0) {
        uint32_t index;
        uint64_t value;
        
        if (context->tokens_count == 0) {
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS;
        }
        
        res = parse_token(input, &index, &value);
        if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
            return res;
        }
        if (index >= context->tokens_table->count) {
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX;
        }
        
        if (context->is_input_box) { // INPUT BOX ID has token ids instead of indexes
            if (!blake2b_update(context->hash,
                               context->tokens_table->tokens[(uint8_t)index].id,
                               TOKEN_ID_LEN)) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
            }
        } else {
            if (context->tokens_table->tokens[index].amount < value) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE;
            }
            buffer_empty(&buffer);
            if (gve_put_u32(&buffer, index) != GVE_OK) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
            }
            if (!blake2b_update(context->hash, buffer.ptr, buffer_data_len(&buffer))) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
            }
        }
        
        buffer_empty(&buffer);
        if (gve_put_u64(&buffer, value) != GVE_OK) {
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
        }
        if (!blake2b_update(context->hash, buffer.ptr, buffer_data_len(&buffer))) {
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
        }
        
        if (context->is_input_box) { // setup amount from input box
            context->tokens_table->tokens[index].amount = value;
        } else { // decrease spent amount
            context->tokens_table->tokens[index].amount -= value;
        }
        context->tokens_count--;
    }
    if (context->tokens_count == 0) {
        return add_registers_count(context);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_register(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input
) {
    if (context->tokens_count > 0) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    if (context->registers_count == 0) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_REGISTERS;
    }
    size_t len = buffer_data_len(input);
    if (!blake2b_update(context->hash, input->ptr, len)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    if (!buffer_seek_read_cur(input, len)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    context->registers_count--;
    if (context->registers_count == 0) {
        return ERGO_TX_SERIALIZER_BOX_RES_OK;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

bool ergo_tx_serializer_box_id_hash_init(
    cx_blake2b_t* hash
) {
    return blake2b_256_init(hash);
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_id_finalize(
    cx_blake2b_t* hash,
    uint8_t tx_id[static TRANSACTION_HASH_LEN],
    uint16_t box_index,
    uint8_t box_id[static BOX_ID_LEN]
) {
    if (!blake2b_update(hash, tx_id, TRANSACTION_HASH_LEN)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    if (gve_put_u16(&buffer, box_index) != GVE_OK) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(hash, buffer.ptr, buffer_data_len(&buffer))) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    
    if (!blake2b_256_finalize(hash, box_id)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}
