; src/ivt.asm

; Copyright (c) 2026 Omar Berrow

BITS 16
CPU 8086

%macro define_isr_handler 1
align 8
global isr%1
isr%1:
	push ax
	mov ax, %1
	push ax
	jmp int_handler_common
%endmacro

define_isr_handler 0
define_isr_handler 1
define_isr_handler 2
define_isr_handler 3
define_isr_handler 4
define_isr_handler 32
define_isr_handler 33
define_isr_handler 34
define_isr_handler 35
define_isr_handler 36
define_isr_handler 37
define_isr_handler 38
define_isr_handler 39
define_isr_handler 40

extern irq_handlers

int_handler_common:
	cld

    push cx
    push dx
    push bx
    push bp
    push si
    push di

	push ds
	push es
	push ss

	mov bp, sp
	
	mov bx, ss:[bp+18]
	shl bx, 1
	lea bx, [bx+irq_handlers]
	push ds

	xor ax,ax
	mov ds, ax
	mov es, ax

	push sp
	mov bx, cs:[bx]
	call bx

	pop ss
	pop es
	pop ds
	pop di
	pop si
	pop bp
	pop bx
	pop dx
	pop cx

	add sp, 2
	pop ax
	iret