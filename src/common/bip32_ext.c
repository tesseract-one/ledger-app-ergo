/*****************************************************************************
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

#include <stdio.h>    // snprintf
#include <string.h>   // memset, strlen
#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool

#include <ledger_assert.h>

#include "bip32_ext.h"

bool bip32_path_validate(const uint32_t *bip32_path,
                         uint8_t bip32_path_len,
                         uint32_t type,
                         uint32_t coin,
                         bip32_path_validation_type_e vtype) {
    if (bip32_path_len <= 2 || bip32_path[0] != type || bip32_path[1] != coin) {
        return false;
    }
    switch (vtype) {
        case BIP32_PATH_VALIDATE_COIN:
            return true;
        case BIP32_PATH_VALIDATE_COIN_GE2_HARD:
            for (uint8_t i = 2; i < bip32_path_len; i++) {
                if (bip32_path[i] < BIP32_HARDENED_CONSTANT) {
                    return false;
                }
            }
            return true;
        case BIP32_PATH_VALIDATE_ACCOUNT_E3:
            return bip32_path_len == 3 && bip32_path[2] >= BIP32_HARDENED_CONSTANT;
        case BIP32_PATH_VALIDATE_ACCOUNT_GE3:
            if (bip32_path_len < 3) {
                return false;
            }
            for (uint8_t i = 2; i < bip32_path_len; i++) {
                if (bip32_path[i] < BIP32_HARDENED_CONSTANT) {
                    return false;
                }
            }
            return true;
        case BIP32_PATH_VALIDATE_ADDRESS_E5:
            return bip32_path_len == 5 && bip32_path[2] >= BIP32_HARDENED_CONSTANT &&
                   (bip32_path[3] == 0 || bip32_path[3] == 1) &&
                   bip32_path[4] < BIP32_HARDENED_CONSTANT;
        case BIP32_PATH_VALIDATE_ADDRESS_GE5:
            if (bip32_path_len < 5) {
                return false;
            }
            if (bip32_path[2] < BIP32_HARDENED_CONSTANT) {
                return false;
            }
            if (bip32_path[3] != 0 && bip32_path[3] != 1) {
                return false;
            }
            for (uint8_t i = 4; i < bip32_path_len; i++) {
                if (bip32_path[i] >= BIP32_HARDENED_CONSTANT) {
                    return false;
                }
            }
            return true;
    }
    LEDGER_ASSERT(false, "Bip32 validation type isn't handled properly");
}