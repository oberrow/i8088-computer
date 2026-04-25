; src/io.asm

; Copyright (c) 2026 Omar Berrow

BITS 16
CPU 8086

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
    
    mov sp, bp
    pop bp
    ret

outw:
    push bp
    mov bp, sp
    
    mov ax, ss:[bp+0x6]
    mov dx, ss:[bp+0x4]
    out dx, ax
    
    mov sp, bp
    pop bp
    ret

inb:
    push bp
    mov bp, sp
    
    mov dx, ss:[bp+0x4]
    in al, dx
    
    mov sp, bp
    pop bp
    ret

inw:
    push bp
    mov bp, sp
    
    mov dx, ss:[bp+0x4]
    in ax, dx
    
    mov sp, bp
    pop bp
    ret

cli: 
    cli
    ret
sti:
    sti
    ret