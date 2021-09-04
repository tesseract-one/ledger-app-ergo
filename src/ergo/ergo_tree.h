#pragma once

#include <stdint.h>
#include "../constants.h"

#define ERGO_TREE_P2PK_LEN (COMPRESSED_PUBLIC_KEY_LEN + 3)

extern const uint8_t C_ERGO_TREE_MINERS_HASH_FEE[105];

void ergo_tree_generate_p2pk(const uint8_t raw_public_key[static PUBLIC_KEY_LEN],
                             uint8_t tree[ERGO_TREE_P2PK_LEN]);