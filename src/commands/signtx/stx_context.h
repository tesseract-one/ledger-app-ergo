#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_full.h"
#include "../../common/buffer.h"
#include "../../ui/ui_application_id.h"

typedef struct {
    uint64_t input;
    uint64_t output;
    uint64_t change;
} _sign_transaction_token_amount_t;

typedef struct {
    uint32_t app_token_value;
    char app_token[APPLICATION_ID_STR_LEN];
} _sign_transaction_app_id_ui_ctx_t;

typedef struct {
    uint64_t inputs;
    uint64_t fee;
    uint64_t change;
    _sign_transaction_token_amount_t tokens[TOKEN_MAX_COUNT];
    token_table_t tokens_table;
} _sign_transaction_amounts_ctx_t;

typedef enum {
    SIGN_TRANSACTION_UI_STATE_NONE,
    SIGN_TRANSACTION_UI_STATE_TX_VALUE,
    SIGN_TRANSACTION_UI_STATE_TX_FEE,
    SIGN_TRANSACTION_UI_STATE_TOKEN_ID,
    SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE
} sign_transaction_ui_state_e;

typedef struct {
    char title[20];  // dynamic screen title
    char text[70];   // dynamic screen text
    sign_transaction_ui_state_e state;
    uint8_t token_idx;
} _sign_transaction_confirm_ui_ctx_t;

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
    union {
        uint8_t tx_id[ERGO_ID_LEN];
        _sign_transaction_app_id_ui_ctx_t ui_app_id;
    };
    sign_transaction_state_e state;
    uint8_t session;
    _sign_transaction_amounts_ctx_t amounts;
    union {
        ergo_tx_serializer_full_context_t tx;
        _sign_transaction_confirm_ui_ctx_t ui_confirm;
    };
} sign_transaction_ctx_t;
