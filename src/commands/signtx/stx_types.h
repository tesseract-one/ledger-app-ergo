#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_full.h"
#include "../../ui/ui_application_id.h"
#include "stx_amounts.h"

typedef enum {
    SIGN_TRANSACTION_STATE_INITIALIZED,
    SIGN_TRANSACTION_STATE_APPROVED,
    SIGN_TRANSACTION_STATE_ERROR
} sign_transaction_state_e;

typedef enum { SIGN_TRANSACTION_OPERATION_P2PK } sign_transaction_operation_type_e;

typedef enum {
    SIGN_TRANSACTION_UI_STATE_NONE,
    SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN,
    SIGN_TRANSACTION_UI_STATE_TX_VALUE,
    SIGN_TRANSACTION_UI_STATE_TX_FEE,
    SIGN_TRANSACTION_UI_STATE_TOKEN_ID,
    SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE
} sign_transaction_ui_state_e;

typedef struct {
    uint32_t app_token_value;                // App token value
    char app_token[APPLICATION_ID_STR_LEN];  // App token string
} sign_transaction_ui_aprove_ctx_t;

typedef void (
    *ui_sign_transaction_operation_show_screen_cb)(uint8_t, char *, size_t, char *, size_t, void *);
typedef void (*ui_sign_transaction_operation_send_response_cb)(void *);

typedef struct {
    char title[20];  // dynamic screen title
    char text[70];   // dynamic screen text
    sign_transaction_ui_state_e state;
    uint8_t token_idx;
    uint8_t op_screen_count;
    uint8_t op_screen_index;
    ui_sign_transaction_operation_show_screen_cb op_screen_cb;
    ui_sign_transaction_operation_send_response_cb op_response_cb;
    void *op_cb_context;
    const sign_transaction_amounts_ctx_t *amounts;
} sign_transaction_ui_confirm_ctx_t;
