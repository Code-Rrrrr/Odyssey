#include <stdint.h>
#include "process.h"
/*
    here we have defined a kernel stack top,
    which would be used by the syscall_entry.S to execute the functions of the stack.
    this would be replaced by the technique which would later give a different stack top to 
    different processes.
*/
#define KERNEL_STACK_SIZE 4096

uint8_t kernel_stack[KERNEL_STACK_SIZE];
uint64_t kernel_stack_top;

// variables to make a process allocator 
// right now the allocator works incremently
// later this would be incorporated with reusing the pids
//and synchronization.
static uint64_t current_pid = 1;
static uint64_t next_pid = 2;


// to store the user stack, to return to in sysret.
uint64_t user_stack_pointer;

//save RCX and R11 properly, because those are the two registers 
// that SYSCALL itself overwrites and SYSRETQ needs to return to user space
uint64_t user_rip;
uint64_t user_rflags;

// the below array is for the process table, right
// now the array is of fixed size

# define MAX_PROCESS 64
static process_t process_table[MAX_PROCESS];

void process_int(void)
{

    // initialize the process table
    for(int i = 0; i < MAX_PROCESS; i++){
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_TERMINIATED;
    }

    // slot 0 is reserved for kernel process
    process_table[1].pid = 1;
    process_table[1].state = PROCESS_RUNNING;



    current_pid = 1;
    next_pid = 2;

    kernel_stack_top = 
        (uint64_t)&kernel_stack[KERNEL_STACK_SIZE];
}

uint64_t process_alloc_pid(void){
    if(next_pid == 0){
        return 0; // signifies the range has
        // exhausted
    }
    return next_pid++;
}

process_t *process_find(uint64_t pid){
    if(pid == 0)
    return 0;

    for(int i = 0; i < MAX_PROCESS; i++){
        if(process_table[i].pid == pid)
        return &process_table[i];
    }
    return 0;
}

// insert a new process in the process table 
// and mark it as ready
int process_register(uint64_t pid)
{
    if (pid == 0 || process_find(pid) != 0)
        return -1;

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i].pid == 0)
        {
            process_table[i].pid = pid;
            process_table[i].state = PROCESS_READY;
            return 0;
        }
    }

    return -1;
}

// creat the process
int process_create(void)
{
    uint64_t pid = process_alloc_pid();

    if (pid == 0 || 
        process_register(pid) != 0)
        return -1;

    process_t *process = process_find(pid);

    if (process == 0)
        return -1;

    process->stack_pointer = stack_alloc();

    if (process->stack_pointer == 0)
    {
        process->pid = 0;
        process->state = PROCESS_TERMINATED;
        return -1;
    }

    return (int)pid;
}


// putting in the scheduler logic
// this performs the round-robic selection
// it searches forward from the last selected 
// slot and returns the next READY process. It
//  reserves slot 0 for the kernel process, as
//   in our earlier design.
process_t *process_schedule_next(void)
{
    static int last_used_index = 0;

    for (int i = 1; i <= MAX_PROCESSES; i++)
    {
        int index = (last_used_index + i) % MAX_PROCESSES;

        if (process_table[index].pid != 0 &&
            process_table[index].state == PROCESS_READY)
        {
            last_used_index = index;
            return &process_table[index];
        }
    }

    return 0;
}



