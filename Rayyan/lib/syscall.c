long getpid(void){

    long result;// store the value that kernel returns.
    //that is the actual process id.

    asm volatile(
        "syscall"
        :(=a)(result)
        :(a)(SYS_GETPID)
        :"rcx", "r11", "memory"
    );

    return result;
}