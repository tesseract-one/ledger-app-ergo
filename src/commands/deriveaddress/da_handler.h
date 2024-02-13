#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include <buffer.h>

/**
 * Handler for CMD_DERIVE_ADDRESS command. If successfully parse BIP32 path,
 * derive public key/chain code and send APDU response or show on screen.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path and optional access token
 * @param[in]     display
 *   Show address on screen or return through APDU
 * @param[in]     has_access_token
 *   Whether data has access token or not
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_derive_address(buffer_t *cdata, bool display, bool has_access_token);