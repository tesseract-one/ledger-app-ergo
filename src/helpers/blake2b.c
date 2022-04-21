#include "blake2b.h"

bool blake2b_256_init(cx_blake2b_t* ctx) {
    return cx_blake2b_init_no_throw(ctx, 256) == 0;
}

bool blake2b_update(cx_blake2b_t* ctx, const uint8_t* data, size_t len) {
    return cx_hash_no_throw((cx_hash_t*) ctx, 0, data, len, NULL, 0) == 0;
}

bool blake2b_256_finalize(cx_blake2b_t* ctx, uint8_t out[static BLAKE2B_256_DIGEST_LEN]) {
    return cx_hash_no_throw((cx_hash_t*) ctx,
                            CX_LAST | CX_NO_REINIT,
                            NULL,
                            0,
                            out,
                            BLAKE2B_256_DIGEST_LEN) == 0;
}

bool blake2b_256(const uint8_t* data, size_t len, uint8_t out[static BLAKE2B_256_DIGEST_LEN]) {
    cx_blake2b_t ctx;
    if (!blake2b_256_init(&ctx)) return false;
    return cx_hash_no_throw((cx_hash_t*) &ctx,
                            CX_LAST | CX_NO_REINIT,
                            data,
                            len,
                            out,
                            BLAKE2B_256_DIGEST_LEN) == 0;
}