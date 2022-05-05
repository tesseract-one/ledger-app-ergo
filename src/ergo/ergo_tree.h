#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../constants.h"

#define ERGO_TREE_P2PK_LEN (COMPRESSED_PUBLIC_KEY_LEN + 3)

void ergo_tree_generate_p2pk(const uint8_t raw_public_key[static PUBLIC_KEY_LEN],
                             uint8_t tree[ERGO_TREE_P2PK_LEN]);

void ergo_tree_miners_fee_tree(bool is_mainnet, const uint8_t** tree, size_t* size);