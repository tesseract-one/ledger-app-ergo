#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <string.h>   // memmove

#include "buffer_ext.h"

bool buffer_copy_bytes(const buffer_t *buffer, uint8_t *out, size_t count) {
    if (!buffer_can_read(buffer, count)) return false;
    memmove(out, buffer_read_ptr(buffer), count);
    return true;
}

bool buffer_read_bytes(buffer_t *buffer, uint8_t *out, size_t count) {
    if (!buffer_copy_bytes(buffer, out, count)) {
        return false;
    }
    return buffer_seek_cur(buffer, count);
}