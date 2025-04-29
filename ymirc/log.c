#include "log.h"

#include <stdarg.h>
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

void log_printf(LogLevel level, const char *fmt, ...) {
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
