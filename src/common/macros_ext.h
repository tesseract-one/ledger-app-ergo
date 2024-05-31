#pragma once

#include <macros.h>

/**
 * Macro for disabling inlining for function (GCC & Clang)
 */
#define NOINLINE __attribute__((noinline))

/**
 * Macros for checking indexes
 */
#define INDEX_NOT_EXIST         ((uint8_t) 0xFF)
#define IS_ELEMENT_FOUND(index) ((uint8_t) (index) != INDEX_NOT_EXIST)