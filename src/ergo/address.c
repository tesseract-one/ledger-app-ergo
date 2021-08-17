#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include <os.h>
#include <cx.h>

#include "address.h"
#include "../common/base58.h"
#include "../helpers/blake2b.h"

bool address_from_pubkey(uint8_t network, const uint8_t public_key[static PUBLIC_KEY_LEN], uint8_t *out, size_t out_len) {
    if (network > 252 || out_len < ADDRESS_LEN) {
        return false;
    }
    size_t offset = 0;

    // P2PK + network id
    out[offset++] = 0x01 + network;

    // Compressed pubkey
    out[offset++] = ((public_key[64] & 1) ? 0x03 : 0x02);
    memcpy(out + offset, public_key + 1, 32);
    offset += 32;

    uint8_t hash[BLAKE2B_256_DIGEST_LEN] = {0};

    if (!blake2b_256(out, offset, hash)) return false;

    memcpy(out + offset, hash, 4);

    return true;
}
