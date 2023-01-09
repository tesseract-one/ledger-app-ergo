#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include <os.h>
#include <cx.h>

#include "address.h"
#include "network_id.h"
#include "../common/base58.h"
#include "../common/buffer.h"
#include "../helpers/blake2b.h"

static inline bool _ergo_address_from_pubkey(uint8_t network,
                                             const uint8_t* public_key,
                                             uint8_t address[static P2PK_ADDRESS_LEN],
                                             bool is_compressed) {
    BUFFER_FROM_ARRAY_EMPTY(buffer, address, P2PK_ADDRESS_LEN);

    if (!network_id_is_valid(network)) {
        return false;
    }
    // P2PK + network id
    if (!buffer_write_u8(&buffer, ERGO_ADDRESS_TYPE_P2PK + network)) {
        return false;
    }

    if (is_compressed) {
        if (!buffer_write_bytes(&buffer, public_key, COMPRESSED_PUBLIC_KEY_LEN)) {
            return false;
        }
    } else {
        // Compress pubkey
        if (!buffer_write_u8(&buffer, ((public_key[64] & 1) ? 0x03 : 0x02))) {
            return false;
        }
        if (!buffer_write_bytes(&buffer, public_key + 1, COMPRESSED_PUBLIC_KEY_LEN - 1)) {
            return false;
        }
    }

    uint8_t hash[BLAKE2B_256_DIGEST_LEN] = {0};

    if (!blake2b_256(buffer_read_ptr(&buffer), buffer_data_len(&buffer), hash)) {
        return false;
    }
    // Checksum
    if (!buffer_write_bytes(&buffer, hash, ADDRESS_CHECKSUM_LEN)) {
        return false;
    }

    return true;
}

bool ergo_address_from_pubkey(uint8_t network,
                              const uint8_t public_key[static PUBLIC_KEY_LEN],
                              uint8_t address[static P2PK_ADDRESS_LEN]) {
    return _ergo_address_from_pubkey(network, public_key, address, false);
}

bool ergo_address_from_compressed_pubkey(uint8_t network,
                                         const uint8_t public_key[static COMPRESSED_PUBLIC_KEY_LEN],
                                         uint8_t address[static P2PK_ADDRESS_LEN]) {
    return _ergo_address_from_pubkey(network, public_key, address, true);
}

bool ergo_address_from_script_hash(uint8_t network,
                                   const uint8_t hash[static P2SH_HASH_LEN],
                                   uint8_t address[static P2SH_ADDRESS_LEN]) {
    BUFFER_FROM_ARRAY_EMPTY(buffer, address, P2SH_ADDRESS_LEN);
    if (!network_id_is_valid(network)) {
        return false;
    }
    // P2SH + network id
    if (!buffer_write_u8(&buffer, ERGO_ADDRESS_TYPE_P2SH + network)) {
        return false;
    }
    if (!buffer_write_bytes(&buffer, hash, P2SH_HASH_LEN)) {
        return false;
    }
    uint8_t checksum[BLAKE2B_256_DIGEST_LEN] = {0};
    if (!blake2b_256(buffer_read_ptr(&buffer), buffer_data_len(&buffer), checksum)) {
        return false;
    }
    // Checksum
    if (!buffer_write_bytes(&buffer, checksum, ADDRESS_CHECKSUM_LEN)) {
        return false;
    }
    return true;
}