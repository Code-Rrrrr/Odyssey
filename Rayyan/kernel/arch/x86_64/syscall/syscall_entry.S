# conceptually it is doing this:
; syscall_entry:

;     # CPU gave us:
;     # RCX = user RIP
;     # R11 = user RFLAGS

;     # Save user RSP

;     # Switch to kernel stack

;     # Save registers into syscall_frame

;     # RDI = pointer to syscall_frame
;       That's because under the normal x86-64 C calling convention, 
;       the first argument to a C function goes in RDI.

;     call syscall_dispatcher

;     # Restore registers

;     # Restore user RSP

;     # RCX = user RIP
;     # R11 = user RFLAGS

;     sysretq
.section .text

.global syscall_entry
.type syscall_entry, @function

.extern kernel_stack_top
.extern user_stack_pointer
.extern user_rip
.extern user_rflags
; these variables are declared in process.c but this assembly file needs
; to tell the assembler/linker that they are external files

syscall_entry:

     # Save state provided by SYSCALL.
    mov %rcx, user_rip(%rip)
    mov %r11, user_rflags(%rip)

    # Save the user's stack pointer.
    mov %rsp, user_stack_pointer(%rip)

    # Get the top of the kernel stack.
    # instead of this kernel stack top, we made a C function that initializes a global containing 
    # the stack top. 
    # we had to do this cause earlier the C call was being done in the user stack(RSP still points 
    # to the user stack.), but now we have a kernel stack.
    # 
    ; call get_kernel_stack_top
    ; the above function is not needed as the corresponding 
    ; function has been removed from the process.c

    # Switch to the kernel stack.
    mov kernel_stack_top(%rip), %rsp

    # uptil here, is what is required by the syscall and sysret minimally, 
    # but we also need to care about the GPRs.(they are now cared for ;)
    push %rax
push %rbx
push %rcx
push %rdx
push %rsi
push %rdi
push %rbp
push %r8
push %r9
push %r10
push %r11
push %r12
push %r13
push %r14
push %r15

    # We are now executing with the kernel stack.
    mov %rsp, %rdi
    call syscall_dispatcher
    ;
    ;when this executes then:- 
    ;
    ;here the dispatcher has put the PID 
    ; into the saved RAX slot

     ; now we need to restore the GPRs
pop %r15
pop %r14
pop %r13
pop %r12
pop %r11
pop %r10
pop %r9
pop %r8
pop %rbp
pop %rdi
pop %rsi
pop %rdx
pop %rcx
pop %rbx
pop %rax
; the reverse order is do cause the top of the stack is
; r15
; here the rcx and r11 restored the user's original
;RCX 

; hence the next lines(Restore the user data)
mov user_rip(%rip), %rcx
mov user_rflags(%rip), %r11
mov user_stack_pointer(%rip), %rsp

sysretq
