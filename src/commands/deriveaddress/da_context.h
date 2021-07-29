#pragma once

#include <stdint.h>
#include "../../constants.h"
#include "../../common/bip32.h"

typedef struct {
    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t chain_code[CHAIN_CODE_LEN];
    uint8_t raw_public_key[PUBLIC_KEY_LEN];
} derive_address_ctx_t;

typedef struct {
    char bip32_path[60]; // Bip32 path string buffer
    char address[ADDRESS_STRING_MAX_LEN]; // Address string
    char app_token[9]; // hexified app token
    uint32_t app_token_value;
    bool send;
} derive_address_ui_ctx_t;