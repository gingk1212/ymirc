#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

typedef void (*SerialWriteFn)(uint8_t c);
typedef uint8_t (*SerialReadFn)(void);

typedef struct {
  SerialWriteFn write_fn;
  SerialReadFn read_fn;
} Serial;

void serial_init(Serial* serial);

static inline void serial_write(Serial* serial, uint8_t c) {
  serial->write_fn(c);
}

static inline void serial_write_string(Serial* serial, const char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    serial_write(serial, str[i]);
  }
}

static inline uint8_t serial_read(Serial* serial) { return serial->read_fn(); }

/** Enable serial console interrupt. */
void enable_serial_interrupt();

#endif  // SERIAL_H
