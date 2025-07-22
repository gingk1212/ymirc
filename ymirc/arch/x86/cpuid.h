#pragma once

#include <stdint.h>

/** Return value of CPUID. */
typedef struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
} CpuidRegisters;

/** Asm CPUID instruction. */
CpuidRegisters cpuid(uint32_t leaf, uint32_t subleaf);
