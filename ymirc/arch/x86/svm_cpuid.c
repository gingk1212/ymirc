#include "svm_cpuid.h"

#include <stdint.h>

#include "cpuid.h"
#include "log.h"

static const CpuidStdFeatureInfoEcx std_feature_info_ecx = {0};
static const CpuidStdFeatureInfoEdx std_feature_info_edx =
    (CpuidStdFeatureInfoEdx){
        .fpu = 1,
        .vme = 1,
        .de = 1,
        .pse = 1,
        .msr = 1,
        .pae = 1,
        .cmpxchg8b = 1,
        .sysentersysexit = 1,
        .pge = 1,
        .cmov = 1,
        .pse36 = 1,
        .fxsr = 1,
        .sse = 1,
        .sse2 = 1,
    };
static const CpuidExtFeatureEbx0 ext_feature0_ebx = (CpuidExtFeatureEbx0){
    .smep = 1,
    .invpcid = 1,
    .smap = 1,
};

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

/** Set an invalid value to the registers. */
static void invalid(SvmVcpu *vcpu) {
  Vmcb *vmcb = vcpu->vmcb;
  GuestRegisters *regs = &vcpu->guest_regs;

  setvalue_safely(&vmcb->rax, 0);
  setvalue(&regs->rbx, 0);
  setvalue(&regs->rcx, 0);
  setvalue(&regs->rdx, 0);
}

/** Use the host value by default, but if the feature also exists in Standard
 * Feature Info, use that instead. */
static uint32_t get_ext_feature_info_edx(uint32_t orig) {
  CpuidExtFeatureInfoEdx feature_info = (CpuidExtFeatureInfoEdx){
      .value = orig,
  };

  feature_info.fpu = 1;
  feature_info.vme = 1;
  feature_info.de = 1;
  feature_info.pse = 1;
  feature_info.tsc = 0;
  feature_info.msr = 1;
  feature_info.pae = 1;
  feature_info.mce = 0;
  feature_info.cmpxchg8b = 1;
  feature_info.apic = 0;
  feature_info.mtrr = 0;
  feature_info.pge = 1;
  feature_info.mca = 0;
  feature_info.cmov = 1;
  feature_info.pat = 0;
  feature_info.pse36 = 1;
  // Not present in Standard Feature Info, but MMX is also false there, so mark
  // this as false too.
  feature_info.mmxext = 0;
  feature_info.mmx = 0;
  feature_info.fxsr = 1;

  return feature_info.value;
}

/** Handle #VMEXIT caused by CPUID instruction.
 * Note that this function does not increment the RIP. */
void handle_svm_cpuid_exit(SvmVcpu *vcpu) {
  Vmcb *vmcb = vcpu->vmcb;
  GuestRegisters *regs = &vcpu->guest_regs;

  switch (vmcb->rax) {
    // Maximum Standard Function Number and Vendor String
    case 0x0:
      setvalue_safely(&vmcb->rax, 0xd);  // Largest Standard Function Number
      setvalue(&regs->rbx, 0x72696D59);  // Ymir
      setvalue(&regs->rdx, 0x696D5943);  // CYmir
      setvalue(&regs->rcx, 0x00004372);  // C
      break;
    // Processor and Processor Feature Identifiers
    case 0x1:
      CpuidRegisters orig_1 = cpuid(0x1, 0x0);
      setvalue_safely(&vmcb->rax,
                      orig_1.eax);  // Family, Model, Stepping Identifiers
      setvalue(&regs->rbx,
               orig_1.ebx);  // LocalApicId, LogicalProcessorCount, CLFlush
      setvalue(&regs->rcx, std_feature_info_ecx.value);
      setvalue(&regs->rdx, std_feature_info_edx.value);
      break;
    // Power Management Related Features
    case 0x6:
      invalid(vcpu);
      break;
    // Structured Extended Feature Identifiers
    case 0x7:
      switch (regs->rcx) {
        case 0:
          setvalue_safely(&vmcb->rax, 1);  // number of subfunctions supported
          setvalue(&regs->rbx, ext_feature0_ebx.value);
          setvalue(&regs->rcx, 0);  // Unimplemented.
          setvalue(&regs->rdx, 0);  // Unimplemented.
          break;
        case 1:
        case 2:
          invalid(vcpu);
          break;
        default:
          LOG_ERROR("Unhandled CPUID: Leaf=0x%x, Sub=0x%x\n", vmcb->rax,
                    regs->rcx);
          svm_vcpu_abort(vcpu);
      }
      break;
    // Processor Extended State Enumeration
    case 0xd:
      switch (regs->rcx) {
        case 1:
          invalid(vcpu);
          break;
        default:
          LOG_ERROR("Unhandled CPUID: Leaf=0x%x, Sub=0x%x\n", vmcb->rax,
                    regs->rcx);
          svm_vcpu_abort(vcpu);
      }
      break;
    // Maximum Extended Function Number and Vendor String
    case 0x80000000:
      setvalue_safely(&vmcb->rax,
                      0x80000000 + 1);   // Largest Extended Function Number
      setvalue(&regs->rbx, 0x72696D59);  // Ymir
      setvalue(&regs->rdx, 0x696D5943);  // CYmir
      setvalue(&regs->rcx, 0x00004372);  // C
      break;
    // Extended Processor and Processor Feature Identifiers
    case 0x80000001:
      CpuidRegisters orig_80000001 = cpuid(0x80000001, 0x0);
      setvalue_safely(&vmcb->rax, 0);           // AMD Family, Model, Stepping
      setvalue(&regs->rbx, 0);                  // BrandId Identifier
      setvalue(&regs->rcx, orig_80000001.ecx);  // Feature Identifiers
      setvalue(&regs->rdx, get_ext_feature_info_edx(
                               orig_80000001.edx));  // Feature Identifiers
      break;
    default:
      LOG_WARN("Unhandled CPUID: Leaf=0x%x, Sub=0x%x\n", vmcb->rax, regs->rcx);
      invalid(vcpu);
  }
}
