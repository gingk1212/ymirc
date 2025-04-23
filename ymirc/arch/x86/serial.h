#ifndef ARCH_X86_SERIAL_H
#define ARCH_X86_SERIAL_H

#include <stdint.h>

typedef enum {
  com1 = 0x3F8,
  com2 = 0x2F8,
  com3 = 0x3E8,
} Ports;

void init_serial(Ports port, uint32_t baud);
void write_byte(uint8_t byte, Ports port);

#endif  // ARCH_X86_SERIAL_H
