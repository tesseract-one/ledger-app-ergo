#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "ergo/tx_ser_box.h"

#define ERGO_TX_SERIALIZER_BOX_INIT(name) \
    ergo_tx_serializer_box_context_t name; \
    uint64_t value = 12345; \
    uint32_t ergo_tree_size = 2; \
    uint32_t creation_height = 3; \
    uint8_t tokens_count = 1; \
    uint32_t registers_size = 1; \
    cx_blake2b_t hash; \
    assert_true(ergo_tx_serializer_box_id_hash_init(&hash)); \
    assert_int_equal( \
        ergo_tx_serializer_box_init( \
            &name, \
            value, \
            ergo_tree_size, \
            creation_height, \
            tokens_count, \
            registers_size, \
            &hash \
        ), \
        ERGO_TX_SERIALIZER_BOX_RES_OK \
    );

static void test_ergo_tx_serializer_box_init(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    assert_int_equal(context.ergo_tree_size, ergo_tree_size);
    assert_int_equal(context.creation_height, creation_height);
    assert_int_equal(context.tokens_count, tokens_count);
    assert_int_equal(context.registers_size, registers_size);
    assert_memory_equal(context.hash, &hash, sizeof(hash));
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
    assert_int_equal(
        ergo_tx_serializer_box_init(
            &context,
            value,
            ergo_tree_size,
            creation_height,
            tokens_count,
            registers_size,
            &hash
        ),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS
    );
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
    assert_int_equal(
        ergo_tx_serializer_box_init(
            &context,
            value,
            ergo_tree_size,
            creation_height,
            tokens_count,
            registers_size,
            &hash
        ),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA
    );
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
    assert_int_equal(
        ergo_tx_serializer_box_init(
            &context,
            value,
            ergo_tree_size,
            creation_height,
            tokens_count,
            registers_size,
            &hash
        ),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA
    );
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
    assert_int_equal(
        ergo_tx_serializer_box_init(
            &context,
            value,
            ergo_tree_size,
            creation_height,
            tokens_count,
            registers_size,
            &hash
        ),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    assert_int_equal(
        ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_TREE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_tree_bad_state(void **state) {
    (void) state;

    ergo_tx_serializer_box_context_t context;
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    assert_int_equal(
        ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree_too_much_data(void **state) {
    (void) state;

    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = sizeof(tree_chunk_array) - 1;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    ergo_tx_serializer_box_init(
        &context,
        value,
        ergo_tree_size,
        creation_height,
        tokens_count,
        registers_size,
        &hash
    );
    assert_int_equal(
        ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_tree_bad_hash(void **state) {
    (void) state;

    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_context_t context;
    uint64_t value = 12345;
    uint32_t ergo_tree_size = 2;
    uint32_t creation_height = 3;
    uint8_t tokens_count = 1;
    uint32_t registers_size = 1;
    cx_blake2b_t hash;
    ergo_tx_serializer_box_id_hash_init(&hash);
    ergo_tx_serializer_box_init(
        &context,
        value,
        ergo_tree_size,
        creation_height,
        tokens_count,
        registers_size,
        &hash
    );
    memset(context.hash, 0, sizeof(context.hash));
    assert_int_equal(
        ergo_tx_serializer_box_add_tree(&context, &tree_chunk),
        ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_ERROR);
}

static void test_ergo_tx_serializer_box_add_miners_fee_tree_mainnet(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    bool is_mainnet = true;
    assert_int_equal(
        ergo_tx_serializer_box_add_miners_fee_tree(&context, is_mainnet),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_FEE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_miners_fee_tree_testnet(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    bool is_mainnet = false;
    assert_int_equal(
        ergo_tx_serializer_box_add_miners_fee_tree(&context, is_mainnet),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_FEE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_change_tree(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    const uint8_t raw_public_key[PUBLIC_KEY_LEN] = {0};
    assert_int_equal(
        ergo_tx_serializer_box_add_change_tree(&context, raw_public_key),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.type, ERGO_TX_SERIALIZER_BOX_TYPE_CHANGE);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TREE_ADDED);
}

static void test_ergo_tx_serializer_box_add_tokens(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_add_tree(&context, &tree_chunk);
    uint8_t input_array[12] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    BUFFER_FROM_ARRAY_FULL(input, input_array, sizeof(input_array));
    token_table_t table = {
        1,
        {
            {
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01
            }
        }
    };
    assert_int_equal(
        ergo_tx_serializer_box_add_tokens(&context, &input, &table),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_TOKENS_ADDED);
}

static void test_ergo_tx_serializer_box_add_registers(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_add_tree(&context, &tree_chunk);
    uint8_t input_array[12] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    BUFFER_FROM_ARRAY_FULL(input, input_array, sizeof(input_array));
    token_table_t table = {
        1,
        {
            {
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01
            }
        }
    };
    ergo_tx_serializer_box_add_tokens(&context, &input, &table);
    uint8_t rc_array[1] = {0x01};
    BUFFER_FROM_ARRAY_FULL(registers_chunk, rc_array, sizeof(rc_array));
    assert_int_equal(
        ergo_tx_serializer_box_add_registers(&context, &registers_chunk),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_FINISHED);
}

static void test_ergo_tx_serializer_box_id_hash(void **state) {
    (void) state;

    ERGO_TX_SERIALIZER_BOX_INIT(context);
    uint8_t tree_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_box_add_tree(&context, &tree_chunk);
    uint8_t input_array[12] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    BUFFER_FROM_ARRAY_FULL(input, input_array, sizeof(input_array));
    token_table_t table = {
        1,
        {
            {
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0x01
            }
        }
    };
    ergo_tx_serializer_box_add_tokens(&context, &input, &table);
    uint8_t rc_array[1] = {0x01};
    BUFFER_FROM_ARRAY_FULL(registers_chunk, rc_array, sizeof(rc_array));
    ergo_tx_serializer_box_add_registers(&context, &registers_chunk);
    const uint8_t tx_id[ERGO_ID_LEN] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01
    };
    uint16_t box_index = 1;
    uint8_t box_id[ERGO_ID_LEN] = {
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01
    };
    assert_int_equal(
        ergo_tx_serializer_box_id_hash(&context, tx_id, box_index, box_id),
        ERGO_TX_SERIALIZER_BOX_RES_OK
    );
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_BOX_STATE_HASH_FINALIZED);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_ergo_tx_serializer_box_init),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_init_too_many_tokens),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_init_too_much_data_ergo_tree_size),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_init_too_much_data_registers_size),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_init_bad_hash),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_tree),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_bad_state),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_too_much_data),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_tree_bad_hash),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_miners_fee_tree_mainnet),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_miners_fee_tree_testnet),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_change_tree),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_tokens),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_add_registers),
                                       cmocka_unit_test(test_ergo_tx_serializer_box_id_hash)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
