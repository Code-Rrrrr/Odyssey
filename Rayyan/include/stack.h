// this is a temporary file
// and will be later replaced
// by Anas' implementation of the stack 
// allocator.

#ifndef STACK_H
#define STACK_H

#include <stdint.h>

#define PROCESS_STACK_SIZE 4096

uint64_t *stack_alloc(void);
void stack_free(uint64_t *stack_pointer);

#endif