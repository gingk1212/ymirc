#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch.h"
#include "asm.h"
#include "log.h"
#include "mem.h"
#include "panic.h"
#include "svm_npt.h"

/** Size in bytes of the guest memory. */
#define GUEST_MEMORY_SIZE (100ULL * 1024 * 1024)
static_assert(GUEST_MEMORY_SIZE % PAGE_SIZE_2MB == 0,
              "Guest memory size must be a multiple of 2MiB.");

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
  Vm vm = (Vm){.error = VM_SUCESS};

  // Check CPU vendor.
  uint8_t vendor[12];
  get_cpu_vendor_id(vendor);
  if (memcmp(vendor, "AuthenticAMD", 12) == 0) {
    vm.vtype = VIRTUALIZE_TYPE_SVM;
  } else {
    LOG_ERROR("Unsupported CPU vendor: %s\n", vendor);
    return (Vm){.error = VM_ERROR_SYSTEM_NOT_SUPPORTED};
  }

  if (vm.vtype == VIRTUALIZE_TYPE_SVM) {
    // Check if SVM is supported.
    if (!is_svm_supported()) {
      LOG_ERROR("Virtualization is not supported.\n");
      return (Vm){.error = VM_ERROR_SYSTEM_NOT_SUPPORTED};
    }

    vm.svmvcpu = svm_vcpu_new(1);
  }

  return vm;
}

void vm_init(Vm *vm, const page_allocator_ops_t *pa_ops) {
  // Initialize vCPU.
  svm_vcpu_virtualize(&vm->svmvcpu, pa_ops);
  LOG_INFO("vCPU #%d is created.\n", vm->svmvcpu.id);

  // Setup VMCB.
  svm_vcpu_setup_vmcb(&vm->svmvcpu);
}

void vm_loop(Vm *vm) {
  disable_intr();
  svm_vcpu_loop(&vm->svmvcpu);
}

void setup_guest_memory(Vm *vm, const page_allocator_ops_t *pa_ops) {
  // Allocate guest memory.
  vm->guest_mem =
      pa_ops->alloc_aligned_pages(GUEST_MEMORY_SIZE / PAGE_SIZE, PAGE_SIZE_2MB);
  if (!vm->guest_mem) {
    panic("Failed to allocate guest memory.");
  }

  // Create simple NPT mapping.
  Phys n_cr3 =
      init_npt(0, virt2phys((Virt)vm->guest_mem), GUEST_MEMORY_SIZE, pa_ops);
  svm_vcpu_set_npt(&vm->svmvcpu, n_cr3, vm->guest_mem);
  LOG_INFO("Guet memory is mapped: HVA=%p (size=0x%x)\n", vm->guest_mem,
           GUEST_MEMORY_SIZE);
}
