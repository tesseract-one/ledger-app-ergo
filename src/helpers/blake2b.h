#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <cx.h>

bool blake2b_256_init(cx_blake2b_t* ctx);
bool blake2b_update(cx_blake2b_t* ctx, const uint8_t* data, size_t len);
bool blake2b_256_finalize(cx_blake2b_t* ctx, uint8_t out[static CX_BLAKE2B_256_SIZE]);
bool blake2b_256(const uint8_t* data, size_t len, uint8_t out[static CX_BLAKE2B_256_SIZE]);