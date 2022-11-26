#pragma once

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool

#include "../constants.h"

/**
 * Length of Address checksum in bytes.
 */
#define ADDRESS_CHECKSUM_LEN 4

/**
 * Length of Address prefix (type + network_id) in bytes.
 */
#define ADDRESS_PREFIX_LEN 1

/**
 * Length of P2PK Address in bytes.
 */
#define P2PK_ADDRESS_LEN (COMPRESSED_PUBLIC_KEY_LEN + ADDRESS_PREFIX_LEN + ADDRESS_CHECKSUM_LEN)

/**
 * Length of P2SH Address hash in bytes.
 */
#define P2SH_HASH_LEN 24  // 192bits

/**
 * Length of P2SH Address in bytes.
 */
#define P2SH_ADDRESS_LEN (P2SH_HASH_LEN + ADDRESS_PREFIX_LEN + ADDRESS_CHECKSUM_LEN)

/**
 * Length of P2PK Address string in chars. (base58 of 38 bytes)
 */
#define P2PK_ADDRESS_STRING_MAX_LEN 55

typedef enum {
    ERGO_ADDRESS_TYPE_P2PK = 0x01,
    ERGO_ADDRESS_TYPE_P2SH = 0x02,
    ERGO_ADDRESS_TYPE_P2S = 0x03
} ergo_address_type_e;

/**
 * Convert public key to address.
 *
 * https://ergoplatform.org/en/blog/2019_07_24_ergo_address/
 *
 * @param[in]  public_key
 *   Pointer to byte buffer with public key.
 *   The public key is represented as 65 bytes with 32 bytes for
 *   each coordinate.
 * @param[out] address
 *   Pointer to output byte buffer for address.
 *
 * @return true if success, false otherwise.
 *
 */
bool ergo_address_from_pubkey(uint8_t network,
                              const uint8_t public_key[static PUBLIC_KEY_LEN],
                              uint8_t address[static P2PK_ADDRESS_LEN]);

/**
 * Convert compressed public key to address.
 *
 * https://ergoplatform.org/en/blog/2019_07_24_ergo_address/
 *
 * @param[in]  public_key
 *   Pointer to byte buffer with public key.
 *   The public key is represented as 33 bytes.
 * @param[out] address
 *   Pointer to output byte buffer for address.
 *
 * @return true if success, false otherwise.
 *
 */
bool ergo_address_from_compressed_pubkey(uint8_t network,
                                         const uint8_t public_key[static COMPRESSED_PUBLIC_KEY_LEN],
                                         uint8_t address[static P2PK_ADDRESS_LEN]);

/**
 * Convert script hash to P2SH address.
 *
 * https://ergoplatform.org/en/blog/2019_07_24_ergo_address/
 *
 * @param[in]  hash
 *   Pointer to byte buffer with 24 bytes of blake2b256 script hash..
 * @param[out] address
 *   Pointer to output byte buffer for address.
 *
 * @return true if success, false otherwise.
 *
 */
bool ergo_address_from_script_hash(uint8_t network,
                                   const uint8_t hash[static P2SH_HASH_LEN],
                                   uint8_t address[static P2SH_ADDRESS_LEN]);
