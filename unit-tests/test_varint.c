#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/varint.h"

static void test_gve_get_u8(void **state) {
    (void) state;

    uint8_t tmp[2] = {0x01, 0x02};
    BUFFER_FROM_ARRAY_FULL(buf, tmp, sizeof(tmp));
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
    BUFFER_FROM_ARRAY_FULL(buf, tmp, sizeof(tmp));
    int8_t val;
    assert_int_equal(gve_get_i8(&buf, &val), GVE_OK);
    assert_int_equal(val, 0x01);
    assert_int_equal(gve_get_i8(&buf, &val), GVE_OK);
    assert_int_equal(val, -0x02);
    assert_int_equal(gve_get_i8(&buf, &val), GVE_ERR_DATA_SIZE);
}

static void test_gve_get_u16(void **state) {
    (void) state;

    uint8_t tmp[3] = {131, 130, 1};
    BUFFER_FROM_ARRAY_FULL(buf, tmp, sizeof(tmp));
    uint16_t val;
    assert_int_equal(gve_get_u16(&buf, &val), GVE_OK);
    assert_int_equal(val, 16643);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_gve_get_u8),
                                       cmocka_unit_test(test_gve_get_i8),
                                       cmocka_unit_test(test_gve_get_u16)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
