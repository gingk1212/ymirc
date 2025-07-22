#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch.h"
#include "asm.h"
#include "cpuid.h"
#include "linux.h"
#include "log.h"
#include "mem.h"
#include "panic.h"
#include "svm_npt.h"

/** Size in bytes of the guest memory. */
#define GUEST_MEMORY_SIZE (100ULL * 1024 * 1024)
static_assert(GUEST_MEMORY_SIZE % PAGE_SIZE_2MB == 0,
              "Guest memory size must be a multiple of 2MiB.");

/** cmdline */
#define KERNEL_CMDLINE "console=ttyS0 earlyprintk=serial nokaslr"
#define KERNEL_CMDLINE_LEN (sizeof(KERNEL_CMDLINE) - 1)

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

  // Setup guest state.
  svm_vcpu_setup_guest_state(&vm->svmvcpu);
}

void vm_loop(Vm *vm) {
  disable_intr();
  svm_vcpu_loop(&vm->svmvcpu);
}

static void load_image(uint8_t *memory, size_t memory_size, const void *image,
                       size_t image_size, size_t addr) {
  if (memory_size < addr + image_size) {
    panic("Guest memory size is insufficient.");
  }
  memcpy(memory + addr, image, image_size);
}

/** Load a protected kernel image and cmdline to the guest physical memory. */
static void load_kernel(Vm *vm, const void *kernel, size_t kernel_size) {
  void *guest_mem = vm->guest_mem;

  if (kernel_size >= GUEST_MEMORY_SIZE) {
    panic("bzImage size exceeds guest memory size.");
  }

  BootParams bp = {0};
  bootparams_from_bzimage(&bp, kernel);
  bp.e820_entries = 0;

  // Setup necessary fields.
  bp.hdr.type_of_loader = 0xFF;
  bp.hdr.ext_loader_ver = 0;
  bp.hdr.loadflags.loaded_high = 1;   // load kernel at 0x10_0000
  bp.hdr.loadflags.can_use_heap = 1;  // use memory 0..BOOTPARAM as heap
  bp.hdr.heap_end_ptr = LINUX_LAYOUT_BOOTPARAM - 0x200;
  bp.hdr.loadflags.keep_segments =
      1;  // we set CS/DS/SS/ES to flag segments with a base of 0.
  bp.hdr.cmd_line_ptr = LINUX_LAYOUT_CMDLINE;
  bp.hdr.vid_mode = 0xFFFF;  // VGA (normal)

  // Setup E820 map
  bootparams_add_e820_entry(&bp, 0, LINUX_LAYOUT_KERNEL_BASE, E820_TYPE_RAM);
  bootparams_add_e820_entry(&bp, LINUX_LAYOUT_KERNEL_BASE,
                            GUEST_MEMORY_SIZE - LINUX_LAYOUT_KERNEL_BASE,
                            E820_TYPE_RAM);

  // Setup cmdline
  uint32_t cmdline_max_size =
      bp.hdr.cmdline_size < 256 ? bp.hdr.cmdline_size : 256;
  uint8_t *cmdline = (uint8_t *)guest_mem + LINUX_LAYOUT_CMDLINE;
  const char *cmdline_val = KERNEL_CMDLINE;
  size_t len = KERNEL_CMDLINE_LEN < cmdline_max_size ? KERNEL_CMDLINE_LEN
                                                     : cmdline_max_size;
  memset(cmdline, 0, cmdline_max_size);
  memcpy(cmdline, cmdline_val, len);

  // Copy boot_params
  load_image(guest_mem, GUEST_MEMORY_SIZE, &bp, sizeof(bp),
             LINUX_LAYOUT_BOOTPARAM);

  // Load protected-mode kernel code
  size_t code_offset = protected_code_offset(&bp.hdr);
  size_t code_size = kernel_size - code_offset;
  load_image(guest_mem, GUEST_MEMORY_SIZE, (uint8_t *)kernel + code_offset,
             code_size, LINUX_LAYOUT_KERNEL_BASE);

  LOG_INFO("Guest memory region: 0x%x - 0x%x\n", 0, GUEST_MEMORY_SIZE);
  LOG_INFO("Guest kernel code offset: 0x%x\n", code_offset);
}

void setup_guest_memory(Vm *vm, const void *guest_image,
                        size_t guest_image_size,
                        const page_allocator_ops_t *pa_ops) {
  // Allocate guest memory.
  vm->guest_mem =
      pa_ops->alloc_aligned_pages(GUEST_MEMORY_SIZE / PAGE_SIZE, PAGE_SIZE_2MB);
  if (!vm->guest_mem) {
    panic("Failed to allocate guest memory.");
  }

  // Load kernel
  load_kernel(vm, guest_image, guest_image_size);

  // Create simple NPT mapping.
  Phys n_cr3 =
      init_npt(0, virt2phys((Virt)vm->guest_mem), GUEST_MEMORY_SIZE, pa_ops);
  svm_vcpu_set_npt(&vm->svmvcpu, n_cr3, vm->guest_mem);
  LOG_INFO("Guet memory is mapped: HVA=%p (size=0x%x)\n", vm->guest_mem,
           GUEST_MEMORY_SIZE);
}
