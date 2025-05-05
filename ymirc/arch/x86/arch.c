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

void endless_halt() {
  __asm__ volatile("cli");
  while (1) {
    __asm__ volatile("hlt");
  }
}
