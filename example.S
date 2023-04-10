    .text
    .globl _start
_start:
    addi zero,zero, 0
    j _start

    .data
beef:
    .word 0xdeadbeef
