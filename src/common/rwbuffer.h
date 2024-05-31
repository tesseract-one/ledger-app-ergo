#pragma once

#include "buffer_ext.h"

#define RW_BUFFER_FROM_ARRAY_FULL(_name, _array, _size) \
    rw_buffer_t _name;                                  \
    rw_buffer_init(&_name, _array, _size, _size)

#define RW_BUFFER_FROM_ARRAY_EMPTY(_name, _array, _size) \
    rw_buffer_t _name;                                   \
    rw_buffer_init(&_name, _array, _size, 0)

#define RW_BUFFER_FROM_VAR_FULL(_name, _var) \
    rw_buffer_t _name;                       \
    rw_buffer_init(&_name, &_var, sizeof(_var), sizeof(_var))

#define RW_BUFFER_FROM_VAR_EMPTY(_name, _var) \
    rw_buffer_t _name;                        \
    rw_buffer_init(&_name, &_var, sizeof(_var), 0)

#define RW_BUFFER_NEW_LOCAL_EMPTY(_name, _size) \
    uint8_t __##_name[_size];                   \
    rw_buffer_t _name;                          \
    rw_buffer_init(&_name, __##_name, _size, 0)

/**
 * Struct for buffer with size and read+write offset.
 */
typedef struct {
    buffer_t read;  /// read buffer. size is a write offset.
    size_t size;    /// full allocated size of the buffer
} rw_buffer_t;

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
static inline void rw_buffer_init(rw_buffer_t *buffer,
                                  uint8_t *ptr,
                                  size_t buf_size,
                                  size_t data_size) {
    buffer_init(&buffer->read, ptr, data_size);
    buffer->size = buf_size;
}

/**
 * Set buffer read and write pointers to zero.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline void rw_buffer_empty(rw_buffer_t *buffer) {
    buffer->read.offset = 0;
    buffer->read.size = 0;
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
static inline size_t rw_buffer_data_len(const rw_buffer_t *buffer) {
    return buffer_data_len(&buffer->read);
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
static inline size_t rw_buffer_empty_space_len(const rw_buffer_t *buffer) {
    return buffer->size - buffer->read.size;
}

/**
 * Return read pointer to the start of the data.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline const uint8_t *rw_buffer_read_ptr(const rw_buffer_t *buffer) {
    return buffer_read_ptr(&buffer->read);
}

/**
 * Return write pointer to the start of the empty space.
 *
 * @param[in] buffer
 *   Pointer to input buffer struct.
 *
 */
static inline uint8_t *rw_buffer_write_ptr(rw_buffer_t *buffer) {
    return ((uint8_t *) buffer->read.ptr) + buffer->read.size;
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
static inline bool rw_buffer_can_read(const rw_buffer_t *buffer, size_t n) {
    return buffer_can_read(&buffer->read, n);
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
static inline bool rw_buffer_can_write(const rw_buffer_t *buffer, size_t n) {
    return rw_buffer_empty_space_len(buffer) >= n;
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
static inline size_t rw_buffer_read_position(const rw_buffer_t *buffer) {
    return buffer->read.offset;
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
static inline size_t rw_buffer_write_position(const rw_buffer_t *buffer) {
    return buffer->read.size;
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
static inline bool rw_buffer_seek_read_set(rw_buffer_t *buffer, size_t offset) {
    return buffer_seek_set(&buffer->read, offset);
}

/**
 * Seek the buffer read position relatively to current offset.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Offset to seek relatively to `buffer->read.offset`.
 *
 * @return true if success, false otherwise.
 *
 */
static inline bool rw_buffer_seek_read_cur(rw_buffer_t *buffer, size_t offset) {
    return buffer_seek_cur(&buffer->read, offset);
}

/**
 * Seek the buffer read position relatively to the end of the data.
 *
 * @param[in,out] buffer
 *   Pointer to input buffer struct.
 * @param[in]     offset
 *   Offset to seek relatively to `buffer->read.size`.
 *
 * @return true if success, false otherwise.
 *
 */
static inline bool rw_buffer_seek_read_end(rw_buffer_t *buffer, size_t offset) {
    return buffer_seek_end(&buffer->read, offset);
}

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
bool rw_buffer_seek_write_set(rw_buffer_t *buffer, size_t offset);

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
bool rw_buffer_seek_write_cur(rw_buffer_t *buffer, size_t offset);

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
bool rw_buffer_seek_write_end(rw_buffer_t *buffer, size_t offset);

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
static inline bool rw_buffer_read_u8(rw_buffer_t *buffer, uint8_t *value) {
    return buffer_read_u8(&buffer->read, value);
}

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
static inline bool rw_buffer_read_u16(rw_buffer_t *buffer,
                                      uint16_t *value,
                                      endianness_t endianness) {
    return buffer_read_u16(&buffer->read, value, endianness);
}

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
static inline bool rw_buffer_read_u32(rw_buffer_t *buffer,
                                      uint32_t *value,
                                      endianness_t endianness) {
    return buffer_read_u32(&buffer->read, value, endianness);
}

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
static inline bool rw_buffer_read_u64(rw_buffer_t *buffer,
                                      uint64_t *value,
                                      endianness_t endianness) {
    return buffer_read_u64(&buffer->read, value, endianness);
}

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
static inline bool rw_buffer_read_bip32_path(rw_buffer_t *buffer, uint32_t *out, size_t out_len) {
    return buffer_read_bip32_path(&buffer->read, out, out_len);
}

/**
 * Move bytes from buffer.
 *
 * @param[in,out]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output byte buffer.
 * @param[in]      count
 *   Amount bytes to read.
 *
 * @return true if success, false otherwise.
 *
 */
static inline bool rw_buffer_read_bytes(rw_buffer_t *buffer, uint8_t *out, size_t count) {
    return buffer_read_bytes(&buffer->read, out, count);
}

/**
 * Copy bytes from buffer without its modification.
 *
 * @param[in]  buffer
 *   Pointer to input buffer struct.
 * @param[out]     out
 *   Pointer to output byte buffer.
 * @param[in]      count
 *   Length of output byte buffer.
 *
 * @return true if success, false otherwise.
 *
 */
static inline bool rw_buffer_copy_bytes(const rw_buffer_t *buffer, uint8_t *out, size_t count) {
    return buffer_copy_bytes(&buffer->read, out, count);
}

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
bool rw_buffer_write_u8(rw_buffer_t *buffer, uint8_t value);

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
bool rw_buffer_write_u16(rw_buffer_t *buffer, uint16_t value, endianness_t endianness);

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
bool rw_buffer_write_u32(rw_buffer_t *buffer, uint32_t value, endianness_t endianness);

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
bool rw_buffer_write_u64(rw_buffer_t *buffer, uint64_t value, endianness_t endianness);

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
bool rw_buffer_write_bytes(rw_buffer_t *buffer, const uint8_t *from, size_t from_len);

/**
 * Move all data to the start of the buffer.
 *
 * @param[in]  buffer
 *   Pointer to input buffer struct.
 *
 */
void rw_buffer_shift_data(rw_buffer_t *buffer);
