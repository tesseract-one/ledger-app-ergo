#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../constants.h"

bool ergo_secp256k1_schnorr_sign(uint8_t signature[static ERGO_SIGNATURE_LEN],
                                 const uint8_t message[static ERGO_ID_LEN],
                                 const uint8_t secret[static PRIVATE_KEY_LEN]);