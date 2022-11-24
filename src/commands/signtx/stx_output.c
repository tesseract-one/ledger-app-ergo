#include "stx_output.h"
#include "../../sw.h"
#include "../../common/macros.h"
#include "../../common/base58.h"
#include "../../ergo/ergo_tree.h"
#include "../../ergo/address.h"
#include <string.h>

static inline uint8_t find_token_index(const token_table_t* table,
                                       const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return INDEX_NOT_EXIST;
}

ergo_tx_serializer_box_result_e stx_output_info_set_expected_type(
    sign_transaction_output_info_ctx_t* ctx,
    ergo_tx_serializer_box_type_e type) {
    if (ctx->type != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_UNKNOWN) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE: {
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_EXPECTED;
            break;
        }
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE: {
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_EXPECTED;
            break;
        }
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE: {
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_TREE_EXPECTED;
            break;
        }
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

uint16_t stx_output_info_set_bip32(sign_transaction_output_info_ctx_t* ctx,
                                   const uint32_t path[MAX_BIP32_PATH],
                                   uint8_t path_len) {
    if (ctx->type != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_EXPECTED) {
        return SW_BAD_STATE;
    }
    ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_SET;
    ctx->bip32_path.len = path_len;
    memmove(ctx->bip32_path.path, path, path_len * sizeof(uint32_t));
    return SW_OK;
}

uint16_t stx_output_info_set_fee(sign_transaction_output_info_ctx_t* ctx) {
    if (ctx->type != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_EXPECTED) {
        return SW_BAD_STATE;
    }
    ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_SET;
    return SW_OK;
}

uint16_t stx_output_info_add_tree_chunk(sign_transaction_output_info_ctx_t* ctx,
                                        const buffer_t* chunk) {
    if (ctx->type != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_TREE_EXPECTED &&
        ctx->type != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_SET) {
        return SW_BAD_STATE;
    }
    if (ctx->type == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_TREE_EXPECTED) {  // first chunk
        if (buffer_data_len(chunk) == ERGO_TREE_P2PK_LEN &&
            ergo_tree_parse_p2pk(buffer_read_ptr(chunk), ctx->public_key)) {  // has public key
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS_SET;
        } else {  // isn't public key. treating as script
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_SET;
            if (!blake2b_256_init(&ctx->tree_hash_ctx)) {
                return SW_HASHER_ERROR;
            }
        }
    }
    if (ctx->type == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_SET) {
        if (!blake2b_update(&ctx->tree_hash_ctx, buffer_read_ptr(chunk), buffer_data_len(chunk))) {
            return SW_HASHER_ERROR;
        }
    }
    return SW_OK;
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

ergo_tx_serializer_box_result_e stx_output_info_set_finished(
    sign_transaction_output_info_ctx_t* ctx) {
    switch (ctx->type) {
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_SET: {
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_FINISHED;
            break;
        }
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS_SET: {
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS_FINISHED;
            break;
        }
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_SET: {
            uint8_t hash[ERGO_ID_LEN];
            if (!blake2b_256_finalize(&ctx->tree_hash_ctx, hash)) {
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER;
            }
            memmove(ctx->tree_hash, hash, ERGO_ID_LEN);
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_FINISHED;
            break;
        }
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_SET: {
            ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_FINISHED;
            break;
        }
        default:
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

bool stx_output_info_has_used_tokens(const sign_transaction_output_info_ctx_t* ctx) {
    for (uint8_t i = 0; i < ctx->tokens_table->count; i++) {
        if (ctx->tokens[i] > 0) return true;
    }
    return false;
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
