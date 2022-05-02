#include "stx_amounts.h"
#include "../../common/macros.h"

static inline uint8_t find_token_index(const token_table_t *table,
                                       const uint8_t id[static ERGO_ID_LEN]) {
    for (uint8_t i = 0; i < table->count; i++) {
        if (memcmp(table->tokens[i], id, ERGO_ID_LEN) == 0) return i;
    }
    return 0xFF;
}

static NOINLINE ergo_tx_serializer_input_result_e
input_token_cb(const uint8_t box_id[static ERGO_ID_LEN],
               const uint8_t tn_id[static ERGO_ID_LEN],
               uint64_t value,
               void *context) {
    (void) (box_id);
    sign_transaction_amounts_ctx_t *ctx = (sign_transaction_amounts_ctx_t *) context;
    // searching for token in table
    uint8_t index = 0;
    if ((index = find_token_index(&ctx->tokens_table, tn_id)) == 0xFF) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID;
    }
    // calculating proper token sum
    if (!checked_add_u64(ctx->tokens[index].input, value, &ctx->tokens[index].input)) {
        return ERGO_TX_SERIALIZER_INPUT_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_INPUT_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e output_type_cb(ergo_tx_serializer_box_type_e type,
                                                               uint64_t value,
                                                               void *context) {
    uint64_t *sum;
    sign_transaction_amounts_ctx_t *ctx = (sign_transaction_amounts_ctx_t *) context;
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            sum = &ctx->change;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_FEE:
            sum = &ctx->fee;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE:
            // we don't need to calculate outputs. We will calc sum from other values
            return ERGO_TX_SERIALIZER_BOX_RES_OK;
    }
    if (!checked_add_u64(*sum, value, sum)) {  // calculating proper sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

static NOINLINE ergo_tx_serializer_box_result_e
output_token_cb(ergo_tx_serializer_box_type_e type,
                const uint8_t id[static ERGO_ID_LEN],
                uint64_t value,
                void *context) {
    uint64_t *sum;
    sign_transaction_amounts_ctx_t *ctx = (sign_transaction_amounts_ctx_t *) context;
    uint8_t index = 0;
    if ((index = find_token_index(&ctx->tokens_table, id)) == 0xFF) {
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID;
    }
    switch (type) {
        case ERGO_TX_SERIALIZER_BOX_TYPE_TREE:
            sum = &ctx->tokens[index].output;
            break;
        case ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE:
            sum = &ctx->tokens[index].change;
            break;
        default:
            // we shouldn't send tokens to miners fee
            // also, can't be input box too (we have only output boxes in sign tx).
            return ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE;
    }
    if (!checked_add_u64(*sum, value, sum)) {  // calculating proper token sum
        return ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW;
    }
    return ERGO_TX_SERIALIZER_BOX_RES_OK;
}

uint16_t stx_amounts_register_input_token_callback(sign_transaction_amounts_ctx_t *ctx,
                                                   ergo_tx_serializer_full_context_t *tx_ctx) {
    return sw_from_ser_res(
        ergo_tx_serializer_full_set_input_callback(tx_ctx, &input_token_cb, (void *) ctx));
}

uint16_t stx_amounts_register_output_callbacks(sign_transaction_amounts_ctx_t *ctx,
                                               ergo_tx_serializer_full_context_t *tx_ctx) {
    return sw_from_ser_res(ergo_tx_serializer_full_set_box_callbacks(tx_ctx,
                                                                     &output_type_cb,
                                                                     &output_token_cb,
                                                                     (void *) ctx));
}