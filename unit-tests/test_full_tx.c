#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmocka.h>

#include "ergo/tx_ser_full.h"
#include "common/rwbuffer.h"
#include "macro_helpers.h"

#include <stdio.h>

static void test_simple_send_tx(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t ctx;
    cx_blake2b_t hash;
    token_table_t tokens_table = {0};

    assert_true(blake2b_256_init(&hash));

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
    BUFFER_FROM_ARRAY(OUT_ERGO_TREE_BUF, OUT_ERGO_TREE, sizeof(OUT_ERGO_TREE));
    assert_int_equal(ergo_tx_serializer_full_add_box_ergo_tree(&ctx, &OUT_ERGO_TREE_BUF),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Miners fee box
    assert_int_equal(ergo_tx_serializer_full_add_box(&ctx, 1100000ULL, 0, 201644, 0, 0),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    assert_int_equal(ergo_tx_serializer_full_add_box_miners_fee_tree(&ctx, false),
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
    BUFFER_FROM_ARRAY(CHANGE_ERGO_TREE_BUF, CHANGE_ERGO_TREE, sizeof(CHANGE_ERGO_TREE));
    assert_int_equal(ergo_tx_serializer_full_add_box_ergo_tree(&ctx, &CHANGE_ERGO_TREE_BUF),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

    // Should be finished
    assert_true(ergo_tx_serializer_full_is_finished(&ctx));

    uint8_t tx_id[ERGO_ID_LEN] = {0};
    const uint8_t EXPECTED_TX_ID[] = {0xe2, 0xd0, 0x4a, 0xde, 0x88, 0x0e, 0xf0, 0x4b,
                                      0xee, 0xdc, 0x6a, 0xc5, 0x70, 0x40, 0xc8, 0x35,
                                      0xc7, 0x18, 0x6d, 0x8d, 0x41, 0x78, 0x01, 0x58,
                                      0xd5, 0xb3, 0xbe, 0x56, 0x45, 0x56, 0x4a, 0x5d};

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

static void test_ergo_tx_serializer_full_init(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 0;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    assert_int_equal(ergo_tx_serializer_full_init(&context,
                                                  inputs_count,
                                                  data_inputs_count,
                                                  outputs_count,
                                                  tokens_count,
                                                  &hash,
                                                  &tokens_table),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
    uint8_t expected_hash[] = {0x01};
    uint8_t* data;
    size_t data_len;
    _cx_blake2b_get_data(context.hash, &data, &data_len);
    assert_int_equal(data_len, sizeof(expected_hash));
    assert_memory_equal(data, expected_hash, sizeof(expected_hash));
    _cx_blake2b_free_data(context.hash);
    assert_int_equal(context.inputs_count, inputs_count);
    assert_int_equal(context.data_inputs_count, data_inputs_count);
    assert_int_equal(context.outputs_count, outputs_count);
    assert_int_equal(tokens_table.count, 0);
    assert_int_equal(context.table_ctx.distinct_tokens_count, tokens_count);
    assert_ptr_equal(context.table_ctx.tokens_table, &tokens_table);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_FULL_STATE_TOKENS_STARTED);
}

static void test_ergo_tx_serializer_full_init_zero_tokens(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 0;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 0;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    assert_int_equal(ergo_tx_serializer_full_init(&context,
                                                  inputs_count,
                                                  data_inputs_count,
                                                  outputs_count,
                                                  tokens_count,
                                                  &hash,
                                                  &tokens_table),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
    uint8_t expected_hash[] = {0x01};
    uint8_t* data;
    size_t data_len;
    _cx_blake2b_get_data(context.hash, &data, &data_len);
    assert_int_equal(data_len, sizeof(expected_hash));
    assert_memory_equal(data, expected_hash, sizeof(expected_hash));
    _cx_blake2b_free_data(context.hash);
    assert_int_equal(context.inputs_count, inputs_count);
    assert_int_equal(context.data_inputs_count, data_inputs_count);
    assert_int_equal(context.outputs_count, outputs_count);
    assert_int_equal(tokens_table.count, 0);
    assert_ptr_equal(context.table_ctx.tokens_table, &tokens_table);
    assert_int_equal(context.table_ctx.distinct_tokens_count, tokens_count);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED);
    assert_int_equal(context.input_ctx.state, ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED);
}

static void test_ergo_tx_serializer_full_add_tokens(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 0;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    assert_int_equal(ergo_tx_serializer_full_init(&context,
                                                  inputs_count,
                                                  data_inputs_count,
                                                  outputs_count,
                                                  tokens_count,
                                                  &hash,
                                                  &tokens_table),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    assert_int_equal(ergo_tx_serializer_full_add_tokens(&context, &tokens),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
    assert_int_equal(context.state, ERGO_TX_SERIALIZER_FULL_STATE_INPUTS_STARTED);
    assert_int_equal(context.input_ctx.state, ERGO_TX_SERIALIZER_INPUT_STATE_FINISHED);
}

static void test_ergo_tx_serializer_full_add_input(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 0;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    ergo_tx_serializer_full_init(&context,
                                 inputs_count,
                                 data_inputs_count,
                                 outputs_count,
                                 tokens_count,
                                 &hash,
                                 &tokens_table);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    ergo_tx_serializer_full_add_tokens(&context, &tokens);
    uint8_t box_id[] = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
    uint8_t token_frames_count = 1;
    uint32_t context_extension_data_size = 2;
    assert_int_equal(ergo_tx_serializer_full_add_input(&context,
                                                       box_id,
                                                       token_frames_count,
                                                       context_extension_data_size),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_input_tokens(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 0;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    ergo_tx_serializer_full_init(&context,
                                 inputs_count,
                                 data_inputs_count,
                                 outputs_count,
                                 tokens_count,
                                 &hash,
                                 &tokens_table);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    ergo_tx_serializer_full_add_tokens(&context, &tokens);
    uint8_t box_id[] = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
    uint8_t token_frames_count = 1;
    uint32_t context_extension_data_size = 2;
    ergo_tx_serializer_full_add_input(&context,
                                      box_id,
                                      token_frames_count,
                                      context_extension_data_size);
    uint8_t token_frame_index = 0;
    uint8_t input_tokens_array[] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    BUFFER_FROM_ARRAY(input_tokens, input_tokens_array, sizeof(input_tokens_array));
    assert_int_equal(ergo_tx_serializer_full_add_input_tokens(&context,
                                                              box_id,
                                                              token_frame_index,
                                                              &input_tokens),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_input_context_extension(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 0;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    ergo_tx_serializer_full_init(&context,
                                 inputs_count,
                                 data_inputs_count,
                                 outputs_count,
                                 tokens_count,
                                 &hash,
                                 &tokens_table);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    ergo_tx_serializer_full_add_tokens(&context, &tokens);
    uint8_t box_id[] = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
    uint8_t token_frames_count = 1;
    uint32_t context_extension_data_size = 2;
    ergo_tx_serializer_full_add_input(&context,
                                      box_id,
                                      token_frames_count,
                                      context_extension_data_size);
    uint8_t token_frame_index = 0;
    uint8_t input_tokens_array[] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    BUFFER_FROM_ARRAY(input_tokens, input_tokens_array, sizeof(input_tokens_array));
    ergo_tx_serializer_full_add_input_tokens(&context, box_id, token_frame_index, &input_tokens);
    uint8_t extension_chunk_array[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(extension_chunk, extension_chunk_array, sizeof(extension_chunk_array));
    assert_int_equal(
        ergo_tx_serializer_full_add_input_context_extension(&context, &extension_chunk),
        ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_data_inputs(void** state) {
    (void) state;

    ergo_tx_serializer_full_context_t context;
    uint16_t inputs_count = 1;
    uint16_t data_inputs_count = 1;
    uint16_t outputs_count = 1;
    uint8_t tokens_count = 1;
    cx_blake2b_t hash;
    blake2b_256_init(&hash);
    token_table_t tokens_table = {0};
    ergo_tx_serializer_full_init(&context,
                                 inputs_count,
                                 data_inputs_count,
                                 outputs_count,
                                 tokens_count,
                                 &hash,
                                 &tokens_table);
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));
    ergo_tx_serializer_full_add_tokens(&context, &tokens);
    uint8_t box_id[] = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};
    uint8_t token_frames_count = 1;
    uint32_t context_extension_data_size = 2;
    ergo_tx_serializer_full_add_input(&context,
                                      box_id,
                                      token_frames_count,
                                      context_extension_data_size);
    uint8_t token_frame_index = 0;
    uint8_t input_tokens_array[] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                                    0x04, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    BUFFER_FROM_ARRAY(input_tokens, input_tokens_array, sizeof(input_tokens_array));
    ergo_tx_serializer_full_add_input_tokens(&context, box_id, token_frame_index, &input_tokens);
    uint8_t extension_chunk_array[] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(extension_chunk, extension_chunk_array, sizeof(extension_chunk_array));
    ergo_tx_serializer_full_add_input_context_extension(&context, &extension_chunk);
    uint8_t inputs_array[] = {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
                              0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
                              0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05};
    BUFFER_FROM_ARRAY(inputs, inputs_array, sizeof(inputs_array));
    assert_int_equal(ergo_tx_serializer_full_add_data_inputs(&context, &inputs),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

#define ERGO_TX_SERIALIZER_FULL_ADD_BOX()                                                         \
    ergo_tx_serializer_full_context_t context;                                                    \
    uint16_t inputs_count = 1;                                                                    \
    uint16_t data_inputs_count = 1;                                                               \
    uint16_t outputs_count = 1;                                                                   \
    uint8_t tokens_count = 1;                                                                     \
    cx_blake2b_t hash;                                                                            \
    blake2b_256_init(&hash);                                                                      \
    token_table_t tokens_table = {0};                                                             \
    ergo_tx_serializer_full_init(&context,                                                        \
                                 inputs_count,                                                    \
                                 data_inputs_count,                                               \
                                 outputs_count,                                                   \
                                 tokens_count,                                                    \
                                 &hash,                                                           \
                                 &tokens_table);                                                  \
    uint8_t tokens_array[] = {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,   \
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,   \
                              0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};        \
    BUFFER_FROM_ARRAY(tokens, tokens_array, sizeof(tokens_array));                                \
    ergo_tx_serializer_full_add_tokens(&context, &tokens);                                        \
    uint8_t box_id[] = {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,         \
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,         \
                        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03};              \
    uint8_t token_frames_count = 1;                                                               \
    uint32_t context_extension_data_size = 2;                                                     \
    ergo_tx_serializer_full_add_input(&context,                                                   \
                                      box_id,                                                     \
                                      token_frames_count,                                         \
                                      context_extension_data_size);                               \
    uint8_t token_frame_index = 0;                                                                \
    uint8_t input_tokens_array[] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,   \
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,   \
                                    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,   \
                                    0x04, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};  \
    BUFFER_FROM_ARRAY(input_tokens, input_tokens_array, sizeof(input_tokens_array));              \
    ergo_tx_serializer_full_add_input_tokens(&context, box_id, token_frame_index, &input_tokens); \
    uint8_t extension_chunk_array[] = {0x01, 0x02};                                               \
    BUFFER_FROM_ARRAY(extension_chunk, extension_chunk_array, sizeof(extension_chunk_array));     \
    ergo_tx_serializer_full_add_input_context_extension(&context, &extension_chunk);              \
    uint8_t inputs_array[] = {0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,   \
                              0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,   \
                              0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05};        \
    BUFFER_FROM_ARRAY(inputs, inputs_array, sizeof(inputs_array));                                \
    ergo_tx_serializer_full_add_data_inputs(&context, &inputs);                                   \
    uint64_t value = 12345;                                                                       \
    uint32_t ergo_tree_size = 1;                                                                  \
    uint32_t creation_height = 1;                                                                 \
    uint32_t registers_size = 1;                                                                  \
    assert_int_equal(ergo_tx_serializer_full_add_box(&context,                                    \
                                                     value,                                       \
                                                     ergo_tree_size,                              \
                                                     creation_height,                             \
                                                     tokens_count,                                \
                                                     registers_size),                             \
                     ERGO_TX_SERIALIZER_FULL_RES_OK);

static void test_ergo_tx_serializer_full_add_box(void** state) {
    (void) state;

    ERGO_TX_SERIALIZER_FULL_ADD_BOX();
}

static void test_ergo_tx_serializer_full_add_box_ergo_tree(void** state) {
    (void) state;

    ERGO_TX_SERIALIZER_FULL_ADD_BOX();
    uint8_t tree_chunk_array[] = {0x01};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    assert_int_equal(ergo_tx_serializer_full_add_box_ergo_tree(&context, &tree_chunk),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_box_change_tree(void** state) {
    (void) state;

    ERGO_TX_SERIALIZER_FULL_ADD_BOX();
    const uint8_t raw_pub_key[] = {0x04, 0x8b, 0x9f, 0xf8, 0x5d, 0xdd, 0x9f, 0x1e, 0x22, 0x88, 0xfc,
                                   0x53, 0x9d, 0x39, 0xc7, 0xc4, 0xee, 0xb7, 0xa5, 0x56, 0xf4, 0xd8,
                                   0x11, 0xcb, 0x73, 0x99, 0x64, 0x18, 0xde, 0x5a, 0xbd, 0xcb, 0x2a,
                                   0xfa, 0x2d, 0x53, 0x17, 0x16, 0x0a, 0x59, 0x50, 0x0f, 0x5d, 0x31,
                                   0xfa, 0xe8, 0x6b, 0xce, 0xe9, 0xab, 0x1a, 0x60, 0x53, 0xa1, 0x1d,
                                   0x53, 0x5d, 0x2d, 0x04, 0x3c, 0xe5, 0xcf, 0xf1, 0x0a, 0xe7};
    assert_int_equal(ergo_tx_serializer_full_add_box_change_tree(&context, raw_pub_key),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_box_miners_fee_tree(void** state) {
    (void) state;

    ERGO_TX_SERIALIZER_FULL_ADD_BOX();
    bool is_mainnet = true;
    assert_int_equal(ergo_tx_serializer_full_add_box_miners_fee_tree(&context, is_mainnet),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_box_tokens(void** state) {
    (void) state;

    ERGO_TX_SERIALIZER_FULL_ADD_BOX();
    uint8_t tree_chunk_array[] = {0x01};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_full_add_box_ergo_tree(&context, &tree_chunk);
    uint8_t box_tokens_array[] =
        {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    BUFFER_FROM_ARRAY(box_tokens, box_tokens_array, sizeof(box_tokens_array));
    assert_int_equal(ergo_tx_serializer_full_add_box_tokens(&context, &box_tokens),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

static void test_ergo_tx_serializer_full_add_box_registers(void** state) {
    (void) state;

    ERGO_TX_SERIALIZER_FULL_ADD_BOX();
    uint8_t tree_chunk_array[1] = {0x01};
    BUFFER_FROM_ARRAY(tree_chunk, tree_chunk_array, sizeof(tree_chunk_array));
    ergo_tx_serializer_full_add_box_ergo_tree(&context, &tree_chunk);
    uint8_t box_tokens_array[12] =
        {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    BUFFER_FROM_ARRAY(box_tokens, box_tokens_array, sizeof(box_tokens_array));
    ergo_tx_serializer_full_add_box_tokens(&context, &box_tokens);
    uint8_t registers_chunk_array[1] = {0x01};
    BUFFER_FROM_ARRAY(registers_chunk, registers_chunk_array, sizeof(registers_chunk_array));
    assert_int_equal(ergo_tx_serializer_full_add_box_registers(&context, &registers_chunk),
                     ERGO_TX_SERIALIZER_FULL_RES_OK);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_simple_send_tx),
        cmocka_unit_test(test_ergo_tx_serializer_full_init),
        cmocka_unit_test(test_ergo_tx_serializer_full_init_zero_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_input),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_input_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_input_context_extension),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_data_inputs),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_box),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_box_ergo_tree),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_box_change_tree),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_box_miners_fee_tree),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_box_tokens),
        cmocka_unit_test(test_ergo_tx_serializer_full_add_box_registers)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
