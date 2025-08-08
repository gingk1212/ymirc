#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "mem.h"
#include "page_allocator_if.h"
#include "serial.h"
#include "svm_common.h"
#include "svm_ioio_guest_state.h"
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
  /** Pointer to host's serial object. */
  Serial *serial;
  /** Saved guest IOIO state. */
  SvmIoioGuestState guest_ioio_state;
  /** Pending IRQ. */
  uint16_t pending_irq;
} SvmVcpu;

/** Create a new virtual CPU. This function does not virtualize the CPU. You
 * MUST call `virtualize` to put the CPU to enable SVM. */
SvmVcpu svm_vcpu_new(uint16_t asid, Serial *serial);

/** Enable SVM extensions. */
void svm_vcpu_virtualize(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops);

/** Set up guest state. */
void svm_vcpu_setup_guest_state(SvmVcpu *vcpu,
                                const page_allocator_ops_t *pa_ops);

/** Set NPT related values. */
void svm_vcpu_set_npt(SvmVcpu *vcpu, Phys n_cr3, void *host_start);

/** Start executing vCPU. */
void svm_vcpu_loop(SvmVcpu *vcpu);

/** Print guest state, and abort. */
noreturn void svm_vcpu_abort(SvmVcpu *vcpu);
