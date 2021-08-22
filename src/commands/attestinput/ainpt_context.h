#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cx.h>
#include "../../constants.h"
#include "../../ergo/tx_ser_simple.h"
#include "../../ergo/tx_ser_box.h"
#include "../../common/buffer.h"
#include "../../helpers/input_frame.h"

typedef struct {
    cx_blake2b_t hash;
    uint16_t box_index;
    uint8_t tokens_count;
    ergo_tx_serializer_box_context_t ctx;
} _attest_input_box_ctx_t;

typedef enum {
    ATTEST_INPUT_STATE_INITIALIZED,
    ATTEST_INPUT_STATE_TX_STARTED,
    ATTEST_INPUT_STATE_TX_FINISHED,
    ATTEST_INPUT_STATE_BOX_STARTED,
    ATTEST_INPUT_STATE_BOX_FINISHED,
    ATTEST_INPUT_STATE_APPROVED,
    ATTEST_INPUT_STATE_ERROR
} attest_input_state_e;

typedef struct {
    attest_input_state_e state;
    uint8_t tx_id[TRANSACTION_HASH_LEN];
    uint8_t box_id[BOX_ID_LEN];
    uint8_t session;
    token_table_t tokens_table;
    uint64_t token_amounts[TOKEN_MAX_COUNT];
    union {
        _attest_input_box_ctx_t box;
        ergo_tx_serializer_simple_context_t tx;
    };
} attest_input_ctx_t;

typedef struct {
    char app_token[11];  // hexified app token
    uint32_t app_token_value;
} attest_input_ui_ctx_t;
