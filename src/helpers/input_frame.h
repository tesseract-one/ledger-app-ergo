#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cx.h>
#include "../constants.h"
#include "../common/buffer.h"
#include "../ergo/tx_ser_table.h"

#define FRAME_MAX_TOKENS_COUNT      4
#define FRAME_TOKEN_VALUE_PAIR_SIZE (ERGO_ID_LEN + sizeof(uint64_t))
#define FRAME_TOKEN_COUNT_POSITION  (ERGO_ID_LEN + 2)
#define FRAME_TOKEN_PREFIX_LEN      (FRAME_TOKEN_COUNT_POSITION + 1 + sizeof(uint64_t))
#define FRAME_MIN_SIZE              (FRAME_TOKEN_PREFIX_LEN + INPUT_FRAME_SIGNATURE_LEN)
#define FRAME_MAX_SIZE                                                               \
    (FRAME_TOKEN_PREFIX_LEN + FRAME_MAX_TOKENS_COUNT * FRAME_TOKEN_VALUE_PAIR_SIZE + \
     INPUT_FRAME_SIGNATURE_LEN)

uint8_t input_frame_data_length(const buffer_t* input);
uint8_t* input_frame_signature_ptr(const buffer_t* input);