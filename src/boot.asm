; src/boot.asm

; Copyright (c) 2026 Omar Berrow

BITS 16
CPU 8086

global btldr_handoff
btldr_handoff:
    push bp
    mov bp, sp

    cli
    cld

    mov ax, [bp+4]
    
    xor bp,bp
    xor sp,sp
    xor bx,bx
    xor cx,cx
    xor dx,dx
    xor si,si
    xor di,di
    
    jmp 0x00:0xfe00