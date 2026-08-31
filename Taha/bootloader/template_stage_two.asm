; ============================================================
; Stage 2 bootloader
;
; Loaded by stage 1 at physical address 0x7E00
;
; Execution starts at:
;     CS:IP = 0000:7E00
;
; Initial mode:
;     16-bit real mode
;
; Responsibilities:
;     1. Initialize environment
;     2. Obtain system information
;     3. Load kernel
;     4. Enable A20
;     5. Prepare GDT
;     6. Prepare paging
;     7. Enter 64-bit long mode
;     8. Transfer control to kernel
; ============================================================

bits 16
org 0x7e00


; ============================================================
; ENTRY POINT
; ============================================================

start:

    ; --------------------------------------------------------
    ; Basic CPU setup
    ; --------------------------------------------------------

    cli

    xor ax, ax

    mov ds, ax
    mov es, ax
    mov ss, ax

    mov sp, 0x7c00

    cld


    ; --------------------------------------------------------
    ; Stage 2 initialization
    ; --------------------------------------------------------

    call init


    ; --------------------------------------------------------
    ; Print startup message
    ; --------------------------------------------------------

    mov si, stage2_msg
    call print


    ; --------------------------------------------------------
    ; Get system information
    ; --------------------------------------------------------

    call detect_memory


    ; --------------------------------------------------------
    ; Enable A20
    ; --------------------------------------------------------

    call enable_a20


    ; --------------------------------------------------------
    ; Load kernel from disk
    ; --------------------------------------------------------

    call load_kernel


    ; --------------------------------------------------------
    ; Prepare GDT
    ; --------------------------------------------------------

    call setup_gdt


    ; --------------------------------------------------------
    ; Prepare paging
    ; --------------------------------------------------------

    call setup_page_tables


    ; --------------------------------------------------------
    ; Enter protected mode
    ; --------------------------------------------------------

    call enter_protected_mode


    ; --------------------------------------------------------
    ; Execution should never return here
    ; --------------------------------------------------------

.hang:
    cli
    hlt
    jmp .hang


; ============================================================
; INITIALIZATION
; ============================================================

init:

    ; Future initialization goes here

    ret


; ============================================================
; PRINT STRING
;
; Input:
;     DS:SI -> zero-terminated string
;
; Uses BIOS INT 10h
; ============================================================

print:

.next:
    lodsb

    test al, al
    jz .done

    mov ah, 0x0e
    mov bh, 0

    int 0x10

    jmp .next

.done:
    ret


; ============================================================
; MEMORY DETECTION
; ============================================================

detect_memory:

    ; TODO:
    ; INT 15h, E820h

    ret


; ============================================================
; ENABLE A20
; ============================================================

enable_a20:

    ; TODO:
    ; Enable A20 line

    ret


; ============================================================
; LOAD KERNEL
; ============================================================

load_kernel:

    ; TODO:
    ;
    ; Read kernel from disk using INT 13h extensions.
    ;
    ; Example:
    ;
    ;     kernel LBA = ...
    ;     kernel size = ...
    ;     kernel address = ...

    ret


; ============================================================
; SETUP GDT
; ============================================================

setup_gdt:

    ; TODO:
    ; Create GDT
    ;
    ; Example:
    ;
    ; GDT entry 0 = null
    ; GDT entry 1 = 32-bit code
    ; GDT entry 2 = 32-bit data
    ; GDT entry 3 = 64-bit code
    ; GDT entry 4 = 64-bit data

    ret


; ============================================================
; SETUP PAGE TABLES
; ============================================================

setup_page_tables:

    ; TODO:
    ; Create PML4 / PDPT / PD / PT
    ;
    ; For a simple kernel, identity mapping
    ; is a good starting point.

    ret


; ============================================================
; ENTER PROTECTED MODE
; ============================================================

enter_protected_mode:

    ; TODO:
    ;
    ; 1. Disable interrupts
    ; 2. Load GDT
    ; 3. Set CR0.PE
    ; 4. Far jump to 32-bit code

    ret


; ============================================================
; 32-BIT CODE
; ============================================================

bits 32

protected_mode_entry:

    ; TODO:
    ;
    ; Set 32-bit data segments
    ; Prepare long-mode transition
    ; Enable PAE
    ; Load CR3
    ; Enable long mode
    ; Enable paging
    ; Far jump to 64-bit code


; ============================================================
; 64-BIT CODE
; ============================================================

bits 64

long_mode_entry:

    ; TODO:
    ;
    ; Set 64-bit data segments
    ; Set up RSP
    ; Set up kernel environment
    ; Jump to kernel


    ; Example:
    ;
    ; mov rsp, kernel_stack
    ; jmp kernel_entry


.hang:
    cli
    hlt
    jmp .hang


; ============================================================
; DATA
; ============================================================

bits 16

stage2_msg:
    db "Stage 2 started", 13, 10, 0


; ============================================================
; DAP
; ============================================================

dap:

    db 0x10
    db 0x00

    dw 0x01              ; sectors to read

    dw 0x0000            ; buffer offset
    dw 0x0000            ; buffer segment

    dq 0x0000000000000000 ; LBA


; ============================================================
; GDT
; ============================================================

gdt_start:

    ; Null descriptor
    dq 0


    ; TODO: 32-bit code descriptor
    dq 0


    ; TODO: 32-bit data descriptor
    dq 0


    ; TODO: 64-bit code descriptor
    dq 0


    ; TODO: 64-bit data descriptor
    dq 0


gdt_end:


gdt_descriptor:

    dw gdt_end - gdt_start - 1
    dd gdt_start


; ============================================================
; PAGE TABLES
; ============================================================

align 4096

pml4:
    times 512 dq 0

pdpt:
    times 512 dq 0

pd:
    times 512 dq 0


; ============================================================
; KERNEL STACK
; ============================================================

align 16

kernel_stack:
    times 4096 db 0

kernel_stack_top:


; ============================================================
; END OF STAGE 2
; ============================================================
