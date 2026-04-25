BITS 16
ORG 0x0000

extern _entry

; Loop forever
_entry:
    jmp $
.end:

times 0xfff0 - (_entry.end - _entry) db 0xcc

; At address 0xffff0
_rstvec:
    jmp 0xf000:_entry
.end:

times 0x10000 - ($ - $$) db 0xcc
