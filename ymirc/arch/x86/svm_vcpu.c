#include "svm_vcpu.h"

#include "arch.h"
#include "asm.h"
#include "bits.h"
#include "gdt.h"
#include "linux.h"
#include "log.h"
#include "panic.h"
#include "svm_asm.h"
#include "svm_cpuid.h"
#include "svm_msr.h"

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
  // CS
  uint64_t cs_attrib = 0;
  set_masked_bits(&cs_attrib, 1, SEGMENT_ACCESSED_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_RW_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_DC_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_EXECUTABLE_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_DESC_TYPE_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_DPL_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_PRESENT_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_AVL_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_LONG_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_DB_MASK);
  set_masked_bits(&cs_attrib, 1, SEGMENT_GRANULARITY_MASK);
  set_masked_bits(&cs_attrib, 0, SEGMENT_RESERVED_MASK);
  vmcb->cs.attr = (uint16_t)cs_attrib;
  vmcb->cs.limit = UINT32_MAX;

  // DS
  uint64_t ds_attrib = 0;
  set_masked_bits(&ds_attrib, 1, SEGMENT_ACCESSED_MASK);
  set_masked_bits(&ds_attrib, 1, SEGMENT_RW_MASK);
  set_masked_bits(&ds_attrib, 0, SEGMENT_DC_MASK);
  set_masked_bits(&ds_attrib, 0, SEGMENT_EXECUTABLE_MASK);
  set_masked_bits(&ds_attrib, 1, SEGMENT_DESC_TYPE_MASK);
  set_masked_bits(&ds_attrib, 0, SEGMENT_DPL_MASK);
  set_masked_bits(&ds_attrib, 1, SEGMENT_PRESENT_MASK);
  set_masked_bits(&ds_attrib, 0, SEGMENT_AVL_MASK);
  set_masked_bits(&ds_attrib, 0, SEGMENT_LONG_MASK);
  set_masked_bits(&ds_attrib, 1, SEGMENT_DB_MASK);
  set_masked_bits(&ds_attrib, 1, SEGMENT_GRANULARITY_MASK);
  set_masked_bits(&ds_attrib, 0, SEGMENT_RESERVED_MASK);
  vmcb->ds.attr = (uint16_t)ds_attrib;
  vmcb->ds.limit = UINT32_MAX;

  // ES
  vmcb->es.attr = (uint16_t)ds_attrib;
  vmcb->es.limit = UINT32_MAX;

  // FS (VMLOAD is required)
  vmcb->fs.attr = (uint16_t)ds_attrib;
  vmcb->fs.limit = UINT32_MAX;

  // GS (VMLOAD is required)
  vmcb->gs.attr = (uint16_t)ds_attrib;
  vmcb->gs.limit = UINT32_MAX;

  // SS
  vmcb->ss.attr = (uint16_t)ds_attrib;
  vmcb->ss.limit = UINT32_MAX;
}

/** Configure intercepts for all MSR read and write instructions. */
static void setup_vmcb_msr(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops) {
  Vmcb *vmcb = vcpu->vmcb;
  void *msrpm = pa_ops->alloc_aligned_pages(2, PAGE_SIZE);
  memset(msrpm, 0xFF, PAGE_SIZE * 2);
  vmcb->msrpm_base_pa = virt2phys((Virt)msrpm);
  vmcb->intercept_msr_prot = 1;
}

SvmVcpu svm_vcpu_new(uint16_t asid) { return (SvmVcpu){.id = 0, .asid = asid}; }

void svm_vcpu_virtualize(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops) {
  // Write to VM_HSAVE_PA MSR.
  void *hsave = pa_ops->alloc(PAGE_SIZE);
  if (!hsave) {
    panic("Failed to allocate memory for saving host state.");
  }
  write_msr(MSR_VM_HSAVE_PA, virt2phys((uintptr_t)hsave));

  // Allocate VMCB region.
  void *vmcb = pa_ops->alloc_aligned_pages(1, PAGE_SIZE);
  if (!vmcb) {
    panic("Failed to allocate memory for VMCB.");
  }
  memset(vmcb, 0, PAGE_SIZE);
  vcpu->vmcb = vmcb;
  vcpu->vmcb_phys = virt2phys((uintptr_t)vmcb);

  // Extended Feature Enable Register (EFER)
  uint64_t efer = read_msr(MSR_EFER);
  efer |= 1ULL << 12;  // 12: EFFR.SVME bit
  write_msr(MSR_EFER, efer);
}

/** Set up VMCB for a logical processor. */
static void setup_vmcb(SvmVcpu *vcpu, const page_allocator_ops_t *pa_ops) {
  Vmcb *vmcb = vcpu->vmcb;

  // Virtualize CPUID.
  vmcb->intercept_cpuid = 1;

  // VMRUN intercept bit must be set.
  vmcb->intercept_vmrun = 1;

  // ASID
  vmcb->guest_asid = vcpu->asid;

  // Enable nested paging.
  vmcb->np_enable = 1;

  // Segment registers.
  setup_vmcb_seg(vmcb);

  // EFER.
  // The effect of turning off EFER.SVME while a guest is running is undefined.
  Efer efer = {0};
  efer.svme = 1;
  vmcb->efer = efer.value;

  // Cr0
  Cr0 cr0 = {0};
  cr0.pe = 1;  // Protection Enabled
  cr0.ne = 1;  // Numeric Error
  cr0.et = 1;  // Extension type
  cr0.pg = 0;  // Paging
  vmcb->cr0 = cr0.value;

  // RIP
  vmcb->rip = LINUX_LAYOUT_KERNEL_BASE;

  // MSR
  setup_vmcb_msr(vcpu, pa_ops);
}

void svm_vcpu_setup_guest_state(SvmVcpu *vcpu,
                                const page_allocator_ops_t *pa_ops) {
  setup_vmcb(vcpu, pa_ops);
  vcpu->guest_regs.rsi = LINUX_LAYOUT_BOOTPARAM;
}

void svm_vcpu_set_npt(SvmVcpu *vcpu, Phys n_cr3, void *host_start) {
  vcpu->vmcb->n_cr3 = n_cr3;
  vcpu->guest_base = virt2phys((uintptr_t)host_start);
}

static void print_guest_state(SvmVcpu *vcpu) {
  LOG_ERROR("=== vCPU Information ===\n");
  LOG_ERROR("[Guest State]\n");
  LOG_ERROR("RIP: 0x%x\n", vcpu->vmcb->rip);
  LOG_ERROR("RSP: 0x%x\n", vcpu->vmcb->rsp);
  LOG_ERROR("RAX: 0x%x\n", vcpu->vmcb->rax);
  LOG_ERROR("RBX: 0x%x\n", vcpu->guest_regs.rbx);
  LOG_ERROR("RCX: 0x%x\n", vcpu->guest_regs.rcx);
  LOG_ERROR("RDX: 0x%x\n", vcpu->guest_regs.rdx);
  LOG_ERROR("RSI: 0x%x\n", vcpu->guest_regs.rsi);
  LOG_ERROR("RDI: 0x%x\n", vcpu->guest_regs.rdi);
  LOG_ERROR("RBP: 0x%x\n", vcpu->guest_regs.rbp);
  LOG_ERROR("R8 : 0x%x\n", vcpu->guest_regs.r8);
  LOG_ERROR("R9 : 0x%x\n", vcpu->guest_regs.r9);
  LOG_ERROR("R10: 0x%x\n", vcpu->guest_regs.r10);
  LOG_ERROR("R11: 0x%x\n", vcpu->guest_regs.r11);
  LOG_ERROR("R12: 0x%x\n", vcpu->guest_regs.r12);
  LOG_ERROR("R13: 0x%x\n", vcpu->guest_regs.r13);
  LOG_ERROR("R14: 0x%x\n", vcpu->guest_regs.r14);
  LOG_ERROR("R15: 0x%x\n", vcpu->guest_regs.r15);
  LOG_ERROR("CR0: 0x%x\n", vcpu->vmcb->cr0);
  LOG_ERROR("CR3: 0x%x\n", vcpu->vmcb->cr3);
  LOG_ERROR("CR4: 0x%x\n", vcpu->vmcb->cr4);
  LOG_ERROR("EFER:0x%x\n", vcpu->vmcb->efer);
  LOG_ERROR("CS : 0x%x 0x%x 0x%x\n", vcpu->vmcb->cs.sel, vcpu->vmcb->cs.base,
            vcpu->vmcb->cs.limit);
}

static void print_exit_info(SvmVcpu *vcpu) {
  LOG_ERROR("=== #VMEXIT Information ===\n");
  LOG_ERROR("EXITCODE : 0x%x\n", vcpu->vmcb->exitcode);
  LOG_ERROR("EXITINFO1: 0x%x\n", vcpu->vmcb->exitinfo1);
  LOG_ERROR("EXITINFO2: 0x%x\n", vcpu->vmcb->exitinfo2);
}

/** Dump guest state. */
static void dump(SvmVcpu *vcpu) { print_guest_state(vcpu); }

noreturn void svm_vcpu_abort(SvmVcpu *vcpu) {
  dump(vcpu);
  endless_halt();
}

/** Increment RIP. */
static void step_next_inst(Vmcb *vmcb) { vmcb->rip = vmcb->nrip; }

/** Load segment registers.
 * Since the CPU does not restore the host's FS and GS registers, they must be
 * restored manually. */
static inline void load_segment_registers() {
  __asm__ volatile(
      "mov %0, %%di\n\t"
      "mov %%di, %%fs\n\t"
      "mov %%di, %%gs"
      :
      : "n"(SEGMENT_SELECTOR(KERNEL_DS_INDEX))
      : "di");
}

/** Handle the #VMEXIT. */
static void handle_exit(SvmVcpu *vcpu) {
  // Load FS, GS.
  load_segment_registers();

  switch (vcpu->vmcb->exitcode) {
    case SVM_EXIT_CODE_CPUID:
      handle_svm_cpuid_exit(vcpu);
      step_next_inst(vcpu->vmcb);
      break;
    case SVM_EXIT_CODE_MSR:
      handle_svm_msr_exit(vcpu);
      step_next_inst(vcpu->vmcb);
      break;
    default:
      print_exit_info(vcpu);
      svm_vcpu_abort(vcpu);
  }
}

void svm_vcpu_loop(SvmVcpu *vcpu) {
  // Start endless VMRUN / #VMEXIT loop.
  while (1) {
    // VMRUN. Clobbers all caller-saved registers since this inline assembly
    // performs a function call.
    __asm__ volatile(
        "mov %0, %%rdi\n\t"
        "call asm_vmrun"
        :
        : "r"(vcpu)
        : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11");

    // Handle #VMEXIT
    handle_exit(vcpu);
  }
}
