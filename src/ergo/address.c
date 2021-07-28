#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include <os.h>
#include <cx.h>

#include "address.h"
#include "../common/base58.h"

bool address_from_pubkey(uint8_t network, const uint8_t public_key[static PUBLIC_KEY_LEN], uint8_t *out, size_t out_len) {
    if (network > 252 || out_len < ADDRESS_LEN) {
        return false;
    }

    // P2PK + network id
    out[0] = 0x01 + network;

    // Compressed pubkey
    out[1] = ((public_key[64] & 1) ? 0x03 : 0x02);
    memcpy(out + 2, public_key + 1, 32);

    uint8_t hash[32] = {0};
    cx_hash_t blake2b;

    if (cx_hash_init_ex(&blake2b, CX_BLAKE2B, 256) != CX_BLAKE2B) {
        return false;
    }

    cx_hash(&blake2b, CX_LAST, out, 34, hash, sizeof(hash));

    memcpy(out + 34, hash, 4);

    return true;
}
