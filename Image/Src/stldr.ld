/*
 * CMSIS Flash Algorithm linker script for STM32H743 + W25Q64.
 * pyocd relocates PrgCode/PrgData into target RAM at load time,
 * so the addresses here just need to be self-consistent.
 */

MEMORY
{
    RAM (xrw) : ORIGIN = 0x00000000, LENGTH = 128K
}

ENTRY(Init)

SECTIONS
{
    PrgCode : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > RAM

    PrgData : {
        . = ALIGN(4);
        *(.data)
        *(.data*)
        . = ALIGN(4);
    } > RAM

    PrgData.bss (NOLOAD) : {
        . = ALIGN(4);
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
    } > RAM

    DevDscr : {
        . = ALIGN(4);
        KEEP(*(.DevDscr))
        KEEP(*(DevDscr))
        . = ALIGN(4);
    } > RAM

    /DISCARD/ : {
        *(.ARM.attributes)
        *(.comment)
        *(.note*)
    }
}
