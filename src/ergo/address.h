#pragma once

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool

#include "../constants.h"

/**
 * Convert public key to address.
 *
 * https://ergoplatform.org/en/blog/2019_07_24_ergo_address/
 *
 * @param[in]  public_key
 *   Pointer to byte buffer with public key.
 *   The public key is represented as 65 bytes with 32 bytes for
 *   each coordinate.
 * @param[out] out
 *   Pointer to output byte buffer for address.
 *
 * @return true if success, false otherwise.
 *
 */
bool ergo_address_from_pubkey(uint8_t network,
                              const uint8_t public_key[static PUBLIC_KEY_LEN],
                              uint8_t address[static ADDRESS_LEN]);

/**
 * Convert compressed public key to address.
 *
 * https://ergoplatform.org/en/blog/2019_07_24_ergo_address/
 *
 * @param[in]  public_key
 *   Pointer to byte buffer with public key.
 *   The public key is represented as 33 bytes.
 * @param[out] out
 *   Pointer to output byte buffer for address.
 *
 * @return true if success, false otherwise.
 *
 */
bool ergo_address_from_compressed_pubkey(uint8_t network,
                                         const uint8_t public_key[static COMPRESSED_PUBLIC_KEY_LEN],
                                         uint8_t address[static ADDRESS_LEN]);
