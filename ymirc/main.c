#include <stdint.h>

#include "../surtrc/def.h"

extern const uint8_t __stackguard_lower;

void kernel_main(BootInfo boot_info);

__attribute__((naked)) void kernel_entry(void) {
  __asm__ __volatile__(
      "movq %[new_stack], %%rsp\n\t"
      "call kernel_trampoline\n\t"
      :
      : [new_stack] "r"(&__stackguard_lower - 0x10));
}

__attribute__((ms_abi)) void kernel_trampoline(BootInfo boot_info) {
  kernel_main(boot_info);
}

static int validate_boot_info(BootInfo *boot_info) {
  if (boot_info->magic != MAGIC) {
    return 1;
  }
  return 0;
}

void kernel_main(BootInfo boot_info) {
  if (validate_boot_info(&boot_info)) {
    return;
  }

  while (1) {
    __asm__ __volatile__("hlt");
  }
}
