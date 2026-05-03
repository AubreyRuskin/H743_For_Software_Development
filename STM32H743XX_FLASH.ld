/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Abstract    : Linker script for STM32H743IITx series
**                2048Kbytes FLASH and 1056Kbytes RAM
**
**                BSS split across RAM / RAM_D2 / RAM_D3 / ITCMRAM / DTCMRAM.
**                FreeRTOS heap in RAM alongside main .bss.
**
**  Target      : STMicroelectronics STM32
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);

_Min_Heap_Size = 0x800;
_Min_Stack_Size = 0x800;

/* Specify the memory areas */
MEMORY
{
ITCMRAM (xrw)      : ORIGIN = 0x00000000, LENGTH = 64K
DTCMRAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
RAM (xrw)          : ORIGIN = 0x24000000, LENGTH = 512K
RAM_D2 (xrw)       : ORIGIN = 0x30000000, LENGTH = 288K
RAM_D3 (xrw)       : ORIGIN = 0x38000000, LENGTH = 64K
FLASH (rx)         : ORIGIN = 0x8000000, LENGTH = 2048K
}

SECTIONS
{
  /* ---- FLASH sections ---- */

  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  /* ---- DTCMRAM: .data, stack/heap ---- */

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
  } >DTCMRAM AT> FLASH

  .tdata : ALIGN(4)
  {
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    . = ALIGN(4);
    _edata = .;
    PROVIDE(__data_end = .);
    PROVIDE(__tdata_end = .);
  } >DTCMRAM AT> FLASH

  PROVIDE( __tdata_start = ADDR(.tdata) );
  PROVIDE( __tdata_size = __tdata_end - __tdata_start );

  PROVIDE( __data_start = ADDR(.data) );
  PROVIDE( __data_size = __data_end - __data_start );

  PROVIDE( __tdata_source = LOADADDR(.tdata) );
  PROVIDE( __tdata_source_end = LOADADDR(.tdata) + SIZEOF(.tdata) );
  PROVIDE( __tdata_source_size = __tdata_source_end - __tdata_source );

  PROVIDE( __data_source = LOADADDR(.data) );
  PROVIDE( __data_source_end = __tdata_source_end );
  PROVIDE( __data_source_size = __data_source_end - __data_source );

  .tbss (NOLOAD) : ALIGN(4)
  {
    *(.tbss .tbss.*)
    . = ALIGN(4);
    PROVIDE( __tbss_end = . );
  } >DTCMRAM

  PROVIDE( __tbss_start = ADDR(.tbss) );
  PROVIDE( __tbss_size = __tbss_end - __tbss_start );
  PROVIDE( __tbss_offset = ADDR(.tbss) - ADDR(.tdata) );

  PROVIDE( __tls_base = __tdata_start );
  PROVIDE( __tls_end = __tbss_end );
  PROVIDE( __tls_size = __tls_end - __tls_base );
  PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
  PROVIDE( __tls_size_align = (__tls_size + __tls_align - 1) & ~(__tls_align - 1) );
  PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );
  PROVIDE( __arm64_tls_tcb_offset = MAX(16, __tls_align) );

  /* Small / fast BSS in DTCMRAM (placed via section attribute) */
  .bss_dtcm (NOLOAD) : ALIGN(4)
  {
    __bss_dtcm_start = .;
    *(.bss_dtcm .bss_dtcm.*)
    . = ALIGN(4);
    __bss_dtcm_end = .;
  } >DTCMRAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM

  /* ---- ITCMRAM (64KB): medium BSS items ---- */

  .bss_itcm (NOLOAD) : ALIGN(4)
  {
    __bss_itcm_start = .;
    *(.bss_itcm .bss_itcm.*)
    . = ALIGN(4);
    __bss_itcm_end = .;
  } >ITCMRAM

  /* ---- RAM (AXI SRAM 512KB): FreeRTOS heap + main .bss ---- */

  .freertos_heap (NOLOAD) :
  {
    . = ALIGN(8);
    *(.freertos_heap)
    . = ALIGN(8);
  } >RAM

  .bss (NOLOAD) : ALIGN(4)
  {
    __bss_ram_start = .;
    _sbss = .;
    __bss_start__ = _sbss;

    *(.bss .bss.*)
    *(COMMON)

    . = ALIGN(4);
    __bss_ram_end = .;
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  PROVIDE( __non_tls_bss_start = ADDR(.bss) );
  PROVIDE( __bss_start = __tbss_start );
  PROVIDE( __bss_size = __bss_end - __bss_start );

  /* ---- RAM_D2 (288KB): large BSS items + LWIP descriptors ---- */

  .lwip_sec (NOLOAD) :
  {
    . = ABSOLUTE(0x30000000);
    *(.RxDecripSection)
    . = ABSOLUTE(0x30000080);
    *(.TxDecripSection)
    . = ABSOLUTE(0x30000200);
    *(.Rx_PoolSection)
  } >RAM_D2

  .bss_ram2 (NOLOAD) : ALIGN(4)
  {
    __bss_ram2_start = .;
    *(.bss_ram2 .bss_ram2.*)
    . = ALIGN(4);
    __bss_ram2_end = .;
  } >RAM_D2

  /* ---- RAM_D3 (64KB): more large BSS items ---- */

  .bss_ram3 (NOLOAD) : ALIGN(4)
  {
    __bss_ram3_start = .;
    *(.bss_ram3 .bss_ram3.*)
    . = ALIGN(4);
    __bss_ram3_end = .;
  } >RAM_D3

  /* ---- Discard ---- */

  /DISCARD/ :
  {
    libc.a:* ( * )
    libm.a:* ( * )
    libgcc.a:* ( * )
  }
}
