#include "tx_ser_box.h"
#include <string.h>
#include "ergo_tree.h"
#include "../common/gve.h"
#include "../common/macros_ext.h"

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

static inline ergo_tx_serializer_box_result_e add_height_and_token_count(
    ergo_tx_serializer_box_context_t* context) {
    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    if (gve_put_u32(&buffer, context->creation_height) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    rw_buffer_empty(&buffer);
    if (gve_put_u8(&buffer, context->tokens_count) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e
add_empty_registers_count(ergo_tx_serializer_box_context_t* context) {
    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 1);
    if (gve_put_u8(&buffer, 0) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline ergo_tx_serializer_box_result_e box_finished(
    ergo_tx_serializer_box_context_t* context) {
    if (context->callbacks.on_finished != NULL) {
        CHECK_CALL_RESULT_OK(
            context,
            context->callbacks.on_finished(context->type, context->callbacks.context));
    }
    context->state = ERGO_TX_SERIALIZER_BOX_STATE_FINISHED;
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
        if (context->registers_size == 0) {
            CHECK_CALL_RESULT_OK(context, add_empty_registers_count(context));
            return box_finished(context);
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
    uint32_t registers_size,
    cx_blake2b_t* hash) {
    memset(context, 0, sizeof(ergo_tx_serializer_box_context_t));

    if (tokens_count > TOKEN_MAX_COUNT) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS);
    }

    if (ergo_tree_size > MAX_TX_DATA_PART_LEN) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    }

    if (registers_size > MAX_TX_DATA_PART_LEN) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    }

    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 10);
    if (gve_put_u64(&buffer, value) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    context->ergo_tree_size = ergo_tree_size;
    context->creation_height = creation_height;
    context->tokens_count = tokens_count;
    context->registers_size = registers_size;
    context->hash = hash;
    context->value = value;
    context->type = ERGO_TX_SERIALIZER_BOX_TYPE_TREE;
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
    if (context->ergo_tree_size > len && len < MAX_DATA_CHUNK_LEN) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_SMALL_CHUNK);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(tree_chunk), len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    if (!buffer_seek_cur(tree_chunk, len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    context->ergo_tree_size -= len;
    if (context->ergo_tree_size == 0) {
        context->type = ERGO_TX_SERIALIZER_BOX_TYPE_TREE;
        return ergo_tree_added(context);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_miners_fee_tree(
    ergo_tx_serializer_box_context_t* context,
    bool is_mainnet) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED);
    const uint8_t* tree;
    size_t tree_len = 0;
    ergo_tree_miners_fee_tree(is_mainnet, &tree, &tree_len);
    if (!blake2b_update(context->hash, tree, tree_len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    context->type = ERGO_TX_SERIALIZER_BOX_TYPE_FEE;
    return ergo_tree_added(context);
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_change_tree(
    ergo_tx_serializer_box_context_t* context,
    const uint8_t raw_public_key[static PUBLIC_KEY_LEN]) {
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
    buffer_t* input,
    const ergo_tx_serializer_table_context_t* table) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 10);

    while (buffer_data_len(input) > 0) {
        union {
            uint32_t index;
            uint8_t id[ERGO_ID_LEN];
        } token_id;
        uint64_t value;

        if (context->tokens_count == 0) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS);
        }

        if (table == NULL) {  // no token table. working with full ids.
            if (!buffer_read_bytes(input, token_id.id, ERGO_ID_LEN)) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID);
            }
            // hashing input id
            if (!blake2b_update(context->hash, token_id.id, ERGO_ID_LEN)) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
            }
        } else {
            if (!buffer_read_u32(input, &token_id.index, BE)) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX;
            }
            // index should be inside table
            if (token_id.index >= table->distinct_tokens_count) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX);
            }
            // hashing index
            rw_buffer_empty(&buffer);
            if (gve_put_u32(&buffer, token_id.index) != GVE_OK) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
            }
            if (!blake2b_update(context->hash,
                                rw_buffer_read_ptr(&buffer),
                                rw_buffer_data_len(&buffer))) {
                return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
            }
        }

        // reading value
        if (!buffer_read_u64(input, &value, BE)) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE);
        }
        rw_buffer_empty(&buffer);
        if (gve_put_u64(&buffer, value) != GVE_OK) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
        }
        if (!blake2b_update(context->hash,
                            rw_buffer_read_ptr(&buffer),
                            rw_buffer_data_len(&buffer))) {
            return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
        }

        if (context->callbacks.on_token != NULL) {
            const uint8_t* tid =
                table == NULL ? token_id.id : table->tokens_table->tokens[token_id.index];
            CHECK_CALL_RESULT_OK(
                context,
                context->callbacks.on_token(context->type, tid, value, context->callbacks.context));
        }
        context->tokens_count--;
    }
    if (context->tokens_count == 0) {
        if (context->registers_size == 0) {
            CHECK_CALL_RESULT_OK(context, add_empty_registers_count(context));
            return box_finished(context);
        } else {
            context->state = ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED;
        }
        return ERGO_TX_SERIALIZER_BOX_RES_OK;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_add_registers(
    ergo_tx_serializer_box_context_t* context,
    buffer_t* registers_chunk) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED);
    size_t len = buffer_data_len(registers_chunk);
    if (context->registers_size < len) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    }
    if (!blake2b_update(context->hash, buffer_read_ptr(registers_chunk), len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }
    if (!buffer_seek_cur(registers_chunk, len)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    context->registers_size -= len;
    if (context->registers_size == 0) {
        return box_finished(context);
    }
    return ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA;
}

bool ergo_tx_serializer_box_id_hash_init(cx_blake2b_t* hash) {
    return blake2b_256_init(hash);
}

ergo_tx_serializer_box_result_e ergo_tx_serializer_box_id_hash(
    ergo_tx_serializer_box_context_t* context,
    const uint8_t tx_id[static ERGO_ID_LEN],
    uint16_t box_index,
    uint8_t box_id[static ERGO_ID_LEN]) {
    CHECK_PROPER_STATE(context, ERGO_TX_SERIALIZER_BOX_STATE_FINISHED);

    if (!blake2b_update(context->hash, tx_id, ERGO_ID_LEN)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    RW_BUFFER_NEW_LOCAL_EMPTY(buffer, 4);
    if (gve_put_u16(&buffer, box_index) != GVE_OK) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER);
    }
    if (!blake2b_update(context->hash, rw_buffer_read_ptr(&buffer), rw_buffer_data_len(&buffer))) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    if (!blake2b_256_finalize(context->hash, box_id)) {
        return res_error(context, ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    }

    context->state = ERGO_TX_SERIALIZER_BOX_STATE_HASH_FINALIZED;

    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}
