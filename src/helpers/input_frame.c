#include "input_frame.h"
#include "../common/buffer_ext.h"

uint8_t input_frame_data_length(const buffer_t* input) {
    if (!buffer_can_read(input, FRAME_MIN_SIZE)) {
        return 0;
    }
    uint8_t token_count = buffer_read_ptr(input)[FRAME_TOKEN_COUNT_POSITION];
    uint8_t data_len = token_count * FRAME_TOKEN_VALUE_PAIR_SIZE + FRAME_TOKEN_PREFIX_LEN;
    if (!buffer_can_read(input, data_len + INPUT_FRAME_SIGNATURE_LEN)) {
        return 0;
    }
    return data_len;
}

const uint8_t* input_frame_signature_ptr(const buffer_t* input) {
    uint8_t data_len = input_frame_data_length(input);
    if (data_len == 0) return NULL;
    return buffer_read_ptr(input) + data_len;
}
