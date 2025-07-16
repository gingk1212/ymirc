#pragma once

#include <stdint.h>

#include "page_allocator_if.h"
#include "svm_vcpu.h"

typedef enum {
  VM_SUCESS = 0,
  VM_ERROR_OUT_OF_MEMORY,
  VM_ERROR_SYSTEM_NOT_SUPPORTED,
  VM_ERROR_UNKNOWN
} VmError;

typedef enum { VIRTUALIZE_TYPE_SVM, VIRTUALIZE_TYPE_VT } VirtualizeType;

typedef struct {
  VmError error;
  VirtualizeType vtype;
  SvmVcpu svmvcpu;
  uint8_t *guest_mem;
} Vm;

/** Create a new virtual machine instance. You MUST initialize the VM before
 * using it. */
Vm vm_new();

/** Initialize the virtual machine, enabling SVM extensions. */
void vm_init(Vm *vm, const page_allocator_ops_t *pa_ops);

/** Kick off the virtual machine. */
void vm_loop(Vm *vm);

/** Setup guest memory. */
void setup_guest_memory(Vm *vm, const page_allocator_ops_t *pa_ops);
