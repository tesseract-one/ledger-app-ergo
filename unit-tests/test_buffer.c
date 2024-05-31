#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include "common/rwbuffer.h"

static void test_buffer_init(void **state) {
    (void) state;

    uint8_t data[3] = {0x01, 0x02, 0x00};
    uint16_t data_size = 2;
    uint16_t buf_size = sizeof(data);
    rw_buffer_t buf;
    rw_buffer_init(&buf, data, buf_size, data_size);
    assert_ptr_equal(data, buf.read.ptr);
    assert_int_equal(buf.size, buf_size);
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, data_size);
}

static void test_buffer_from_array_full(void **state) {
    (void) state;

    uint8_t array[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_FULL(buf, array, 1);
    assert_ptr_equal(array, buf.read.ptr);
    assert_int_equal(buf.size, 1);
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, 1);
}

static void test_buffer_from_array_empty(void **state) {
    (void) state;

    uint8_t array[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, array, 1);
    assert_ptr_equal(array, buf.read.ptr);
    assert_int_equal(buf.size, 1);
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, 0);
}

static void test_buffer_from_var_full(void **state) {
    (void) state;

    uint8_t var = 0x01;
    RW_BUFFER_FROM_VAR_FULL(buf, var);
    assert_ptr_equal(&var, buf.read.ptr);
    assert_int_equal(buf.size, sizeof(var));
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, sizeof(var));
}

static void test_buffer_from_var_empty(void **state) {
    (void) state;

    uint8_t var = 0x01;
    RW_BUFFER_FROM_VAR_EMPTY(buf, var);
    assert_ptr_equal(&var, buf.read.ptr);
    assert_int_equal(buf.size, sizeof(var));
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, 0);
}

static void test_buffer_new_local_empty(void **state) {
    (void) state;

    uint16_t size = 1;
    RW_BUFFER_NEW_LOCAL_EMPTY(buf, size);
    assert_int_equal(buf.size, size);
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, 0);
}

static void test_buffer_empty(void **state) {
    (void) state;

    uint8_t temp[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));
    buf.read.offset = 1;
    rw_buffer_empty(&buf);
    assert_int_equal(buf.read.offset, 0);
    assert_int_equal(buf.read.size, 0);
}

static void test_buffer_read_ptr(void **state) {
    (void) state;

    uint8_t temp[2] = {0x01, 0x02};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));
    buf.read.offset = 1;
    const uint8_t *ptr = rw_buffer_read_ptr(&buf);
    assert_ptr_equal(ptr, buf.read.ptr + buf.read.offset);
}

static void test_buffer_write_ptr(void **state) {
    (void) state;

    uint8_t temp[2] = {0x01, 0x02};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));
    uint8_t *ptr = rw_buffer_write_ptr(&buf);
    assert_ptr_equal(ptr, buf.read.ptr + buf.read.size);
}

static void test_buffer_can_read(void **state) {
    (void) state;

    uint8_t temp[20] = {0};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));

    assert_true(rw_buffer_can_read(&buf, 20));

    assert_true(rw_buffer_seek_read_cur(&buf, 20));
    assert_false(rw_buffer_can_read(&buf, 1));
}

static void test_buffer_can_write(void **state) {
    (void) state;

    uint8_t temp[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));
    assert_true(rw_buffer_can_write(&buf, 1));
    assert_false(rw_buffer_can_write(&buf, sizeof(temp) + 1));
    assert_true(rw_buffer_seek_write_cur(&buf, 1));
    assert_false(rw_buffer_can_write(&buf, 1));
}

static void test_buffer_empty_space_len(void **state) {
    (void) state;

    uint8_t temp[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));
    assert_int_equal(rw_buffer_empty_space_len(&buf), 1);
    assert_true(rw_buffer_seek_write_cur(&buf, 1));
    assert_int_equal(rw_buffer_empty_space_len(&buf), 0);
}

static void test_buffer_data_len(void **state) {
    (void) state;

    uint8_t temp[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));
    assert_int_equal(rw_buffer_data_len(&buf), 0);
    assert_true(rw_buffer_seek_write_cur(&buf, 1));
    assert_int_equal(rw_buffer_data_len(&buf), 1);
    assert_true(rw_buffer_seek_read_cur(&buf, 1));
    assert_int_equal(rw_buffer_data_len(&buf), 0);
}

static void test_buffer_read_position(void **state) {
    (void) state;

    uint8_t temp[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));
    assert_int_equal(rw_buffer_read_position(&buf), 0);
    assert_true(rw_buffer_seek_read_cur(&buf, 1));
    assert_int_equal(rw_buffer_read_position(&buf), 1);
}

static void test_buffer_write_position(void **state) {
    (void) state;

    uint8_t temp[1] = {0x01};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));
    assert_int_equal(rw_buffer_write_position(&buf), 0);
    assert_true(rw_buffer_seek_write_cur(&buf, 1));
    assert_int_equal(rw_buffer_write_position(&buf), 1);
}

static void test_buffer_seek_read(void **state) {
    (void) state;

    uint8_t temp[20] = {0};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));

    assert_true(rw_buffer_can_read(&buf, 20));

    assert_true(rw_buffer_seek_read_cur(&buf, 20));  // seek at offset 20
    assert_false(rw_buffer_can_read(&buf, 1));       // can't read 1 byte
    assert_false(rw_buffer_seek_read_cur(&buf, 1));  // can't move at offset 21

    assert_true(rw_buffer_seek_read_end(&buf, 19));
    assert_int_equal(buf.read.offset, 1);
    assert_false(rw_buffer_seek_read_end(&buf, 21));  // can't seek at offset -1

    assert_true(rw_buffer_seek_read_set(&buf, 10));
    assert_int_equal(buf.read.offset, 10);
    assert_false(rw_buffer_seek_read_set(&buf, 21));  // can't seek at offset 21
}

static void test_buffer_seek_write(void **state) {
    (void) state;

    uint8_t temp[20] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));

    assert_true(rw_buffer_can_write(&buf, 20));

    assert_true(rw_buffer_seek_write_cur(&buf, 20));  // seek at offset 20
    assert_false(rw_buffer_can_write(&buf, 1));       // can't write 1 byte
    assert_false(rw_buffer_seek_write_cur(&buf, 1));  // can't move at offset 21

    assert_true(rw_buffer_seek_write_end(&buf, 19));
    assert_int_equal(buf.read.size, 1);
    assert_false(rw_buffer_seek_write_end(&buf, 21));  // can't seek at offset -1

    assert_true(rw_buffer_seek_write_set(&buf, 10));
    assert_int_equal(buf.read.size, 10);
    assert_false(rw_buffer_seek_write_set(&buf, 21));  // can't seek at offset 21
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
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));

    uint8_t first = 0;
    assert_true(rw_buffer_read_u8(&buf, &first));
    assert_int_equal(first, 255);                // 0xFF
    assert_true(rw_buffer_seek_read_end(&buf, 0));  // seek at offset 19
    assert_false(rw_buffer_read_u8(&buf, &first));  // can't read 1 byte

    uint16_t second = 0;
    assert_true(rw_buffer_seek_read_set(&buf, 1));        // set back to offset 1
    assert_true(rw_buffer_read_u16(&buf, &second, BE));   // big endian
    assert_int_equal(second, 258);                     // 0x01 0x02
    assert_true(rw_buffer_seek_read_set(&buf, 1));        // set back to offset 1
    assert_true(rw_buffer_read_u16(&buf, &second, LE));   // little endian
    assert_int_equal(second, 513);                     // 0x02 0x01
    assert_true(rw_buffer_seek_read_set(&buf, 14));       // seek at offset 14
    assert_false(rw_buffer_read_u16(&buf, &second, BE));  // can't read 2 bytes

    uint32_t third = 0;
    assert_true(rw_buffer_seek_read_set(&buf, 3));        // set back to offset 3
    assert_true(rw_buffer_read_u32(&buf, &third, BE));    // big endian
    assert_int_equal(third, 50595078);                 // 0x03 0x04 0x05 0x06
    assert_true(rw_buffer_seek_read_set(&buf, 3));        // set back to offset 3
    assert_true(rw_buffer_read_u32(&buf, &third, LE));    // little endian
    assert_int_equal(third, 100992003);                // 0x06 0x05 0x04 0x03
    assert_true(rw_buffer_seek_read_set(&buf, 12));       // seek at offset 12
    assert_false(rw_buffer_read_u32(&buf, &third, BE));   // can't read 4 bytes

    uint64_t fourth = 0;
    assert_true(rw_buffer_seek_read_set(&buf, 7));        // set back to offset 7
    assert_true(rw_buffer_read_u64(&buf, &fourth, BE));   // big endian
    assert_int_equal(fourth, 506664896818842894);      // 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E
    assert_true(rw_buffer_seek_read_set(&buf, 7));        // set back to offset 7
    assert_true(rw_buffer_read_u64(&buf, &fourth, LE));   // little endian
    assert_int_equal(fourth, 1012478732780767239);     // 0x0E 0x0D 0x0C 0x0B 0x0A 0x09 0x08 0x07
    assert_true(rw_buffer_seek_read_set(&buf, 8));        // seek at offset 8
    assert_false(rw_buffer_read_u64(&buf, &fourth, BE));  // can't read 8 bytes
}

static void test_buffer_read_bip32_path(void **state) {
    (void) state;

    uint8_t temp[20] = {
        0x80, 0x00, 0x00, 0x2C,
        0x80, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t expected[5] = {0x8000002C, 0x80000001, 0x80000000, 0, 0};
    uint32_t out[5] = {0};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));
    assert_true(rw_buffer_read_bip32_path(&buf, out, 5));
    assert_memory_equal(out, expected, 5);
    assert_int_equal(buf.read.offset, sizeof(expected));
    assert_false(rw_buffer_read_bip32_path(&buf, out, 6));
}

static void test_buffer_write(void **state) {
    (void) state;

    // clang-format off
    uint8_t temp[8] = {0x0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));

    uint8_t first = 0xFF;
    assert_true(rw_buffer_write_u8(&buf, first));
    assert_int_equal(temp[0], 255);                 // 0xFF
    assert_true(rw_buffer_seek_write_end(&buf, 0));  // seek at offset 19
    assert_false(rw_buffer_write_u8(&buf, first));   // can't write 1 byte

    uint16_t second = 0x0102;
    uint8_t second_be[] = {0x01, 0x02};
    uint8_t second_le[] = {0x02, 0x01};
    assert_true(rw_buffer_seek_write_set(&buf, 0));          // set back to offset 0
    assert_true(rw_buffer_write_u16(&buf, second, BE));      // big endian
    assert_memory_equal(temp, second_be, 2);                 // 0x01 0x02
    assert_true(rw_buffer_seek_write_set(&buf, 0));          // set back to offset 0
    assert_true(rw_buffer_write_u16(&buf, second, LE));      // little endian
    assert_memory_equal(temp, second_le, 2);                 // 0x02 0x01
    assert_true(rw_buffer_seek_write_set(&buf, 7));          // seek at offset 7
    assert_false(rw_buffer_write_u16(&buf, second, BE));     // can't write 2 bytes

    uint32_t third = 0x03040506;
    uint8_t third_be[] = {0x03, 0x04, 0x05, 0x06};
    uint8_t third_le[] = {0x06, 0x05, 0x04, 0x03};
    assert_true(rw_buffer_seek_write_set(&buf, 0));          // set back to offset 0
    assert_true(rw_buffer_write_u32(&buf, third, BE));       // big endian
    assert_memory_equal(temp, third_be, 4);                  // 0x03 0x04 0x05 0x06
    assert_true(rw_buffer_seek_write_set(&buf, 0));          // set back to offset 0
    assert_true(rw_buffer_write_u32(&buf, third, LE));       // little endian
    assert_memory_equal(temp, third_le, 4);                  // 0x06 0x05 0x04 0x03
    assert_true(rw_buffer_seek_write_set(&buf, 5));          // seek at offset 5
    assert_false(rw_buffer_write_u32(&buf, third, BE));      // can't write 4 bytes

    uint64_t fourth = 0x0708090A0B0C0D0E;
    uint8_t fourth_be[] = {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
    uint8_t fourth_le[] = {0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07};
    assert_true(rw_buffer_seek_write_set(&buf, 0));          // set back to offset 0
    assert_true(rw_buffer_write_u64(&buf, fourth, BE));      // big endian
    assert_memory_equal(temp, fourth_be, 8);                 // 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E
    assert_true(rw_buffer_seek_write_set(&buf, 0));          // set back to offset 0
    assert_true(rw_buffer_write_u64(&buf, fourth, LE));      // little endian
    assert_memory_equal(temp, fourth_le, 8);                 // 0x0E 0x0D 0x0C 0x0B 0x0A 0x09 0x08 0x07
    assert_true(rw_buffer_seek_write_set(&buf, 1));          // seek at offset 1
    assert_false(rw_buffer_write_u64(&buf, fourth, BE));     // can't write 8 bytes
}

static void test_buffer_read_bytes(void **state) {
    (void) state;

    uint8_t output[5] = {0};
    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));

    assert_true(rw_buffer_read_bytes(&buf, output, sizeof(output)));
    assert_memory_equal(output, temp, sizeof(output));
    assert_int_equal(buf.read.offset, sizeof(output));

    uint8_t output2[3] = {0};
    assert_false(rw_buffer_read_bytes(&buf, output2, sizeof(output2)));  // can't read 3 bytes
}

static void test_buffer_copy_bytes(void **state) {
    (void) state;

    uint8_t output[5] = {0};
    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));

    assert_true(rw_buffer_copy_bytes(&buf, output, sizeof(output)));
    assert_memory_equal(output, temp, sizeof(output));

    uint8_t output2[3] = {0};
    assert_true(rw_buffer_seek_read_set(&buf, 2));
    assert_true(rw_buffer_copy_bytes(&buf, output2, sizeof(output2)));
    assert_memory_equal(output2, ((uint8_t[3]){0x03, 0x04, 0x05}), 3);
    assert_true(rw_buffer_seek_read_set(&buf, 3));                       // seek at offset 3
    assert_false(rw_buffer_copy_bytes(&buf, output2, sizeof(output2)));  // can't read 3 bytes
}

static void test_buffer_write_bytes(void **state) {
    (void) state;

    uint8_t input[2] = {0x01, 0x02};
    uint8_t temp[5] = {0};
    RW_BUFFER_FROM_ARRAY_EMPTY(buf, temp, sizeof(temp));

    assert_true(rw_buffer_write_bytes(&buf, input, sizeof(input)));
    assert_memory_equal(input, temp, sizeof(input));
    assert_int_equal(buf.read.size, sizeof(input));

    uint8_t input2[3] = {0x03, 0x04, 0x05};
    assert_true(rw_buffer_write_bytes(&buf, input2, sizeof(input2)));
    assert_int_equal(buf.read.size, sizeof(temp));
    uint8_t expected[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    assert_memory_equal(expected, temp, sizeof(expected));

    uint8_t input3[1] = {0x01};
    assert_false(rw_buffer_write_bytes(&buf, input3, sizeof(input3)));  // can't write bytes
    assert_int_equal(buf.read.size, sizeof(temp));                // nothing is changed
    assert_memory_equal(expected, temp, sizeof(expected));           // nothing is changed
}

static void test_buffer_shift_data(void **state) {
    (void) state;

    uint8_t temp[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t shift = 2;
    RW_BUFFER_FROM_ARRAY_FULL(buf, temp, sizeof(temp));
    assert_true(rw_buffer_seek_read_cur(&buf, shift));
    rw_buffer_shift_data(&buf);
    assert_int_equal(buf.read.size, sizeof(temp) - shift);
    assert_int_equal(buf.read.offset, 0);
    uint8_t expected[3] = {0x03, 0x04, 0x05};
    assert_memory_equal(expected, temp, sizeof(expected));
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_buffer_init),
                                       cmocka_unit_test(test_buffer_from_array_full),
                                       cmocka_unit_test(test_buffer_from_array_empty),
                                       cmocka_unit_test(test_buffer_from_var_full),
                                       cmocka_unit_test(test_buffer_from_var_empty),
                                       cmocka_unit_test(test_buffer_new_local_empty),
                                       cmocka_unit_test(test_buffer_empty),
                                       cmocka_unit_test(test_buffer_read_ptr),
                                       cmocka_unit_test(test_buffer_write_ptr),
                                       cmocka_unit_test(test_buffer_can_read),
                                       cmocka_unit_test(test_buffer_can_write),
                                       cmocka_unit_test(test_buffer_empty_space_len),
                                       cmocka_unit_test(test_buffer_data_len),
                                       cmocka_unit_test(test_buffer_read_position),
                                       cmocka_unit_test(test_buffer_write_position),
                                       cmocka_unit_test(test_buffer_seek_read),
                                       cmocka_unit_test(test_buffer_seek_write),
                                       cmocka_unit_test(test_buffer_read),
                                       cmocka_unit_test(test_buffer_read_bip32_path),
                                       cmocka_unit_test(test_buffer_write),
                                       cmocka_unit_test(test_buffer_read_bytes),
                                       cmocka_unit_test(test_buffer_copy_bytes),
                                       cmocka_unit_test(test_buffer_write_bytes),
                                       cmocka_unit_test(test_buffer_shift_data)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
