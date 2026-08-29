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

.extern get_kernel_stack_top
.extern user_stack_pointer

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
    call get_kernel_stack_top

    # Switch to the kernel stack.
    mov kernel_stack_top(%rip), %rsp

    # uptil here, is what is required by the syscall and sysret minimally, 
    # but we also need to care about the GPRs.

    # We are now executing with the kernel stack.