#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../constants.h"
#include "../helpers/blake2b.h"

bool ergo_secp256k1_schnorr_p2pk_sign_init(cx_blake2b_t* hash,
                                           uint8_t key[static PRIVATE_KEY_LEN],
                                           const uint8_t secret[static PRIVATE_KEY_LEN]);

bool ergo_secp256k1_schnorr_p2pk_sign_finish(uint8_t signature[static ERGO_SIGNATURE_LEN],
                                             cx_blake2b_t* hash,
                                             const uint8_t secret[static PRIVATE_KEY_LEN],
                                             const uint8_t key[static PRIVATE_KEY_LEN]);
