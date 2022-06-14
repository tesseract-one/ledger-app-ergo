#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "common/write.h"

static void test_write_u16_be(void **state) {
    (void) state;

    uint8_t tmp[2] = {0};
    uint8_t expected[2] = {0x01, 0x07};
    write_u16_be(tmp, 0, (uint16_t) 263U);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_write_u16_le(void **state) {
    (void) state;

    uint8_t tmp[2] = {0};
    uint8_t expected[2] = {0x07, 0x01};
    write_u16_le(tmp, 0, (uint16_t) 263U);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_write_u32_be(void **state) {
    (void) state;

    uint8_t tmp[4] = {0};
    uint8_t expected[4] = {0x01, 0x3B, 0xAC, 0xC7};
    write_u32_be(tmp, 0, (uint32_t) 20688071UL);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_write_u32_le(void **state) {
    (void) state;

    uint8_t tmp[4] = {0};
    uint8_t expected[4] = {0xC7, 0xAC, 0x3B, 0x01};
    write_u32_le(tmp, 0, (uint32_t) 20688071UL);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_write_u64_be(void **state) {
    (void) state;

    uint8_t tmp[8] = {0};
    uint8_t expected[8] = {0xEB, 0x68, 0x44, 0xC0, 0x2C, 0x61, 0xB0, 0x99};
    write_u64_be(tmp, 0, (uint64_t) 16962883588659982489ULL);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

static void test_write_u64_le(void **state) {
    (void) state;

    uint8_t tmp[8] = {0};
    uint8_t expected[8] = {0x99, 0xB0, 0x61, 0x2C, 0xC0, 0x44, 0x68, 0xEB};
    write_u64_le(tmp, 0, (uint64_t) 16962883588659982489ULL);
    assert_memory_equal(tmp, expected, sizeof(expected));
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_write_u16_be),
                                       cmocka_unit_test(test_write_u16_le),
                                       cmocka_unit_test(test_write_u32_be),
                                       cmocka_unit_test(test_write_u32_le),
                                       cmocka_unit_test(test_write_u64_be),
                                       cmocka_unit_test(test_write_u64_le)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
