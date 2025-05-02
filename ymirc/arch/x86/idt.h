#pragma once

#include <stdint.h>

#include "isr.h"

/** Maximum number of gates in the IDT. */
#define MAX_NUM_GATES 256

/** Gate type of the gate descriptor in IDT. */
typedef enum {
  invalid = 0b0000,
  interrupt64 = 0b1110,
  trap64 = 0b1111,
} GateType;

void idt_init();
void set_gate(unsigned int index, GateType gate_type, Isr offset);
