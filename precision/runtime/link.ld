/* Bare-metal layout for rocket-chip: DRAM at 0x80000000, the boot ROM jumps there. */

OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 256M
}

STACK_SIZE = 256K;

SECTIONS
{
  .text.init : { *(.text.init) } > RAM

  . = ALIGN(0x1000);
  .tohost : { *(.tohost) } > RAM

  .text : { *(.text .text.*) } > RAM
  .rodata : { *(.rodata .rodata.*) *(.srodata .srodata.*) } > RAM

  .data : ALIGN(16) { *(.data .data.*) } > RAM
  .sdata : {
    __global_pointer$ = . + 0x800;
    *(.sdata .sdata.*)
  } > RAM

  .bss (NOLOAD) : ALIGN(16) {
    __bss_start = .;
    *(.sbss .sbss.*) *(.bss .bss.*) *(COMMON)
    . = ALIGN(8);
    __bss_end = .;
  } > RAM

  .stack (NOLOAD) : ALIGN(16) {
    . += STACK_SIZE;
    _stack_top = .;
  } > RAM

  /* the heap grows from here up to the end of RAM */
  _end = .;
}
