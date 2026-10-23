// this is a temporary file
// and will be later replaced
// by Anas' implementation of the stack 
// allocator.
#include <stdint.h>
#include "stack.h"

#define MAX_STACKS 64

static uint8_t stack_pool[MAX_STACKS][PROCESS_STACK_SIZE]
    __attribute__((aligned(16)));

static uint8_t stack_used[MAX_STACKS] = {0};

uint64_t *stack_alloc(void)
{
    for (int i = 0; i < MAX_STACKS; i++)
    {
        if (stack_used[i] == 0)
        {
            stack_used[i] = 1;

            return (uint64_t *)
                (stack_pool[i] + PROCESS_STACK_SIZE);
        }
    }

    return 0;
}

void stack_free(uint64_t *stack_pointer)
{
    if (stack_pointer == 0)
        return;

    for (int i = 0; i < MAX_STACKS; i++)
    {
        if (stack_pointer ==
            (uint64_t *)(stack_pool[i] + PROCESS_STACK_SIZE))
        {
            stack_used[i] = 0;
            return;
        }
    }
}