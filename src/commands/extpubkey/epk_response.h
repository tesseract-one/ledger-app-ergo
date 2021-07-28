#pragma once

/**
 * Send APDU response with public key and chain code.
 *
 * response = G_context.ext_pub_ctx.raw_public_key (PUBLIC_KEY_LEN) ||
 *            G_context.ext_pub_ctx.chain_code (CHAIN_CODE_LEN)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_extended_pubkey(void);