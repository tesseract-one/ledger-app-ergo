#pragma once

#include <stdint.h>   // uint*_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool

#define BUFFER_FROM_ARRAY_FULL(name, array, size) \
    buffer_t name;                                \
    buffer_init(&name, array, size, size)

#define BUFFER_FROM_ARRAY_EMPTY(name, array, size) \
    buffer_t name;                                 \
    buffer_init(&name, array, size, 0)

#define BUFFER_FROM_VAR_FULL(name, var) \
    buffer_t name;                      \
    buffer_init(&name, &var, sizeof(var), sizeof(var))

#define BUFFER_FROM_VAR_EMPTY(name, var) \
    buffer_t name;                       \
    buffer_init(&name, &var, sizeof(var), 0)

#define BUFFER_NEW_LOCAL_EMPTY(name, size) \
    uint8_t __##name[size];                \
    buffer_t name;                         \
    buffer_init(&name, __##name, size, 0)

/**
 * Enumeration for endianness.
 */
typedef enum {
    BE,  /// Big Endian
    LE   /// Little Endian
} endianness_t;

/**
 * Struct for buffer with size and read+write offset.
 */
typedef struct {
    uint8_t *ptr;         /// Pointer to byte buffer
    size_t size;          /// Size of byte buffer
    size_t read_offset;   /// Read offset in byte buffer
    size_t write_offset;  /// Write offset in byte buffer
} buffer_t;

/**
 * Initialize buffer.
 *
 * @param[out] buffer
 *   Pointer to input buffer struct.
 * @param[in] ptr
 *   Pointer to byte buffert.
 * @param[in] buf_size
 *   Size of the buffer.
 * @param[in] data_size
 *   Data size of the buffer.
 *
 */
static inline void buffer_init(buffer_t *buffer, uint8_t *ptr, size_t buf_size, size_t data_size) {
    buffer->ptr = ptr;
    buffer->size = buf_size;
    buffer->read_offset = 0;
    buffer->write_offset = data_size;
}

/**
 * Set buffer read and write pointers to zero.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline void buffer_empty(buffer_t *buffer) {
    buffer->read_offset = 0;
    buffer->write_offset = 0;
}

/**
 * Return read pointer to the start of the data.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline uint8_t *buffer_read_ptr(const buffer_t *buffer) {
    return buffer->ptr + buffer->read_offset;
}

/**
 * Return write pointer to the start of the empty space.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline uint8_t *buffer_write_ptr(const buffer_t *buffer) {
    return buffer->ptr + buffer->write_offset;
}

/**
 * Tell whether buffer can read bytes or not.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 * @param[in] n
 *   Number of bytes to read in buffer.
 *
 * @return true if success, false otherwise.
 *
 */
static inline bool buffer_can_read(const buffer_t *buffer, size_t n) {
    return (buffer->write_offset - buffer->read_offset) >= n;
}

/**
 * Tell whether buffer can write bytes or not.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 * @param[in] n
 *   Number of bytes to write in buffer.
 *
 * @return true if success, false otherwise.
 *
 */
static inline bool buffer_can_write(const buffer_t *buffer, size_t n) {
    return (buffer->size - buffer->write_offset) >= n;
}

/**
 * Tell whether buffer can write bytes or not.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 * @return length of the empty space in buffer.
 *
 */
static inline size_t buffer_empty_space_len(const buffer_t *buffer) {
    return buffer->size - buffer->write_offset;
}

/**
 * Tell whether buffer can write bytes or not.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 * @return length of the data in buffer.
 *
 */
static inline size_t buffer_data_len(const buffer_t *buffer) {
    return buffer->write_offset - buffer->read_offset;
}

/**
 * Get current read position of the buffer.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 * @return current read position in the buffer.
 *
 */
static inline size_t buffer_read_position(const buffer_t *buffer) {
    return buffer->read_offset;
}

/**
 * Get current write position of the buffer.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 * @return current write position in the buffer.
 *
 */
static inline size_t buffer_write_position(const buffer_t *buffer) {
    return buffer->write_offset;
}

/**
 * Seek the buffer read position to specific offset.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Specific offset to seek.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_seek_read_set(buffer_t *buffer, size_t offset);

/**
 * Seek the buffer read position relatively to current offset.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Offset to seek relatively to `buffer->read_offset`.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_seek_read_cur(buffer_t *buffer, size_t offset);

/**
 * Seek the buffer read position relatively to the end of the data.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Offset to seek relatively to `buffer->write_offset`.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_seek_read_end(buffer_t *buffer, size_t offset);

/**
 * Seek the buffer write position to specific offset.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Specific offset to seek.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_seek_write_set(buffer_t *buffer, size_t offset);

/**
 * Seek the buffer write position relatively to current offset.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Offset to seek relatively to `buffer->write_offset`.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_seek_write_cur(buffer_t *buffer, size_t offset);

/**
 * Seek the buffer write position relatively to the buffer end.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Offset to seek relatively to `buffer->size`.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_seek_write_end(buffer_t *buffer, size_t offset);

/**
 * Read 1 byte from buffer into uint8_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     value
 *   Pointer to 8-bit unsigned integer read from buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_u8(buffer_t *buffer, uint8_t *value);

/**
 * Read 2 bytes from buffer into uint16_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     value
 *   Pointer to 16-bit unsigned integer read from buffer.
 * @param[in]      endianness
 *   Either BE (Big Endian) or LE (Little Endian).
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_u16(buffer_t *buffer, uint16_t *value, endianness_t endianness);

/**
 * Read 4 bytes from buffer into uint32_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     value
 *   Pointer to 32-bit unsigned integer read from buffer.
 * @param[in]      endianness
 *   Either BE (Big Endian) or LE (Little Endian).
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_u32(buffer_t *buffer, uint32_t *value, endianness_t endianness);

/**
 * Read 8 bytes from buffer into uint64_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     value
 *   Pointer to 64-bit unsigned integer read from buffer.
 * @param[in]      endianness
 *   Either BE (Big Endian) or LE (Little Endian).
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_u64(buffer_t *buffer, uint64_t *value, endianness_t endianness);

/**
 * Read BIP32 path from buffer.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output 32-bit integer buffer.
 * @param[in]      out_len
 *   Number of BIP32 paths read in the output buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_bip32_path(buffer_t *buffer, uint32_t *out, size_t out_len);

/**
 * Move bytes from buffer.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output byte buffer.
 * @param[in]      out_len
 *   Length of output byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_read_bytes(buffer_t *buffer, uint8_t *out, size_t out_len);

/**
 * Copy bytes from buffer without its modification.
 *
 * @param[in]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output byte buffer.
 * @param[in]      out_len
 *   Length of output byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_copy_bytes(const buffer_t *buffer, uint8_t *out, size_t out_len);

/**
 * Write 1 byte to buffer from uint8_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[in]     value
 *   8-bit unsigned integer to write into buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_write_u8(buffer_t *buffer, uint8_t value);

/**
 * Write 2 bytes to buffer from uint16_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[in]     value
 *   16-bit unsigned integer to write into buffer.
 * @param[in]      endianness
 *   Either BE (Big Endian) or LE (Little Endian).
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_write_u16(buffer_t *buffer, uint16_t value, endianness_t endianness);

/**
 * Write 4 bytes to buffer from uint32_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[in]     value
 *   32-bit unsigned integer to write into buffer.
 * @param[in]      endianness
 *   Either BE (Big Endian) or LE (Little Endian).
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_write_u32(buffer_t *buffer, uint32_t value, endianness_t endianness);

/**
 * Write 8 bytes to buffer from uint64_t.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[in]     value
 *   64-bit unsigned integer to write into buffer.
 * @param[in]      endianness
 *   Either BE (Big Endian) or LE (Little Endian).
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_write_u64(buffer_t *buffer, uint64_t value, endianness_t endianness);

/**
 * Write bytes to buffer.
 *
 * @param[in]  buffer
 *   Pointer to input buffer struct.
 * @param[out] from
 *   Pointer to input byte buffer.
 * @param[in]  from_len
 *   Length of input byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
bool buffer_write_bytes(buffer_t *buffer, const uint8_t *from, size_t from_len);

/**
 * Move all data to the start of the buffer.
 *
 * @param[in]  buffer
 *   Pointer to input buffer struct.
 *
 */
void buffer_shift_data(buffer_t *buffer);
