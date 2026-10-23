.intel_syntax noprefix
.section .text

.global syscall_entry
.type syscall_entry, @function

;all these functions would be resolved
;by the linker, as they are defined
;elsewhere
.extern kernel_stack_top
.extern user_stack_pointer
.extern user_rip
.extern user_rflags

*syscall_entry*:

   ; Save state provided by SYSCALL.
    mov QWORD PTR [rip + user_rip], rcx
    mov QWORD PTR [rip + user_rflags], r11

  ; Save the user's stack pointer.
    mov QWORD PTR [rip + user_stack_pointer], rsp

  ; Get the top of the kernel stack.
  ; instead of this kernel stack top, we made a C function that initializes a global containing
  ; the stack top.
  ; we had to do this cause earlier the C call was being done in the user stack(RSP still points to the user stack.), but now we have a kernel stack.
  ; 
    ; call get_kernel_stack_top
    ; the above function is not needed as the corresponding
    ; function has been removed from the process.c

  ; Switch to the kernel stack.
    mov rsp, QWORD PTR [rip + kernel_stack_top]

  ; uptil here, is what is required by the syscall and sysret minimally,
  ; but we also need to care about the GPRs.(they are now cared for ;)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

  ; We are now executing with the kernel stack.
    mov rdi, rsp
    call syscall_dispatcher
    ;
    ;when this executes then:-
    ;
    ;here the dispatcher has put the PID
    ; into the saved RAX slot

     ; now we need to restore the GPRs
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ; the reverse order is do cause the top of the stack is
    ; r15
    ; here the rcx and r11 restored the user's original
    ;RCX
    ; hence the next lines(Restore the user data)
    mov rcx, QWORD PTR [rip + user_rip]
    mov r11, QWORD PTR [rip + user_rflags]
    mov rsp, QWORD PTR [rip + user_stack_pointer]

    sysretq