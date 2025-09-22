#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

typedef void (*SerialWriteFn)(uint8_t c);
typedef uint8_t (*SerialReadFn)(void);

typedef enum {
  SERIAL_PORT_COM1,
  SERIAL_PORT_COM2,
  SERIAL_PORT_COM3,
} SerialPort;

typedef struct {
  uint64_t addr;
  SerialWriteFn write_fn;
  SerialReadFn read_fn;
} Serial;

/** Initialize a serial console. Set write and read function pointers in this
 * function. */
void serial_init(Serial* serial, SerialPort port, uint32_t baud);

static inline void serial_write(const Serial* serial, uint8_t c) {
  serial->write_fn(c);
}

static inline void serial_write_string(const Serial* serial, const char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    serial_write(serial, str[i]);
  }
}

static inline uint8_t serial_read(Serial* serial) { return serial->read_fn(); }

/** Enable serial console interrupt. */
void enable_serial_interrupt(const Serial* serial);

#endif  // SERIAL_H
