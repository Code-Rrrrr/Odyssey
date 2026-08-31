; memory model used in this program
; code segment = 0
; data segment = 0
; only 0x0000 to 0xffff physical memory will be used
; total 64 KB of physical memory will be used

bits 16
org 0x7c00

start:
    jmp 0x0000:main    ; jump to instruction at label main and change code segment (cs) to 0x0000
        main:
            cli    ; disable interrupt
            xor ax, ax    ; set ax to zero
            mov ds, ax    ; set ds to zero
            mov es, ax    ; set es to zero
            mov ss, ax    ; set ss to zero
            mov sp, 0xffff    ; set stack pointer to highest memory (0xffff = 65535)
            sti    ; enable interrupt
            cld    ; clear direction flag

            mov [boot_drive], dl

            ; ====================
            ; WRITE YOUR PROGRAM HERE

            ; DS:SI points to Disk Address Packet (DAP)
            mov si, dap
            call read_disk

            jc hang    ; if read operation fails then carry flag is set by BIOS

            jmp 0x0000:0x7e00

            hang:
                hlt
                jmp hang
            ; ====================

; ====================
; DEFINE ALL FUNCTIONS HERE

read_disk:
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc .error
    mov si, success_msg
    call print
    clc        ; clear carry flag to indicate read opeartion sucess because it can overwrite when print function is used
    ret

    .error:
        mov si, error_msg
        call print
        stc    ; set carry flag to indicate read opeartion fail because it can overwrite when print function is used 
        ret

print:   ; uses QEMU 0xe9 debug port for printing
    .next:
        lodsb
        cmp al, 0
        jz .done
        out 0xe9, al
        jmp .next
    .done:
        ret

; ====================

; ====================
boot_drive db 0
; DATA SECTION
; PUT ALL DATA HERE

success_msg db "disk read successful", 0
error_msg db "disk read failed", 0

dap:
    db 0x10    ; size of DAP, 16 bytes
    db 0x00    ; reserved, always 0
    dw 0x01    ; number of sectors to read
    dw buffer    ; memory buffer to store data, low 16 bits is offset
    dw 0x00    ; high 16 bits is segment
    dq 0x01    ; starting LBA

buffer equ 0x7e00

; ====================

times 510 - ($ - $$) db 0
dw 0xaa55