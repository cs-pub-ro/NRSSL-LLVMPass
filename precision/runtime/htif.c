// Host communication over HTIF (the emulator's fesvr): console output through
// the SYS_write proxy, exit code through tohost. Also the trap handler.

#include <stddef.h>
#include <stdint.h>

#define SYS_write 64

volatile uint64_t tohost __attribute__((section(".tohost")));
volatile uint64_t fromhost __attribute__((section(".tohost")));

static uint64_t htif_syscall(uint64_t which, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
  volatile uint64_t magic_mem[8] __attribute__((aligned(64)));
  magic_mem[0] = which;
  magic_mem[1] = arg0;
  magic_mem[2] = arg1;
  magic_mem[3] = arg2;
  __sync_synchronize();

  tohost = (uintptr_t)magic_mem;
  while (fromhost == 0)
    ;
  fromhost = 0;

  __sync_synchronize();
  return magic_mem[0];
}

void htif_write(int fd, const char *buf, size_t len) {
  if (len)
    htif_syscall(SYS_write, fd, (uintptr_t)buf, len);
}

void stdout_flush(void);

void __attribute__((noreturn)) _exit(int code) {
  stdout_flush();
  tohost = ((uint64_t)code << 1) | 1;
  for (;;)
    ;
}

void __attribute__((noreturn)) exit(int code) { _exit(code); }

void __attribute__((noreturn)) abort(void) { _exit(134); }

static void put_str(const char *s) {
  size_t n = 0;
  while (s[n])
    n++;
  htif_write(2, s, n);
}

static void put_hex(uint64_t v) {
  char buf[19] = "0x";
  for (int i = 0; i < 16; i++)
    buf[2 + i] = "0123456789abcdef"[(v >> (60 - 4 * i)) & 0xf];
  buf[18] = 0;
  put_str(buf);
}

// Called from crt.S on any trap. Tests run in M-mode and never expect traps,
// so report and bail out (e.g. an illegal instruction from an unknown opcode).
void __attribute__((noreturn)) handle_trap(uint64_t mcause, uint64_t mepc, uint64_t mtval) {
  stdout_flush();
  put_str("\n*** trap: mcause=");
  put_hex(mcause);
  put_str(" mepc=");
  put_hex(mepc);
  put_str(" mtval=");
  put_hex(mtval);
  put_str("\n");
  _exit(1337);
}
