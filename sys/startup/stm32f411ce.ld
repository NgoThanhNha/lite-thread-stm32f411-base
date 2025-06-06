/**
 ******************************************************************************
 * @author: GaoDen
 * @date:   18/11/2024
 ******************************************************************************
**/

/* entry point */
ENTRY(reset_handler)

/* heap size */
HEAP_SIZE = 1K;

/* memories definition */
MEMORY {
    FLASH   (rx)    : ORIGIN = 0x08000000,  LENGTH = 512K
    RAM     (xrw)   : ORIGIN = 0x20000000,  LENGTH = 128K
}

/* sections */
SECTIONS {
    /* start of FLASH */
    .start_flash : {
        _start_flash = .;
    } > FLASH

    /* the startup code into "FLASH" Rom type memory */
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* seeding at the startup code */
        . = ALIGN(4);
    } > FLASH

    /* the program code and other data into "FLASH" Rom type memory */
    .text : {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;        /* define a global symbols at end of code */
    } > FLASH

    /* constant data into "FLASH" Rom type memory */
    .rodata : {
        . = ALIGN(4);
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    .ARM : {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    .preinit_array : {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .init_array : {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .fini_array : {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH

    /* end of FLASH */
    .end_flash : {
        _end_flash = .;
    } > FLASH

    /* start SRAM */
    .start_ram : {
        _start_ram = . ;
    } > RAM

    /* used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    /* initialized data sections into "RAM" Ram type memory */
    .data : {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */
        *(.RamFunc)        /* .RamFunc sections */
        *(.RamFunc*)       /* .RamFunc* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end */

    } >RAM AT> FLASH

    /* uninitialized data section into "RAM" Ram type memory */
    . = ALIGN(4);
    .bss : {
        /* this is used by the startup in order to initialize the .bss section */
        _sbss = .;         /* define a global symbol at bss start */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end */
        __bss_end__ = _ebss;
    } > RAM

    /* heap section */
    .heap : {
        __heap_start__ = .;
        . = . + HEAP_SIZE;
        . = ALIGN(4);
        __heap_end__ = .;
    } > RAM

    /* define the end of the heap */
    PROVIDE(end = __heap_end__);

    /* end of RAM */
    .end_ram : {
        _end_ram = .;
    } > RAM

    /* start of stack */
    .sstack : {
        _sstack = .;
    } > RAM

    /* end of stack */
    _estack = ORIGIN(RAM) + LENGTH(RAM);

    /* remove information from the compiler libraries */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
