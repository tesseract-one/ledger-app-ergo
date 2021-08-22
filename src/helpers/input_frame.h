#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <cx.h>
#include "../constants.h"
#include "../common/buffer.h"
#include "../ergo/tx_ser_table.h"

#define FRAME_MAX_TOKENS_COUNT      4
#define FRAME_TOKEN_VALUE_PAIR_SIZE (TOKEN_ID_LEN + sizeof(uint64_t))
#define FRAME_MAX_SIZE                                     \
    (BOX_ID_LEN + 3 * sizeof(uint8_t) + sizeof(uint64_t) + \
     FRAME_MAX_TOKENS_COUNT * FRAME_TOKEN_VALUE_PAIR_SIZE + INPUT_FRAME_SIGNATURE_LEN)

typedef enum {
    INPUT_FRAME_READ_RES_OK,
    INPUT_FRAME_READ_RES_ERR_BUFFER,
    INPUT_FRAME_READ_RES_ERR_BAD_SIGNATURE
} input_frame_read_result_e;

input_frame_read_result_e input_frame_read(buffer_t* input,
                                           uint8_t box_id[static BOX_ID_LEN],
                                           uint8_t* frames_count,
                                           uint8_t* frame_index,
                                           uint8_t* tokens_count,
                                           uint64_t* value,
                                           buffer_t* tokens,
                                           const uint8_t session_key[static SESSION_KEY_LEN]);
