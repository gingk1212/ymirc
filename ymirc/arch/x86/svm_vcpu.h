#pragma once

#include <stddef.h>
#include <stdint.h>

#include "page_allocator_if.h"
#include "svm_common.h"
#include "svm_vmcb.h"

typedef struct {
  /** Id of the logical processor. */
  size_t id;
  /** ASID of the virtual machine. */
  uint16_t asid;
  /** VMCB (Virtual Machine Control Block). */
  Vmcb *vmcb;
  /** Physical address of VMCB. */
  uintptr_t vmcb_phys;
  /** Saved guest registers. */
  GuestRegisters guest_regs;
} SvmVcpu;

/** Create a new virtual CPU. This function does not virtualize the CPU. You
 * MUST call `virtualize` to put the CPU to enable SVM. */
SvmVcpu svm_vcpu_new(uint16_t asid);

/** Enable SVM extensions. */
void svm_vcpu_virtualize(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops);

/** Set up VMCB for a logical processor. */
void svm_vcpu_setup_vmcb(SvmVcpu *vcpu);

/** Start executing vCPU. */
void svm_vcpu_loop(SvmVcpu *vcpu);
