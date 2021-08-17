#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <cx.h>

#define BLAKE2B_256_DIGEST_LEN 32

bool blake2b_256_init(cx_blake2b_t* ctx);
bool blake2b_update(cx_blake2b_t* ctx, uint8_t* data, size_t len);
bool blake2b_256_finalize(cx_blake2b_t* ctx, uint8_t out[static BLAKE2B_256_DIGEST_LEN]);
bool blake2b_256(uint8_t* data, size_t len, uint8_t out[static BLAKE2B_256_DIGEST_LEN]);