; src/entry.asm

; Copyright (c) 2026 Omar Berrow

BITS 16

global _entry
extern entry

_entry:
    jmp entry
.end:

section .rstvec
; At address 0xffff0
_rstvec:
    jmp 0xf000:0x0000
.end:

times 0x10 - ($ - $$) db 0xcc
