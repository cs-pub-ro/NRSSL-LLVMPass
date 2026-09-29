// The few libc functions the tests need. The toolchain's newlib is built for
// medlow and cannot be linked at 0x80000000, so we bring our own.
//
// printf supports: %d %i %u %x %X %o %c %s %p %%, the l/ll/z length modifiers,
// a field width and the '0' / '-' flags. No floating point on purpose: the
// pass does not convert libc, so floats must be printed as their bits.

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

void htif_write(int fd, const char *buf, size_t len);

void *memcpy(void *dst, const void *src, size_t n) {
  char *d = dst;
  const char *s = src;
  while (n--)
    *d++ = *s++;
  return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *d = dst;
  const char *s = src;
  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    while (n--)
      d[n] = s[n];
  }
  return dst;
}

void *memset(void *dst, int c, size_t n) {
  char *d = dst;
  while (n--)
    *d++ = (char)c;
  return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *x = a, *y = b;
  for (; n; n--, x++, y++)
    if (*x != *y)
      return *x - *y;
  return 0;
}

size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n])
    n++;
  return n;
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b)
    a++, b++;
  return (unsigned char)*a - (unsigned char)*b;
}

// Console output is collected here and written out when the buffer is full
// or at exit: every HTIF call is a round trip to the host, which is slow in
// the emulator.
static char stdout_buf[4096];
static size_t stdout_len;

void stdout_flush(void) {
  htif_write(1, stdout_buf, stdout_len);
  stdout_len = 0;
}

static void stdout_putc(char c) {
  if (stdout_len == sizeof(stdout_buf))
    stdout_flush();
  stdout_buf[stdout_len++] = c;
}

// Output sink: either a bounded string (snprintf) or stdout (printf).
struct sink {
  char *buf;
  size_t cap;
  size_t len;   // characters stored in buf
  size_t total; // characters produced, the printf return value
  int console;
};

static void sink_putc(struct sink *s, char c) {
  s->total++;
  if (s->console)
    stdout_putc(c);
  else if (s->len + 1 < s->cap)
    s->buf[s->len++] = c;
}

static void put_padded(struct sink *s, const char *str, size_t n, int width, int left, char pad) {
  int fill = width > (int)n ? width - (int)n : 0;
  if (!left)
    while (fill-- > 0)
      sink_putc(s, pad);
  while (n--)
    sink_putc(s, *str++);
  if (left)
    while (fill-- > 0)
      sink_putc(s, ' ');
}

static void format(struct sink *s, const char *fmt, va_list ap) {
  for (; *fmt; fmt++) {
    if (*fmt != '%') {
      sink_putc(s, *fmt);
      continue;
    }
    fmt++;

    int left = 0;
    char pad = ' ';
    for (;; fmt++) {
      if (*fmt == '-')
        left = 1;
      else if (*fmt == '0')
        pad = '0';
      else
        break;
    }

    int width = 0;
    if (*fmt == '*') {
      width = va_arg(ap, int);
      fmt++;
    }
    while (*fmt >= '0' && *fmt <= '9')
      width = width * 10 + (*fmt++ - '0');

    int lng = 0;
    while (*fmt == 'l' || *fmt == 'z') {
      lng++;
      fmt++;
    }

    char tmp[24];
    char *end = tmp + sizeof(tmp);
    char *p = end;
    unsigned base = 10;
    int is_signed = 0;
    int upper = 0;
    uint64_t u;

    switch (*fmt) {
    case 'c':
      tmp[0] = (char)va_arg(ap, int);
      put_padded(s, tmp, 1, width, left, ' ');
      continue;
    case 's': {
      const char *str = va_arg(ap, const char *);
      if (!str)
        str = "(null)";
      put_padded(s, str, strlen(str), width, left, ' ');
      continue;
    }
    case '%':
      sink_putc(s, '%');
      continue;
    case 'p':
      lng = 1;
      base = 16;
      sink_putc(s, '0');
      sink_putc(s, 'x');
      break;
    case 'd':
    case 'i':
      is_signed = 1;
      break;
    case 'u':
      break;
    case 'X':
      upper = 1;
      /* fallthrough */
    case 'x':
      base = 16;
      break;
    case 'o':
      base = 8;
      break;
    default: // unknown conversion: print it verbatim
      sink_putc(s, '%');
      if (!*fmt)
        return;
      sink_putc(s, *fmt);
      continue;
    }

    int neg = 0;
    if (is_signed) {
      int64_t v = lng ? va_arg(ap, int64_t) : va_arg(ap, int);
      neg = v < 0;
      u = neg ? -(uint64_t)v : (uint64_t)v;
    } else {
      u = lng ? va_arg(ap, uint64_t) : va_arg(ap, unsigned);
    }

    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    do {
      *--p = digits[u % base];
      u /= base;
    } while (u);

    if (neg) {
      if (pad == '0') {
        sink_putc(s, '-');
        width--;
      } else {
        *--p = '-';
      }
    }
    put_padded(s, p, end - p, width, left, pad);
  }
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
  struct sink s = {buf, size, 0, 0, 0};
  format(&s, fmt, ap);
  if (size)
    buf[s.len] = 0;
  return s.total;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return n;
}

int vprintf(const char *fmt, va_list ap) {
  struct sink s = {NULL, 0, 0, 0, 1};
  format(&s, fmt, ap);
  return s.total;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vprintf(fmt, ap);
  va_end(ap);
  return n;
}

int putchar(int c) {
  stdout_putc((char)c);
  return c;
}

int puts(const char *s) {
  while (*s)
    stdout_putc(*s++);
  stdout_putc('\n');
  return 0;
}
