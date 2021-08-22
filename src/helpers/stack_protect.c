#include "stack_protect.h"
#include <os.h>
#include "../sw.h"

#define STACK_CANARY (*((volatile uint32_t*) &_stack))

void init_canary() {
    STACK_CANARY = 0xDEADBEEF;
}
void check_canary() {
    if (STACK_CANARY != 0xDEADBEEF) THROW(SW_STACK_OVERFLOW);
}