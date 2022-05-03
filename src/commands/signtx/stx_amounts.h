#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_full.h"
#include "../../common/int_ops.h"
#include "stx_sw.h"

typedef struct {
    uint64_t input;
    uint64_t output;
    uint64_t change;
} sign_transaction_token_amount_t;

typedef struct {
    uint64_t inputs;
    uint64_t fee;
    uint64_t change;
    sign_transaction_token_amount_t tokens[TOKEN_MAX_COUNT];
    token_table_t tokens_table;
} sign_transaction_amounts_ctx_t;

static inline void stx_amounts_init(sign_transaction_amounts_ctx_t *ctx) {
    memset(ctx, 0, sizeof(sign_transaction_amounts_ctx_t));
}

static inline uint16_t stx_amounts_add_input(sign_transaction_amounts_ctx_t *ctx, uint64_t value) {
    // Add input amount to the stored one
    return checked_add_u64(ctx->inputs, value, &ctx->inputs) ? SW_OK : SW_U64_OVERFLOW;
}

static inline bool stx_amounts_is_token_used(sign_transaction_token_amount_t *amount) {
    return amount->output > 0 || amount->input != amount->change;
}

void stx_amounts_remove_unused_tokens(sign_transaction_amounts_ctx_t *ctx);

uint16_t stx_amounts_register_input_token_callback(sign_transaction_amounts_ctx_t *ctx,
                                                   ergo_tx_serializer_full_context_t *tx_ctx);

uint16_t stx_amounts_register_output_callbacks(sign_transaction_amounts_ctx_t *ctx,
                                               ergo_tx_serializer_full_context_t *tx_ctx);