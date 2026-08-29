long getpid(void){

    long ret;// store the value that kernel returns.

    asm volatile(
        "syscall"
        :(=a)(ret)
        :(a)(SYS_GETPID)
        :"rcx", "r11", "memory"
    );

    return ret;
}