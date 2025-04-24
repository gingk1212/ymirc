#include <stdint.h>

#include "../surtrc/def.h"
#include "serial.h"

extern const uint8_t __stackguard_lower;

__attribute__((naked)) void kernel_entry(void) {
  __asm__ __volatile__(
      "movq %[new_stack], %%rsp\n\t"
      "call kernel_main\n\t"
      :
      : [new_stack] "r"(&__stackguard_lower - 0x10));
}

static int validate_boot_info(BootInfo *boot_info) {
  if (boot_info->magic != MAGIC) {
    return 1;
  }
  return 0;
}

void kernel_main(BootInfo *boot_info) {
  if (validate_boot_info(boot_info)) {
    return;
  }

  // Initialize the serial console
  Serial serial[1];
  serial_init(serial);
  serial_write_string(serial, "Hello, world!\n");

  while (1) {
    __asm__ __volatile__("hlt");
  }
}
