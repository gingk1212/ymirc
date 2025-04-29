#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static Serial *serial = NULL;

void log_set_serial(Serial *s) { serial = s; }

static void serial_write_int(Serial *serial, int val) {
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
    serial_write(serial, buf[i]);
  }
}

void log_printf(LogLevel level, const char *fmt, ...) {
  if (serial == NULL) {
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

  serial_write_string(serial, level_str);

  va_list args;
  va_start(args, fmt);

  for (int i = 0; fmt[i] != '\0'; i++) {
    if (fmt[i] == '%') {
      i++;
      switch (fmt[i]) {
        case 'd': {
          int val = va_arg(args, int);
          serial_write_int(serial, val);
          break;
        }
        case 's': {
          const char *str = va_arg(args, const char *);
          serial_write_string(serial, str);
          break;
        }
        case '%':
          serial_write(serial, '%');
          break;
        default:
          serial_write(serial, '%');
          serial_write(serial, fmt[i]);
          break;
      }
    } else {
      serial_write(serial, fmt[i]);
    }
  }

  va_end(args);
}
