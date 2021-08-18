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

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include "buffer.h"
#include "read.h"
#include "write.h"
#include "bip32.h"

bool buffer_seek_read_set(buffer_t *buffer, size_t offset) {
    if (offset > buffer->write_offset) {
        return false;
    }

    buffer->read_offset = offset;

    return true;
}

bool buffer_seek_read_cur(buffer_t *buffer, size_t offset) {
    if (buffer->read_offset + offset < buffer->read_offset ||  // overflow
        buffer->read_offset + offset > buffer->write_offset) {    // exceed buffer size
        return false;
    }

    buffer->read_offset += offset;

    return true;
}

bool buffer_seek_read_end(buffer_t *buffer, size_t offset) {
    if (offset > buffer->write_offset) {
        return false;
    }

    buffer->read_offset = buffer->write_offset - offset;

    return true;
}

bool buffer_seek_write_set(buffer_t *buffer, size_t offset) {
    if (offset > buffer->size) {
        return false;
    }

    buffer->write_offset = offset;

    return true;
}

bool buffer_seek_write_cur(buffer_t *buffer, size_t offset) {
    if (buffer->write_offset + offset < buffer->write_offset ||  // overflow
        buffer->write_offset + offset > buffer->size) {    // exceed buffer size
        return false;
    }

    buffer->write_offset += offset;

    return true;
}

bool buffer_seek_write_end(buffer_t *buffer, size_t offset) {
    if (offset > buffer->size) {
        return false;
    }

    buffer->write_offset = buffer->size - offset;

    return true;
}

bool buffer_read_u8(buffer_t *buffer, uint8_t *value) {
    if (!buffer_can_read(buffer, 1)) {
        *value = 0;

        return false;
    }

    *value = buffer->ptr[buffer->read_offset];
    buffer_seek_read_cur(buffer, 1);

    return true;
}

bool buffer_read_u16(buffer_t *buffer, uint16_t *value, endianness_t endianness) {
    if (!buffer_can_read(buffer, 2)) {
        *value = 0;

        return false;
    }

    *value = ((endianness == BE) ? read_u16_be(buffer->ptr, buffer->read_offset)
                                 : read_u16_le(buffer->ptr, buffer->read_offset));

    buffer_seek_read_cur(buffer, 2);

    return true;
}

bool buffer_read_u32(buffer_t *buffer, uint32_t *value, endianness_t endianness) {
    if (!buffer_can_read(buffer, 4)) {
        *value = 0;

        return false;
    }

    *value = ((endianness == BE) ? read_u32_be(buffer->ptr, buffer->read_offset)
                                 : read_u32_le(buffer->ptr, buffer->read_offset));

    buffer_seek_read_cur(buffer, 4);

    return true;
}

bool buffer_read_u64(buffer_t *buffer, uint64_t *value, endianness_t endianness) {
    if (!buffer_can_read(buffer, 8)) {
        *value = 0;

        return false;
    }

    *value = ((endianness == BE) ? read_u64_be(buffer->ptr, buffer->read_offset)
                                 : read_u64_le(buffer->ptr, buffer->read_offset));

    buffer_seek_read_cur(buffer, 8);

    return true;
}

bool buffer_read_bip32_path(buffer_t *buffer, uint32_t *out, size_t out_len) {
    if (!bip32_path_read(buffer->ptr + buffer->read_offset,
                         buffer->write_offset - buffer->read_offset,
                         out,
                         out_len)) {
        return false;
    }

    buffer_seek_read_cur(buffer, sizeof(*out) * out_len);

    return true;
}

bool buffer_write_u8(buffer_t *buffer, uint8_t value) {
    if (!buffer_can_write(buffer, 1)) {
        return false;
    }

    buffer->ptr[buffer->write_offset] = value;
    buffer_seek_write_cur(buffer, 1);

    return true;
}

bool buffer_write_u16(buffer_t *buffer, uint16_t value, endianness_t endianness) {
    if (!buffer_can_write(buffer, 2)) {
        return false;
    }

    if (endianness == BE) {
        write_u16_be(buffer->ptr, buffer->write_offset, value);
    } else {
        write_u16_le(buffer->ptr, buffer->write_offset, value);
    }

    buffer_seek_write_cur(buffer, 2);

    return true;
}

bool buffer_write_u32(buffer_t *buffer, uint32_t value, endianness_t endianness) {
    if (!buffer_can_write(buffer, 4)) {
        return false;
    }

    if (endianness == BE) {
        write_u32_be(buffer->ptr, buffer->write_offset, value);
    } else {
        write_u32_le(buffer->ptr, buffer->write_offset, value);
    }

    buffer_seek_write_cur(buffer, 4);

    return true;
}

bool buffer_write_u64(buffer_t *buffer, uint64_t value, endianness_t endianness) {
    if (!buffer_can_write(buffer, 8)) {
        return false;
    }

    if (endianness == BE) {
        write_u64_be(buffer->ptr, buffer->write_offset, value);
    } else {
        write_u64_le(buffer->ptr, buffer->write_offset, value);
    }

    buffer_seek_write_cur(buffer, 8);

    return true;
}


bool buffer_read_bytes(buffer_t *buffer, uint8_t *out, size_t out_len) {
    if (buffer_data_len(buffer) < out_len) {
        return false;
    }
    
    memmove(out, buffer->ptr + buffer->read_offset, out_len);

    buffer_seek_read_cur(buffer, out_len);

    return true;
}

bool buffer_copy_bytes(const buffer_t *buffer, uint8_t *out, size_t out_len) {
    if (buffer_data_len(buffer) > out_len) {
        return false;
    }
    memmove(out, buffer->ptr + buffer->read_offset, buffer_data_len(buffer));
    return true;
}

bool buffer_write_bytes(buffer_t *buffer, const uint8_t *from, size_t from_len) {
    if (!buffer_can_write(buffer, from_len)) {
        return false;
    }
    
    memcpy(buffer->ptr + buffer->write_offset, from, from_len);
    
    buffer_seek_write_cur(buffer, from_len);
    
    return true;
}

void buffer_shift_data(buffer_t *buffer) {
    if (buffer->read_offset == 0) return;
    size_t data_len = buffer_data_len(buffer);
    memmove(buffer->ptr, buffer->ptr + buffer->read_offset, data_len);
    buffer->write_offset = data_len;
    buffer->read_offset = 0;
}
