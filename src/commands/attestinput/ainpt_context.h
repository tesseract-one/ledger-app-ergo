#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cx.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_box.h"
#include "../../helpers/input_frame.h"
#include "../../ui/ui_application_id.h"

typedef struct {
    char app_token[APPLICATION_ID_STR_LEN];  // hexified app token
    uint32_t app_token_value;
} _attest_input_ui_ctx_t;

typedef enum {
    ATTEST_INPUT_STATE_INITIALIZED,
    ATTEST_INPUT_STATE_FINISHED,
    ATTEST_INPUT_STATE_APPROVED,
    ATTEST_INPUT_STATE_ERROR
} attest_input_state_e;

typedef struct {
    uint8_t box_id[ERGO_ID_LEN];  // TXID && BOXID
    token_table_t tokens_table;
    union {
        uint64_t token_amounts[TOKEN_MAX_COUNT];
        _attest_input_ui_ctx_t ui;
    };
    ergo_tx_serializer_box_context_t box;
    cx_blake2b_t hash;
    uint16_t box_index;
    uint8_t session;
    attest_input_state_e state;
} attest_input_ctx_t;
