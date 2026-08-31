; stage 2 bootloader is loaded at memory location 0x7e00

bits 16
org 0x7e00


start:
    main:


hang:
    hlt
    jmp hang



; times 512 - ($ - $$) db 0
; use this to make stage 2 exactly one sector

; no need of boot signature