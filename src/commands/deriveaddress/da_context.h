#pragma once

#include <stdint.h>
#include "../../constants.h"
#include "../../common/bip32.h"
#include "../../ui/ui_application_id.h"

typedef struct {
    uint32_t app_token_value;
    uint8_t raw_address[ADDRESS_LEN];       // response data address
    char bip32_path[MAX_BIP32_STRING_LEN];  // Bip32 path string
    char address[ADDRESS_STRING_MAX_LEN];   // Address string
    char app_id[APPLICATION_ID_STR_LEN];    // hexified app token
    bool send;
} derive_address_ctx_t;