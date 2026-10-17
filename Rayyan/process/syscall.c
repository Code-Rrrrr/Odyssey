#include <stdint.h>
#include <syscall.h>

void syscall_dispatcher(struct syscall_frame *frame)
{
    switch (frame->rax) {
        case SYS_GETPID:
            frame->rax = sys_getpid();
            break;
        
        default: 
            frame->rax = (unit64_t) - 1;
            break;
    }
}

uint64_t sys_getpid(void)
{
    return 1;
}