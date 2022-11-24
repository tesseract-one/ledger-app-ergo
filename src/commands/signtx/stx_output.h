#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t
#include <string.h>   // memset

#include "../../common/bip32.h"
#include "../../constants.h"
#include "../../helpers/blake2b.h"
#include "../../ergo/tx_ser_full.h"

typedef struct {
    uint8_t len;
    uint32_t path[MAX_BIP32_PATH];
} sign_transaction_bip32_path_t;

typedef enum {
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_UNKNOWN = 0,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_EXPECTED,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_SET,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_FINISHED,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_TREE_EXPECTED,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS_SET,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS_FINISHED,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_SET,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_FINISHED,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_EXPECTED,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_SET,
    SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_FINISHED
} sign_transaction_output_info_type_e;

typedef struct {
    uint64_t value;
    uint64_t tokens[TOKEN_MAX_COUNT];
    sign_transaction_output_info_type_e type;
    union {
        cx_blake2b_t tree_hash_ctx;
        uint8_t public_key[COMPRESSED_PUBLIC_KEY_LEN];
        uint8_t tree_hash[ERGO_ID_LEN];
        sign_transaction_bip32_path_t bip32_path;
    };
    const token_table_t* tokens_table;
} sign_transaction_output_info_ctx_t;

static inline void stx_output_info_init(sign_transaction_output_info_ctx_t* ctx,
                                        uint64_t value,
                                        const token_table_t* tokens) {
    memset(ctx, 0, sizeof(sign_transaction_output_info_ctx_t));
    ctx->type = SIGN_TRANSACTION_OUTPUT_INFO_TYPE_UNKNOWN;
    ctx->value = value;
    ctx->tokens_table = tokens;
}

ergo_tx_serializer_box_result_e stx_output_info_set_expected_type(
    sign_transaction_output_info_ctx_t* ctx,
    ergo_tx_serializer_box_type_e type);

uint16_t stx_output_info_set_bip32(sign_transaction_output_info_ctx_t* ctx,
                                   const uint32_t path[MAX_BIP32_PATH],
                                   uint8_t path_len);

uint16_t stx_output_info_set_fee(sign_transaction_output_info_ctx_t* ctx);

uint16_t stx_output_info_add_tree_chunk(sign_transaction_output_info_ctx_t* ctx,
                                        const buffer_t* chunk);

ergo_tx_serializer_box_result_e stx_output_info_add_token(sign_transaction_output_info_ctx_t* ctx,
                                                          const uint8_t tn_id[static ERGO_ID_LEN],
                                                          uint64_t value);

ergo_tx_serializer_box_result_e stx_output_info_set_finished(
    sign_transaction_output_info_ctx_t* ctx);

uint8_t stx_output_info_used_tokens_count(const sign_transaction_output_info_ctx_t* ctx);

uint8_t stx_output_info_used_token_index(const sign_transaction_output_info_ctx_t* ctx,
                                         uint8_t used_index);

bool stx_output_info_has_used_tokens(const sign_transaction_output_info_ctx_t* ctx);

static inline bool stx_output_info_is_finished(const sign_transaction_output_info_ctx_t* ctx) {
    return ctx->type == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32_FINISHED ||
           ctx->type == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS_FINISHED ||
           ctx->type == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_FINISHED ||
           ctx->type == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE_FINISHED;
}