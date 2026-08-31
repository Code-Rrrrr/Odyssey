
; ==========================
; USEFUL FUNCTIONS FOR 16 BIT REAL MODE
; ==========================

; function - 
;     - use to print a single 8 bit character
;     - uses 0xe9 port for printing

; usage -
;     - copy character to al register
;     - call function

; clobbers -
;     - al

putc:
    out 0xe9, al
    ret

; ==========================

; function -
;     - use to print a null terminated string
;     - uses 0xe9 port for printing

; usage - 
;     - define a null terminated string in memory
;     - point si to address of null terminated string
;     - call function

; clobbers -
;     - al, si

print:
    .next:
        lodsb
        cmp al, 0
        jz .done
        out 0xe9, al
        jmp .next
    .done:
        ret

; ==========================

; function -
;     - use to print content of a register
;     - prints all 16 bits as hex
;     - uses 0xe9 port for printing

; usage -

;     - define a buffer in memory
;         buffer dw 0

;     - copy value of register to buffer
;         mov [buffer], dx
    
;     - call function
;         call printregister

printregister:
    mov cx, [buffer]
    mov dx, 4
    .printloop:
        rol cx, 4
        mov al, cl
        and al, 0x0f
        mov bx, table
        xlat
        out 0xe9, al
        dec dx
        jnz .printloop
        ret

table db '0123456789ABCDEF'

; ==========================

; function -
;     - use to print content of a register
;     - prints all 16 bits as hex
;     - uses 0xe9 port for printing

; usage -

;     - define a buffer in memory
;         buffer dw 0

;     - copy value of register to buffer
;         mov [buffer], dx
    
;     - call function
;         call printregister

; example -

;     - value stored in 16 bit register = 0xABCD

;                     +--------------+  low memory
;           buffer -> |      CD      |
;                     +--------------+
;       buffer + 1 -> |      AB      |
;                     +--------------+  high memory
                
;      - x86 is little endian
;      - x86 is byte addressal (one memory location holds 1 byte)


printregister:
    mov bx, table
    mov al, [buffer+1]
    shr al, 4
    xlat
    out 0xe9, al    ; first nibble printed
;-------------------------------------- 
    mov al, [buffer+1]
    and al, 0xf
    xlat
    out 0xe9, al    ; second nibble printed
;--------------------------------------
    mov al, [buffer]
    shr al, 4
    xlat
    out 0xe9, al    ; third nibble printed
;--------------------------------------
    mov al, [buffer]
    and al, 0xf
    xlat
    out 0xe9, al    ; fourth nibble printed
;--------------------------------------
    ret
    table db '0123456789ABCDEF'

; ==========================

; function -
;     - use to read from disk
;     - uses bios interrupt int 0x13, 0x42

; usage -
;     - define a disk address packet (DAP) in memory
;         dap:
;             db 0x10    ; size of DAP, 16 bytes
;             db 0x00    ; reserved, always 0
;             dw 0x01    ; number of sectors to read
;             dw buffer    ; memory buffer to store data, low 16 bits is offset
;             dw 0x00    ; high 16 bits is segment
;             dq 0x01    ; starting LBA

;     - point si to DAP
;         mov si, dap

;     - define a location in memory that contains bootdrive number
;         boot_drive db 0

;     - define a buffer in memory where data is to be stored after reading from disk
;         buffer equ 0x7e00
    
;     - call function

; status -
;   - carry flag = 0 -> read operation completed sucessfully
; 	- carry flag = 1 -> there is error during read operation

; clobbers -
;     - ah, si, cl

read_disk:
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc .error
    mov si, success_msg
    call print
    ret

    .error:
        mov si, error_msg
        call print
        ret

success_msg db "disk read successful", 0
error_msg db "disk read failed", 0

; ==========================

; - use this to shut down virtual machine
; - works only in qemu
; - not works on real hardware

mov dx, 0x604
mov ax, 0x2000
out dx, ax

; ==========================

; ==========================