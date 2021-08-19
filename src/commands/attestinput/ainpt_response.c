//
//  autxo_response.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "ainpt_response.h"
#include "ainpt_context.h"
#include "ainpt_sw.h"
#include "../../globals.h"
#include "../../helpers/hmac.h"
#include "../../helpers/response.h"

#define FRAME_MAX_TOKENS_COUNT      4
#define FRAME_TOKEN_VALUE_PAIR_SIZE (TOKEN_ID_LEN + sizeof(uint64_t))
#define FRAME_SIGNATURE_SIZE        16
#define FRAME_MAX_SIZE                                     \
    (BOX_ID_LEN + 3 * sizeof(uint8_t) + sizeof(uint64_t) + \
     FRAME_MAX_TOKENS_COUNT * FRAME_TOKEN_VALUE_PAIR_SIZE + FRAME_SIGNATURE_SIZE)

#define TOKENS_COUNT(ctx)     ctx->box.tokens_count
#define BOX_VALUE(ctx)        ctx->box.ctx.value
#define BOX_ID(ctx)           ctx->box_id
#define TOKEN_INFO_COUNT(ctx) ctx->tokens_table.count
#define TOKEN_INFO(ctx, idx)  ctx->tokens_table.tokens[idx]

static inline uint8_t get_frames_count(uint8_t tokens_count) {
    uint8_t frames_count = (tokens_count + (FRAME_MAX_TOKENS_COUNT - 1)) / FRAME_MAX_TOKENS_COUNT;
    return frames_count == 0 ? 1 : frames_count;
}

int send_response_attested_input_frame(attest_input_ctx_t *ctx,
                                       uint8_t session_key[static SESSION_KEY_LEN],
                                       uint8_t index) {
    uint8_t frames_count = get_frames_count(TOKENS_COUNT(ctx));
    if (index >= frames_count) {
        return res_error(SW_ATTEST_UTXO_BAD_FRAME_INDEX);
    }

    BUFFER_NEW_LOCAL_EMPTY(buffer, FRAME_MAX_SIZE);

    if (!buffer_write_bytes(&buffer, BOX_ID(ctx), BOX_ID_LEN)) {
        return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u8(&buffer, frames_count)) {
        return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u8(&buffer, index)) {
        return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u8(&buffer, TOKENS_COUNT(ctx))) {
        return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u64(&buffer, BOX_VALUE(ctx), BE)) {
        return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    uint8_t offset = index * FRAME_MAX_TOKENS_COUNT;
    uint8_t non_empty = 0;
    for (uint8_t i = 0; i < TOKEN_INFO_COUNT(ctx); i++) {
        if (TOKEN_INFO(ctx, i).amount > 0) non_empty++;
        if (TOKEN_INFO(ctx, i).amount > 0 && non_empty >= offset) {
            if (!buffer_write_bytes(&buffer, TOKEN_INFO(ctx, i).id, TOKEN_ID_LEN)) {
                return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
            }
            if (!buffer_write_u64(&buffer, TOKEN_INFO(ctx, i).amount, BE)) {
                return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
            }
            if (non_empty - offset == FRAME_MAX_TOKENS_COUNT - 1) break;
        }
    }

    uint8_t hmac[CX_SHA256_SIZE] = {0};

    if (!hmac_sha256(session_key, SESSION_KEY_LEN, buffer.ptr, buffer_data_len(&buffer), hmac)) {
        return res_error(SW_ATTEST_UTXO_HMAC_ERROR);
    }

    if (!buffer_write_bytes(&buffer, hmac, FRAME_SIGNATURE_SIZE)) {
        return res_error(SW_ATTEST_UTXO_BUFFER_ERROR);
    }

    return res_ok_data(&buffer);
}

int send_response_attested_input_frame_count(uint8_t tokens_count) {
    uint8_t frames_count = get_frames_count(tokens_count);
    BUFFER_FROM_VAR_FULL(buf, frames_count);
    return res_ok_data(&buf);
}

int send_response_attested_input_session_id(uint8_t session_id) {
    BUFFER_FROM_VAR_FULL(buf, session_id);
    return res_ok_data(&buf);
}
