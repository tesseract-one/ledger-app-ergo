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
    size_t offset = 0;

    // P2PK + network id
    out[offset++] = 0x01 + network;

    // Compressed pubkey
    out[offset++] = ((public_key[64] & 1) ? 0x03 : 0x02);
    memcpy(out + offset, public_key + 1, 32);
    offset += 32;

    uint8_t hash[32] = {0};
    cx_blake2b_t blake2b;

    if (cx_blake2b_init_no_throw(&blake2b, 256) != 0) {
        return false;
    }

    if (cx_hash_no_throw((cx_hash_t*) &blake2b,
                          CX_LAST,
                          out,
                          offset,
                          hash,
                          sizeof(hash)) != 0) {
        return false;
    }

    memcpy(out + offset, hash, 4);

    return true;
}
