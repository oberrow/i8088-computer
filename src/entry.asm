; src/entry.asm

; Copyright (c) 2026 Omar Berrow

BITS 16
CPU 8086

global _entry
global set_ds
global set_es

extern entry
extern __data_end
extern __data_start

section .ivt
ivt:
    times 256 dd 0
section .stack
stack_bottom:
    times 0x2000 db 0
stack_top:
section .text

_entry:
    cld
    cli
    jmp entry

section .rstvec
; At address 0xffff0
_rstvec:
    jmp 0xf000:0x0000
.end:

times 0x10 - ($ - $$) db 0xcc
