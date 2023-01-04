#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "stx_types.h"
#include "./operations/stx_op_p2pk.h"

typedef struct {
    sign_transaction_state_e state;
    uint8_t session;
    sign_transaction_operation_type_e operation;
    union {
        sign_transaction_operation_p2pk_ctx_t p2pk;
    };
} sign_transaction_ctx_t;
