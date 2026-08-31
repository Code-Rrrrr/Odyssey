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
            mov sp, 0xffff    ; set stack pointer to highest memory
            sti    ; enable interrupt
            cld    ; clear direction flag

            mov [boot_drive], dl

            ; ====================
            ; WRITE YOUR PROGRAM HERE

            hang:
                hlt
                jmp hang
            ; ====================

; ====================
; DEFINE ALL FUNCTIONS HERE
; ====================

; ====================
boot_drive db 0
; DATA SECTION
; PUT ALL DATA HERE
; ====================

times 510 - ($ - $$) db 0
dw 0xaa55