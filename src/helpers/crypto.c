/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>   // uint*_t
#include <string.h>   // memset, explicit_bzero
#include <stdbool.h>  // bool

#include "crypto.h"
#include "globals.h"

#define PRIVATE_KEY_SIZE 32

uint16_t crypto_derive_private_key(cx_ecfp_private_key_t *private_key,
                                   uint8_t chain_code[static CHAIN_CODE_LEN],
                                   const uint32_t *bip32_path,
                                   uint8_t bip32_path_len) {
    uint8_t raw_private_key[PRIVATE_KEY_SIZE] = {0};
    uint16_t result = 0;
    BEGIN_TRY {
        TRY {
            // derive the seed with bip32_path
            os_perso_derive_node_bip32(CX_CURVE_256K1,
                                       bip32_path,
                                       bip32_path_len,
                                       raw_private_key,
                                       chain_code);
            // new private_key from raw
            cx_ecfp_init_private_key(CX_CURVE_256K1,
                                     raw_private_key,
                                     sizeof(raw_private_key),
                                     private_key);
        }
        CATCH_OTHER(e) {
            result = e;
        }
        FINALLY {
            explicit_bzero(raw_private_key, sizeof(raw_private_key));
        }
    }
    END_TRY;
    return result;
}

uint16_t crypto_init_public_key(cx_ecfp_private_key_t *private_key,
                                cx_ecfp_public_key_t *public_key,
                                uint8_t raw_public_key[static PUBLIC_KEY_LEN]) {
    // generate corresponding public key
    cx_err_t result = cx_ecfp_generate_pair_no_throw(CX_CURVE_256K1, public_key, private_key, 1);
    if (result == 0) {
        memmove(raw_public_key, public_key->W, PUBLIC_KEY_LEN);
    }
    return (uint16_t) result;
}

uint16_t crypto_generate_public_key(const uint32_t *bip32_path,
                                    uint8_t bip32_path_len,
                                    uint8_t raw_public_key[static PUBLIC_KEY_LEN],
                                    uint8_t chain_code[CHAIN_CODE_LEN]) {
    bool has_chain_code = chain_code != NULL;
    uint8_t temp_chain_code[CHAIN_CODE_LEN];
    if (!has_chain_code) {
        chain_code = temp_chain_code;
    }
    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};

    // derive private key according to BIP32 path
    uint16_t result =
        crypto_derive_private_key(&private_key, chain_code, bip32_path, bip32_path_len);
    if (result == 0) {
        // generate corresponding public key
        result = crypto_init_public_key(&private_key, &public_key, raw_public_key);
    }
    // reset private key and chaincode
    explicit_bzero(&private_key, sizeof(private_key));
    if (!has_chain_code) {
        explicit_bzero(chain_code, CHAIN_CODE_LEN);
    }
    return result;
}
