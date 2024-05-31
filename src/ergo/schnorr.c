#include "schnorr.h"
#include <cx.h>
#include <os.h>
#include <string.h>
#include "../helpers/blake2b.h"

#define ERGO_SOUNDNESS_BYTES 24
#define MAX_ITERATIONS       100

// Gx: 0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
// Gy: 0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
static uint8_t const SECP256K1_G[] = {
    0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
    0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,

    0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
    0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19, 0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8};

// n: 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
static uint8_t const SECP256K1_N[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
    0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48, 0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41};

static uint8_t const P2PK_PREFIX[] = {0x01, 0x00, 0x27, 0x10, 0x01, 0x08, 0xcd};

static uint8_t const P2PK_SUFFIX[] = {0x73, 0x00, 0x00, 0x21};

bool ergo_secp256k1_schnorr_p2pk_sign_init(cx_blake2b_t* hash,
                                           uint8_t key[static PRIVATE_KEY_LEN],
                                           const uint8_t secret[static PRIVATE_KEY_LEN]) {
    uint8_t buf[PUBLIC_KEY_LEN];
    for (uint8_t i = 0; i < MAX_ITERATIONS; i++) {
        bool result = false;
        do {
            if (!blake2b_256_init(hash)) break;
            // compute commitment prefix P(c) = H(prefix || pk || postfix || w)
            // adds preifx
            if (!blake2b_update(hash, PIC(P2PK_PREFIX), sizeof(P2PK_PREFIX))) break;

            int cmp_diff;
            // check private key has a proper value
            if (cx_math_is_zero(secret, PRIVATE_KEY_LEN)) break;
            if (cx_math_cmp_no_throw(secret, PIC(SECP256K1_N), PRIVATE_KEY_LEN, &cmp_diff) != 0 ||
                cmp_diff > 0)
                break;

            // pk = G * secret (pub key)
            buf[0] = 0x04;
            memcpy(buf + 1, PIC(SECP256K1_G), sizeof(SECP256K1_G));
            if (cx_ecfp_scalar_mult_no_throw(CX_CURVE_SECP256K1, buf, secret, PRIVATE_KEY_LEN) != 0)
                break;
            // compress pk
            buf[0] = (buf[PUBLIC_KEY_LEN - 1] & 1) == 1 ? 0x03 : 0x02;

            // compute commitment prefix P(c) = H(prefix || pk || postfix || w)
            // add pk and postfix
            if (!blake2b_update(hash, buf, COMPRESSED_PUBLIC_KEY_LEN)) break;
            if (!blake2b_update(hash, PIC(P2PK_SUFFIX), sizeof(P2PK_SUFFIX))) break;

            // generate ephemeral private key
            cx_rng_no_throw(key, PRIVATE_KEY_LEN);

            // check it has a proper value
            if (cx_math_is_zero(key, PRIVATE_KEY_LEN)) break;
            if (cx_math_cmp_no_throw(key, PIC(SECP256K1_N), PRIVATE_KEY_LEN, &cmp_diff) != 0 ||
                cmp_diff > 0)
                break;

            // w = G * y (pub key)
            buf[0] = 0x04;
            memcpy(buf + 1, PIC(SECP256K1_G), sizeof(SECP256K1_G));
            if (cx_ecfp_scalar_mult_no_throw(CX_CURVE_SECP256K1, buf, key, PRIVATE_KEY_LEN) != 0)
                break;
            // compress w
            buf[0] = (buf[PUBLIC_KEY_LEN - 1] & 1) == 1 ? 0x03 : 0x02;

            // compute commitment prefix P(c) = H(prefix || pk || postfix || w)
            // add w
            if (!blake2b_update(hash, buf, COMPRESSED_PUBLIC_KEY_LEN)) break;
            result = true;
        } while (0);

        explicit_bzero(buf, sizeof(buf));

        if (result) return result;
    }
    return false;
}

bool ergo_secp256k1_schnorr_p2pk_sign_finish(uint8_t signature[static ERGO_SIGNATURE_LEN],
                                             cx_blake2b_t* hash,
                                             const uint8_t secret[static PRIVATE_KEY_LEN],
                                             const uint8_t key[static PRIVATE_KEY_LEN]) {
    uint8_t buf[PUBLIC_KEY_LEN];
    // compute hash
    if (!blake2b_256_finalize(hash, signature)) return false;

    // build c
    // important: we only use the first 24 bytes of the hash output!
    memset(buf, 0, CX_BLAKE2B_256_SIZE - ERGO_SOUNDNESS_BYTES);
    memcpy(buf + CX_BLAKE2B_256_SIZE - ERGO_SOUNDNESS_BYTES, signature, ERGO_SOUNDNESS_BYTES);

    if (cx_math_is_zero(buf, CX_BLAKE2B_256_SIZE)) return false;

    // z = c * secret + key
    if (cx_math_multm_no_throw(buf, buf, secret, PIC(SECP256K1_N), PRIVATE_KEY_LEN) != 0)
        return false;
    if (cx_math_addm_no_throw(signature + ERGO_SOUNDNESS_BYTES,
                              key,
                              buf,
                              PIC(SECP256K1_N),
                              PRIVATE_KEY_LEN) != 0)
        return false;

    explicit_bzero(buf, sizeof(buf));

    return true;
}
