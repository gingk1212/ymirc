#include "vm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "asm.h"
#include "log.h"
#include "mem.h"

/** Return value of CPUID. */
typedef struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
} CpuidRegisters;

/** Asm CPUID instruction. */
static CpuidRegisters cpuid(uint32_t leaf, uint32_t subleaf) {
  CpuidRegisters regs = {0};
  __asm__ volatile("cpuid"
                   : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx),
                     "=d"(regs.edx)
                   : "a"(leaf), "c"(subleaf));
  return regs;
}

/** Length of out must be 12. */
static void get_cpu_vendor_id(uint8_t *out) {
  if (out == NULL) return;
  // Fn0000_0000_E[D,C,B]X: Processor Vendor
  CpuidRegisters regs = cpuid(0x0, 0);
  memcpy(out + 0, &regs.ebx, 4);
  memcpy(out + 4, &regs.edx, 4);
  memcpy(out + 8, &regs.ecx, 4);
}

static bool is_svm_supported() {
  // Fn8000_0001_ECX[2]: Feature Identifiers, SVM bit
  CpuidRegisters regs = cpuid(0x80000001, 0);
  if ((regs.ecx & (1 << 2)) == 0) {
    return false;
  }

  // VM_CR MSR (C001_0114h), SVMDIS bit
  if ((read_msr(0xC0010114) & (1 << 4)) != 0) {
    return false;
  }

  return true;
}

Vm vm_new() {
  // Check CPU vendor.
  uint8_t vendor[12];
  get_cpu_vendor_id(vendor);
  if (memcmp(vendor, "AuthenticAMD", 12) != 0) {
    LOG_ERROR("Unsupported CPU vendor: %s\n", vendor);
    return (Vm){.error = VM_ERROR_SYSTEM_NOT_SUPPORTED};
  }

  // Check if SVM is supported.
  if (!is_svm_supported()) {
    LOG_ERROR("Virtualization is not supported.\n");
    return (Vm){.error = VM_ERROR_SYSTEM_NOT_SUPPORTED};
  }

  Vcpu vcpu = vcpu_new(1);

  return (Vm){.error = VM_SUCESS, .vcpu = vcpu};
}

void vm_init(Vm *vm) {
  vcpu_virtualize(&vm->vcpu);
  LOG_INFO("vCPU #%d is created.\n", vm->vcpu.id);
}
