#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "ergo/tx_ser_input.h"
#include "common/rwbuffer.h"
#include "macro_helpers.h"

const uint8_t test_box_id[] = {0xf1, 0xca, 0x1e, 0x06, 0x0a, 0xa1, 0x1f, 0x98, 0x9b, 0x3d, 0x70,
                               0xec, 0x0e, 0x0c, 0x98, 0x7c, 0x95, 0x2e, 0x23, 0x89, 0xbe, 0x5b,
                               0x82, 0x0b, 0xc7, 0xdb, 0xfc, 0x32, 0x6f, 0x86, 0x13, 0x73};

uint8_t test_tokens_array[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                               0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

token_table_t test_tokens_table = {1, {{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}};

#define ERGO_TX_SERIALIZER_INPUT_INIT(name)                            \
    ergo_tx_serializer_input_context_t name;                           \
    uint8_t token_frames_count = 1;                                    \
    uint32_t proof_data_size = 3;                                      \
    cx_blake2b_t hash;                                                 \
    blake2b_256_init(&hash);                                           \
    assert_int_equal(ergo_tx_serializer_input_init(&context,           \
                                                   test_box_id,        \
                                                   token_frames_count, \
                                                   proof_data_size,    \
                                                   &test_tokens_table, \
                                                   &hash),             \
                     ERGO_TX_SERIALIZER_INPUT_RES_OK);

#define VERIFY_HASH(hash, expected)                        \
    uint8_t *data;                                         \
    size_t data_len;                                       \
    _cx_blake2b_get_data(hash, &data, &data_len);          \
    assert_int_equal(data_len, sizeof(expected));          \
    assert_memory_equal(data, expected, sizeof(expected)); \
    _cx_blake2b_free_data(hash);

static void test_ergo_tx_serializer_input_init(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    assert_memory_equal(context.box_id, test_box_id, ERGO_ID_LEN);
    assert_int_equal(context.frames_count, token_frames_count);
    assert_int_equal(context.frames_processed, 0);
    assert_int_equal(context.context_extension_data_size, proof_data_size);
    assert_memory_equal(context.tokens_table, &test_tokens_table, sizeof(test_tokens_table));
    assert_memory_equal(context.hash, &hash, sizeof(hash));
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_FRAMES_STARTED);
}

static void test_ergo_tx_serializer_input_init_bad_extension_size(void **state) {
    (void) state;

    ergo_tx_serializer_input_context_t context;
    uint8_t token_frames_count = 1;
    uint32_t proof_data_size = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    assert_int_equal(ergo_tx_serializer_input_init(&context,
                                                   test_box_id,
                                                   token_frames_count,
                                                   proof_data_size,
                                                   &test_tokens_table,
                                                   &hash),
                     ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_OK);
    uint8_t expected_hash[33] = {0xf1, 0xca, 0x1e, 0x06, 0x0a, 0xa1, 0x1f, 0x98, 0x9b, 0x3d, 0x70,
                                 0xec, 0x0e, 0x0c, 0x98, 0x7c, 0x95, 0x2e, 0x23, 0x89, 0xbe, 0x5b,
                                 0x82, 0x0b, 0xc7, 0xdb, 0xfc, 0x32, 0x6f, 0x86, 0x13, 0x73, 0x00};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_EXTENSION_STARTED);
}

static void test_ergo_tx_serializer_input_add_tokens_empty_extension(void **state) {
    (void) state;

    ergo_tx_serializer_input_context_t context;
    uint8_t token_frames_count = 1;
    uint32_t proof_data_size = 0;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    ergo_tx_serializer_input_init(&context,
                                  test_box_id,
                                  token_frames_count,
                                  proof_data_size,
                                  &test_tokens_table,
                                  &hash);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_OK);
    uint8_t expected_hash[34] = {0xf1, 0xca, 0x1e, 0x06, 0x0a, 0xa1, 0x1f, 0x98, 0x9b,
                                 0x3d, 0x70, 0xec, 0x0e, 0x0c, 0x98, 0x7c, 0x95, 0x2e,
                                 0x23, 0x89, 0xbe, 0x5b, 0x82, 0x0b, 0xc7, 0xdb, 0xfc,
                                 0x32, 0x6f, 0x86, 0x13, 0x73, 0x00, 0x00};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED);
}

static void test_ergo_tx_serializer_input_add_tokens_bad_state(void **state) {
    (void) state;

    ergo_tx_serializer_input_context_t context;
    context.state = ERGO_TX_SERIALIZER_INPUT_STATE_EXTENSION_STARTED;
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens_bad_input(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    const uint8_t box_id[32] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_INPUT_ID);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens_too_many_input_frames(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 1;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MANY_INPUT_FRAMES);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens_bad_frame_index(void **state) {
    (void) state;

    ergo_tx_serializer_input_context_t context;
    uint8_t token_frames_count = 2;
    uint32_t proof_data_size = 3;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    ergo_tx_serializer_input_init(&context,
                                  test_box_id,
                                  token_frames_count,
                                  proof_data_size,
                                  &test_tokens_table,
                                  &hash);
    uint8_t token_frame_index = 1;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_FRAME_INDEX);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens_bad_token_id(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    uint8_t tokens_array[31] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_ID);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens_bad_token_value(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    uint8_t tokens_array[39] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_TOKEN_VALUE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_tokens_more_data(void **state) {
    (void) state;

    ergo_tx_serializer_input_context_t context;
    uint8_t token_frames_count = 2;
    uint32_t proof_data_size = 3;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    ergo_tx_serializer_input_init(&context,
                                  test_box_id,
                                  token_frames_count,
                                  proof_data_size,
                                  &test_tokens_table,
                                  &hash);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    assert_int_equal(
        ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens),
        ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA);
}

static void test_ergo_tx_serializer_input_add_context_extension(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens);
    uint8_t chunk_array[3] = {0x01, 0x02, 0x03};
    BUFFER_FROM_ARRAY(chunk, chunk_array, sizeof(chunk_array));
    assert_int_equal(ergo_tx_serializer_input_add_context_extension(&context, &chunk),
                     ERGO_TX_SERIALIZER_INPUT_RES_OK);
    assert_int_equal(context.context_extension_data_size, 0);
    uint8_t expected_hash[36] = {0xf1, 0xca, 0x1e, 0x06, 0x0a, 0xa1, 0x1f, 0x98, 0x9b,
                                 0x3d, 0x70, 0xec, 0x0e, 0x0c, 0x98, 0x7c, 0x95, 0x2e,
                                 0x23, 0x89, 0xbe, 0x5b, 0x82, 0x0b, 0xc7, 0xdb, 0xfc,
                                 0x32, 0x6f, 0x86, 0x13, 0x73, 0x00, 0x01, 0x02, 0x03};
    VERIFY_HASH(context.hash, expected_hash);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED);
}

static void test_ergo_tx_serializer_input_add_context_extension_bad_state(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t chunk_array[3] = {0x01, 0x02, 0x03};
    BUFFER_FROM_ARRAY(chunk, chunk_array, sizeof(chunk_array));
    assert_int_equal(ergo_tx_serializer_input_add_context_extension(&context, &chunk),
                     ERGO_TX_SERIALIZER_INPUT_RES_ERR_BAD_STATE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_context_extension_too_much_data(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens);
    uint8_t chunk_array[4] = {0x01, 0x02, 0x03, 0x04};
    BUFFER_FROM_ARRAY(chunk, chunk_array, sizeof(chunk_array));
    assert_int_equal(ergo_tx_serializer_input_add_context_extension(&context, &chunk),
                     ERGO_TX_SERIALIZER_INPUT_RES_ERR_TOO_MUCH_PROOF_DATA);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_INPUT_STATE_ERROR);
}

static void test_ergo_tx_serializer_input_add_context_extension_more_data(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_INPUT_INIT(context);
    uint8_t token_frame_index = 0;
    BUFFER_FROM_ARRAY(tokens, test_tokens_array, sizeof(test_tokens_array));
    ergo_tx_serializer_input_add_tokens(&context, test_box_id, token_frame_index, &tokens);
    uint8_t chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(chunk, chunk_array, sizeof(chunk_array));
    assert_int_equal(ergo_tx_serializer_input_add_context_extension(&context, &chunk),
                     ERGO_TX_SERIALIZER_INPUT_RES_MORE_DATA);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ergo_tx_serializer_input_init),
        cmocka_unit_test(test_ergo_tx_serializer_input_init_bad_extension_size),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_empty_extension),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_bad_state),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_bad_input),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_too_many_input_frames),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_bad_frame_index),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_bad_token_id),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_bad_token_value),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_tokens_more_data),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_context_extension),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_context_extension_bad_state),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_context_extension_too_much_data),
        cmocka_unit_test(test_ergo_tx_serializer_input_add_context_extension_more_data)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
