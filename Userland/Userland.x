ENTRY(_start);

/* FIXME: Keep debugging information! */

PHDRS {
    text PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    .text : {
        *(.init)
        *(*.text*)
        *(.fini)
        *(.rodata*)
        *(.glue*)

        __preinit_array_start = .;
        KEEP(*(.preinit_array*))
        __preinit_array_end = .;

        __init_array_start = .;
        KEEP(*(.init_array*))
        __init_array_end = .;

        __fini_array_start = .;
        KEEP(*(.fini_array*))
        __fini_array_end = .;
    } :text
    .data : {
        *(.data*)
        *(.noinit)
    } :data
    .bss (NOLOAD) : {
        __bss_start__ = .;
        *(.bss*)
        __bss_end__ = .;
    } :data
    .heap (NOLOAD) : {
        __heap_start__ = .;
        . += 0x10000;
        __heap_end__ = .;
    } :data
    .stack (NOLOAD) : {
        . += 0x10100;
    } :data

    /DISCARD/ : {
        /* Why are these sections even emitted? They are not releavant for the Cortex-M0+. */
        *(.vfp11_veneer)
        *(.v4_bx)

        /* Why are these sections even emitted? We are linking statically?! */
        *(.iplt)
        *(.rel.iplt)
        *(.igot.plt)
    }
}
