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

global timer_irq

define_isr_handler 0
define_isr_handler 1
define_isr_handler 2
define_isr_handler 3
define_isr_handler 4
;define_isr_handler 32
define_isr_handler 33
define_isr_handler 34
define_isr_handler 35
define_isr_handler 36
define_isr_handler 37
define_isr_handler 38
define_isr_handler 39
define_isr_handler 40

extern irq_handlers

global impossible_isr

impossible_isr:
	cli
.l:
	hlt
	jmp .l

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

	xor ax,ax
	mov ds, ax
	mov es, ax

	push sp
	mov bx, cs:[bx]
	cmp bx, 0
	je .abort
	call bx

.abort:
	add sp, 2

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

%ifdef NO_PROBE
; IRQ Line 0 is connected to a 1000hz square wave.
timer_irq:
    push ax
    push dx
    mov al, 0x20
    mov dx, 0x20
    out dx, al
    pop dx
    pop ax
    add [current_tick_ms], 1
    adc [current_tick_ms+2], 0
    iret
%else
timer_irq:
	push ax
	mov ax, 0x20
	push ax
	jmp int_handler_common
%endif