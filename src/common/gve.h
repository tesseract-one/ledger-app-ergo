//
//  gve.h
//  ErgoTxParser
//
//  Created by Yehor Popovych on 11.08.2021.
//

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "rwbuffer.h"
#include "zigzag.h"

typedef enum { GVE_OK = 0, GVE_ERR_INT_TO_BIG, GVE_ERR_DATA_SIZE } gve_result_e;

static inline gve_result_e gve_get_u8(buffer_t *buffer, uint8_t *val) {
    return buffer_read_u8(buffer, val) ? GVE_OK : GVE_ERR_DATA_SIZE;
}

static inline gve_result_e gve_get_i8(buffer_t *buffer, int8_t *val) {
    return gve_get_u8(buffer, (uint8_t *) val);
}

gve_result_e gve_get_i16(buffer_t *buffer, int16_t *val);
gve_result_e gve_get_u16(buffer_t *buffer, uint16_t *val);
gve_result_e gve_get_i32(buffer_t *buffer, int32_t *val);
gve_result_e gve_get_u32(buffer_t *buffer, uint32_t *val);
gve_result_e gve_get_i64(buffer_t *buffer, int64_t *val);
gve_result_e gve_get_u64(buffer_t *buffer, uint64_t *val);

gve_result_e gve_put_u64(rw_buffer_t *buffer, uint64_t val);

static inline gve_result_e gve_put_i64(rw_buffer_t *buffer, int64_t val) {
    return gve_put_u64(buffer, zigzag_encode_i64(val));
}

static inline gve_result_e gve_put_u32(rw_buffer_t *buffer, uint32_t val) {
    return gve_put_u64(buffer, val);
}

static inline gve_result_e gve_put_i32(rw_buffer_t *buffer, int32_t val) {
    return gve_put_u64(buffer, zigzag_encode_i32(val));
}

static inline gve_result_e gve_put_u8(rw_buffer_t *buffer, uint8_t val) {
    return rw_buffer_write_u8(buffer, val) ? GVE_OK : GVE_ERR_DATA_SIZE;
}

static inline gve_result_e gve_put_i8(rw_buffer_t *buffer, int8_t val) {
    return gve_put_u8(buffer, (uint8_t) val);
}

static inline gve_result_e gve_put_i16(rw_buffer_t *buffer, int16_t val) {
    return gve_put_u32(buffer, (uint32_t) zigzag_encode_i32(val));
}

static inline gve_result_e gve_put_u16(rw_buffer_t *buffer, uint16_t val) {
    return gve_put_u64(buffer, val);
}
