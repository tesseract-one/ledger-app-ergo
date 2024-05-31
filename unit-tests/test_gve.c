#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/gve.h"
#include "macro_helpers.h"

static void test_gve_get_u8(void **state) {
    (void) state;

    uint8_t tmp[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    uint8_t val;
    assert_int_equal(gve_get_u8(&buf, &val), GVE_OK);
    assert_int_equal(val, 0x01);
    assert_int_equal(gve_get_u8(&buf, &val), GVE_OK);
    assert_int_equal(val, 0x02);
    assert_int_equal(gve_get_u8(&buf, &val), GVE_ERR_DATA_SIZE);
}

static void test_gve_get_i8(void **state) {
    (void) state;

    uint8_t tmp[2] = {0x01, -0x02};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    int8_t val;
    assert_int_equal(gve_get_i8(&buf, &val), GVE_OK);
    assert_int_equal(val, 0x01);
    assert_int_equal(gve_get_i8(&buf, &val), GVE_OK);
    assert_int_equal(val, -0x02);
    assert_int_equal(gve_get_i8(&buf, &val), GVE_ERR_DATA_SIZE);
}

static void test_gve_get_u16(void **state) {
    (void) state;

    uint8_t tmp[3] = {0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    uint16_t val;
    assert_int_equal(gve_get_u16(&buf, &val), GVE_OK);
    assert_int_equal(val, 16643);
}

static void test_gve_get_u16_too_big(void **state) {
    (void) state;

    uint8_t tmp[4] = {0x84, 0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    uint16_t val;
    assert_int_equal(gve_get_u16(&buf, &val), GVE_ERR_INT_TO_BIG);
}

static void test_gve_get_i16(void **state) {
    (void) state;

    uint8_t tmp[3] = {0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    int16_t val;
    assert_int_equal(gve_get_i16(&buf, &val), GVE_OK);
    assert_int_equal(val, -8322);
}

static void test_gve_get_u32(void **state) {
    (void) state;

    uint8_t tmp[4] = {0x84, 0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    uint32_t val;
    assert_int_equal(gve_get_u32(&buf, &val), GVE_OK);
    assert_int_equal(val, 2130308);
}

static void test_gve_get_i32(void **state) {
    (void) state;

    uint8_t tmp[4] = {0x85, 0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    int32_t val;
    assert_int_equal(gve_get_i32(&buf, &val), GVE_OK);
    assert_int_equal(val, -1065155);
}

static void test_gve_get_u64(void **state) {
    (void) state;

    uint8_t tmp[6] = {0x86, 0x85, 0x84, 0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    uint64_t val;
    assert_int_equal(gve_get_u64(&buf, &val), GVE_OK);
    assert_int_equal(val, 34902966918);
}

static void test_gve_get_i64(void **state) {
    (void) state;

    uint8_t tmp[6] = {0x87, 0x85, 0x84, 0x83, 0x82, 0x01};
    BUFFER_FROM_ARRAY(buf, tmp, sizeof(tmp));
    int64_t val;
    assert_int_equal(gve_get_i64(&buf, &val), GVE_OK);
    assert_int_equal(val, -17451483460);
}

static void test_gve_put_u8(void **state) {
    (void) state;

    uint8_t tmp[2] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    uint8_t val1 = 0x01;
    uint8_t expected1[1] = {0x01};
    assert_int_equal(gve_put_u8(&buf, val1), GVE_OK);
    assert_memory_equal(tmp, expected1, sizeof(expected1));
    uint8_t val2 = 0x02;
    uint8_t expected2[2] = {0x01, 0x02};
    assert_int_equal(gve_put_u8(&buf, val2), GVE_OK);
    assert_memory_equal(tmp, expected2, sizeof(expected2));
    assert_int_equal(gve_put_u8(&buf, 0x03), GVE_ERR_DATA_SIZE);
}

static void test_gve_put_i8(void **state) {
    (void) state;

    uint8_t tmp[2] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    int8_t val1 = -0x01;
    uint8_t expected1[1] = {-0x01};
    assert_int_equal(gve_put_i8(&buf, val1), GVE_OK);
    assert_memory_equal(tmp, expected1, sizeof(expected1));
    int8_t val2 = -0x02;
    uint8_t expected2[2] = {-0x01, -0x02};
    assert_int_equal(gve_put_i8(&buf, val2), GVE_OK);
    assert_memory_equal(tmp, expected2, sizeof(expected2));
    assert_int_equal(gve_put_i8(&buf, -0x03), GVE_ERR_DATA_SIZE);
}

static void test_gve_put_u16(void **state) {
    (void) state;

    uint8_t tmp[3] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    uint16_t val = 16643;
    uint8_t expected[3] = {0x83, 0x82, 0x01};
    assert_int_equal(gve_put_u16(&buf, val), GVE_OK);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_gve_put_i16(void **state) {
    (void) state;

    uint8_t tmp[3] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    int16_t val = -8322;
    uint8_t expected[3] = {0x83, 0x82, 0x01};
    assert_int_equal(gve_put_i16(&buf, val), GVE_OK);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_gve_put_u32(void **state) {
    (void) state;

    uint8_t tmp[4] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    uint32_t val = 2130308;
    uint8_t expected[4] = {0x84, 0x83, 0x82, 0x01};
    assert_int_equal(gve_put_u32(&buf, val), GVE_OK);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_gve_put_i32(void **state) {
    (void) state;

    uint8_t tmp[4] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    int32_t val = -1065155;
    uint8_t expected[4] = {0x85, 0x83, 0x82, 0x01};
    assert_int_equal(gve_put_i32(&buf, val), GVE_OK);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_gve_put_u64(void **state) {
    (void) state;

    uint8_t tmp[6] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    uint64_t val = 34902966918;
    uint8_t expected[6] = {0x86, 0x85, 0x84, 0x83, 0x82, 0x01};
    assert_int_equal(gve_put_u64(&buf, val), GVE_OK);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_gve_put_i64(void **state) {
    (void) state;

    uint8_t tmp[6] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, tmp, sizeof(tmp));
    int64_t val = -17451483460;
    uint8_t expected[6] = {0x87, 0x85, 0x84, 0x83, 0x82, 0x01};
    assert_int_equal(gve_put_i64(&buf, val), GVE_OK);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_gve_get_u8),
                                       cmocka_unit_test(test_gve_get_i8),
                                       cmocka_unit_test(test_gve_get_u16),
                                       cmocka_unit_test(test_gve_get_u16_too_big),
                                       cmocka_unit_test(test_gve_get_i16),
                                       cmocka_unit_test(test_gve_get_u32),
                                       cmocka_unit_test(test_gve_get_i32),
                                       cmocka_unit_test(test_gve_get_u64),
                                       cmocka_unit_test(test_gve_get_i64),
                                       cmocka_unit_test(test_gve_put_u8),
                                       cmocka_unit_test(test_gve_put_i8),
                                       cmocka_unit_test(test_gve_put_u16),
                                       cmocka_unit_test(test_gve_put_i16),
                                       cmocka_unit_test(test_gve_put_u32),
                                       cmocka_unit_test(test_gve_put_i32),
                                       cmocka_unit_test(test_gve_put_u64),
                                       cmocka_unit_test(test_gve_put_i64)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
