#pragma once

#include <stdint.h>
#include "../../constants.h"
#include "../../common/bip32.h"

typedef struct {
    char account[11];    // account number in string form + '\0'
    char app_token[11];  // hexified app token
    uint32_t app_token_value;
    uint8_t chain_code[CHAIN_CODE_LEN];
    uint8_t raw_public_key[PUBLIC_KEY_LEN];
} extended_public_key_ui_ctx_t;