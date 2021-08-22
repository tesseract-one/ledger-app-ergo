#include "input_frame.h"
#include <cx.h>
#include <string.h>

input_frame_read_result_e input_frame_read(buffer_t* input,
                                           uint8_t box_id[static BOX_ID_LEN],
                                           uint8_t* frames_count,
                                           uint8_t* frame_index,
                                           uint8_t* tokens_count,
                                           uint64_t* value,
                                           buffer_t* tokens,
                                           const uint8_t session_key[static SESSION_KEY_LEN]) {
    buffer_t temp;
    buffer_init(&temp, buffer_read_ptr(input), buffer_data_len(input), buffer_data_len(input));
    if (!buffer_read_bytes(&temp, box_id, BOX_ID_LEN)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    if (!buffer_read_u8(&temp, frames_count)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    if (!buffer_read_u8(&temp, frame_index)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    if (!buffer_read_u8(&temp, tokens_count)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    if (!buffer_read_u64(&temp, value, BE)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    size_t tokens_len =
        (buffer_data_len(&temp) / FRAME_TOKEN_VALUE_PAIR_SIZE) * FRAME_TOKEN_VALUE_PAIR_SIZE;
    buffer_init(tokens, buffer_read_ptr(&temp), buffer_data_len(&temp), tokens_len);
    if (!buffer_seek_read_cur(&temp, tokens_len)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    uint8_t signature[INPUT_FRAME_SIGNATURE_LEN];
    if (!buffer_read_bytes(&temp, signature, INPUT_FRAME_SIGNATURE_LEN)) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    uint8_t hash[CX_SHA256_SIZE] = {0};
    cx_hmac_sha256_t hmac;

    if (cx_hmac_sha256_init_no_throw(&hmac, session_key, SESSION_KEY_LEN) != 0) {
        return INPUT_FRAME_READ_RES_ERR_BAD_SIGNATURE;
    }
    if (cx_hmac_no_throw((cx_hmac_t*) &hmac,
                         CX_LAST,
                         buffer_read_ptr(input),
                         buffer_read_position(&temp) - INPUT_FRAME_SIGNATURE_LEN,
                         hash,
                         CX_SHA256_SIZE) != 0) {
        return INPUT_FRAME_READ_RES_ERR_BAD_SIGNATURE;
    }

    if (memcmp(signature, hash, INPUT_FRAME_SIGNATURE_LEN) != 0) {
        return INPUT_FRAME_READ_RES_ERR_BAD_SIGNATURE;
    }
    if (!buffer_seek_read_cur(input, buffer_read_position(&temp))) {
        return INPUT_FRAME_READ_RES_ERR_BUFFER;
    }
    return INPUT_FRAME_READ_RES_OK;
}