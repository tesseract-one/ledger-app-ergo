//
//  zigzag.h
//  ErgoTxParser
//
//  Created by Yehor Popovych on 11.08.2021.
//

#pragma once

#include <stdint.h>

static inline uint64_t zigzag_encode_i32(int32_t v) {
    return (uint64_t)((v << 1) ^ (v >> 31));
}

static inline int32_t zigzag_decode_i32(uint64_t v) {
    return (int32_t)((uint32_t)v >> 1) ^ -((int32_t)v & 1);
}

static inline uint64_t zigzag_encode_i64(int64_t v) {
    return (uint64_t)((v << 1) ^ (v >> 63));
}

static inline int64_t zigzag_decode_i64(uint64_t v) {
    return (int64_t)((v >> 1) ^ ((uint64_t)-((int64_t)(v & 1))));
}
