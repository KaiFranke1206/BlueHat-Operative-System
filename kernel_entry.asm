section .multiboot
align 4
    dd 0x1BADB002         ; magic
    dd 0x00000000         ; flags
    dd 0xE4524FFE         ; checksum (-(magic + flags))

section .text
global _start

_start:
    cli
    extern kernel_main
    push ebx              ; multiboot_info pointer
    push eax              ; magic number
    call kernel_main
    hlt
