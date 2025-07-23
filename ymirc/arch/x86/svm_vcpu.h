#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "mem.h"
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
  /** Host physical address where the guest is mapped. */
  Phys guest_base;
} SvmVcpu;

/** Create a new virtual CPU. This function does not virtualize the CPU. You
 * MUST call `virtualize` to put the CPU to enable SVM. */
SvmVcpu svm_vcpu_new(uint16_t asid);

/** Enable SVM extensions. */
void svm_vcpu_virtualize(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops);

/** Set up guest state. */
void svm_vcpu_setup_guest_state(SvmVcpu *vcpu);

/** Set NPT related values. */
void svm_vcpu_set_npt(SvmVcpu *vcpu, uint64_t n_cr3, void *host_start);

/** Start executing vCPU. */
void svm_vcpu_loop(SvmVcpu *vcpu);

/** Print guest state, and abort. */
noreturn void svm_vcpu_abort(SvmVcpu *vcpu);
