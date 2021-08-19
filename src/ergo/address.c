#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include <os.h>
#include <cx.h>

#include "address.h"
#include "../common/base58.h"
#include "../common/buffer.h"
#include "../helpers/blake2b.h"

bool address_from_pubkey(uint8_t network,
                         const uint8_t public_key[static PUBLIC_KEY_LEN],
                         uint8_t *out,
                         size_t out_len) {
    BUFFER_FROM_ARRAY_EMPTY(buffer, out, out_len);

    if (network > 252 || out_len < ADDRESS_LEN) {
        return false;
    }
    // P2PK + network id
    if (!buffer_write_u8(&buffer, 0x01 + network)) {
        return false;
    }
    // Compressed pubkey
    if (!buffer_write_u8(&buffer, ((public_key[64] & 1) ? 0x03 : 0x02))) {
        return false;
    }
    if (!buffer_write_bytes(&buffer, public_key + 1, 32)) {
        return false;
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
