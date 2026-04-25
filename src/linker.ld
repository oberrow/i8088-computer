ENTRY(_entry)

SECTIONS
{
    . = 0xf0000;
    .text : { *(.text) *(.text.*) }
    .data : { *(.rodata) *(.rodata.*) *(.data) *(.data.*) }
    .bss  : { *(.bss) *(.bss.*) *(COMMON) }
    . = 0xffff0;
    .rstvec : { *(.rstvec) }
    /DISCARD/ : { *(*) }
}