#include <io.h>

#include "ainpt_response.h"
#include "ainpt_context.h"
#include "../../helpers/response.h"
#include "../../helpers/input_frame.h"

#define WRITE_ERROR_HANDLER send_error
#include "../../helpers/cmd_macros.h"

static inline uint8_t get_frames_count(uint8_t tokens_count) {
    uint8_t frames_count = (tokens_count + (FRAME_MAX_TOKENS_COUNT - 1)) / FRAME_MAX_TOKENS_COUNT;
    return frames_count == 0 ? 1 : frames_count;
}

static inline int send_error(uint16_t error) {
    app_set_current_command(CMD_NONE);
    return res_error(error);
}

int send_response_attested_input_frame(attest_input_ctx_t *ctx,
                                       const uint8_t session_key[static SESSION_KEY_LEN],
                                       uint8_t index) {
    uint8_t frames_count = get_frames_count(ctx->tokens_table.count);
    if (index >= frames_count) {
        return send_error(SW_BAD_FRAME_INDEX);
    }

    // Hack for the stack overflow. Writing directly to the IO buffer.
    // Not elegant but works.
    RW_BUFFER_FROM_ARRAY_EMPTY(output, G_io_apdu_buffer, IO_APDU_BUFFER_SIZE - 2);

    CHECK_WRITE_PARAM(rw_buffer_write_bytes(&output, ctx->box_id, ERGO_ID_LEN));
    CHECK_WRITE_PARAM(rw_buffer_write_u8(&output, frames_count));
    CHECK_WRITE_PARAM(rw_buffer_write_u8(&output, index));
    CHECK_WRITE_PARAM(rw_buffer_write_u64(&output, ctx->box.value, BE));

    uint8_t tokens_count = (ctx->tokens_table.count - index * FRAME_MAX_TOKENS_COUNT);
    tokens_count = MIN(tokens_count, FRAME_MAX_TOKENS_COUNT);
    CHECK_WRITE_PARAM(rw_buffer_write_u8(&output, tokens_count));

    uint8_t offset = index * FRAME_MAX_TOKENS_COUNT;
    for (uint8_t i = offset; i < offset + tokens_count; i++) {
        CHECK_WRITE_PARAM(rw_buffer_write_bytes(&output, ctx->tokens_table.tokens[i], ERGO_ID_LEN));
        CHECK_WRITE_PARAM(rw_buffer_write_u64(&output, ctx->token_amounts[i], BE));
    }

    CHECK_WRITE_PARAM(rw_buffer_can_write(&output, CX_SHA256_SIZE));
    cx_hmac_sha256(session_key,
                   SESSION_KEY_LEN,
                   rw_buffer_read_ptr(&output),
                   rw_buffer_data_len(&output),
                   rw_buffer_write_ptr(&output),
                   CX_SHA256_SIZE);
    CHECK_WRITE_PARAM(rw_buffer_seek_write_cur(&output, INPUT_FRAME_SIGNATURE_LEN));

    return res_ok_data(&output);
}

int send_response_attested_input_frame_count(uint8_t tokens_count) {
    uint8_t frames_count = get_frames_count(tokens_count);
    RW_BUFFER_FROM_VAR_FULL(buf, frames_count);
    return res_ok_data(&buf);
}

int send_response_attested_input_session_id(uint8_t session_id) {
    RW_BUFFER_FROM_VAR_FULL(buf, session_id);
    return res_ok_data(&buf);
}
