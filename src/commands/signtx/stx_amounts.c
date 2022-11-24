#include "stx_amounts.h"
#include "../../common/macros.h"

static inline uint8_t find_token_index(const token_table_t *table,
                                       const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return 0xFF;
}

ergo_tx_serializer_input_result_e stx_amounts_add_input_token(
    sign_transaction_amounts_ctx_t *ctx,
    const uint8_t box_id[static ERGO_ID_LEN],
    const uint8_t tn_id[static ERGO_ID_LEN],
    uint64_t value) {
    (void) (box_id);
    // searching for token in table
    uint8_t index = 0;
    if ((index = find_token_index(&ctx->tokens_table, tn_id)) == 0xFF) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID;
    }
    // calculating proper token sum
    if (!checked_add_i64(ctx->tokens[index], value, &ctx->tokens[index])) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_OK;
}

ergo_tx_serializer_box_result_e stx_amounts_add_output(sign_transaction_amounts_ctx_t *ctx,
                                                       ergo_tx_serializer_box_type_e type,
                                                       uint64_t value) {
    uint64_t *sum;
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            // we don't need to calculate changes.
            return ERGO_TX_SERIALIZER_BOX_RES_OK;
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE:
            sum = &ctx->fee;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE:
            sum = &ctx->value;
            break;
    }
    if (!checked_add_u64(*sum, value, sum)) {  // calculating proper sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

ergo_tx_serializer_box_result_e stx_amounts_add_output_token(sign_transaction_amounts_ctx_t *ctx,
                                                             ergo_tx_serializer_box_type_e type,
                                                             const uint8_t id[static ERGO_ID_LEN],
                                                             uint64_t value) {
    UNUSED(type);
    uint8_t index = 0;
    if ((index = find_token_index(&ctx->tokens_table, id)) == 0xFF) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID;
    }
    if (!checked_sub_i64(ctx->tokens[index],
                         value,
                         &ctx->tokens[index])) {  // calculating proper token sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

uint8_t stx_amounts_non_zero_tokens_count(const sign_transaction_amounts_ctx_t *ctx) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < TOKEN_MAX_COUNT; i++) {
        if (ctx->tokens[i] != 0) count++;
    }
    return count;
}

uint8_t stx_amounts_non_zero_token_index(const sign_transaction_amounts_ctx_t *ctx,
                                         uint8_t zero_index) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < TOKEN_MAX_COUNT; i++) {
        if (ctx->tokens[i] != 0 && count++ == zero_index) return i;
    }
    return 0xFF;
}

// uint16_t stx_amounts_register_input_token_callback(sign_transaction_amounts_ctx_t *ctx,
//                                                    ergo_tx_serializer_full_context_t *tx_ctx) {
//     return sw_from_ser_res(
//         ergo_tx_serializer_full_set_input_callback(tx_ctx, &input_token_cb, (void *) ctx));
// }

// uint16_t stx_amounts_register_output_callbacks(sign_transaction_amounts_ctx_t *ctx,
//                                                ergo_tx_serializer_full_context_t *tx_ctx) {
//     return sw_from_ser_res(ergo_tx_serializer_full_set_box_callbacks(tx_ctx,
//                                                                      &output_type_cb,
//                                                                      &output_token_cb,
//                                                                      (void *) ctx));
// }

// void stx_amounts_remove_unused_tokens(sign_transaction_amounts_ctx_t *ctx) {
//     uint8_t index = 0;
//     while (index < ctx->tokens_table.count) {
//         if (stx_amounts_is_token_used(&ctx->tokens[index])) {
//             index++;
//             continue;
//         }
//         for (uint8_t i = index; i < ctx->tokens_table.count - 1; i++) {
//             ctx->tokens[i] = ctx->tokens[i + 1];
//             memmove(ctx->tokens_table.tokens[i], ctx->tokens_table.tokens[i + 1], ERGO_ID_LEN);
//         }
//         ctx->tokens_table.count--;
//     }
// }