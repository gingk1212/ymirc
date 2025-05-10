#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "../surtrc/def.h"
#include "arch.h"
#include "bin_allocator.h"
#include "log.h"
#include "page_allocator.h"
#include "panic.h"
#include "serial.h"

extern const uint8_t __stackguard_lower;

static Serial serial;

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

static void serial_log_output(char c) { serial_write(&serial, c); }

void kernel_main(BootInfo *boot_info) {
  // Initialize the serial console
  serial_init(&serial);

  // Initialize logger
  log_set_writefn(serial_log_output);
  LOG_INFO("Booting YmirC...\n");

  // Perform architecture-specific initialization
  arch_init();

  // Validate the boot info
  if (validate_boot_info(boot_info)) {
    panic("Invalid boot info.");
  }

  // Initialize page allocator
  page_allocator_init(&boot_info->map);
  LOG_INFO("Initialized page allocator.\n");

  // Reconstruct memory mapping from the one provided by UEFI and SutrC.
  LOG_INFO("Reconstructing memory mapping...\n");
  reconstruct_mapping(&pa_ops);

  // Initialize general allocator
  init_bin_allocator(&pa_ops);
  LOG_INFO("Initialized general allocator.\n");

  while (1) {
    __asm__ __volatile__("hlt");
  }
}
