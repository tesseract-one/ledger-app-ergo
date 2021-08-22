#pragma once

/**
 * Macro for the size of a specific structure field.
 */
#define MEMBER_SIZE(type, member) (sizeof(((type *) 0)->member))

/**
 * Macro for disabling inlining for function (GCC & Clang)
 */
#define NOINLINE __attribute__((noinline))