#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "common/base58.h"

#define VERIFY_BAD_BASE58_ENCODE(in) \
    char out[100] = {0}; \
    int out_len = 0; \
    out_len = base58_encode((uint8_t *) in, strlen(in), out, sizeof(out)); \
    assert_int_equal(out_len, -1); \
    memset(out, 0, sizeof(out));

#define VERIFY_BASE58_ENCODE(in, expected_out) \
    char out[100] = {0}; \
    int out_len = 0; \
    out_len = base58_encode((uint8_t *) in, strlen(in), out, sizeof(out)); \
    assert_int_equal(out_len, strlen(expected_out)); \
    assert_string_equal((char *) out, expected_out); \
    memset(out, 0, sizeof(out));

#define VERIFY_BAD_BASE58_DECODE(in) \
    uint8_t out[100] = {0}; \
    int out_len = 0; \
    out_len = base58_decode(in, strlen(in), out, sizeof(out)); \
    assert_int_equal(out_len, -1); \
    memset(out, 0, sizeof(out));

#define VERIFY_BASE58_DECODE(in, expected_out) \
    uint8_t out[100] = {0}; \
    int out_len = 0; \
    out_len = base58_decode(in, strlen(in), out, sizeof(out)); \
    assert_int_equal(out_len, strlen(expected_out)); \
    assert_string_equal((char *) out, expected_out); \
    memset(out, 0, sizeof(out));

static void test_base58(void **state) {
    (void) state;

    const char in[] = "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z";
    const char expected_out[] = "The quick brown fox jumps over the lazy dog.";
    uint8_t out[100] = {0};
    int out_len = base58_decode(in, sizeof(in) - 1, out, sizeof(out));
    assert_int_equal(out_len, strlen(expected_out));
    assert_string_equal((char *) out, expected_out);

    const char in2[] = "The quick brown fox jumps over the lazy dog.";
    const char expected_out2[] = "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z";
    char out2[100] = {0};
    int out_len2 = base58_encode((uint8_t *) in2, sizeof(in2) - 1, out2, sizeof(out2));
    assert_int_equal(out_len2, strlen(expected_out2));
    assert_string_equal((char *) out2, expected_out2);
}

static void test_bad_base58_encode_out_of_range(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_ENCODE("1111111111111111111111111111111111111111"
                             "1111111111111111111111111111111111111111"
                             "11111111111111111111111111111111111111111");
}

static void test_base58_encode_zero_characters(void **state) {
    (void) state;
    VERIFY_BASE58_ENCODE("", "");
}

static void test_base58_encode_one_character(void **state) {
    (void) state;
    VERIFY_BASE58_ENCODE("!", "a");
}

static void test_base58_encode_max_number_of_characters(void **state) {
    (void) state;
    VERIFY_BASE58_ENCODE("1111111111111111111111111111111111111111"
                         "1111111111111111111111111111111111111111"
                         "1111111111111111111111111111111111111111",
                         "7pURzZxewQxq8JBWyAdgyj3h3xxhjY1cWcyv2GYQ"
                         "tophYQA4CpLhMJpjKVW8conmkFmRwDd8Ut4XKWiK"
                         "SbKZ8BqEfXxANAne3kugUNtKHy2S6RKLb9rhCbTq"
                         "DJFLEwmK7izZWSCbeBU6hph56skH6xcc2DU8B3MWr3Zr");
}

static void test_bad_base58_decode_zero_characters(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("");
}

static void test_bad_base58_decode_0_symbol(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("0");
}

static void test_bad_base58_decode_I_symbol(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("I");
}

static void test_bad_base58_decode_O_symbol(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("O");
}

static void test_bad_base58_decode_l_symbol(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("l");
}

static void test_bad_base58_decode_plus_symbol(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("+");
}

static void test_bad_base58_decode_slash_symbol(void **state) {
    (void) state;
    VERIFY_BAD_BASE58_DECODE("/");
}

static void test_base58_decode_one_character(void **state) {
    (void) state;
    VERIFY_BASE58_DECODE("a", "!");
}

static void test_base58_decode_max_number_of_characters(void **state) {
    (void) state;
    VERIFY_BASE58_DECODE("7pURzZxewQxq8JBWyAdgyj3h3xxhjY1cWcyv2GYQ"
                         "tophYQA4CpLhMJpjKVW8conmkFmRwDd8Ut4XKWiK"
                         "SbKZ8BqEfXxANAne3kugUNtKHy2S6RKLb9rhCbTq"
                         "DJFLEwmK7izZWSCbeBU6hph56skH6xcc2DU8B3MWr3Zr",
                         "1111111111111111111111111111111111111111"
                         "1111111111111111111111111111111111111111"
                         "1111111111111111111111111111111111111111");
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_base58),
                                       cmocka_unit_test(test_bad_base58_encode_out_of_range),
                                       cmocka_unit_test(test_base58_encode_zero_characters),
                                       cmocka_unit_test(test_base58_encode_one_character),
                                       cmocka_unit_test(test_base58_encode_max_number_of_characters),
                                       cmocka_unit_test(test_bad_base58_decode_zero_characters),
                                       cmocka_unit_test(test_bad_base58_decode_0_symbol),
                                       cmocka_unit_test(test_bad_base58_decode_I_symbol),
                                       cmocka_unit_test(test_bad_base58_decode_O_symbol),
                                       cmocka_unit_test(test_bad_base58_decode_l_symbol),
                                       cmocka_unit_test(test_bad_base58_decode_plus_symbol),
                                       cmocka_unit_test(test_bad_base58_decode_slash_symbol),
                                       cmocka_unit_test(test_base58_decode_one_character),
                                       cmocka_unit_test(test_base58_decode_max_number_of_characters)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
