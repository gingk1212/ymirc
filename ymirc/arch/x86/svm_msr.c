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
    case MSR_APIC_BASE:
      set_ret_val(vcpu, UINT64_MAX);  // disable APIC in Linux kernel.
      break;
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
  Vmcb *vmcb = vcpu->vmcb;
  uint64_t value = concat_64(regs->rdx, vmcb->rax);

  switch (regs->rcx) {
    case MSR_SYSENTER_CS:
      vmcb->sysenter_cs = value;  // Unused on host, no restore needed
      break;
    case MSR_SYSENTER_ESP:
      vmcb->sysenter_esp = value;  // Unused on host, no restore needed
      break;
    case MSR_SYSENTER_EIP:
      vmcb->sysenter_eip = value;  // Unused on host, no restore needed
      break;
    case MSR_EFER:
      Efer efer = {.value = value};
      if (efer.lme || efer.nxe) {
        // Flush guest's TLB entries.
        vmcb->tlb_control = 0x03;
      }
      vmcb->efer = value;
      break;
    case MSR_STAR:
      vmcb->star = value;  // TODO: restore host value on #VMEXIT as needed
      break;
    case MSR_LSTAR:
      vmcb->lstar = value;  // TODO: restore host value on #VMEXIT as needed
      break;
    case MSR_CSTAR:
      vmcb->cstar = value;  // TODO: restore host value on #VMEXIT as needed
      break;
    case MSR_SF_MASK:
      vmcb->sfmask = value;  // TODO: restore host value on #VMEXIT as needed
      break;
    case MSR_FS_BASE:
      vmcb->fs.base = value;
      break;
    case MSR_GS_BASE:
      vmcb->gs.base = value;
      break;
    case MSR_KERNEL_GS_BASE:
      vmcb->kernel_gs_base =
          value;  // TODO: restore host value on #VMEXIT as needed
      break;
    case MSR_TSC_AUX:
      write_msr(MSR_TSC_AUX,
                value);  // TODO: restore host value on #VMEXIT as needed
      break;
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
