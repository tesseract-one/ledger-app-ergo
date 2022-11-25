#include "stx_output.h"
#include "../../sw.h"
#include "../../common/macros.h"
#include "../../common/base58.h"
#include "../../ergo/ergo_tree.h"
#include "../../ergo/address.h"
#include <string.h>

#define STX_OUTPUT_SET_BOX_FINISHED(ctx) (ctx->state = ctx->state | 0x80)
#define STX_OUTPUT_SET_TREE_SET(ctx)     (ctx->state = ctx->state | 0x40)
#define STX_OUTPUT_SET_TYPE(ctx, type) \
    (ctx->state = (ctx->state & 0xC0) | (((uint8_t) type) & 0x3F))

static inline uint8_t find_token_index(const token_table_t* table,
                                       const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return INDEX_NOT_EXIST;
}

static inline ergo_tx_serializer_box_result_e maybe_finished(
    sign_transaction_output_info_ctx_t* ctx) {
    if (stx_output_info_is_finished(ctx)) {
        if (STX_OUTPUT_INFO_TYPE(ctx) == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT) {
            uint8_t hash[ERGO_ID_LEN];
            if (!blake2b_256_finalize(&ctx->tree_hash_ctx, hash)) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
            }
            memmove(ctx->tree_hash, hash, ERGO_ID_LEN);
        }
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static inline uint16_t map_box_result(ergo_tx_serializer_box_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return SW_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return SW_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX:
            return SW_BAD_TOKEN_INDEX;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID:
            return SW_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE:
            return SW_BAD_TOKEN_VALUE;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS:
            return SW_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA:
            return SW_TOO_MUCH_DATA;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER:
            return SW_HASHER_ERROR;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER:
            return SW_BUFFER_ERROR;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW:
            return SW_U64_OVERFLOW;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE:
            return SW_BAD_STATE;
    }
}

ergo_tx_serializer_box_result_e stx_output_info_set_expected_type(
    sign_transaction_output_info_ctx_t* ctx,
    ergo_tx_serializer_box_type_e type) {
    if (STX_OUTPUT_INFO_TYPE(ctx) != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_UNKNOWN) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE: {
            STX_OUTPUT_SET_TYPE(ctx, SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE);
            break;
        }
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE: {
            STX_OUTPUT_SET_TYPE(ctx, SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32);
            break;
        }
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE: {
            STX_OUTPUT_SET_TYPE(ctx, SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS);
            break;
        }
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

uint16_t stx_output_info_set_bip32(sign_transaction_output_info_ctx_t* ctx,
                                   const uint32_t path[MAX_BIP32_PATH],
                                   uint8_t path_len) {
    if (STX_OUTPUT_INFO_TYPE(ctx) != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32 &&
        STX_OUTPUT_INFO_IS_TREE_SET(ctx)) {
        return SW_BAD_STATE;
    }
    STX_OUTPUT_SET_TREE_SET(ctx);
    ctx->bip32_path.len = path_len;
    memmove(ctx->bip32_path.path, path, path_len * sizeof(uint32_t));
    return map_box_result(maybe_finished(ctx));
}

uint16_t stx_output_info_set_fee(sign_transaction_output_info_ctx_t* ctx) {
    if (STX_OUTPUT_INFO_TYPE(ctx) != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE &&
        STX_OUTPUT_INFO_IS_TREE_SET(ctx)) {
        return SW_BAD_STATE;
    }
    STX_OUTPUT_SET_TREE_SET(ctx);
    return map_box_result(maybe_finished(ctx));
}

uint16_t stx_output_info_add_tree_chunk(sign_transaction_output_info_ctx_t* ctx,
                                        const uint8_t* chunk,
                                        uint16_t len,
                                        bool is_finished) {
    if (STX_OUTPUT_INFO_TYPE(ctx) != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS &&
        STX_OUTPUT_INFO_TYPE(ctx) != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT) {
        return SW_BAD_STATE;
    }
    if (STX_OUTPUT_INFO_IS_TREE_SET(ctx)) {
        return SW_BAD_STATE;
    }
    if (STX_OUTPUT_INFO_TYPE(ctx) == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS) {
        if (len == ERGO_TREE_P2PK_LEN && is_finished &&
            ergo_tree_parse_p2pk(chunk, ctx->public_key)) {  // has public key
            STX_OUTPUT_SET_TREE_SET(ctx);
        } else {  // isn't public key. treating as script
            STX_OUTPUT_SET_TYPE(ctx, SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT);
            if (!blake2b_256_init(&ctx->tree_hash_ctx)) {
                return SW_HASHER_ERROR;
            }
        }
    }
    if (STX_OUTPUT_INFO_TYPE(ctx) == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT) {
        if (!blake2b_update(&ctx->tree_hash_ctx, chunk, len)) {
            return SW_HASHER_ERROR;
        }
        if (is_finished) {
            STX_OUTPUT_SET_TREE_SET(ctx);
        }
    }
    return map_box_result(maybe_finished(ctx));
}

ergo_tx_serializer_box_result_e stx_output_info_add_token(sign_transaction_output_info_ctx_t* ctx,
                                                          const uint8_t tn_id[static ERGO_ID_LEN],
                                                          uint64_t value) {
    uint8_t token_index = 0;
    if (!IS_ELEMENT_FOUND(token_index = find_token_index(ctx->tokens_table, tn_id))) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID;
    }
    if (ctx->tokens[token_index] != 0) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE;
    }
    ctx->tokens[token_index] = value;
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

ergo_tx_serializer_box_result_e stx_output_info_set_box_finished(
    sign_transaction_output_info_ctx_t* ctx) {
    STX_OUTPUT_SET_BOX_FINISHED(ctx);
    return maybe_finished(ctx);
}

uint8_t stx_output_info_used_tokens_count(const sign_transaction_output_info_ctx_t* ctx) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < TOKEN_MAX_COUNT; i++) {
        if (ctx->tokens[i] > 0) count++;
    }
    return count;
}

uint8_t stx_output_info_used_token_index(const sign_transaction_output_info_ctx_t* ctx,
                                         uint8_t used_index) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < TOKEN_MAX_COUNT; i++) {
        if (ctx->tokens[i] > 0 && count++ == used_index) return i;
    }
    return INDEX_NOT_EXIST;
}
