#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "ergo/tx_ser_box.h"
#include "common/rwbuffer.h"
#include "macro_helpers.h"

#define ERGO_TX_SERIALIZER_BOX_INIT(name)                         \
    ergo_tx_serializer_box_context_t name;                        \
    uint64_t value = 12345;                                       \
    uint32_t ergo_tree_size = 2;                                  \
    uint32_t creation_height = 3;                                 \
    uint8_t tokens_count = 2;                                     \
    uint32_t registers_size = 2;                                  \
    cx_blake2b_t hash;                                            \
    assert_true(ergo_tx_serializer_box_id_hash_init(&hash));      \
    assert_int_equal(ergo_tx_serializer_box_init(&name,           \
                                                 value,           \
                                                 ergo_tree_size,  \
                                                 creation_height, \
                                                 tokens_count,    \
                                                 registers_size,  \
                                                 &hash),          \
                     ERGO_TX_SERIALIZER_BOX_RES_OK);

#define VERIFY_HASH(hash, expected)                        \
    uint8_t *data;                                         \
    size_t data_len;                                       \
    _cx_blake2b_get_data(hash, &data, &data_len);          \
    assert_int_equal(data_len, sizeof(expected));          \
    assert_memory_equal(data, expected, sizeof(expected)); \
    _cx_blake2b_free_data(hash);

static void test_ergo_tx_serializer_box_init(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    assert_int_equal(context.ergo_tree_size, ergo_tree_size);
    assert_int_equal(context.creation_height, creation_height);
    assert_int_equal(context.tokens_count, tokens_count);
    assert_int_equal(context.registers_size, registers_size);
    uint8_t expected_hash[2] = {0xb9, 0x60};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.value, value);
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_TREE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED);
}

static void test_ergo_tx_serializer_box_init_too_many_tokens(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = 2;
    uint32_t creation_height = 3;
    uint8_t tokens_count = TOKEN_MAX_COUNT + 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    assert_int_equal(ergo_tx_serializer_box_init(&context,
                                                 value,
                                                 ergo_tree_size,
                                                 creation_height,
                                                 tokens_count,
                                                 registers_size,
                                                 &hash),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_init_too_much_data_ergo_tree_size(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = MAX_TX_DATA_PART_LEN + 1;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    assert_int_equal(ergo_tx_serializer_box_init(&context,
                                                 value,
                                                 ergo_tree_size,
                                                 creation_height,
                                                 tokens_count,
                                                 registers_size,
                                                 &hash),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_init_too_much_data_registers_size(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = 2;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = MAX_TX_DATA_PART_LEN + 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    assert_int_equal(ergo_tx_serializer_box_init(&context,
                                                 value,
                                                 ergo_tree_size,
                                                 creation_height,
                                                 tokens_count,
                                                 registers_size,
                                                 &hash),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_init_bad_hash(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = 2;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    assert_int_equal(ergo_tx_serializer_box_init(&context,
                                                 value,
                                                 ergo_tree_size,
                                                 creation_height,
                                                 tokens_count,
                                                 registers_size,
                                                 &hash),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    assert_int_equal(ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);
    uint8_t expected_hash[6] = {0xb9, 0x60, 0x01, 0x02, 0x03, 0x02};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_TREE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_tree_bad_state(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    memset(&context, 0, sizeof(ergo_tx_serializer_box_context_t));
    context.state = ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED;
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    assert_int_equal(ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree_too_much_data(void **state) {
    (void) state;

    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = sizeof(tree_chunk_array) - 1;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    ergo_tx_serializer_box_init(&context,
                                value,
                                ergo_tree_size,
                                creation_height,
                                tokens_count,
                                registers_size,
                                &hash);
    assert_int_equal(ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree_bad_hash(void **state) {
    (void) state;

    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = 2;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    ergo_tx_serializer_box_init(&context,
                                value,
                                ergo_tree_size,
                                creation_height,
                                tokens_count,
                                registers_size,
                                &hash);
    memset(context.hash, 0, sizeof(*context.hash));
    assert_int_equal(ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree_more_data(void **state) {
    (void) state;

    uint8_t tree_chunk_array[MAX_DATA_CHUNK_LEN] = {0};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = MAX_DATA_CHUNK_LEN + 1;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    ergo_tx_serializer_box_init(&context,
                                value,
                                ergo_tree_size,
                                creation_height,
                                tokens_count,
                                registers_size,
                                &hash);
    assert_int_equal(ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
                     ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA);
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_TREE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_INITIALIZED);
}

static void test_ergo_tx_serializer_box_add_miners_fee_tree_mainnet(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    bool is_mainnet = true;
    assert_int_equal(ergo_tx_serializer_box_add_miners_fee_tree(&context, is_mainnet),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);
    uint8_t expected_hash[] = {
        0xb9, 0x60, 0x10, 0x05, 0x04, 0x00, 0x04, 0x00, 0x0e, 0x36, 0x10, 0x02, 0x04, 0xa0,
        0x0b, 0x08, 0xcd, 0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0,
        0x62, 0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98, 0xea, 0x02, 0xd1, 0x92, 0xa3, 0x9a,
        0x8c, 0xc7, 0xa7, 0x01, 0x73, 0x00, 0x73, 0x01, 0x10, 0x01, 0x02, 0x04, 0x02, 0xd1,
        0x96, 0x83, 0x03, 0x01, 0x93, 0xa3, 0x8c, 0xc7, 0xb2, 0xa5, 0x73, 0x00, 0x00, 0x01,
        0x93, 0xc2, 0xb2, 0xa5, 0x73, 0x01, 0x00, 0x74, 0x73, 0x02, 0x73, 0x03, 0x83, 0x01,
        0x08, 0xcd, 0xee, 0xac, 0x93, 0xb1, 0xa5, 0x73, 0x04, 0x03, 0x02};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_FEE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_miners_fee_tree_testnet(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    bool is_mainnet = false;
    assert_int_equal(ergo_tx_serializer_box_add_miners_fee_tree(&context, is_mainnet),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);
    uint8_t expected_hash[] = {
        0xb9, 0x60, 0x10, 0x05, 0x04, 0x00, 0x04, 0x00, 0x0e, 0x36, 0x10, 0x02, 0x04, 0xa0,
        0x0b, 0x08, 0xcd, 0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0,
        0x62, 0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98, 0xea, 0x02, 0xd1, 0x92, 0xa3, 0x9a,
        0x8c, 0xc7, 0xa7, 0x01, 0x73, 0x00, 0x73, 0x01, 0x10, 0x01, 0x02, 0x04, 0x02, 0xd1,
        0x96, 0x83, 0x03, 0x01, 0x93, 0xa3, 0x8c, 0xc7, 0xb2, 0xa5, 0x73, 0x00, 0x00, 0x01,
        0x93, 0xc2, 0xb2, 0xa5, 0x73, 0x01, 0x00, 0x74, 0x73, 0x02, 0x73, 0x03, 0x83, 0x01,
        0x08, 0xcd, 0xee, 0xac, 0x93, 0xb1, 0xa5, 0x73, 0x04, 0x03, 0x02};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_FEE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_miners_fee_tree_bad_state(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    bool is_mainnet = true;
    assert_int_equal(ergo_tx_serializer_box_add_miners_fee_tree(&context, is_mainnet),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_miners_fee_tree_bad_hash(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    bool is_mainnet = true;
    memset(context.hash, 0, sizeof(*context.hash));
    assert_int_equal(ergo_tx_serializer_box_add_miners_fee_tree(&context, is_mainnet),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_change_tree(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    const uint8_t raw_public_key[] = {
        0x04, 0x8b, 0x9f, 0xf8, 0x5d, 0xdd, 0x9f, 0x1e, 0x22, 0x88, 0xfc, 0x53, 0x9d,
        0x39, 0xc7, 0xc4, 0xee, 0xb7, 0xa5, 0x56, 0xf4, 0xd8, 0x11, 0xcb, 0x73, 0x99,
        0x64, 0x18, 0xde, 0x5a, 0xbd, 0xcb, 0x2a, 0xfa, 0x2d, 0x53, 0x17, 0x16, 0x0a,
        0x59, 0x50, 0x0f, 0x5d, 0x31, 0xfa, 0xe8, 0x6b, 0xce, 0xe9, 0xab, 0x1a, 0x60,
        0x53, 0xa1, 0x1d, 0x53, 0x5d, 0x2d, 0x04, 0x3c, 0xe5, 0xcf, 0xf1, 0x0a, 0xe7};
    assert_int_equal(ergo_tx_serializer_box_add_change_tree(&context, raw_public_key),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);
    uint8_t expected_hash[] = {0xb9, 0x60, 0x00, 0x08, 0xcd, 0x03, 0x8b, 0x9f, 0xf8, 0x5d,
                               0xdd, 0x9f, 0x1e, 0x22, 0x88, 0xfc, 0x53, 0x9d, 0x39, 0xc7,
                               0xc4, 0xee, 0xb7, 0xa5, 0x56, 0xf4, 0xd8, 0x11, 0xcb, 0x73,
                               0x99, 0x64, 0x18, 0xde, 0x5a, 0xbd, 0xcb, 0x2a, 0x03, 0x02};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_change_tree_bad_state(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    memset(&context, 0, sizeof(ergo_tx_serializer_box_context_t));
    context.state = ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED;
    const uint8_t raw_public_key[PUBLIC_KEY_LEN] = {0};
    assert_int_equal(ergo_tx_serializer_box_add_change_tree(&context, raw_public_key),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_change_tree_bad_hash(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    const uint8_t raw_public_key[PUBLIC_KEY_LEN] = {0};
    memset(context.hash, 0, sizeof(*context.hash));
    assert_int_equal(ergo_tx_serializer_box_add_change_tree(&context, raw_public_key),
                     ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tokens(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);

    uint8_t table_tokens_array[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(table_tokens, table_tokens_array, sizeof(table_tokens_array));
    token_table_t table = {0};
    ergo_tx_serializer_table_context_t table_ctx;
    assert_int_equal(
        ergo_tx_serializer_table_init(&table_ctx, sizeof(table_tokens_array) / ERGO_ID_LEN, &table),
        ERGO_TX_SERIALIZER_TABLE_RES_OK);
    assert_int_equal(ergo_tx_serializer_table_add(&table_ctx, &table_tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_OK);

    uint8_t tree_chunk_array[] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_add_tree(&context, &tree_chunk);
    uint8_t tokens_array[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_box_add_tokens(&context, &tokens, &table_ctx),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);

    uint8_t expected_hash[] = {0xb9, 0x60, 0x01, 0x02, 0x03, 0x02, 0x00, 0x81, 0x82,
                               0x84, 0x88, 0x90, 0xa0, 0xc0, 0x80, 0x01, 0x01, 0x82,
                               0x84, 0x88, 0x90, 0xa0, 0xc0, 0x80, 0x81, 0x02};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED);
}

static void test_ergo_tx_serializer_box_add_registers(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);

    token_table_t table = {0};
    ergo_tx_serializer_table_context_t table_ctx;
    uint8_t distinct_tokens_array[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(distinct_tokens, distinct_tokens_array, sizeof(distinct_tokens_array));
    ergo_tx_serializer_table_init(&table_ctx, sizeof(distinct_tokens_array) / ERGO_ID_LEN, &table);
    ergo_tx_serializer_table_add(&table_ctx, &distinct_tokens);

    uint8_t tree_chunk_array[] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_add_tree(&context, &tree_chunk);
    uint8_t tokens_array[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    ergo_tx_serializer_box_add_tokens(&context, &tokens, &table_ctx);

    uint8_t rc_array[] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(registers_chunk, rc_array, sizeof(rc_array));
    assert_int_equal(ergo_tx_serializer_box_add_registers(&context, &registers_chunk),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);
    uint8_t expected_hash[] = {0xb9, 0x60, 0x01, 0x02, 0x03, 0x02, 0x00, 0x81, 0x82, 0x84,
                               0x88, 0x90, 0xa0, 0xc0, 0x80, 0x01, 0x01, 0x82, 0x84, 0x88,
                               0x90, 0xa0, 0xc0, 0x80, 0x81, 0x02, 0x01, 0x02};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_FINISHED);
}

static void test_ergo_tx_serializer_box_id_hash(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);

    token_table_t table = {0};
    ergo_tx_serializer_table_context_t table_ctx;
    uint8_t distinct_tokens_array[] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(distinct_tokens, distinct_tokens_array, sizeof(distinct_tokens_array));
    ergo_tx_serializer_table_init(&table_ctx, sizeof(distinct_tokens_array) / ERGO_ID_LEN, &table);
    ergo_tx_serializer_table_add(&table_ctx, &distinct_tokens);

    uint8_t tree_chunk_array[] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_add_tree(&context, &tree_chunk);
    uint8_t tokens_array[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    ergo_tx_serializer_box_add_tokens(&context, &tokens, &table_ctx);

    uint8_t rc_array[] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(registers_chunk, rc_array, sizeof(rc_array));
    ergo_tx_serializer_box_add_registers(&context, &registers_chunk);
    const uint8_t tx_id[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                             0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint16_t box_index = 1;
    uint8_t box_id[ERGO_ID_LEN];
    assert_int_equal(ergo_tx_serializer_box_id_hash(&context, tx_id, box_index, box_id),
                     ERGO_TX_SERIALIZER_BOX_RES_OK);
    uint8_t expected_box_id[] = {0xf1, 0xca, 0x1e, 0x06, 0x0a, 0xa1, 0x1f, 0x98, 0x9b, 0x3d, 0x70,
                                 0xec, 0x0e, 0x0c, 0x98, 0x7c, 0x95, 0x2e, 0x23, 0x89, 0xbe, 0x5b,
                                 0x82, 0x0b, 0xc7, 0xdb, 0xfc, 0x32, 0x6f, 0x86, 0x13, 0x73};
    assert_memory_equal(box_id, expected_box_id, ERGO_ID_LEN);
    uint8_t expected_hash[] = {0xb9, 0x60, 0x01, 0x02, 0x03, 0x02, 0x00, 0x81, 0x82, 0x84, 0x88,
                               0x90, 0xa0, 0xc0, 0x80, 0x01, 0x01, 0x82, 0x84, 0x88, 0x90, 0xa0,
                               0xc0, 0x80, 0x81, 0x02, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_HASH_FINALIZED);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ergo_tx_serializer_box_init),
        cmocka_unit_test(test_ergo_tx_serializer_box_init_too_many_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_box_init_too_much_data_ergo_tree_size),
        cmocka_unit_test(test_ergo_tx_serializer_box_init_too_much_data_registers_size),
        cmocka_unit_test(test_ergo_tx_serializer_box_init_bad_hash),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_tree),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_bad_state),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_too_much_data),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_bad_hash),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_more_data),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_miners_fee_tree_mainnet),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_miners_fee_tree_testnet),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_miners_fee_tree_bad_state),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_miners_fee_tree_bad_hash),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_change_tree),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_change_tree_bad_state),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_change_tree_bad_hash),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_box_add_registers),
        cmocka_unit_test(test_ergo_tx_serializer_box_id_hash)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
