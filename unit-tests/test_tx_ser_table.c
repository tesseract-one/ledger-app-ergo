#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "ergo/tx_ser_table.h"
#include "common/rwbuffer.h"
#include "macro_helpers.h"

static void test_ergo_tx_serializer_table_init(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 1;
    token_table_t tokens_table = {
        0,
        {{
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        }}};
    assert_int_equal(ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table),
                     ERGO_TX_SERIALIZER_TABLE_RES_OK);
    assert_int_equal(context.distinct_tokens_count, tokens_count);
    assert_ptr_equal(context.tokens_table, &tokens_table);
    token_table_t empty = {0};
    assert_memory_equal(context.tokens_table, &empty, sizeof(empty));
}

static void test_ergo_tx_serializer_table_init_too_many_tokens(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = TOKEN_MAX_COUNT + 1;
    token_table_t tokens_table = {0};
    assert_int_equal(ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table),
                     ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS);
}

static void test_ergo_tx_serializer_table_add(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 2;
    token_table_t tokens_table = {0};
    ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table);
    uint8_t tokens_array[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_table_add(&context, &tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_OK);
    const token_table_t expected_tokens_table = {
        2,
        {{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
          0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
         {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
          0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
          0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}}};
    assert_memory_equal(context.tokens_table,
                        &expected_tokens_table,
                        sizeof(expected_tokens_table));
}

static void test_ergo_tx_serializer_table_add_too_many_tokens(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 1;
    token_table_t tokens_table = {0};
    ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table);
    uint8_t tokens_array[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_table_add(&context, &tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_ERR_TOO_MANY_TOKENS);
}

static void test_ergo_tx_serializer_table_add_bad_token_id(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 2;
    token_table_t tokens_table = {0};
    ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table);
    uint8_t tokens_array[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_table_add(&context, &tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_ERR_BAD_TOKEN_ID);
}

static void test_ergo_tx_serializer_table_add_more_data(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 2;
    token_table_t tokens_table = {0};
    ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_table_add(&context, &tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_MORE_DATA);
}

static void test_ergo_tx_serializer_table_hash(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 1;
    token_table_t tokens_table = {0};
    ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table);
    uint8_t tokens_array[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                              0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_table_add(&context, &tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_OK);
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    assert_int_equal(ergo_tx_serializer_table_hash(&context, &hash),
                     ERGO_TX_SERIALIZER_TABLE_RES_OK);
    uint8_t *data;
    size_t data_len;
    _cx_blake2b_get_data(&hash, &data, &data_len);
    uint8_t expected[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                          0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                          0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    assert_int_equal(data_len, sizeof(expected));
    assert_memory_equal(data, expected, sizeof(expected));
    _cx_blake2b_free_data(&hash);
}

static void test_ergo_tx_serializer_table_hash_bad_hash(void **state) {
    (void) state;

    ergo_tx_serializer_table_context_t context;
    uint8_t tokens_count = 1;
    token_table_t tokens_table = {0};
    ergo_tx_serializer_table_init(&context, tokens_count, &tokens_table);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_table_add(&context, &tokens),
                     ERGO_TX_SERIALIZER_TABLE_RES_OK);
    cx_blake2b_t hash = {0};
    assert_int_equal(ergo_tx_serializer_table_hash(&context, &hash),
                     ERGO_TX_SERIALIZER_TABLE_RES_ERR_HASHER);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ergo_tx_serializer_table_init),
        cmocka_unit_test(test_ergo_tx_serializer_table_init_too_many_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_table_add),
        cmocka_unit_test(test_ergo_tx_serializer_table_add_too_many_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_table_add_bad_token_id),
        cmocka_unit_test(test_ergo_tx_serializer_table_add_more_data),
        cmocka_unit_test(test_ergo_tx_serializer_table_hash),
        cmocka_unit_test(test_ergo_tx_serializer_table_hash_bad_hash)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
