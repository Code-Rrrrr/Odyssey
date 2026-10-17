#include "process.h"

void syscall_init(void);

void kernel_main(void)
{
    process_init();
    syscall_init();

    // Continue kernel initialization here.
}