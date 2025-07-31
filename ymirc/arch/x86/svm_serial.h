#pragma once

#include <stdint.h>

/** Serial port register values VMM needs to preserve. */
typedef struct {
  uint8_t ier;  // Interrupt Enable Register.
  uint8_t mcr;  // Modem Control Register.
} SvmSerial;
