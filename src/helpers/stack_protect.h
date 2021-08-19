#pragma once

// This symbol is defined by the link script to be at the start of the stack
// area.
extern unsigned long _stack;

// Init value for stack overflow checking
void init_canary(void);

// Check for stack overflow
void check_canary(void);