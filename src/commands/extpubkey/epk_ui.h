#pragma once

#include <stdint.h>   // uint*
#include <stdbool.h>  // bool
#include "epk_context.h"
#include "../../constants.h"

/**
 * Display account on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_account(extended_public_key_ctx_t* ctx,
                       uint32_t app_access_token,
                       uint32_t* bip32_path,
                       uint8_t bip32_path_len,
                       uint8_t raw_pub_key[static PUBLIC_KEY_LEN],
                       uint8_t chain_code[static CHAIN_CODE_LEN]);
