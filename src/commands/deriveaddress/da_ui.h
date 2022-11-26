#pragma once

#include <stdint.h>   // uint*
#include <stdbool.h>  // bool
#include "da_context.h"
#include "../../constants.h"

/**
 * Display account on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address(derive_address_ctx_t* ctx,
                       bool send,
                       uint32_t app_access_token,
                       uint32_t* bip32_path,
                       uint8_t bip32_path_len,
                       uint8_t raw_address[static P2PK_ADDRESS_LEN]);