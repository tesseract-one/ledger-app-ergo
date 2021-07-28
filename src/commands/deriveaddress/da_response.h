#pragma once

/**
 * Send APDU response with address public key.
 *
 * response = G_context.derive_ctx.raw_public_key (PUBLIC_KEY_LEN)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_address(void);