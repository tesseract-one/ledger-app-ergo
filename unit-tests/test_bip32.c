#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

#include <cmocka.h>

#include "common/bip32_ext.h"

#define ARRAYLEN(array) (sizeof(array) / sizeof(array[0]))

#define BIP32_VALIDATE_OK(tvar, type)                                                             \
    b = bip32_path_validate(tvar, ARRAYLEN(tvar), BIP32_HARDENED(44), BIP32_HARDENED(429), type); \
    assert_true(b);

#define BIP32_VALIDATE_ERR(tvar, type)                                                            \
    b = bip32_path_validate(tvar, ARRAYLEN(tvar), BIP32_HARDENED(44), BIP32_HARDENED(429), type); \
    assert_false(b);

static void test_bip32_validate_account(void **state) {
    (void) state;

    uint32_t input1[3] = {0x8000002C, 0x800001AD, 0x80000000};
    uint32_t input2[4] = {0x8000002C, 0x800001AD, 0x80000000, 0x80000001};
    uint32_t input3[5] = {0x8000002C, 0x800001AD, 0x80000000, 0x80000001, 0x80000002};
    bool b = false;

    BIP32_VALIDATE_OK(input1, BIP32_PATH_VALIDATE_ACCOUNT_E3)

    BIP32_VALIDATE_OK(input1, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_OK(input2, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_OK(input3, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
}

static void test_bip32_validate_address(void **state) {
    (void) state;

    uint32_t input1[5] = {0x8000002C, 0x800001AD, 0x80000000, 0, 1};
    uint32_t input2[6] = {0x8000002C, 0x800001AD, 0x80000000, 0, 1, 2};
    bool b = false;

    BIP32_VALIDATE_OK(input1, BIP32_PATH_VALIDATE_ADDRESS_E5)

    BIP32_VALIDATE_OK(input1, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_OK(input2, BIP32_PATH_VALIDATE_ADDRESS_GE5)
}

static void test_bad_bip32_validate_account(void **state) {
    (void) state;

    uint32_t input1[2] = {0x8000002C, 0x800001AD};
    uint32_t input2[3] = {0x8000002C, 0x800001AD, 0x80};
    uint32_t input3[3] = {0x80000000, 0x800001AD, 0x80000000};
    uint32_t input4[3] = {0x8000002C, 0x80000000, 0x80000000};
    uint32_t input5[4] = {0x8000002C, 0x800001AD, 0x80000000, 0};
    uint32_t input6[5] = {0x8000002C, 0x800001AD, 0x80000000, 1, 2};
    uint32_t input7[5] = {0x8000002C, 0x800001AD, 0x80000000, 0x80000000, 2};
    uint32_t input8[5] = {0x8000002C, 0x800001AD, 0x80000000, 1, 0x80000000};
    bool b = false;

    BIP32_VALIDATE_ERR(input1, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input2, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input3, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input4, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input5, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input6, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input7, BIP32_PATH_VALIDATE_ACCOUNT_E3)
    BIP32_VALIDATE_ERR(input8, BIP32_PATH_VALIDATE_ACCOUNT_E3)

    BIP32_VALIDATE_ERR(input1, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input2, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input3, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input4, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input5, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input6, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input7, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
    BIP32_VALIDATE_ERR(input8, BIP32_PATH_VALIDATE_ACCOUNT_GE3)
}

static void test_bad_bip32_validate_address(void **state) {
    (void) state;

    uint32_t input1[2] = {0x8000002C, 0x800001AD};
    uint32_t input2[3] = {0x8000002C, 0x800001AD, 0x80000000};
    uint32_t input3[4] = {0x8000002C, 0x800001AD, 0x80000000, 0};
    uint32_t input4[5] = {0x8000002C, 0x800001AD, 0x80000000, 0x80000000, 2};
    uint32_t input5[5] = {0x8000002C, 0x800001AD, 0x80000000, 3, 2};
    uint32_t input6[5] = {0x8000002C, 0x800001AD, 0x80000000, 1, 0x80000000};
    uint32_t input7[5] = {0x80000000, 0x800001AD, 0x80000000, 1, 1};
    uint32_t input8[5] = {0x8000002C, 0x80000000, 0x80000000, 1, 1};
    uint32_t input9[6] = {0x8000002C, 0x800001AD, 0x80000000, 1, 1, 1};

    bool b = false;

    BIP32_VALIDATE_ERR(input1, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input2, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input3, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input4, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input5, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input6, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input7, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input8, BIP32_PATH_VALIDATE_ADDRESS_E5)
    BIP32_VALIDATE_ERR(input9, BIP32_PATH_VALIDATE_ADDRESS_E5)

    BIP32_VALIDATE_ERR(input1, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input2, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input3, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input4, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input5, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input6, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input7, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_ERR(input8, BIP32_PATH_VALIDATE_ADDRESS_GE5)
    BIP32_VALIDATE_OK(input9, BIP32_PATH_VALIDATE_ADDRESS_GE5)
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_bip32_validate_account),
                                       cmocka_unit_test(test_bip32_validate_address),
                                       cmocka_unit_test(test_bad_bip32_validate_account),
                                       cmocka_unit_test(test_bad_bip32_validate_address)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
