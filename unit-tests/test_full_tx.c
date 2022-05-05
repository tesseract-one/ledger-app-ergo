#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmocka.h>

#include "ergo/tx_ser_full.h"

#include <stdio.h>

static void test_simple_send_tx(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t ctx;
    cx_blake2b_t hash;
    token_table_t tokens_table = {0};

    // One input. 3 outputs. No tokens. Simple ERG transfer.
    assert_int_equal(ergo_tx_serializer_full_init(&ctx, 1, 0, 3, 0, &hash, &tokens_table),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    const uint8_t INPUT_BOX_0_ID[ERGO_ID_LEN] = {0xee, 0x52, 0x85, 0xa4, 0x1e, 0x36, 0x80, 0x26,
                                                 0x9c, 0x80, 0xbf, 0x87, 0xd3, 0x0e, 0xf7, 0x02,
                                                 0x9d, 0x64, 0x60, 0x39, 0xc3, 0x5d, 0xfe, 0x96,
                                                 0x78, 0x61, 0xb3, 0x9f, 0x33, 0x03, 0x15, 0x64};

    // No tokens so single frame
    assert_int_equal(ergo_tx_serializer_full_add_input(&ctx, INPUT_BOX_0_ID, 1, 0),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
    // We have to call add tokens with empty buffer to finish input.
    BUFFER_NEW_LOCAL_EMPTY(empty_buffer, 1);
    assert_int_equal(
        ergo_tx_serializer_full_add_input_tokens(&ctx, INPUT_BOX_0_ID, 0, &empty_buffer),
        ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Sending ERG to the address. Address output with 0 tokens and registers
    // Address is 3WxoJWccGfjzGAo6XXp4ugrUuMz3SjkqkMS21yfNm7DP1iwakvXs
    uint8_t OUT_ERGO_TREE[] = {0x00, 0x08, 0xcd, 0x03, 0x20, 0x94, 0xbc, 0x1f, 0xc3,
                               0xe5, 0x13, 0x36, 0x2a, 0x2e, 0x3d, 0x2e, 0xb8, 0x02,
                               0x28, 0x3c, 0x98, 0xbc, 0x31, 0x40, 0xdf, 0xf8, 0xf5,
                               0x4c, 0x6f, 0x40, 0x9d, 0xd6, 0x4e, 0xae, 0x18, 0xc3};
    assert_int_equal(
        ergo_tx_serializer_full_add_box(&ctx, 1000000ULL, sizeof(OUT_ERGO_TREE), 201644, 0, 0),
        ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Adding address tree.
    BUFFER_FROM_ARRAY_FULL(OUT_ERGO_TREE_BUF, OUT_ERGO_TREE, sizeof(OUT_ERGO_TREE));
    assert_int_equal(ergo_tx_serializer_full_add_box_ergo_tree(&ctx, &OUT_ERGO_TREE_BUF),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Miners fee box
    assert_int_equal(ergo_tx_serializer_full_add_box(&ctx, 1100000ULL, 0, 201644, 0, 0),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    assert_int_equal(ergo_tx_serializer_full_add_box_miners_fee_tree(&ctx),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Change ERG address. Address output with 0 tokens and registers
    // Address is 3Wy87D2awNE4TDFDo6vtF3oq6tVDuRcgN6NMd3fH488RutMkrouS
    uint8_t CHANGE_ERGO_TREE[] = {0x00, 0x08, 0xcd, 0x03, 0x4b, 0x47, 0xc2, 0xa9, 0x67,
                                  0x60, 0xcb, 0xce, 0x18, 0xf8, 0x47, 0x9a, 0xd9, 0x1a,
                                  0x8d, 0x3c, 0x29, 0x46, 0xcd, 0x29, 0xa1, 0xbd, 0xe4,
                                  0xb2, 0x3f, 0xb5, 0x90, 0xc3, 0x84, 0x14, 0xf7, 0x7d};
    assert_int_equal(ergo_tx_serializer_full_add_box(&ctx,
                                                     4997900000ULL,
                                                     sizeof(CHANGE_ERGO_TREE),
                                                     201644,
                                                     0,
                                                     0),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Adding tree.
    BUFFER_FROM_ARRAY_FULL(CHANGE_ERGO_TREE_BUF, CHANGE_ERGO_TREE, sizeof(CHANGE_ERGO_TREE));
    assert_int_equal(ergo_tx_serializer_full_add_box_ergo_tree(&ctx, &CHANGE_ERGO_TREE_BUF),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Should be finished
    assert_true(ergo_tx_serializer_full_is_finished(&ctx));

    uint8_t tx_id[ERGO_ID_LEN] = {0};
    const uint8_t EXPECTED_TX_ID[] = {0x20, 0x80, 0x1b, 0x85, 0x80, 0xb1, 0x01, 0xa2,
                                      0xb8, 0x70, 0x5c, 0x59, 0xda, 0xb7, 0x24, 0xdb,
                                      0x21, 0x16, 0x4d, 0x2e, 0x10, 0x32, 0xda, 0x95,
                                      0xcf, 0xf1, 0x26, 0x3b, 0x09, 0x01, 0x6a, 0xd7};

    

    // Calculating hash
    assert_true(blake2b_256_finalize(&hash, tx_id));

    uint8_t* tx_data = NULL;
    size_t tx_data_len = 0;

    _cx_blake2b_get_data(&hash, &tx_data, &tx_data_len);

    // Should be equal to expected
    assert_memory_equal(tx_id, EXPECTED_TX_ID, ERGO_ID_LEN);

    // Freeing TX data
    _cx_blake2b_free_data(&hash);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_simple_send_tx)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
