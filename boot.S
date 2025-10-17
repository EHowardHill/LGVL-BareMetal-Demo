.section .multiboot
.align 4

/* Multiboot header */
.long 0x1BADB002         /* magic */
.long 0x00000007         /* flags: ALIGN + MEMINFO + VIDEO */
.long -(0x1BADB002 + 0x00000007)  /* checksum */
.long 0, 0, 0, 0, 0      /* unused */
.long 0                  /* mode_type (0 = linear graphics) */
.long 640                /* width */
.long 480                /* height */
.long 32                 /* depth (32-bit color) */

.section .bss
.align 16
stack_bottom:
.skip 65536  /* 64 KiB stack - increased for LVGL */
stack_top:

.section .text
.global _start
.type _start, @function

_start:
    /* Set up stack */
    movl $stack_top, %esp
    
    /* Save multiboot info */
    pushl %ebx  /* multiboot info pointer */
    pushl %eax  /* multiboot magic */
    
    /* Call kernel main */
    call kernel_main
    
    /* Hang if kernel returns */
    cli
1:  hlt
    jmp 1b

.size _start, . - _start

.section .note.GNU-stack,"",@progbits