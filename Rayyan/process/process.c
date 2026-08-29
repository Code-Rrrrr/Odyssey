#include <stdint.h>
/*
    here we have defined a kernel stack top,
    which would be used by the syscall_entry.S to execute the functions of the stack.
    this would be replaced by the technique which would later give a different stack top to 
    different processes.
*/
#define KERNEL_STACK_SIZE 4096

uint8_t kernel_stack[KERNEL_STACK_SIZE];
uint64_t kernel_stack_top;

uint64_t user_stack_pointer;// to store the user stack, to return to in sysret.

//save RCX and R11 properly, because those are the two registers 
// that SYSCALL itself overwrites and SYSRETQ needs to return to user space
uint64_t user_rip;
uint64_t user_rflags;

void process_int(void)
{
    kernel_stack_top = 
        &kernel_stack[KERNEL_STACK_SIZE];
}