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

global stack_top
global stack_bottom

section .ivt
ivt:
    times 256 dd 0
section .stack
stack_bottom:
    times 0x1000 db 0
stack_top:
section .text

_entry:
    cld
    cli
    mov sp, stack_top

%ifndef NO_PROBE

; We made it here, probe the RAM through the system probe port 0x5000

    mov dx, 0x5000
    in al, dx
    and al, 0x3
    xor al, 0x3
    jz .probe_writeability
    jmp .abort

.probe_writeability:

; Test if 0x00000 is writeable
; RAM size will be determined in probe_io_bus

    mov ax, 0xdead
    mov [0x00000], ax
    mov ax, [0x00000]
    cmp ax, 0xdead
    jne .abort ; We do not actually have a RAM chip on 0x00000, abort.

    jmp entry

.abort:
    hlt
    jmp .abort
%else
    jmp entry
%endif

section .rstvec
; At address 0xffff0
_rstvec:
    jmp 0xc000:_entry
.end:

times 0x10 - ($ - $$) db 0xff
