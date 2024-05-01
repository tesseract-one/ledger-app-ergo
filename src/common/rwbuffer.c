#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include <write.h>
#include "rwbuffer.h"

bool rw_buffer_seek_write_set(rw_buffer_t *buffer, size_t offset) {
    if (offset > buffer->size) {
        return false;
    }
    buffer->read.size = offset;
    return true;
}

bool rw_buffer_seek_write_cur(rw_buffer_t *buffer, size_t offset) {
    if (buffer->read.size + offset < buffer->read.size ||  // overflow
        buffer->read.size + offset > buffer->size) {       // exceed buffer size
        return false;
    }
    buffer->read.size += offset;
    return true;
}

bool rw_buffer_seek_write_end(rw_buffer_t *buffer, size_t offset) {
    if (offset > buffer->size) {
        return false;
    }
    buffer->read.size = buffer->size - offset;
    return true;
}

bool rw_buffer_write_u8(rw_buffer_t *buffer, uint8_t value) {
    if (!rw_buffer_can_write(buffer, 1)) {
        return false;
    }

    ((uint8_t *) buffer->read.ptr)[buffer->read.size] = value;

    rw_buffer_seek_write_cur(buffer, 1);
    return true;
}

bool rw_buffer_write_u16(rw_buffer_t *buffer, uint16_t value, endianness_t endianness) {
    if (!rw_buffer_can_write(buffer, 2)) {
        return false;
    }

    if (endianness == BE) {
        write_u16_be((uint8_t *) buffer->read.ptr, buffer->read.size, value);
    } else {
        write_u16_le((uint8_t *) buffer->read.ptr, buffer->read.size, value);
    }

    rw_buffer_seek_write_cur(buffer, 2);

    return true;
}

bool rw_buffer_write_u32(rw_buffer_t *buffer, uint32_t value, endianness_t endianness) {
    if (!rw_buffer_can_write(buffer, 4)) {
        return false;
    }

    if (endianness == BE) {
        write_u32_be((uint8_t *) buffer->read.ptr, buffer->read.size, value);
    } else {
        write_u32_le((uint8_t *) buffer->read.ptr, buffer->read.size, value);
    }

    rw_buffer_seek_write_cur(buffer, 4);

    return true;
}

bool rw_buffer_write_u64(rw_buffer_t *buffer, uint64_t value, endianness_t endianness) {
    if (!rw_buffer_can_write(buffer, 8)) {
        return false;
    }

    if (endianness == BE) {
        write_u64_be((uint8_t *) buffer->read.ptr, buffer->read.size, value);
    } else {
        write_u64_le((uint8_t *) buffer->read.ptr, buffer->read.size, value);
    }

    rw_buffer_seek_write_cur(buffer, 8);

    return true;
}

bool rw_buffer_write_bytes(rw_buffer_t *buffer, const uint8_t *from, size_t from_len) {
    if (!rw_buffer_can_write(buffer, from_len)) {
        return false;
    }

    memmove(rw_buffer_write_ptr(buffer), from, from_len);

    rw_buffer_seek_write_cur(buffer, from_len);

    return true;
}

void rw_buffer_shift_data(rw_buffer_t *buffer) {
    if (rw_buffer_read_position(buffer) == 0) return;
    size_t data_len = rw_buffer_data_len(buffer);
    memmove((uint8_t *) buffer->read.ptr, rw_buffer_read_ptr(buffer), data_len);
    rw_buffer_seek_read_set(buffer, 0);
    rw_buffer_seek_write_set(buffer, data_len);
}
