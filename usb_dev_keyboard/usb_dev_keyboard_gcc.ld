__stack_size = 1024;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 32K
}

SECTIONS
{
    .vectors : ALIGN(4)
    {
        KEEP(*(.vectors))
    } > FLASH

    .text : ALIGN(4)
    {
        *(.text*)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
    } > FLASH

    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab*)
    } > FLASH

    .ARM : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    __data_load = LOADADDR(.data);
    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data*)
        __data_end = .;
    } > SRAM AT > FLASH

    .bss : ALIGN(4)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > SRAM

    .stack : ALIGN(8)
    {
        __stack_start = .;
        . = . + __stack_size;
        . = ALIGN(8);
        __stack_top = .;
    } > SRAM

    /DISCARD/ :
    {
        *(.ARM.attributes*)
    }
}
