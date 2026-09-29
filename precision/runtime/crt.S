# Minimal bare-metal startup: single hart, M-mode, FPU on, then main().

  .section .text.init
  .globl _start
_start:
  csrr  t0, mhartid
  bnez  t0, park

  .option push
  .option norelax
  la    gp, __global_pointer$
  .option pop
  la    sp, _stack_top

  la    t0, trap_entry
  csrw  mtvec, t0

  # enable the FPU and clear fcsr
  li    t0, 0x6000            # MSTATUS_FS
  csrs  mstatus, t0
  fscsr x0

  # zero .bss
  la    t0, __bss_start
  la    t1, __bss_end
1:
  bgeu  t0, t1, 2f
  sd    zero, 0(t0)
  addi  t0, t0, 8
  j     1b
2:
  li    a0, 0
  li    a1, 0
  call  main
  call  _exit

park:
  wfi
  j     park

  .align 2
trap_entry:
  csrr  a0, mcause
  csrr  a1, mepc
  csrr  a2, mtval
  j     handle_trap
