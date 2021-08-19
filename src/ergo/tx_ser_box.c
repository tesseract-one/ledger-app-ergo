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
#include "../common/int_ops.h"

static const uint8_t S_MINERS_HASH_FEE[] = {
    0x10, 0x05, 0x04, 0x00, 0x04, 0x00, 0x0e, 0x36, 0x10, 0x02, 0x04, 0xa0, 0x0b, 0x08, 0xcd,
    0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87,
    0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16,
    0xf8, 0x17, 0x98, 0xea, 0x02, 0xd1, 0x92, 0xa3, 0x9a, 0x8c, 0xc7, 0xa7, 0x01, 0x73, 0x00,
    0x73, 0x01, 0x10, 0x01, 0x02, 0x04, 0x02, 0xd1, 0x96, 0x83, 0x03, 0x01, 0x93, 0xa3, 0x8c,
    0xc7, 0xb2, 0xa5, 0x73, 0x00, 0x00, 0x01, 0x93, 0xc2, 0xb2, 0xa5, 0x73, 0x01, 0x00, 0x74,
    0x73, 0x02, 0x73, 0x03, 0x83, 0x01, 0x08, 0xcd, 0xee, 0xac, 0x93, 0xb1, 0xa5, 0x73, 0x04};

static inline ergo_tx_serializer_box_result_e parse_token(buffer_t* input,
                                                          uint32_t* index,
                                                          uint64_t* value) {
    if (!buffer_read_u32(input, index, BE)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX;
    }
    if (!buffer_read_u64(input, value, BE)) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline ergo_tx_serializer_box_result_e add_height_and_token_count(
    ergo_tx_serializer_box_context_t* context) {
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    if (gve_put_u32(&buffer, context->creation_height) != GVE_OK) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }

    buffer_empty(&buffer);
    if (gve_put_u8(&buffer, context->tokens_count) != GVE_OK) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }

    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline ergo_tx_serializer_box_result_e add_registers_count(
    ergo_tx_serializer_box_context_t* context) {
    BUFFER_NEW_LOCAL_EMPTY(buffer, 1);
    if (gve_put_u8(&buffer, context->registers_count) != GVE_OK) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }

    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline ergo_tx_serializer_box_result_e ergo_tree_added(
    ergo_tx_serializer_box_context_t* context) {
    ergo_tx_serializer_box_result_e res = add_height_and_token_count(context);
    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return res;
    }
    if (context->tokens_count == 0) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED;
        if (context->registers_count == 0) {
            res = add_registers_count(context);
            if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
                return res;
            }
            if (context->is_input_box) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_REGISTERS_ADDED;
            } else {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_FINISHED;
            }
        }
    } else {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED;
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
    cx_blake2b_t* hash) {
    memset(context, 0, sizeof(ergo_tx_serializer_box_context_t));

    if (tokens_count > tokens_table->count) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS;
    }

    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    if (gve_put_u64(&buffer, value) != GVE_OK) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
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
    context->state = ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED;

    if (is_input_box) {
        for (uint8_t i = 0; i < tokens_table->count; i++) {
            tokens_table->tokens[i].amount = 0;
        }
    }

    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tree(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* tree_chunk) {
    if (context->state != ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    size_t len = buffer_data_len(tree_chunk);
    if (context->ergo_tree_size < len) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA;
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(tree_chunk), len)) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    if (!buffer_seek_read_cur(tree_chunk, len)) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    context->ergo_tree_size -= len;
    if (context->ergo_tree_size == 0) {
        return ergo_tree_added(context);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_miners_fee_tree(
    ergo_tx_serializer_box_context_t* context) {
    if (!blake2b_update(context->hash, PIC(S_MINERS_HASH_FEE), sizeof(S_MINERS_HASH_FEE))) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    return ergo_tree_added(context);
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_change_tree(
    ergo_tx_serializer_box_context_t* context,
    uint32_t* bip32_path,
    uint8_t bip32_path_len);

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tokens(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input) {
    if (context->state != ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    ergo_tx_serializer_box_result_e res = ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
    while (buffer_data_len(input) > 0) {
        uint32_t index;
        uint64_t value;

        if (context->tokens_count == 0) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS;
        }

        res = parse_token(input, &index, &value);
        if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
            return res;
        }
        if (index >= context->tokens_table->count) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX;
        }

        if (context->is_input_box) {  // INPUT BOX ID has token ids instead of indexes
            if (!blake2b_update(context->hash,
                                context->tokens_table->tokens[(uint8_t) index].id,
                                TOKEN_ID_LEN)) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
            }
        } else {
            if (context->tokens_table->tokens[index].amount < value) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE;
            }
            buffer_empty(&buffer);
            if (gve_put_u32(&buffer, index) != GVE_OK) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
            }
            if (!blake2b_update(context->hash,
                                buffer_read_ptr(&buffer),
                                buffer_data_len(&buffer))) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
            }
        }

        buffer_empty(&buffer);
        if (gve_put_u64(&buffer, value) != GVE_OK) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
        }
        if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
        }

        if (context->is_input_box) {  // setup amount from input box
            context->tokens_table->tokens[index].amount = value;
        } else {  // decrease spent amount
            if (!checked_sub_u64(context->tokens_table->tokens[index].amount,
                                 value,
                                 &context->tokens_table->tokens[index].amount)) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
            }
        }
        context->tokens_count--;
    }
    if (context->tokens_count == 0) {
        ergo_tx_serializer_box_result_e res = add_registers_count(context);
        if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
            return res;
        }
        if (context->registers_count == 0) {
            if (context->is_input_box) {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_REGISTERS_ADDED;
            } else {
                context->state = ERGO_TX_SERIALIZER_BOX_STATE_FINISHED;
            }
        } else {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED;
        }
        return ERGO_TX_SERIALIZER_BOX_RES_OK;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_register(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* value) {
    if (context->state > ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    if (context->registers_count == 0) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_REGISTERS;
    }
    size_t len = buffer_data_len(value);
    if (!blake2b_update(context->hash, buffer_read_ptr(value), len)) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }
    if (!buffer_seek_read_cur(value, len)) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    context->registers_count--;
    if (context->registers_count == 0) {
        if (context->is_input_box) {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_REGISTERS_ADDED;
        } else {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_FINISHED;
        }
        return ERGO_TX_SERIALIZER_BOX_RES_OK;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tx_id_and_index(
    ergo_tx_serializer_box_context_t* context,
    uint8_t tx_id[static TRANSACTION_HASH_LEN],
    uint16_t box_index) {
    if (context->state != ERGO_TX_SERIALIZER_BOX_STATE_REGISTERS_ADDED) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    if (!blake2b_update(context->hash, tx_id, TRANSACTION_HASH_LEN)) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }

    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    if (gve_put_u16(&buffer, box_index) != GVE_OK) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER;
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
    }

    context->state = ERGO_TX_SERIALIZER_BOX_STATE_FINISHED;
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

bool ergo_tx_serializer_box_id_hash_init(cx_blake2b_t* hash) {
    return blake2b_256_init(hash);
}

bool ergo_tx_serializer_box_id_hash(ergo_tx_serializer_box_context_t* context,
                                    uint8_t box_id[static BOX_ID_LEN]) {
    if (context->state != ERGO_TX_SERIALIZER_BOX_STATE_FINISHED) {
        return false;
    }
    return blake2b_256_finalize(context->hash, box_id);
}
