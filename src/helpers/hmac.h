#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <cx.h>

bool hmac_sha256(const uint8_t *key,
                 size_t key_len,
                 const uint8_t *data,
                 size_t data_len,
                 uint8_t hash[CX_SHA256_SIZE]);