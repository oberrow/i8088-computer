BITS 16
ORG 0x0000

extern _entry

; Loop forever
_entry:
    jmp $
.end:

times 0x3fff0 - (_entry.end - _entry) db 0xcc

; At address 0xffff0
_rstvec:
    jmp 0xc000:_entry
.end:

times 0x40000 - ($ - $$) db 0xcc
