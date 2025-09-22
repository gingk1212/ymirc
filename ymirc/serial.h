#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

typedef void (*SerialWriteFn)(uint8_t c);
typedef int (*SerialReadFn)(void);

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

/** Write a single byte to the serial console. */
static inline void serial_write(const Serial* serial, uint8_t c) {
  serial->write_fn(c);
}

/** Write a string to the serial console. */
static inline void serial_write_string(const Serial* serial, const char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    serial_write(serial, str[i]);
  }
}

/** Try to read a character from the serial console. Returns -1 if no character
 * is available in Rx-buffer. */
static inline int serial_read(Serial* serial) { return serial->read_fn(); }

/** Read a byte from the serial port (blocking). This function repeatedly calls
 * serial_read() until a byte becomes available. It will never return -1. */
static inline uint8_t serial_read_blocking(Serial* serial) {
  int ch;
  while ((ch = serial_read(serial)) == -1);
  return ch;
}

/** Enable serial console interrupt. */
void enable_serial_interrupt(const Serial* serial);

#endif  // SERIAL_H
