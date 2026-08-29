/*
this is a header file. The process header describes the CPU state associated with a syscall.
include guard.    
*/
#ifndef PROCESS_H
#define PROCESS_H
/*
    ifndef = if not defined
    If PROCESS_H has NOT already been defined, execute everything until #endif

*/
#include <stdint.h>

/*

right now there are different global variables for different registers.hat was useful for understanding the 
mechanism, but it will 
become a mess as soon as we implement fork() or have multiple processes/CPUs.
so now we make a struct so that we can have a complete snapshot of cpu state at syscall.

    extern uint64_t kernel_stack_top;
extern uint64_t user_stack_pointer;

extern uint64_t user_rip;
extern uint64_t user_rflags;

void process_init(void);
*/


struct syscall_frame {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;

    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t user_rip;
    uint64_t user_rflags;
    uint64_t user_rsp;
};


#endif