#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/read.h"

static void test_read_u16_be(void **state) {
    (void) state;

    uint8_t tmp[2] = {0x01, 0x02};
    assert_int_equal(read_u16_be(tmp, 0), 0x0102);
}

static void test_read_u16_le(void **state) {
    (void) state;

    uint8_t tmp[2] = {0x01, 0x02};
    assert_int_equal(read_u16_le(tmp, 0), 0x0201);
}

static void test_read_u32_be(void **state) {
    (void) state;

    uint8_t tmp[4] = {0x01, 0x02, 0x03, 0x04};
    assert_int_equal(read_u32_be(tmp, 0), 0x01020304);
}

static void test_read_u32_le(void **state) {
    (void) state;

    uint8_t tmp[4] = {0x01, 0x02, 0x03, 0x04};
    assert_int_equal(read_u32_le(tmp, 0), 0x04030201);
}

static void test_read_u64_be(void **state) {
    (void) state;

    uint8_t tmp[8] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };
    assert_int_equal(read_u64_be(tmp, 0), 0x0102030405060708);
}

static void test_read_u64_le(void **state) {
    (void) state;

    uint8_t tmp[8] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };
    assert_int_equal(read_u64_le(tmp, 0), 0x0807060504030201);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_read_u16_be),
                                       cmocka_unit_test(test_read_u16_le),
                                       cmocka_unit_test(test_read_u32_be),
                                       cmocka_unit_test(test_read_u32_le),
                                       cmocka_unit_test(test_read_u64_be),
                                       cmocka_unit_test(test_read_u64_le)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
