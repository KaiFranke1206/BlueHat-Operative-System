ENTRY(_start)

SECTIONS {
    . = 1M;

    _kernel_start = .;

    .text : {
        *(.multiboot)       /* Multiboot1 header in first 8KB */
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = .;
}
