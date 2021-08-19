#include "hmac.h"

bool hmac_sha256(const uint8_t *key,
                 size_t key_len,
                 const uint8_t *data,
                 size_t data_len,
                 uint8_t hash[CX_SHA256_SIZE]) {
    cx_hmac_sha256_t context;
    if (cx_hmac_sha256_init_no_throw(&context, key, key_len) != 0) {
        return false;
    }
    return cx_hmac_no_throw((cx_hmac_t *) &context,
                            CX_LAST | CX_NO_REINIT,
                            data,
                            data_len,
                            hash,
                            CX_SHA256_SIZE) == 0;
}