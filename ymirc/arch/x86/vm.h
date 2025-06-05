#pragma once

#include <stdint.h>

#include "vcpu.h"

typedef enum {
  VM_SUCESS = 0,
  VM_ERROR_OUT_OF_MEMORY,
  VM_ERROR_SYSTEM_NOT_SUPPORTED,
  VM_ERROR_UNKNOWN
} VmError;

typedef struct {
  VmError error;
  Vcpu vcpu;
} Vm;

/** Create a new virtual machine instance. You MUST initialize the VM before
 * using it. */
Vm vm_new();

/** Initialize the virtual machine, enabling SVM extensions. */
void vm_init(Vm *vm);
