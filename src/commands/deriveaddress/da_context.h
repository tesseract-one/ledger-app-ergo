#pragma once

#include <stdint.h>
#include "../../constants.h"
#include "../../ui/ui_application_id.h"
#include "../../ergo/address.h"

typedef struct {
    uint32_t app_token_value;
    uint8_t raw_address[P2PK_ADDRESS_LEN];      // response data address
    char bip32_path[MAX_BIP32_STRING_LEN];      // Bip32 path string
    char address[P2PK_ADDRESS_STRING_MAX_LEN];  // Address string
    char app_id[APPLICATION_ID_STR_LEN];        // hexified app token
    bool send;
} derive_address_ctx_t;