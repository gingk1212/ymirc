#include "svm_msr.h"

#include <stdint.h>

#include "asm.h"
#include "bits.h"
#include "log.h"

/** Set a 32-bit value to the given 64-bit without modifying the upper
 * 32-bits. Note: Parameter `reg` must be aligned. */
static inline void setvalue(uint64_t *reg, uint32_t val) {
  *((uint32_t *)reg) = val;
}

/** Set a 32-bit value to the given 64-bit without modifying the upper 32-bits.
 * Note: Parameter `reg` may be unaligned. */
static inline void setvalue_safely(void *reg, uint32_t val) {
  uint64_t tmp;
  memcpy(&tmp, reg, sizeof(tmp));
  tmp = (tmp & 0xffffffff00000000ULL) | val;
  memcpy(reg, &tmp, sizeof(tmp));
}

/** Set the 64-bit return value to the guest registers. */
static void set_ret_val(SvmVcpu *vcpu, uint64_t val) {
  GuestRegisters *regs = &vcpu->guest_regs;
  Vmcb *vmcb = vcpu->vmcb;

  setvalue(&regs->rdx, (uint32_t)(val >> 32));
  setvalue_safely(&vmcb->rax, val);
}

static void handle_rdmsr(SvmVcpu *vcpu) {
  GuestRegisters *regs = &vcpu->guest_regs;
  Vmcb *vmcb = vcpu->vmcb;

  switch (regs->rcx) {
    case MSR_EFER:
      set_ret_val(vcpu, vmcb->efer);
      break;
    default:
      LOG_ERROR("Unhandled RDMSR: 0x%x\n", regs->rcx);
      svm_vcpu_abort(vcpu);
  }
}

static void handle_wrmsr(SvmVcpu *vcpu) {
  GuestRegisters *regs = &vcpu->guest_regs;

  switch (regs->rcx) {
    default:
      LOG_ERROR("Unhandled WRMSR: 0x%x\n", regs->rcx);
      svm_vcpu_abort(vcpu);
  }
}

void handle_svm_msr_exit(SvmVcpu *vcpu) {
  uint64_t exitinfo1 = vcpu->vmcb->exitinfo1;

  if (exitinfo1 == 0) {
    handle_rdmsr(vcpu);
  } else if (exitinfo1 == 1) {
    handle_wrmsr(vcpu);
  } else {
    svm_vcpu_abort(vcpu);
  }
}
