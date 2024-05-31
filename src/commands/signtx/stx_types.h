#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_full.h"
#include "../../ui/ui_application_id.h"
#include "stx_amounts.h"
#include "stx_output.h"

typedef enum {
    SIGN_TRANSACTION_STATE_INITIALIZED,
    SIGN_TRANSACTION_STATE_APPROVED,
    SIGN_TRANSACTION_STATE_ERROR
} sign_transaction_state_e;

typedef enum { SIGN_TRANSACTION_OPERATION_P2PK } sign_transaction_operation_type_e;

typedef enum {
    SIGN_TRANSACTION_UI_TRANSACTION_STATE_NONE,
    SIGN_TRANSACTION_UI_TRANSACTION_STATE_OPERATION_SCREEN,
    SIGN_TRANSACTION_UI_TRANSACTION_STATE_TX_VALUE,
    SIGN_TRANSACTION_UI_TRANSACTION_STATE_TX_FEE,
    SIGN_TRANSACTION_UI_TRANSACTION_STATE_TOKEN_ID,
    SIGN_TRANSACTION_UI_TRANSACTION_STATE_TOKEN_VALUE
} sign_transaction_ui_transaction_state_e;

typedef enum {
    SIGN_TRANSACTION_UI_OUTPUT_STATE_NONE,
    SIGN_TRANSACTION_UI_OUTPUT_STATE_ADDRESS,
    SIGN_TRANSACTION_UI_OUTPUT_STATE_VALUE,
    SIGN_TRANSACTION_UI_OUTPUT_STATE_TOKEN_ID,
    SIGN_TRANSACTION_UI_OUTPUT_STATE_TOKEN_VALUE
} sign_transaction_ui_output_state_e;

typedef struct {
    uint32_t app_token_value;                // App token value
    char app_token[APPLICATION_ID_STR_LEN];  // App token string
    bool is_known_application;
    void *sign_tx_context;
} sign_transaction_ui_aprove_ctx_t;

typedef struct {
    char title[20];  // dynamic screen title
    char text[70];   // dynamic screen text
    uint8_t network_id;
    const sign_transaction_output_info_ctx_t *output;
    sign_transaction_bip32_path_t *last_approved_change;
} sign_transaction_ui_output_confirm_ctx_t;

// Show screen callback: (index, title, title_len, text, text_len, cb_context)
typedef uint16_t (
    *ui_sign_transaction_operation_show_screen_cb)(uint8_t, char *, size_t, char *, size_t, void *);
// Send response callback (cb_context)
typedef void (*ui_sign_transaction_operation_send_response_cb)(void *);

typedef struct {
    char title[20];  // dynamic screen title
    char text[70];   // dynamic screen text
    uint8_t op_screen_count;
    ui_sign_transaction_operation_show_screen_cb op_screen_cb;
    ui_sign_transaction_operation_send_response_cb op_response_cb;
    void *op_cb_context;
    const sign_transaction_amounts_ctx_t *amounts;
} sign_transaction_ui_sign_confirm_ctx_t;
