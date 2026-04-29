ENTRY(_entry)

MEMORY {
    ram (rwx)  : ORIGIN = 0x0000, LENGTH = 64k
    rom (rx)   : ORIGIN = 0x0000, LENGTH = 256K
    rst (rx)   : ORIGIN = 0x3fff0, LENGTH = 16
}

SECTIONS
{
    . = 0x30000;
    _bss_begin = .;
    .bss (NOLOAD) : {
        *(.ivt)
        *(.stack)
        *(.bss)
        *(.bss.*)
        *(COMMON)
    } > ram
    . = ALIGN(4);
    _bss_end = .;
    .data : {
        __data_start = .;
        *(.data .data.*)
        *(.rodata) *(.rodata.*)
        __data_end = .;
        . = ALIGN(4);
    } >ram AT >rom
    __data_loadaddr = LOADADDR(.data);
    .text : { *(.text) *(.text.*) } > rom
    .flash : { *(.flash) *(.flash.*) } > rom
    . = 0xffff0;
    .rstvec : { *(.rstvec) } > rst
}