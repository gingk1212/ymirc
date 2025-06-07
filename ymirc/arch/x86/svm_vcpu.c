#include "svm_vcpu.h"

#include <stdnoreturn.h>

#include "arch.h"
#include "asm.h"
#include "bits.h"
#include "log.h"
#include "mem.h"
#include "panic.h"

static void setup_vmcb_seg(Vmcb *vmcb);
__attribute__((naked)) noreturn void blob_guest();

SvmVcpu svm_vcpu_new(uint16_t asid) { return (SvmVcpu){.id = 0, .asid = asid}; }

void svm_vcpu_virtualize(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops) {
  // Write to VM_HSAVE_PA MSR.
  void *hsave = pa_ops->alloc(PAGE_SIZE);
  if (!hsave) {
    panic("Failed to allocate memory for saving host state.");
  }
  write_msr(0xC0010117, virt2phys((uintptr_t)hsave));

  // Allocate VMCB region.
  void *vmcb = pa_ops->alloc_aligned_pages(1, PAGE_SIZE);
  if (!vmcb) {
    panic("Failed to allocate memory for VMCB.");
  }
  memset(vmcb, 0, PAGE_SIZE);
  vcpu->vmcb = vmcb;

  // Extended Feature Enable Register (EFER)
  uint64_t efer = read_msr(0xC0000080);
  efer |= 1ULL << 12;  // 12: EFFR.SVME bit
  write_msr(0xC0000080, efer);
}

void svm_vcpu_setup_vmcb(SvmVcpu *vcpu) {
  Vmcb *vmcb = vcpu->vmcb;

  // VMRUN intercept bit must be set.
  vmcb->intercept_vmrun = 1;

  // ASID
  vmcb->guest_asid = vcpu->asid;

  // Segment registers.
  setup_vmcb_seg(vmcb);

  // EFER
  vmcb->efer = read_msr(0xC0000080);

  // Control registers
  vmcb->cr4 = (uint64_t)read_cr4();
  vmcb->cr3 = (uint64_t)read_cr3();
  vmcb->cr0 = (uint64_t)read_cr0();

  // RIP
  vmcb->rip = (uint64_t)&blob_guest;
}

static void vmexit_handler(Vmcb *vmcb) {
  LOG_DEBUG("[VMEXIT handler]\n");
  LOG_DEBUG("  EXITCODE=0x%x\n", vmcb->exitcode);
  LOG_DEBUG("  EXITINFO1=0x%x\n", vmcb->exitinfo1);
  LOG_DEBUG("  EXITINFO2=0x%x\n", vmcb->exitinfo2);
  endless_halt();
}

void svm_vcpu_loop(SvmVcpu *vcpu) {
  __asm__ volatile(
      "mov %0, %%rax\n\t"
      "vmrun %%rax\n\t"
      :
      : "r"(virt2phys((uintptr_t)vcpu->vmcb))
      : "rax", "memory");

  vmexit_handler(vcpu->vmcb);
}

/** segment attributes are stored as 12-bit values formed by the concatenation
 * of bits 55:52 and 47:40 from the original 64-bit (in-memory) segment
 * descriptors; */
#define SEGMENT_ACCESSED_MASK 0x1ULL << 0
#define SEGMENT_RW_MASK 0x1ULL << 1
#define SEGMENT_DC_MASK 0x1ULL << 2
#define SEGMENT_EXECUTABLE_MASK 0x1ULL << 3
#define SEGMENT_DESC_TYPE_MASK 0x1ULL << 4
#define SEGMENT_DPL_MASK 0x3ULL << 5
#define SEGMENT_PRESENT_MASK 0x1ULL << 7
#define SEGMENT_AVL_MASK 0x1ULL << 8
#define SEGMENT_LONG_MASK 0x1ULL << 9
#define SEGMENT_DB_MASK 0x1ULL << 10
#define SEGMENT_GRANULARITY_MASK 0x1ULL << 11
#define SEGMENT_RESERVED_MASK 0xFULL << 12

static void setup_vmcb_seg(Vmcb *vmcb) {
  // CS attrib
  uint64_t cs_attrib = 0;
  set_masked_bits(&cs_attrib, 1, SEGMENT_ACCESSED_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_RW_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_DC_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_EXECUTABLE_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_DESC_TYPE_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_DPL_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_PRESENT_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_AVL_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_LONG_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_DB_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_GRANULARITY_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_RESERVED_MASK);

  // CS
  vmcb->cs.attr = (uint16_t)cs_attrib;
  vmcb->cs.base = 0xDEAD00;  // Marker to indicate the guest.
}

__attribute__((naked)) noreturn void blob_guest() {
  while (1) {
    __asm__ volatile("hlt");
  }
}
