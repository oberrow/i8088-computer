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
global clis
global resf
global get_flags
global hlt
global delay_cycles

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

clis:
    cli
    pushf
    pop ax
    ret
resf:
    push bp
    mov bp, sp

    mov ax, ss:[bp+0x4]
    push ax
    popf

    mov sp, bp
    pop bp
    ret
get_flags:
    pushf
    pop ax
    ret
hlt:
    hlt
    ret

delay_cycles:
    push bp
    mov bp, sp

    mov cx, ss:[bp+0x4]

.loop:
    loop .loop

    mov sp, bp
    pop bp
    ret