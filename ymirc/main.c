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

#if defined(__x86_64__)
#include "arch/x86/interrupt.h"
#include "arch/x86/pic.h"
#include "arch/x86/vm.h"
#endif

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

#if defined(__x86_64__)
static void blob_irq_handler(Context *ctx) {
  uint16_t vector = ctx->vector - primary_vector_offset;
  notify_eoi(vector);
}
#endif

void kernel_main(BootInfo *boot_info) {
  // Initialize the serial console
  serial_init(&serial);

  // Initialize logger
  log_set_writefn(serial_log_output);
  LOG_INFO("Booting YmirC...\n");

  // Validate the boot info
  if (validate_boot_info(boot_info)) {
    panic("Invalid boot info.");
  }

  // Perform architecture-specific initialization
  arch_init();

  // Initialize page allocator
  page_allocator_init(&boot_info->map);
  LOG_INFO("Initialized page allocator.\n");

  // Reconstruct memory mapping from the one provided by UEFI and SutrC.
  LOG_INFO("Reconstructing memory mapping...\n");
  reconstruct_mapping(&pa_ops);

  // Initialize general allocator
  init_bin_allocator(&pa_ops);
  LOG_INFO("Initialized general allocator.\n");

#if defined(__x86_64__)
  // Initialize PIC.
  pic_init();
  LOG_INFO("Initialized PIC.\n");

  // Enable PIT.
  register_handler(irq_timer + primary_vector_offset, blob_irq_handler);
  unset_mask(irq_timer);
  LOG_INFO("Enabled PIT.\n");

  // Unmask serial interrupt.
  register_handler(irq_serial1 + primary_vector_offset, blob_irq_handler);
  unset_mask(irq_serial1);
  enable_serial_interrupt();

  // Create VM instance.
  Vm vm = vm_new();
  if (vm.error != VM_SUCESS) {
    LOG_ERROR("Failed to create VM instance.\n");
  } else {
    // Enable SVM extensions.
    vm_init(&vm, &pa_ops);
    LOG_INFO("Enabled SVM extensions.\n");

    // Setup guest memory.
    setup_guest_memory(&vm, &pa_ops);
    LOG_INFO("Setup guest memory.\n");

    // Launch
    LOG_INFO("Starting the virtual machine...\n");
    vm_loop(&vm);
  }
#endif

  LOG_WARN("End of Life...\n");
  while (1) {
    __asm__ __volatile__("hlt");
  }
}
