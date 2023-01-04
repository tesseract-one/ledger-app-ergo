#pragma once

#include "../../constants.h"
#include "../../ergo/address.h"

/**
 * Send APDU response with address public key.
 *
 * response = G_context.derive_ctx.raw_address (ADDRESS_LEN)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_address(uint8_t address[static P2PK_ADDRESS_LEN]);