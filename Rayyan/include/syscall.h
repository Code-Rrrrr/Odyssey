#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_GETPID 0

struct syscall_frame;

uint64_t sys_getpid(void);
void syscall_dispatcher(struct syscall_frame *frame);

#endif