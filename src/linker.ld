ENTRY(_entry)

MEMORY {
    ram (rwx) : ORIGIN = 0x00400, LENGTH = 0x10000-0x400
    rom (rx)  : ORIGIN = 0xf0000, LENGTH = 64k
    rst (rx)  : ORIGIN = 0xffff0, LENGTH = 16
}

SECTIONS
{
    .ivt (NOLOAD) : { *(.ivt) } > ram
    _bss_begin = .;
    .bss (NOLOAD) : { *(.bss) *(.bss.*) *(COMMON) } > ram
    . = ALIGN(4);
    _bss_end = .;
    .stack (NOLOAD) : { *(.stack) } > ram
    . = 0xf0000;
    .data : {
        __data_start = .;
        *(.data .data.*)
        *(.rodata) *(.rodata.*)
        __data_end = .;
        . = ALIGN(4);
    } >ram AT >rom
    __data_loadaddr = LOADADDR(.data);
    .text : { *(.text) *(.text.*) } > rom
    . = 0xffff0;
    .rstvec : { *(.rstvec) } > rst
}