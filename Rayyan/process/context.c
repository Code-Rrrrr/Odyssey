#include <stdint.h>
#include "context.h"

static void process_exit_trampoline(void)
{
    for (;;)
    {
        __asm__ volatile ("cli; hlt");
    }
}

uint64_t *context_init(
    uint64_t *stack_top,
    void (*entry)(void)
)
{
    if (stack_top == 0 || entry == 0)
        return 0;

    uint64_t *sp = stack_top;

    *(--sp) = (uint64_t)process_exit_trampoline;
    *(--sp) = (uint64_t)entry;

    return sp;
}