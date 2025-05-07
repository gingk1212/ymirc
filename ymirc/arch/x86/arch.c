#include "arch.h"

#include "arch/x86/page.h"
#include "gdt.h"
#include "interrupt.h"
#include "log.h"

void arch_init() {
  gdt_init();
  LOG_INFO("Initialized GDT.\n");
  itr_init();
  LOG_INFO("Initialized IDT.\n");
}

void reconstruct_mapping() { reconstruct(); }

void disable_intr() { __asm__ volatile("cli"); }

void endless_halt() {
  disable_intr();
  while (1) {
    __asm__ volatile("hlt");
  }
}

void print_stack_trace() {
  uint64_t *rbp;
  __asm__ volatile("mov %%rbp, %0" : "=r"(rbp));

  // Skip panic()
  if (rbp) {
    rbp = (uint64_t *)(*rbp);
  } else {
    LOG_ERROR("??\n");
    return;
  }

  // Limit to 10 stack frames to avoid infinite loops
  for (int i = 0; i < 10 && rbp; i++) {
    uint64_t ret_addr = *(rbp + 1);
    uint64_t caller_addr = ret_addr - 1;
    LOG_ERROR("#%d: 0x%x\n", i, caller_addr);
    rbp = (uint64_t *)(*rbp);
  }
}
