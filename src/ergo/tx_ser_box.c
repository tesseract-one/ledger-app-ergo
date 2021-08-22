#include "tx_ser_box.h"
#include <os.h>
#include <string.h>
#include "ergo_tree.h"
#include "../common/varint.h"
#include "../common/macros.h"

#define CHECK_PROPER_STATE(_ctx, _state) \
    if (_ctx->state != _state) return res_error(_ctx, ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE)

#define CHECK_CALL_RESULT_OK(_ctx, _call)                                                        \
    do {                                                                                         \
        ergo_tx_serializer_box_result_e res = _call;                                             \
        if (res != ERGO_TX_SERIALIZER_BOX_RES_OK && res != ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA) \
            return res_error(_ctx, res);                                                         \
    } while (0)

static inline ergo_tx_serializer_box_result_e res_error(ergo_tx_serializer_box_context_t* context,
                                                        ergo_tx_serializer_box_result_e err) {
    context->state = ERGO_TX_SERIALIZER_BOX_STATE_ERROR;
    return err;
}

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
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    buffer_empty(&buffer);
    if (gve_put_u8(&buffer, context->tokens_count) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e
add_registers_count(ergo_tx_serializer_box_context_t* context) {
    BUFFER_NEW_LOCAL_EMPTY(buffer, 1);
    if (gve_put_u8(&buffer, context->registers_count) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e
ergo_tree_added(ergo_tx_serializer_box_context_t* context) {
    if (context->callbacks.on_type != NULL) {
        CHECK_CALL_RESULT_OK(
            context,
            context->callbacks.on_type(context->type, context->value, context->callbacks.context));
    }
    CHECK_CALL_RESULT_OK(context, add_height_and_token_count(context));
    if (context->tokens_count == 0) {
        context->state = ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED;
        if (context->registers_count == 0) {
            CHECK_CALL_RESULT_OK(context, add_registers_count(context));
            if (context->type == ERGO_TX_SERIALIZER_BOX_TYPE_INPUT) {
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
    bool is_input,
    token_table_t* tokens_table,
    cx_blake2b_t* hash) {
    memset(context, 0, sizeof(ergo_tx_serializer_box_context_t));

    if (tokens_count > tokens_table->count) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS);
    }

    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    if (gve_put_u64(&buffer, value) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    context->ergo_tree_size = ergo_tree_size;
    context->creation_height = creation_height;
    context->tokens_count = tokens_count;
    context->registers_count = registers_count;
    context->tokens_table = tokens_table;
    context->hash = hash;
    context->type =
        is_input ? ERGO_TX_SERIALIZER_BOX_TYPE_INPUT : ERGO_TX_SERIALIZER_BOX_TYPE_OUTPUT;
    context->value = value;
    context->state = ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED;

    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tree(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* tree_chunk) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED);
    size_t len = buffer_data_len(tree_chunk);
    if (context->ergo_tree_size < len) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(tree_chunk), len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    if (!buffer_seek_read_cur(tree_chunk, len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    context->ergo_tree_size -= len;
    if (context->ergo_tree_size == 0) {
        if (context->type != ERGO_TX_SERIALIZER_BOX_TYPE_INPUT) {
            context->type = ERGO_TX_SERIALIZER_BOX_TYPE_OUTPUT;
        }
        return ergo_tree_added(context);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_miners_fee_tree(
    ergo_tx_serializer_box_context_t* context) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED);
    if (!blake2b_update(context->hash,
                        PIC(C_ERGO_TREE_MINERS_HASH_FEE),
                        sizeof(C_ERGO_TREE_MINERS_HASH_FEE))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    context->type = ERGO_TX_SERIALIZER_BOX_TYPE_FEE;
    return ergo_tree_added(context);
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_change_tree(
    ergo_tx_serializer_box_context_t* context,
    uint8_t raw_public_key[static PUBLIC_KEY_LEN]) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED);

    uint8_t tree[ERGO_TREE_P2PK_LEN];
    ergo_tree_generate_p2pk(raw_public_key, tree);
    if (!blake2b_update(context->hash, tree, ERGO_TREE_P2PK_LEN)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    context->type = ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE;
    return ergo_tree_added(context);
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_tokens(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* input) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
    BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    while (buffer_data_len(input) > 0) {
        uint32_t index;
        uint64_t value;

        if (context->tokens_count == 0) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS);
        }

        CHECK_CALL_RESULT_OK(context, parse_token(input, &index, &value));

        if (index >= context->tokens_table->count) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX);
        }

        if (context->type == ERGO_TX_SERIALIZER_BOX_TYPE_INPUT) {
            // INPUT BOX ID has token ids instead of indexes
            if (!blake2b_update(context->hash,
                                context->tokens_table->tokens[(uint8_t) index],
                                TOKEN_ID_LEN)) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
            }
        } else {
            buffer_empty(&buffer);
            if (gve_put_u32(&buffer, index) != GVE_OK) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
            }
            if (!blake2b_update(context->hash,
                                buffer_read_ptr(&buffer),
                                buffer_data_len(&buffer))) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
            }
        }

        buffer_empty(&buffer);
        if (gve_put_u64(&buffer, value) != GVE_OK) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
        }
        if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
        }

        if (context->callbacks.on_token != NULL) {
            CHECK_CALL_RESULT_OK(context,
                                 context->callbacks.on_token(context->type,
                                                             (uint8_t) index,
                                                             value,
                                                             context->callbacks.context));
        }
        context->tokens_count--;
    }
    if (context->tokens_count == 0) {
        CHECK_CALL_RESULT_OK(context, add_registers_count(context));
        if (context->registers_count == 0) {
            if (context->type == ERGO_TX_SERIALIZER_BOX_TYPE_INPUT) {
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
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED);
    if (context->registers_count == 0) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_REGISTERS);
    }
    size_t len = buffer_data_len(value);
    if (!blake2b_update(context->hash, buffer_read_ptr(value), len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    if (!buffer_seek_read_cur(value, len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    context->registers_count--;
    if (context->registers_count == 0) {
        if (context->type == ERGO_TX_SERIALIZER_BOX_TYPE_INPUT) {
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
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_REGISTERS_ADDED);

    if (!blake2b_update(context->hash, tx_id, TRANSACTION_HASH_LEN)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    BUFFER_NEW_LOCAL_EMPTY(buffer, 4);
    if (gve_put_u16(&buffer, box_index) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(&buffer), buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
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
