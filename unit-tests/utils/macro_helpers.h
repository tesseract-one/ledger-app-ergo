// macros that can't be passed through CMake
#pragma once

// UNUSED macro. Defined in the Makefile
#define UNUSED(x) (void)(x)


#define BUFFER_FROM_ARRAY(_name, _array, _size)             \
    RW_BUFFER_FROM_ARRAY_FULL(__rw_##_name, _array, _size); \
    buffer_t _name = __rw_##_name.read


#define BUFFER_NEW_LOCAL_EMPTY(_name, _size)          \
    RW_BUFFER_NEW_LOCAL_EMPTY(__rw_##_name, _size);   \
    buffer_t _name = __rw_##_name.read