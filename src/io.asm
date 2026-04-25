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
    push bp
    mov bp, sp

    mov ax, ss:[bp+0x6]
    mov dx, ss:[bp+0x4]
    out dx, al
    
    leave
    ret

outw:
    push bp
    mov bp, sp
    
    mov ax, ss:[bp+0x6]
    mov dx, ss:[bp+0x4]
    out dx, ax
    
    leave
    ret

inb:
    push bp
    mov bp, sp
    
    mov dx, ss:[bp+0x4]
    in al, dx
    
    leave
    ret

inw:
    push bp
    mov bp, sp
    
    mov dx, ss:[bp+0x4]
    in ax, dx
    
    leave
    ret

cli: 
    cli
    ret
sti:
    sti
    ret