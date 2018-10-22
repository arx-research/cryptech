/* Linker script to configure memory regions.
 *
 * This is the script for the firmware (meaning any application except the bootloader).
 * It should be placed 128 KB from the start of the STM32 internal flash.
 *
 */
MEMORY
{
  /* FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048k */
  BOOTLOADER (rx) : ORIGIN = 0x08000000, LENGTH = 128K
  FIRMWARE (rx)   : ORIGIN = 0x08000000 + 128K, LENGTH = 2048K - 128K
  FLASH (rx)      : ORIGIN = 0x08000000 + 128K, LENGTH = 2048K - 128K
  CCMRAM (rwx)    : ORIGIN = 0x10000000, LENGTH = 64K
  RAM (rwx)       : ORIGIN = 0x20000000, LENGTH = 192K - 4
  SDRAM1 (rwx)    : ORIGIN = 0xC0000000, LENGTH = 64M
  SDRAM2 (rwx)    : ORIGIN = 0xD0000000, LENGTH = 64M
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   _estack
 */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)

        KEEP(*(.eh_frame*))
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    __etext = .;
    _sidata = .;

    .data :
    {
        __data_start__ = .;
        _sdata = .;
        *(vtable)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;
        _edata = .;

    } > RAM AT> FLASH

    /* If initialized variables are placed in this section, 
     * the startup code needs to be modified to copy the init-values.  
     */
    .ccmram :
    {
      . = ALIGN(4);
      _sccmram = .;
      *(.ccmram)
      *(.ccmram*)

      . = ALIGN(4);
      _eccmram = .;
    } >CCMRAM AT> FLASH

     __end_ccmram = ORIGIN(CCMRAM) + LENGTH(CCMRAM);

    .sdram1 :
    {
      . = ALIGN(4);
      _ssdram1 = .;
      *(.sdram1)
      *(.sdram1*)

      . = ALIGN(4);
      _esdram1 = .;
    } >SDRAM1

    __end_sdram1 = ORIGIN(SDRAM1) + LENGTH(SDRAM1);

    .sdram2 :
    {
      . = ALIGN(4);
      _ssdram2 = .;
      *(.sdram2)
      *(.sdram2*)

      . = ALIGN(4);
      _esdram2 = .;
    } >SDRAM2

    __end_sdram2 = ORIGIN(SDRAM2) + LENGTH(SDRAM2);

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        _ebss = .;
    } > RAM

    .heap (COPY):
    {
        __end__ = .;
        end = __end__;
        *(.heap*)
        __HeapLimit = .;
    } > RAM

    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (COPY):
    {
        *(.stack*)
    } > RAM

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    _estack = __StackTop;
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    /* The DFU code needs to know where the firmware lives */
    CRYPTECH_BOOTLOADER_START = ORIGIN(BOOTLOADER);
    CRYPTECH_BOOTLOADER_END = ORIGIN(BOOTLOADER) + LENGTH(BOOTLOADER) - 1;
    CRYPTECH_FIRMWARE_START = ORIGIN(FIRMWARE);
    CRYPTECH_FIRMWARE_END = ORIGIN(FIRMWARE) + LENGTH(FIRMWARE) - 1;
    /* The last 4 bytes of RAM is used to control the DFU reset+jumping.
     * Have to be reserved in here to not get overwritten by the Reset_Handler
     * that zeros memory. Maybe there is a better way?
     */
    CRYPTECH_DFU_CONTROL = ORIGIN(RAM) + LENGTH(RAM);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
