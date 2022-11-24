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
                                             uint8_t address[static ADDRESS_LEN],
                                             bool is_compressed) {
    BUFFER_FROM_ARRAY_EMPTY(buffer, address, ADDRESS_LEN);

    if (!network_id_is_valid(network)) {
        return false;
    }
    // P2PK + network id
    if (!buffer_write_u8(&buffer, 0x01 + network)) {
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
    if (!buffer_write_bytes(&buffer, hash, 4)) {
        return false;
    }

    return true;
}

bool ergo_address_from_pubkey(uint8_t network,
                              const uint8_t public_key[static PUBLIC_KEY_LEN],
                              uint8_t address[static ADDRESS_LEN]) {
    return _ergo_address_from_pubkey(network, public_key, address, false);
}

bool ergo_address_from_compressed_pubkey(uint8_t network,
                                         const uint8_t public_key[static COMPRESSED_PUBLIC_KEY_LEN],
                                         uint8_t address[static ADDRESS_LEN]) {
    return _ergo_address_from_pubkey(network, public_key, address, true);
}
