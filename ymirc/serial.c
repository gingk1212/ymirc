#include "serial.h"

#include "arch/x86/serial_impl.h"

void serial_init(Serial* serial) { serial_impl_init(serial, com1, 115200); }
