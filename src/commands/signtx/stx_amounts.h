#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_full.h"
#include "../../common/safeint.h"
#include "../../sw.h"

typedef struct {
    uint64_t fee;
    uint64_t value;
    int64_t tokens[TOKEN_MAX_COUNT];
    token_table_t tokens_table;
} sign_transaction_amounts_ctx_t;

static inline void stx_amounts_init(sign_transaction_amounts_ctx_t *ctx) {
    memset(ctx, 0, sizeof(sign_transaction_amounts_ctx_t));
}

static inline uint16_t stx_amounts_add_input(sign_transaction_amounts_ctx_t *ctx, uint64_t value) {
    // Add input amount to the stored value
    return checked_add_u64(ctx->value, value, &ctx->value) ? SW_OK : SW_U64_OVERFLOW;
}

ergo_tx_serializer_input_result_e stx_amounts_add_input_token(
    sign_transaction_amounts_ctx_t *ctx,
    const uint8_t box_id[static ERGO_ID_LEN],
    const uint8_t tn_id[static ERGO_ID_LEN],
    uint64_t value);

ergo_tx_serializer_box_result_e stx_amounts_add_output(sign_transaction_amounts_ctx_t *ctx,
                                                       ergo_tx_serializer_box_type_e type,
                                                       uint64_t value);

ergo_tx_serializer_box_result_e stx_amounts_add_output_token(sign_transaction_amounts_ctx_t *ctx,
                                                             ergo_tx_serializer_box_type_e type,
                                                             const uint8_t id[static ERGO_ID_LEN],
                                                             uint64_t value);

uint8_t stx_amounts_non_zero_tokens_count(const sign_transaction_amounts_ctx_t *ctx);

uint8_t stx_amounts_non_zero_token_index(const sign_transaction_amounts_ctx_t *ctx,
                                         uint8_t zero_index);
