//
//  gve.c
//  ErgoTxParser
//
//  Created by Yehor Popovych on 11.08.2021.
//

#include "gve.h"

gve_result_e gve_get_i16(buffer_t *buffer, int16_t *val) {
    int32_t i32;
    gve_result_e res;
    *val = 0;
    if ((res = gve_get_i32(buffer, &i32)) != GVE_OK) return res;
    if (i32 > INT16_MAX || i32 < INT16_MIN) return GVE_ERR_INT_TO_BIG;
    *val = (int16_t) i32;
    return GVE_OK;
}

gve_result_e gve_get_u16(buffer_t *buffer, uint16_t *val) {
    uint64_t u64;
    gve_result_e res;
    *val = 0;
    if ((res = gve_get_u64(buffer, &u64)) != GVE_OK) return res;
    if (u64 > UINT16_MAX) return GVE_ERR_INT_TO_BIG;
    *val = (uint16_t) u64;
    return GVE_OK;
}

gve_result_e gve_get_i32(buffer_t *buffer, int32_t *val) {
    uint64_t u64;
    gve_result_e res;
    *val = 0;
    if ((res = gve_get_u64(buffer, &u64)) != GVE_OK) return res;
    if (u64 > 0xFFFFFFFFULL) return GVE_ERR_INT_TO_BIG;
    *val = zigzag_decode_i32(u64);
    return GVE_OK;
}

gve_result_e gve_get_u32(buffer_t *buffer, uint32_t *val) {
    uint64_t u64;
    gve_result_e res;
    *val = 0;
    if ((res = gve_get_u64(buffer, &u64)) != GVE_OK) return res;
    if (u64 > UINT32_MAX) return GVE_ERR_INT_TO_BIG;
    *val = (uint32_t) u64;
    return GVE_OK;
}

gve_result_e gve_get_i64(buffer_t *buffer, int64_t *val) {
    uint64_t u64;
    gve_result_e res;
    *val = 0;
    if ((res = gve_get_u64(buffer, &u64)) != GVE_OK) return res;
    *val = zigzag_decode_i64(u64);
    return GVE_OK;
}

gve_result_e gve_get_u64(buffer_t *buffer, uint64_t *val) {
    *val = 0;
    uint8_t byte = 0;
    size_t shift = 0;
    gve_result_e res = GVE_ERR_DATA_SIZE;
    while (shift < 64) {
        if ((res = gve_get_u8(buffer, &byte)) != GVE_OK) break;
        *val |= (uint64_t) (byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            res = GVE_OK;
            break;
        }
        shift += 7;
    }
    return res;
}

gve_result_e gve_put_u64(rw_buffer_t *buffer, uint64_t val) {
    uint8_t out[10];
    size_t i = 0;
    do {
        uint8_t byte = val & 0x7FU;
        val >>= 7;
        if (val) byte |= 0x80U;
        out[i++] = byte;
    } while (val);
    return rw_buffer_write_bytes(buffer, out, i) ? GVE_OK : GVE_ERR_DATA_SIZE;
}
