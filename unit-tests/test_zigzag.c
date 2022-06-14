#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/zigzag.h"

static void test_zigzag_encode_i32_zero(void **state) {
    (void) state;

    int32_t val = 0;
    assert_int_equal(zigzag_encode_i32(val), (int32_t) 0);
}

static void test_zigzag_encode_i32_minus_one(void **state) {
    (void) state;

    int32_t val = -1;
    assert_int_equal(zigzag_encode_i32(val), (int32_t) 1);
}

static void test_zigzag_encode_i32_one(void **state) {
    (void) state;

    int32_t val = 1;
    assert_int_equal(zigzag_encode_i32(val), (int32_t) 2);
}

static void test_zigzag_encode_i32(void **state) {
    (void) state;

    int32_t val = -123456;
    assert_int_equal(zigzag_encode_i32(val), (int32_t) 246911);
}

static void test_zigzag_encode_i32_min(void **state) {
    (void) state;

    int32_t val = INT32_MIN;
    assert_int_equal(zigzag_encode_i32(val), (int32_t) 4294967295);
}

static void test_zigzag_encode_i32_max(void **state) {
    (void) state;

    int32_t val = INT32_MAX;
    assert_int_equal(zigzag_encode_i32(val), (int32_t) 4294967294);
}

static void test_zigzag_encode_i64_zero(void **state) {
    (void) state;

    int64_t val = 0;
    assert_int_equal(zigzag_encode_i64(val), (int64_t) 0);
}

static void test_zigzag_encode_i64_minus_one(void **state) {
    (void) state;

    int64_t val = -1;
    assert_int_equal(zigzag_encode_i64(val), (int64_t) 1);
}

static void test_zigzag_encode_i64_one(void **state) {
    (void) state;

    int64_t val = 1;
    assert_int_equal(zigzag_encode_i64(val), (int64_t) 2);
}

static void test_zigzag_encode_i64(void **state) {
    (void) state;

    int64_t val = -12345678901;
    assert_int_equal(zigzag_encode_i64(val), (int64_t) 24691357801);
}

static void test_zigzag_encode_i64_min(void **state) {
    (void) state;

    int64_t val = INT64_MIN;
    assert_int_equal(zigzag_encode_i64(val), (int64_t) 18446744073709551615ull);
}

static void test_zigzag_encode_i64_max(void **state) {
    (void) state;

    int64_t val = INT64_MAX;
    assert_int_equal(zigzag_encode_i64(val), (int64_t) 18446744073709551614ull);
}

static void test_zigzag_decode_i32_zero(void **state) {
    (void) state;

    int32_t result = zigzag_decode_i32(0);
    assert_int_equal(result, (int32_t) 0);
}

static void test_zigzag_decode_i32_minus_one(void **state) {
    (void) state;

    int32_t result = zigzag_decode_i32(1);
    assert_int_equal(result, (int32_t) -1);
}

static void test_zigzag_decode_i32_one(void **state) {
    (void) state;

    int32_t result = zigzag_decode_i32(2);
    assert_int_equal(result, (int32_t) 1);
}

static void test_zigzag_decode_i32(void **state) {
    (void) state;

    int32_t result = zigzag_decode_i32(246911);
    assert_int_equal(result, (int32_t) -123456);
}

static void test_zigzag_decode_i32_min(void **state) {
    (void) state;

    int32_t result = zigzag_decode_i32(4294967295);
    assert_int_equal(result, (int32_t) INT32_MIN);
}

static void test_zigzag_decode_i32_max(void **state) {
    (void) state;

    int32_t result = zigzag_decode_i32(4294967294);
    assert_int_equal(result, (int32_t) INT32_MAX);
}

static void test_zigzag_decode_i64_zero(void **state) {
    (void) state;

    int64_t result = zigzag_decode_i64(0);
    assert_int_equal(result, (int64_t) 0);
}

static void test_zigzag_decode_i64_minus_one(void **state) {
    (void) state;

    int64_t result = zigzag_decode_i64(1);
    assert_int_equal(result, (int64_t) -1);
}

static void test_zigzag_decode_i64_one(void **state) {
    (void) state;

    int64_t result = zigzag_decode_i64(2);
    assert_int_equal(result, (int64_t) 1);
}

static void test_zigzag_decode_i64(void **state) {
    (void) state;

    int64_t result = zigzag_decode_i64(24691357801);
    assert_int_equal(result, (int64_t) -12345678901);
}

static void test_zigzag_decode_i64_min(void **state) {
    (void) state;

    int64_t result = zigzag_decode_i64(18446744073709551615ull);
    assert_int_equal(result, (int64_t) INT64_MIN);
}

static void test_zigzag_decode_i64_max(void **state) {
    (void) state;

    int64_t result = zigzag_decode_i64(18446744073709551614ull);
    assert_int_equal(result, (int64_t) INT64_MAX);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_zigzag_encode_i32_zero),
                                       cmocka_unit_test(test_zigzag_encode_i32_minus_one),
                                       cmocka_unit_test(test_zigzag_encode_i32_one),
                                       cmocka_unit_test(test_zigzag_encode_i32),
                                       cmocka_unit_test(test_zigzag_encode_i32_min),
                                       cmocka_unit_test(test_zigzag_encode_i32_max),
                                       cmocka_unit_test(test_zigzag_encode_i64_zero),
                                       cmocka_unit_test(test_zigzag_encode_i64_minus_one),
                                       cmocka_unit_test(test_zigzag_encode_i64_one),
                                       cmocka_unit_test(test_zigzag_encode_i64),
                                       cmocka_unit_test(test_zigzag_encode_i64_min),
                                       cmocka_unit_test(test_zigzag_encode_i64_max),
                                       cmocka_unit_test(test_zigzag_decode_i32_zero),
                                       cmocka_unit_test(test_zigzag_decode_i32_minus_one),
                                       cmocka_unit_test(test_zigzag_decode_i32_one),
                                       cmocka_unit_test(test_zigzag_decode_i32),
                                       cmocka_unit_test(test_zigzag_decode_i32_min),
                                       cmocka_unit_test(test_zigzag_decode_i32_max),
                                       cmocka_unit_test(test_zigzag_decode_i64_zero),
                                       cmocka_unit_test(test_zigzag_decode_i64_minus_one),
                                       cmocka_unit_test(test_zigzag_decode_i64_one),
                                       cmocka_unit_test(test_zigzag_decode_i64),
                                       cmocka_unit_test(test_zigzag_decode_i64_min),
                                       cmocka_unit_test(test_zigzag_decode_i64_max)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
