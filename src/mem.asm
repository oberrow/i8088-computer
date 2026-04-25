; src/mem.asm

; Copyright (c) 2026 Omar Berrow

BITS 16
CPU 8086

global memcpy_far
global set_ds
global set_es

memcpy_far:
    push bp
    mov bp, sp

    mov di, ss:[bp+0x4]
    mov ax, ss:[bp+0x6]
    mov es, ax
    mov di, ss:[bp+0x8]
    mov ax, ss:[bp+0xa]
    mov ds, ax
    mov cx, ss:[bp+0xc]
    rep movsb
    
    mov ax, ss:[bp+0x4]

    mov sp, bp
    pop bp
    ret

set_ds:
    push bp
    mov bp, sp

    mov ax, ss:[bp+0x4]
    mov ds, ax
    
    mov sp, bp
    pop bp
    ret

set_es:
    push bp
    mov bp, sp

    mov ax, ss:[bp+0x4]
    mov es, ax
    
    mov sp, bp
    pop bp
    ret