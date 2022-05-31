#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/buffer.h"

static void test_buffer_init(void **state) {
    (void) state;

    uint8_t data[1] = {0x01};
    uint16_t buf_size = 1;
    uint16_t data_size = sizeof(data);
    buffer_t buf;
    buffer_init(&buf, data, buf_size, data_size);
    assert_ptr_equal(data, buf.ptr);
    assert_int_equal(buf.size, buf_size);
    assert_int_equal(buf.read_offset, 0);
    assert_int_equal(buf.write_offset, data_size);
}

static void test_buffer_from_array_full(void **state) {
    (void) state;

    uint16_t array[1] = {0x01};
    BUFFER_FROM_ARRAY_FULL(buf, array, 1);
    assert_ptr_equal(array, buf.ptr);
    assert_int_equal(buf.size, 1);
    assert_int_equal(buf.read_offset, 0);
    assert_int_equal(buf.write_offset, 1);
}

static void test_buffer_from_array_empty(void **state) {
    (void) state;

    uint16_t array[1] = {0x01};
    BUFFER_FROM_ARRAY_EMPTY(buf, array, 1);
    assert_ptr_equal(array, buf.ptr);
    assert_int_equal(buf.size, 1);
    assert_int_equal(buf.read_offset, 0);
    assert_int_equal(buf.write_offset, 0);
}

static void test_buffer_from_var_full(void **state) {
    (void) state;

    uint8_t var[1] = {0x01};
    BUFFER_FROM_VAR_FULL(buf, var);
    assert_ptr_equal(var, buf.ptr);
    assert_int_equal(buf.size, sizeof(var));
    assert_int_equal(buf.read_offset, 0);
    assert_int_equal(buf.write_offset, sizeof(var));
}

static void test_buffer_from_var_empty(void **state) {
    (void) state;

    uint8_t var[1] = {0x01};
    BUFFER_FROM_VAR_EMPTY(buf, var);
    assert_ptr_equal(var, buf.ptr);
    assert_int_equal(buf.size, sizeof(var));
    assert_int_equal(buf.read_offset, 0);
    assert_int_equal(buf.write_offset, 0);
}

static void test_buffer_can_read(void **state) {
    (void) state;

    uint8_t temp[20] = {0};
    BUFFER_FROM_VAR_FULL(buf, temp);

    assert_true(buffer_can_read(&buf, 20));

    assert_true(buffer_seek_read_cur(&buf, 20));
    assert_false(buffer_can_read(&buf, 1));
}

static void test_buffer_seek(void **state) {
    (void) state;

    uint8_t temp[20] = {0};
    BUFFER_FROM_VAR_FULL(buf, temp);

    assert_true(buffer_can_read(&buf, 20));

    assert_true(buffer_seek_read_cur(&buf, 20));  // seek at offset 20
    assert_false(buffer_can_read(&buf, 1));       // can't read 1 byte
    assert_false(buffer_seek_read_cur(&buf, 1));  // can't move at offset 21

    assert_true(buffer_seek_read_end(&buf, 19));
    assert_int_equal(buf.read_offset, 1);
    assert_false(buffer_seek_read_end(&buf, 21));  // can't seek at offset -1

    assert_true(buffer_seek_read_set(&buf, 10));
    assert_int_equal(buf.read_offset, 10);
    assert_false(buffer_seek_read_set(&buf, 21));  // can't seek at offset 21
}

static void test_buffer_read(void **state) {
    (void) state;

    // clang-format off
    uint8_t temp[15] = {
        0xFF,
        0x01, 0x02,
        0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E
    };
    BUFFER_FROM_VAR_FULL(buf, temp);

    uint8_t first = 0;
    assert_true(buffer_read_u8(&buf, &first));
    assert_int_equal(first, 255);                // 0xFF
    assert_true(buffer_seek_read_end(&buf, 0));  // seek at offset 19
    assert_false(buffer_read_u8(&buf, &first));  // can't read 1 byte

    uint16_t second = 0;
    assert_true(buffer_seek_read_set(&buf, 1));        // set back to offset 1
    assert_true(buffer_read_u16(&buf, &second, BE));   // big endian
    assert_int_equal(second, 258);                     // 0x01 0x02
    assert_true(buffer_seek_read_set(&buf, 1));        // set back to offset 1
    assert_true(buffer_read_u16(&buf, &second, LE));   // little endian
    assert_int_equal(second, 513);                     // 0x02 0x01
    assert_true(buffer_seek_read_set(&buf, 14));       // seek at offset 14
    assert_false(buffer_read_u16(&buf, &second, BE));  // can't read 2 bytes

    uint32_t third = 0;
    assert_true(buffer_seek_read_set(&buf, 3));        // set back to offset 3
    assert_true(buffer_read_u32(&buf, &third, BE));    // big endian
    assert_int_equal(third, 50595078);                 // 0x03 0x04 0x05 0x06
    assert_true(buffer_seek_read_set(&buf, 3));        // set back to offset 3
    assert_true(buffer_read_u32(&buf, &third, LE));    // little endian
    assert_int_equal(third, 100992003);                // 0x06 0x05 0x04 0x03
    assert_true(buffer_seek_read_set(&buf, 12));       // seek at offset 12
    assert_false(buffer_read_u32(&buf, &third, BE));   // can't read 4 bytes

    uint64_t fourth = 0;
    assert_true(buffer_seek_read_set(&buf, 7));        // set back to offset 7
    assert_true(buffer_read_u64(&buf, &fourth, BE));   // big endian
    assert_int_equal(fourth, 506664896818842894);      // 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E
    assert_true(buffer_seek_read_set(&buf, 7));        // set back to offset 7
    assert_true(buffer_read_u64(&buf, &fourth, LE));   // little endian
    assert_int_equal(fourth, 1012478732780767239);     // 0x0E 0x0D 0x0C 0x0B 0x0A 0x09 0x08 0x07
    assert_true(buffer_seek_read_set(&buf, 8));        // seek at offset 8
    assert_false(buffer_read_u64(&buf, &fourth, BE));  // can't read 8 bytes
}

static void test_buffer_copy(void **state) {
    (void) state;

    uint8_t output[5] = {0};
    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    BUFFER_FROM_VAR_FULL(buf, temp);

    assert_true(buffer_copy_bytes(&buf, output, sizeof(output)));
    assert_memory_equal(output, temp, sizeof(output));

    uint8_t output2[3] = {0};
    assert_true(buffer_seek_read_set(&buf, 2));
    assert_true(buffer_copy_bytes(&buf, output2, sizeof(output2)));
    assert_memory_equal(output2, ((uint8_t[3]){0x03, 0x04, 0x05}), 3);
    assert_true(buffer_seek_read_set(&buf, 0));                       // seek at offset 0
    assert_false(buffer_copy_bytes(&buf, output2, sizeof(output2)));  // can't read 5 bytes
}

static void test_buffer_read_bytes(void **state) {
    (void) state;

    uint8_t output[5] = {0};
    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    BUFFER_FROM_VAR_FULL(buf, temp);

    assert_true(buffer_read_bytes(&buf, output, sizeof(output)));
    assert_memory_equal(output, temp, sizeof(output));
    assert_int_equal(buf.read_offset, sizeof(output));

    uint8_t output2[3] = {0};
    assert_false(buffer_read_bytes(&buf, output2, sizeof(output2)));  // can't read 3 bytes
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_buffer_init),
                                       cmocka_unit_test(test_buffer_from_array_full),
                                       cmocka_unit_test(test_buffer_from_array_empty),
                                       cmocka_unit_test(test_buffer_from_var_full),
                                       cmocka_unit_test(test_buffer_from_var_empty),
                                       cmocka_unit_test(test_buffer_can_read),
                                       cmocka_unit_test(test_buffer_seek),
                                       cmocka_unit_test(test_buffer_read),
                                       cmocka_unit_test(test_buffer_copy),
                                       cmocka_unit_test(test_buffer_read_bytes)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
