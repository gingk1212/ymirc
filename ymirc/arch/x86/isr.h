#pragma once

#include <stdint.h>

/** ISR signature. */
typedef void (*Isr)(void);

extern Isr isr_table[];

/** Structure holding general purpose registers as saved by PUSHA. */
typedef struct {
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rbp;
  uint64_t rsp;
  uint64_t rbx;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rax;
} __attribute__((packed)) Registers;

/** Execution Context */
typedef struct {
  // General purpose registers.
  Registers registers;
  // Interrupt vector.
  uint64_t vector;
  // Error code.
  uint64_t error_code;

  // CPU status:
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
} __attribute__((packed)) Context;
