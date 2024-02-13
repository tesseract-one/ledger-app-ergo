#include "stx_amounts.h"
#include "../../common/macros_ext.h"

static inline uint8_t find_token_index(const token_table_t *table,
                                       const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return INDEX_NOT_EXIST;
}

ergo_tx_serializer_input_result_e stx_amounts_add_input_token(
    sign_transaction_amounts_ctx_t *ctx,
    const uint8_t box_id[static ERGO_ID_LEN],
    const uint8_t tn_id[static ERGO_ID_LEN],
    uint64_t value) {
    (void) (box_id);
    // searching for token in table
    uint8_t index = 0;
    if (!IS_ELEMENT_FOUND(index = find_token_index(&ctx->tokens_table, tn_id))) {
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
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE: {
            if (!checked_sub_u64(ctx->value, value, &ctx->value)) {  // decrease TX out value.
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
            }
            break;
        }
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE:
            if (!checked_sub_u64(ctx->value, value, &ctx->value)) {  // decrease TX out value.
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
            }
            if (!checked_add_u64(ctx->fee, value, &ctx->fee)) {  // add value to fee.
                return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
            }
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE:
            // Do nothing
            break;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

ergo_tx_serializer_box_result_e stx_amounts_add_output_token(sign_transaction_amounts_ctx_t *ctx,
                                                             ergo_tx_serializer_box_type_e type,
                                                             const uint8_t id[static ERGO_ID_LEN],
                                                             uint64_t value) {
    UNUSED(type);
    uint8_t index = 0;
    if (!IS_ELEMENT_FOUND(index = find_token_index(&ctx->tokens_table, id))) {
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
    return INDEX_NOT_EXIST;
}