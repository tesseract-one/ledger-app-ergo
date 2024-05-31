#pragma once

#include <buffer.h>

/**
 * Initialize buffer.
 *
 * @param[out] buffer
 *   Pointer to input buffer struct.
 * @param[in] ptr
 *   Pointer to byte buffert.
 * @param[in] data_size
 *   Data size of the buffer.
 *
 */
static inline void buffer_init(buffer_t *buffer, const uint8_t *ptr, size_t data_size) {
    buffer->ptr = ptr;
    buffer->offset = 0;
    buffer->size = data_size;
}

/**
 * Return read pointer to the start of the data.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline const uint8_t *buffer_read_ptr(const buffer_t *buffer) {
    return buffer->ptr + buffer->offset;
}

/**
 * Tell whether buffer has bytes to read or not.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 * @return length of the data in buffer.
 *
 */
static inline size_t buffer_data_len(const buffer_t *buffer) {
    return buffer->size - buffer->offset;
}

/**
 * Copy bytes from buffer without its modification.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output byte buffer.
 * @param[in]      count
 *   Amount of bytes to copy.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_copy_bytes(const buffer_t *buffer, uint8_t *out, size_t count);

/**
 * Read bytes from buffer.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output byte buffer.
 * @param[in]      count
 *   Amount of bytes to copy.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_bytes(buffer_t *buffer, uint8_t *out, size_t count);