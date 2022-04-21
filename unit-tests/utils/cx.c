#include "cx.h"
#include "common/buffer.h"
#include "blake2b-ref.h"
#include <stdlib.h>
#include <memory.h>

#define BUFFER_SIZE 65535

typedef struct {
    uint8_t hashed_data[BUFFER_SIZE];
    buffer_t hashed_data_buf;
    blake2b_state ctx;
} b2b_context;

static cx_err_t blake2_hash(cx_blake2b_t *hash,
                            uint32_t mode,
                            const uint8_t *in,
                            size_t len,
                            uint8_t *out,
                            size_t out_len) {
    b2b_context *_ctx = (b2b_context *) hash->ctx;
    if (!buffer_write_bytes(&_ctx->hashed_data_buf, in, len)) return -1;
    if (blake2b_ref_update(&_ctx->ctx, in, len) != 0) return -1;
    if (mode & CX_LAST) {
        if (blake2b_ref_final(&_ctx->ctx, out, out_len) != 0) return -1;
        if (mode & CX_NO_REINIT) return 0;
        memset(_ctx->hashed_data, 0, BUFFER_SIZE);
        buffer_init(&_ctx->hashed_data_buf, _ctx->hashed_data, BUFFER_SIZE, 0);
        return blake2b_ref_init(&_ctx->ctx, hash->info.output_size / 8);
    }
    return 0;
}

cx_err_t cx_blake2b_init_no_throw(cx_blake2b_t *hash, size_t out_len) {
    b2b_context *ctx = (b2b_context *) malloc(sizeof(b2b_context));
    if (blake2b_ref_init(&ctx->ctx, out_len / 8) == 0) {
        hash->ctx = (void *) ctx;
        hash->info.md_type = CX_BLAKE2B;
        hash->info.output_size = out_len;
        buffer_init(&ctx->hashed_data_buf, ctx->hashed_data, BUFFER_SIZE, 0);
        return 0;
    } else {
        free(ctx);
        return -1;
    }
}

cx_err_t cx_hash_no_throw(cx_hash_t *hash,
                          uint32_t mode,
                          const uint8_t *in,
                          size_t len,
                          uint8_t *out,
                          size_t out_len) {
    switch (hash->md_type) {
        case CX_BLAKE2B:
            return blake2_hash((cx_blake2b_t *) hash, mode, in, len, out, out_len);
        default:
            return -1;
    }
}

void _cx_blake2b_get_data(cx_blake2b_t *ctx, uint8_t **data, size_t *len) {
    b2b_context *_ctx = (b2b_context *) ctx->ctx;
    *data = _ctx->hashed_data;
    *len = buffer_data_len(&_ctx->hashed_data_buf);
}

void _cx_blake2b_free_data(cx_blake2b_t *ctx) {
    free(ctx->ctx);
}