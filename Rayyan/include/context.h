// the above file 
#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>

uint64_t *context_init(
    uint64_t *stack_top,
    void (*entry)(void)
);

#endif