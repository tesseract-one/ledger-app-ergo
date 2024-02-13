#pragma once

#include <stdint.h>
#include "../../constants.h"
#include "../../ui/ui_application_id.h"

typedef struct {
    uint32_t app_token_value;
    uint8_t chain_code[CHAIN_CODE_LEN];
    uint8_t raw_public_key[PUBLIC_KEY_LEN];
    char bip32_path[MAX_BIP32_STRING_LEN];   // Bip32 path string
    char app_token[APPLICATION_ID_STR_LEN];  // hexified app token
} extended_public_key_ctx_t;