#include "blake2b.h"

bool blake2b_256_init(cx_blake2b_t* ctx) {
    return cx_blake2b_init_no_throw(ctx, 256) == CX_OK;
}

bool blake2b_update(cx_blake2b_t* ctx, const uint8_t* data, size_t len) {
    return cx_hash_no_throw((cx_hash_t*) ctx, 0, data, len, NULL, 0) == CX_OK;
}

bool blake2b_256_finalize(cx_blake2b_t* ctx, uint8_t out[static CX_BLAKE2B_256_SIZE]) {
    return cx_hash_no_throw((cx_hash_t*) ctx,
                            CX_LAST | CX_NO_REINIT,
                            NULL,
                            0,
                            out,
                            CX_BLAKE2B_256_SIZE) == CX_OK;
}

bool blake2b_256(const uint8_t* data, size_t len, uint8_t out[static CX_BLAKE2B_256_SIZE]) {
    return cx_blake2b_256_hash(data, len, out) == CX_OK;
}