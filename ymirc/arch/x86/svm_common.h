#pragma once

#include <stdint.h>

typedef struct {
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rbx;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  // Align to 16 bytes, otherwise movaps would cause #GP.
  unsigned __int128 xmm0 __attribute__((aligned(16)));
  unsigned __int128 xmm1 __attribute__((aligned(16)));
  unsigned __int128 xmm2 __attribute__((aligned(16)));
  unsigned __int128 xmm3 __attribute__((aligned(16)));
  unsigned __int128 xmm4 __attribute__((aligned(16)));
  unsigned __int128 xmm5 __attribute__((aligned(16)));
  unsigned __int128 xmm6 __attribute__((aligned(16)));
  unsigned __int128 xmm7 __attribute__((aligned(16)));
} GuestRegisters;
