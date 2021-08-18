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
#include "../../io.h"

#include <cx.h>

static inline uint8_t get_frames_count(uint8_t tokens_count) {
    uint8_t frames_count = tokens_count / 4;
    if (tokens_count % 4 > 0) {
        frames_count++;
    }
    if (frames_count == 0) {
        frames_count = 1;
    }
    return frames_count;
}

int send_response_attested_input_frame(uint8_t index) {
    if (index >= get_frames_count(G_context.input_ctx.box.tokens_count)) {
        return io_send_sw(SW_ATTEST_UTXO_BAD_FRAME_INDEX);
    }
    uint8_t _buf[218];
    buffer_t buffer = {0};
    buffer_init(&buffer, _buf, sizeof(_buf), 0);
    
    if (!buffer_write_bytes(&buffer, G_context.input_ctx.box_id, BOX_ID_LEN)) {
        return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u8(&buffer, index)) {
        return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u8(&buffer, G_context.input_ctx.box.tokens_count)) {
        return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    if (!buffer_write_u64(&buffer, G_context.input_ctx.box.ctx.value, BE)) {
        return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
    }
    uint8_t offset = index * 4;
    uint8_t non_empty = 0;
    for (uint8_t i = 0; i < G_context.input_ctx.tokens_table.count; i++) {
        if (G_context.input_ctx.tokens_table.tokens[i].amount > 0) {
            non_empty++;
        }
        if (G_context.input_ctx.tokens_table.tokens[i].amount > 0 && 
            non_empty >= offset) {
            if (!buffer_write_bytes(&buffer,
                                    G_context.input_ctx.tokens_table.tokens[i].id,
                                    TOKEN_ID_LEN)) {
                return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
            }
            if (!buffer_write_u64(&buffer,
                                  G_context.input_ctx.tokens_table.tokens[i].amount,
                                  BE)) {
                return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
            }
            if (non_empty - offset == 3) break;
        }
    }
    
    uint8_t hmac[CX_SHA256_SIZE] = {0};

    if (!hmac_sha256(G_context.session_key,
                     SESSION_KEY_LEN,
                     buffer.ptr,
                     buffer_data_len(&buffer),
                     hmac)) {
        return io_send_sw(SW_ATTEST_UTXO_HMAC_ERROR);
    }
    
    if (!buffer_write_bytes(&buffer, hmac, 16)) {
        return io_send_sw(SW_ATTEST_UTXO_BUFFER_ERROR);
    }

    return io_send_response(&buffer, SW_OK);
}

int send_response_attested_input_frame_count(void) {
    uint8_t frames_count = get_frames_count(G_context.input_ctx.box.tokens_count);
    buffer_t buffer = {0};
    buffer_init(&buffer, &frames_count, 1, 1);
    return io_send_response(&buffer, SW_OK);
}

int send_response_attested_input_session_id(void) {
    buffer_t buffer = {0};
    buffer_init(&buffer, &G_context.input_ctx.session, 1, 1);
    return io_send_response(&buffer, SW_OK);
}
