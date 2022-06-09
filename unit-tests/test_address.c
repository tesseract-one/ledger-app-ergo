#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "ergo/address.h"

static void test_ergo_address_from_pubkey(void **state) {
    (void) state;

    uint8_t network = 0;
    const uint8_t public_key[65] = {
        3, 139, 159, 248,  93, 221, 159,  30,
        34, 136, 252,  83, 157,  57, 199, 196,
        238, 183, 165,  86, 244, 216,  17, 203,
        115, 153, 100,  24, 222,  90, 189, 203,
        42,
        120,  87,  68, 130, 203,  26, 122, 160,
        39, 164, 219, 125, 215,  33, 221,  26,
        3,  30, 239, 164, 231, 152,  56, 245,
        154,  69,  25, 196, 255, 225,  49,   6
    };
    uint8_t address[38];
    uint8_t expected[38] = {
        1,   3, 139, 159, 248,  93, 221, 159,
        30,  34, 136, 252,  83, 157,  57, 199,
        196, 238, 183, 165,  86, 244, 216,  17,
        203, 115, 153, 100,  24, 222,  90, 189,
        203,  42, 181,  45, 202, 206
    };
    assert_true(ergo_address_from_pubkey(network, public_key, address));
    assert_memory_equal(address, expected, sizeof(expected));
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_ergo_address_from_pubkey)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
