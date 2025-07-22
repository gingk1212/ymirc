#include "cpuid.h"

/** Asm CPUID instruction. */
CpuidRegisters cpuid(uint32_t leaf, uint32_t subleaf) {
  CpuidRegisters regs = {0};
  __asm__ volatile("cpuid"
                   : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx),
                     "=d"(regs.edx)
                   : "a"(leaf), "c"(subleaf));
  return regs;
}
