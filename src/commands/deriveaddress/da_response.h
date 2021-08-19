#pragma once

#include "../../constants.h"

/**
 * Send APDU response with address public key.
 *
 * response = G_context.derive_ctx.raw_public_key (PUBLIC_KEY_LEN)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_address(uint8_t raw_pub_key[static PUBLIC_KEY_LEN]);