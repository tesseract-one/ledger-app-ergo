#pragma once

#include <stdint.h>
#include <os.h>
#include "../sw.h"

#ifdef TARGET_NANOS
// This symbol is defined by the link script to be at the start of the stack
// area.
extern unsigned long _stack;

#define STACK_CANARY (*((volatile uint32_t*) &_stack))

// Init value for stack overflow checking
static inline void init_canary() {
    STACK_CANARY = 0xDEADBEEF;
}

// Check for stack overflow
static inline void check_canary() {
    if (STACK_CANARY != 0xDEADBEEF) THROW(SW_STACK_OVERFLOW);
}
#else
static inline void init_canary() {
}

static inline void check_canary() {
}
#endif