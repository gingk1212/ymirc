#ifndef ARCH_X86_SERIAL_IMPL_H
#define ARCH_X86_SERIAL_IMPL_H

#include <stdint.h>

#include "serial.h"

typedef enum {
  com1 = 0x3F8,
  com2 = 0x2F8,
  com3 = 0x3E8,
} Ports;

void serial_impl_init(Serial *serial, Ports port, uint32_t baud);

/** Enable serial console interrupt for Rx-available and Tx-empty. */
void enable_serial_interrupt_impl(Ports port);

#endif  // ARCH_X86_SERIAL_IMPL_H
