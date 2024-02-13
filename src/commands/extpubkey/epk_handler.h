#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include <buffer.h>

/**
 * Handler for CMD_GET_EXTENDED_PUBLIC_KEY command. If successfully parse BIP32 path,
 * derive public key/chain code and send APDU response.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path and optional access token
 * @param[in]     has_access_token
 *   Whether data has access token or not
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_get_extended_public_key(buffer_t *cdata, bool has_access_token);