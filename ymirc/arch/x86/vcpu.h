#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  /** Id of the logical processor. */
  size_t id;
  /** ASID of the virtual machine. */
  uint16_t asid;
} Vcpu;

/** Create a new virtual CPU. This function does not virtualize the CPU. You
 * MUST call `virtualize` to put the CPU to enable SVM. */
Vcpu vcpu_new(uint16_t asid);

/** Enable SVM extensions. */
void vcpu_virtualize(Vcpu *vcpu);
