#pragma once

#include <stdint.h>
#include <stddef.h>

typedef int cx_err_t;

typedef enum cx_md_e { CX_BLAKE2B = 9 } cx_md_t;

/** Convenience type. See #cx_hash_info_t. */
typedef struct cx_hash_header_s cx_hash_t;

/* Generic API */
struct cx_hash_header_s {
    cx_md_t md_type;
    size_t output_size;
};

struct cx_blake2b_s {
    struct cx_hash_header_s info;
    void *ctx;
};
/** Convenience type. See #cx_blake2b_s. */
typedef struct cx_blake2b_s cx_blake2b_t;

#define CX_OK 0

#define CX_BLAKE2B_256_SIZE 32

#define CX_FLAG
/*
 * Bit 0
 */
#define CX_LAST (1 << 0)
/*
 * Bit 15
 */
#define CX_NO_REINIT (1 << 15)

cx_err_t cx_blake2b_init_no_throw(cx_blake2b_t *hash, size_t out_len);
cx_err_t cx_hash_no_throw(cx_hash_t *hash,
                          uint32_t mode,
                          const uint8_t *in,
                          size_t len,
                          uint8_t *out,
                          size_t out_len);

cx_err_t cx_blake2b_256_hash(const uint8_t* data,
                             size_t len,
                             uint8_t out[static CX_BLAKE2B_256_SIZE]);

void _cx_blake2b_get_data(cx_blake2b_t *ctx, uint8_t **data, size_t *len);
void _cx_blake2b_free_data(cx_blake2b_t *ctx);