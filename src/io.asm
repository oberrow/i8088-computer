; src/io.asm

; Copyright (c) 2026 Omar Berrow

BITS 16

global outb
global outw
global inb
global inw
global cli
global sti

outb:
    pop dx
    pop ax
    out dx, al
    ret
outw:
    pop dx
    pop ax
    out dx, ax
    ret
inb:
    pop dx
    in al, dx
    ret
inw:
    pop dx
    in ax, dx
    ret
cli: 
    cli
    ret
sti:
    sti
    ret