
; ==========================================
; nasm -f bin -l home.lst home.asm -o home.bin
; qemu-system-x86_64 -m 16M -drive format=raw,file=text.bin
; ==========================================

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

            mov ah, 0x00    ; set mode
            mov al, 0x03    ; requested mode
            int 0x10        ; video interrupt

            ; VGA memory segment
            mov ax,0xB800
            mov es,ax


            ; Write H
            mov word [es:0x0000],0x0748

            ; Write E
            mov word [es:0x0002],0x0745

            ; Write L
            mov word [es:0x0004],0x074C

            ; Write L
            mov word [es:0x0006],0x074C

            ; Write O
            mov word [es:0x0008],0x074F


            mov word [es:0x000c],0x1F48   ; H white/blue
            mov word [es:0x0010],0x2E45   ; E yellow/green
            mov word [es:0x0014],0x4C4C   ; L red
            mov word [es:0x0018],0x5A4C   ; L magenta
            mov word [es:0x001c],0x3F4F   ; O white/cyan

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