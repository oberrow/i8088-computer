; src/entry.asm

; Copyright (c) 2026 Omar Berrow

BITS 16

global _entry
extern entry
extern __data_end
extern __data_start

section .ivt
times 256 dd 0
section .stack
stack_bottom:
    times 0x1fff resb 0
stack_top:
section .text

_entry:
    cld
    cli
 
    xor ax, ax
    mov ss, ax
    mov sp, stack_top   

    mov ax, 0xf000
    mov ds, ax
    xor si,si ; si = 0
    ; Source is 0xf000:0x0000, aka 0xf0'0000 (ROM begin, __data_loadaddr)

    xor ax, ax
    mov es, ax
    mov di, __data_start
    ; Destination is 0x0000:__data_start, (RAM begin + __data_start)

    mov cx, __data_end
    mov ax, __data_start
    sub cx, ax
    ; Length is __data_end - __data_start

    rep movsb

    jmp entry
.end:

section .rstvec
; At address 0xffff0
_rstvec:
    jmp 0xf000:0x0000
.end:

times 0x10 - ($ - $$) db 0xcc
