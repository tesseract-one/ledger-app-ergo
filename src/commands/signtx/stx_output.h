#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t
#include <string.h>   // memset

#include "../../common/bip32_ext.h"
#include "../../constants.h"
#include "../../helpers/blake2b.h"
#include "../../ergo/tx_ser_full.h"

typedef struct {
    uint8_t len;
    uint32_t path[MAX_BIP32_PATH];
} sign_transaction_bip32_path_t;

typedef enum {
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_UNKNOWN = 0,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_HASH,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE
} sign_transaction_output_info_type_e;

typedef struct {
    uint64_t value;
    uint64_t tokens[TOKEN_MAX_COUNT];
    uint8_t state;  // type + is_set + is_finished
    union {
        cx_blake2b_t tree_hash_ctx;
        uint8_t public_key[COMPRESSED_PUBLIC_KEY_LEN];
        uint8_t tree_hash[CX_BLAKE2B_256_SIZE];
        sign_transaction_bip32_path_t bip32_path;
    };
    const token_table_t* tokens_table;
} sign_transaction_output_info_ctx_t;

#define STX_OUTPUT_INFO_TYPE(ctx) ((sign_transaction_output_info_type_e) (ctx->state & 0x3F))

#define STX_OUTPUT_INFO_IS_TREE_SET(ctx) ((ctx->state & 0x40) != 0)

#define STX_OUTPUT_INFO_IS_BOX_FINISHED(ctx) ((ctx->state & 0x80) != 0)

static inline void stx_output_info_init(sign_transaction_output_info_ctx_t* ctx,
                                        uint64_t value,
                                        const token_table_t* tokens) {
    memset(ctx, 0, sizeof(sign_transaction_output_info_ctx_t));
    ctx->state = 0;
    ctx->value = value;
    ctx->tokens_table = tokens;
}

uint16_t stx_output_info_set_bip32(sign_transaction_output_info_ctx_t* ctx,
                                   const uint32_t path[MAX_BIP32_PATH],
                                   uint8_t path_len);

uint16_t stx_output_info_set_fee(sign_transaction_output_info_ctx_t* ctx);

uint16_t stx_output_info_add_tree_chunk(sign_transaction_output_info_ctx_t* ctx,
                                        const uint8_t* chunk,
                                        uint16_t len,
                                        bool is_finished);

ergo_tx_serializer_box_result_e stx_output_info_add_token(sign_transaction_output_info_ctx_t* ctx,
                                                          const uint8_t tn_id[static ERGO_ID_LEN],
                                                          uint64_t value);

ergo_tx_serializer_box_result_e stx_output_info_set_box_finished(
    sign_transaction_output_info_ctx_t* ctx);

uint8_t stx_output_info_used_tokens_count(const sign_transaction_output_info_ctx_t* ctx);

uint8_t stx_output_info_used_token_index(const sign_transaction_output_info_ctx_t* ctx,
                                         uint8_t used_index);

static inline sign_transaction_output_info_type_e stx_output_info_type(
    const sign_transaction_output_info_ctx_t* ctx) {
    return STX_OUTPUT_INFO_TYPE(ctx);
}

static inline bool stx_output_info_is_finished(const sign_transaction_output_info_ctx_t* ctx) {
    return STX_OUTPUT_INFO_IS_TREE_SET(ctx) && STX_OUTPUT_INFO_IS_BOX_FINISHED(ctx);
}

static inline bool stx_bip32_path_is_equal(const sign_transaction_bip32_path_t* p1,
                                           const sign_transaction_bip32_path_t* p2) {
    return bip32_path_is_equal(p1->path, p1->len, p2->path, p2->len);
}

static inline bool stx_bip32_path_same_account(const sign_transaction_bip32_path_t* p1,
                                               const sign_transaction_bip32_path_t* p2) {
    return bip32_path_same_account(p1->path, p1->len, p2->path, p2->len);
}