#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_full.h"
#include "../../common/buffer.h"

typedef struct {
    uint64_t input;
    uint64_t output;
    uint64_t change;
} _sign_transaction_token_amount_t;

typedef enum {
    SIGN_TRANSACTION_STATE_INITIALIZED,
    SIGN_TRANSACTION_STATE_DATA_APPROVED,
    SIGN_TRANSACTION_STATE_INPUTS_STARTED,
    SIGN_TRANSACTION_STATE_OUTPUTS_STARTED,
    SIGN_TRANSACTION_STATE_TX_FINISHED,
    SIGN_TRANSACTION_STATE_CONFIRMED,
    SIGN_TRANSACTION_STATE_ERROR
} sign_transaction_state_e;

typedef struct {
    sign_transaction_state_e state;
    uint8_t tx_id[ERGO_ID_LEN];
    uint8_t session;
    token_table_t tokens_table;
    ergo_tx_serializer_full_context_t tx;
} sign_transaction_ctx_t;

typedef struct {
    char app_token[11];  // hexified app token
    uint64_t inputs_value;
    uint64_t fee_value;
    uint64_t change_value;
    _sign_transaction_token_amount_t token_amounts[TOKEN_MAX_COUNT];
    uint32_t app_token_value;
} sign_transaction_ui_ctx_t;
