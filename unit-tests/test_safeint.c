#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/safeint.h"

static void test_checked_add_u64_min(void **state) {
    (void) state;

    uint64_t l = 0;
    uint64_t r = 0;
    uint64_t out;
    assert_true(checked_add_u64(l, r, &out));
    assert_int_equal(out, 0);
}

static void test_checked_add_u64_max(void **state) {
    (void) state;

    uint64_t l = 0;
    uint64_t r = UINT64_MAX;
    uint64_t out;
    assert_true(checked_add_u64(l, r, &out));
    assert_int_equal(out, UINT64_MAX);
}

static void test_checked_add_u64_more_than_max(void **state) {
    (void) state;

    uint64_t l = 1;
    uint64_t r = UINT64_MAX;
    uint64_t out;
    assert_false(checked_add_u64(l, r, &out));
}

static void test_checked_add_u64(void **state) {
    (void) state;

    uint64_t l = 1;
    uint64_t r = 2;
    uint64_t out;
    assert_true(checked_add_u64(l, r, &out));
    assert_int_equal(out, 3);
}

static void test_checked_sub_u64_min(void **state) {
    (void) state;

    uint64_t l = 0;
    uint64_t r = 0;
    uint64_t out;
    assert_true(checked_sub_u64(l, r, &out));
    assert_int_equal(out, 0);
}

static void test_checked_sub_u64_max(void **state) {
    (void) state;

    uint64_t l = UINT64_MAX;
    uint64_t r = UINT64_MAX;
    uint64_t out;
    assert_true(checked_sub_u64(l, r, &out));
    assert_int_equal(out, 0);
}

static void test_checked_sub_u64_r_more_than_l(void **state) {
    (void) state;

    uint64_t l = 0;
    uint64_t r = 1;
    uint64_t out;
    assert_false(checked_sub_u64(l, r, &out));
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_checked_add_u64_min),
                                       cmocka_unit_test(test_checked_add_u64_max),
                                       cmocka_unit_test(test_checked_add_u64_more_than_max),
                                       cmocka_unit_test(test_checked_add_u64),
                                       cmocka_unit_test(test_checked_sub_u64_min),
                                       cmocka_unit_test(test_checked_sub_u64_max),
                                       cmocka_unit_test(test_checked_sub_u64_r_more_than_l)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
