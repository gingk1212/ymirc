#include "log.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

static LogWriteFn log_write_fn = NULL;

void log_set_writefn(LogWriteFn fn) { log_write_fn = fn; }

static void write_char(char c) {
  if (log_write_fn) {
    log_write_fn(c);
  }
}

static void write_string(const char *str) {
  while (*str) {
    write_char(*str++);
  }
}

static void write_int(int val) {
  char buf[12];  // Enough for 32-bit integer
  int i = 0;
  int negative = 0;

  if (val < 0) {
    negative = 1;
    val = -val;
  }

  do {
    buf[i++] = (val % 10) + '0';
    val /= 10;
  } while (val > 0);

  if (negative) {
    buf[i++] = '-';
  }

  while (i--) {
    write_char(buf[i]);
  }
}

// TODO: support negative hex values
// TODO: strip leading zeros
static void write_hex(uint64_t value) {
  const char *hex = "0123456789ABCDEF";
  // 64 bits = 4 bits x 16
  for (int i = 15; i >= 0; i--) {
    write_char(hex[(value >> (i * 4)) & 0xF]);
  }
}

static void write_ptr(void *ptr) {
  const char *hex = "0123456789ABCDEF";
  uintptr_t value = (uint64_t)ptr;

  write_char('0');
  write_char('x');
  for (int i = sizeof(uintptr_t) * 2 - 1; i >= 0; i--) {
    write_char(hex[(value >> (i * 4)) & 0xF]);
  }
}

void log_printf(int level, const char *fmt, ...) {
  if (log_write_fn == NULL) {
    return;
  }

  const char *level_str = NULL;
  switch (level) {
    case LOG_LEVEL_DEBUG:
      level_str = "[DEBUG] ";
      break;
    case LOG_LEVEL_INFO:
      level_str = "[INFO ] ";
      break;
    case LOG_LEVEL_WARN:
      level_str = "[WARN ] ";
      break;
    case LOG_LEVEL_ERROR:
      level_str = "[ERROR] ";
      break;
    case LOG_LEVEL_NONE:
    default:
      level_str = "";
      break;
  }

  write_string(level_str);

  va_list args;
  va_start(args, fmt);

  for (int i = 0; fmt[i] != '\0'; i++) {
    if (fmt[i] == '%') {
      i++;
      switch (fmt[i]) {
        case 'd': {
          int val = va_arg(args, int);
          write_int(val);
          break;
        }
        case 'x': {
          uint64_t val = va_arg(args, uint64_t);
          write_hex(val);
          break;
        }
        case 'p': {
          void *ptr = va_arg(args, void *);
          write_ptr(ptr);
          break;
        }
        case 's': {
          const char *str = va_arg(args, const char *);
          write_string(str);
          break;
        }
        case '%':
          write_char('%');
          break;
        default:
          write_char('%');
          write_char(fmt[i]);
          break;
      }
    } else {
      write_char(fmt[i]);
    }
  }

  va_end(args);
}
