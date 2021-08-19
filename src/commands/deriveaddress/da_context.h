#pragma once

#include <stdint.h>
#include "../../constants.h"
#include "../../common/bip32.h"

typedef struct {
    char bip32_path[60];                   // Bip32 path string buffer
    char address[ADDRESS_STRING_MAX_LEN];  // Address string
    char app_token[11];                    // hexified app token
    uint32_t app_token_value;
    uint8_t raw_public_key[PUBLIC_KEY_LEN];  // response data address
    bool send;
} derive_address_ui_ctx_t;