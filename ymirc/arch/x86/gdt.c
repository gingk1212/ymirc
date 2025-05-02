#include "gdt.h"

#include <stdint.h>

#include "bits.h"
#include "log.h"

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) Gdtr;

typedef struct {
  uint64_t value;
} SegmentDescriptor;

// Lower 16 bits of the segment limit.
#define SEGMENT_LIMIT_LOW_MASK 0xFFFFULL << 0
#define SEGMENT_LIMIT_LOW_WIDTH 16
// Lower 24 bits of the base address.
#define SEGMENT_BASE_LOW_MASK 0xFFFFFFULL << 16
#define SEGMENT_BASE_LOW_WIDTH 24
// Segment is accessed.
// You should set to true in case the descriptor is stored in the read-only
// pages.
#define SEGMENT_ACCESSED_MASK 0x1ULL << 40
// Readable / Writable.
// For code segment, true means the segment is readable (write access is not
// allowed for CS).
// For data segment, true means the segment is writable (read access is always
// allowed for DS).
#define SEGMENT_RW_MASK 0x1ULL << 41
// Direction / Conforming.
// For code selectors, conforming bit. If set to 1, code in the segment can be
// executed from an equal or lower privilege level.
// For data selectors, direction bit. If set to 0, the segment grows up; if set
// to 1, the segment grows down.
#define SEGMENT_DC_MASK 0x1ULL << 42
// Executable.
// If set to true, code segment. If set to false, data segment.
#define SEGMENT_EXECUTABLE_MASK 0x1ULL << 43
// Descriptor type.
#define SEGMENT_DESC_TYPE_MASK 0x1ULL << 44
// Descriptor Privilege Level.
#define SEGMENT_DPL_MASK 0x3ULL << 45
// Segment present.
#define SEGMENT_PRESENT_MASK 0x1ULL << 47
// Upper 4 bits of the segment limit.
#define SEGMENT_LIMIT_HIGH_MASK 0xFULL << 48
#define SEGMENT_LIMIT_HIGH_WIDTH 4
// Available for use by system software.
#define SEGMENT_AVL_MASK 0x1ULL << 52
// 64-bit code segment.
// If set to true, the code segment contains native 64-bit code.
// For data segments, this bit must be cleared to 0.
#define SEGMENT_LONG_MASK 0x1ULL << 53
// Size flag.
#define SEGMENT_DB_MASK 0x1ULL << 54
// Granularity.
// If set to .Byte, the segment limit is interpreted in byte units.
// Otherwise, the limit is interpreted in 4-KByte units.
// This field is ignored in 64-bit mode.
#define SEGMENT_GRANULARITY_MASK 0x1ULL << 55
// Upper 8 bits of the base address.
#define SEGMENT_BASE_HIGH_MASK 0xFFULL << 56
#define SEGMENT_BASE_HIGH_WIDTH 8

typedef struct {
  unsigned __int128 value;
} TssDescriptor;

// Lower 16 bits of the segment limit.
#define TSS_LIMIT_LOW_MASK 0xFFFFULL << 0
#define TSS_LIMIT_LOW_WIDTH 16
// Lower 24 bits of the base address.
#define TSS_BASE_LOW_MASK 0xFFFFFFULL << 16
#define TSS_BASE_LOW_WIDTH 24
// Type: TSS.
#define TSS_TYPE_MASK 0xFULL << 40
// Descriptor type: System.
#define TSS_DESC_TYPE_MASK 0x1ULL << 44
// Descriptor Privilege Level.
#define TSS_DPL_MASK 0x3ULL << 45
// Segment present.
#define TSS_PRESENT_MASK 0x1ULL << 47
// Upper 4 bits of the segment limit.
#define TSS_LIMIT_HIGH_MASK 0xFULL << 48
#define TSS_LIMIT_HIGH_WIDTH 4
// Available for use by system software.
#define TSS_AVL_MASK 0x1ULL << 52
// Reserved.
#define TSS_LONG_MASK 0x1ULL << 53
// Size flag.
#define TSS_DB_MASK 0x1ULL << 54
// Granularity.
#define TSS_GRANULARITY_MASK 0x1ULL << 55
// Upper 40 bits of the base address.
#define TSS_BASE_HIGH_MASK 0xFFFFFFFFFFULL << 56
#define TSS_BASE_HIGH_WIDTH 40
// Reserved.
#define TSS_RESERVED_MASK (unsigned __int128)0xFFFFFFFF << 96

typedef enum { system = 0, code_data } DescriptorType;
typedef enum { byte = 0, kbyte } Granularity;

#define MAX_NUM_GDT 0x10

// Global Desscriptor Table.
__attribute__((aligned(16))) static SegmentDescriptor gdt[MAX_NUM_GDT] = {0};

// Unused TSS segment.
__attribute__((aligned(4096))) static uint8_t tssUnused[4096] = {0};

static void init_segment_descriptor(SegmentDescriptor *desc, uint8_t rw,
                                    uint8_t dc, uint8_t executable,
                                    uint32_t base, uint32_t limit, uint8_t dpl,
                                    Granularity granularity) {
  set_masked_bits(&desc->value, get_lower_bits(limit, SEGMENT_LIMIT_LOW_WIDTH),
                  SEGMENT_LIMIT_LOW_MASK);
  set_masked_bits(&desc->value, get_lower_bits(base, SEGMENT_BASE_LOW_WIDTH),
                  SEGMENT_BASE_LOW_MASK);
  set_masked_bits(&desc->value, 0, SEGMENT_ACCESSED_MASK);
  set_masked_bits(&desc->value, rw, SEGMENT_RW_MASK);
  set_masked_bits(&desc->value, dc, SEGMENT_DC_MASK);
  set_masked_bits(&desc->value, executable, SEGMENT_EXECUTABLE_MASK);
  set_masked_bits(&desc->value, code_data, SEGMENT_DESC_TYPE_MASK);
  set_masked_bits(&desc->value, dpl, SEGMENT_DPL_MASK);
  set_masked_bits(&desc->value, 1, SEGMENT_PRESENT_MASK);
  set_masked_bits(&desc->value,
                  get_lower_bits(limit >> SEGMENT_LIMIT_LOW_WIDTH,
                                 SEGMENT_LIMIT_HIGH_WIDTH),
                  SEGMENT_LIMIT_HIGH_MASK);
  set_masked_bits(&desc->value, 0, SEGMENT_AVL_MASK);
  set_masked_bits(&desc->value, executable, SEGMENT_LONG_MASK);
  set_masked_bits(&desc->value, !executable, SEGMENT_DB_MASK);
  set_masked_bits(&desc->value, granularity, SEGMENT_GRANULARITY_MASK);
  set_masked_bits(
      &desc->value,
      get_lower_bits(base >> SEGMENT_BASE_LOW_WIDTH, SEGMENT_BASE_HIGH_WIDTH),
      SEGMENT_BASE_HIGH_MASK);
}

static void init_tss_descriptor(TssDescriptor *desc, uint64_t base,
                                uint32_t limit) {
  set_masked_bits_128(&desc->value,
                      get_lower_bits_128(limit, TSS_LIMIT_LOW_WIDTH),
                      TSS_LIMIT_LOW_MASK);
  set_masked_bits_128(&desc->value,
                      get_lower_bits_128(base, TSS_BASE_LOW_WIDTH),
                      TSS_BASE_LOW_MASK);
  set_masked_bits_128(&desc->value, 0b1001, TSS_TYPE_MASK);
  set_masked_bits_128(&desc->value, system, TSS_DESC_TYPE_MASK);
  set_masked_bits_128(&desc->value, 0, TSS_DPL_MASK);
  set_masked_bits_128(&desc->value, 1, TSS_PRESENT_MASK);
  set_masked_bits_128(
      &desc->value,
      get_lower_bits_128(limit >> TSS_LIMIT_LOW_WIDTH, TSS_LIMIT_HIGH_WIDTH),
      TSS_LIMIT_HIGH_MASK);
  set_masked_bits_128(&desc->value, 0, TSS_AVL_MASK);
  set_masked_bits_128(&desc->value, 1, TSS_LONG_MASK);
  set_masked_bits_128(&desc->value, 0, TSS_DB_MASK);
  set_masked_bits_128(&desc->value, kbyte, TSS_GRANULARITY_MASK);
  set_masked_bits_128(
      &desc->value,
      get_lower_bits_128(base >> TSS_BASE_LOW_WIDTH, TSS_BASE_HIGH_WIDTH),
      TSS_BASE_HIGH_MASK);
  set_masked_bits_128(&desc->value, 0, TSS_RESERVED_MASK);
}

static inline void lgdt(Gdtr *gdtr) {
  __asm__ volatile("lgdt (%0)" : : "r"(gdtr));
}

static void setupGdtr() {
  Gdtr gdtr = {.limit = sizeof(gdt) - 1, .base = (uintptr_t)&gdt};
  // Ymir uses UEFI's identity map, so &gdt (virtual) equals its physical
  // address.
  lgdt(&gdtr);
}

/**
 * Load the kernel data segment selector. This function flushes the changes of
 * DS in the GDT.
 */
static inline void load_kernel_ds() {
  __asm__ volatile(
      "mov %0, %%di\n\t"
      "mov %%di, %%ds\n\t"
      "mov %%di, %%es\n\t"
      "mov %%di, %%fs\n\t"
      "mov %%di, %%gs\n\t"
      "mov %%di, %%ss"
      :
      : "n"(SEGMENT_SELECTOR(KERNEL_DS_INDEX))
      : "di");
}

/**
 * Load the kernel code segment selector. This function flushes the changes of
 * CS in the GDT. CS cannot be loaded directly by mov, so we use far-return.
 */
static inline void load_kernel_cs() {
  __asm__ volatile(
      "mov %0, %%rax\n\t"
      "push %%rax\n\t"
      "leaq next(%%rip), %%rax\n\t"
      "pushq %%rax\n\t"
      "lretq\n\t"
      "next:"
      :
      : "n"(SEGMENT_SELECTOR(KERNEL_CS_INDEX)));
}

/** Load the kernel TSS selector to TR. Not used in YmirC. */
static inline void load_kernel_tss() {
  __asm__ volatile(
      "mov %0, %%di\n\t"
      "ltr %%di"
      :
      : "n"(SEGMENT_SELECTOR(KERNEL_TSS_INDEX))
      : "di");
}

/** tss_addr is virtual address. */
static void setTss(uintptr_t tss_addr) {
  TssDescriptor *tss_desc = (TssDescriptor *)&gdt[KERNEL_TSS_INDEX];
  init_tss_descriptor(tss_desc, tss_addr, 0xFFFFF);
  load_kernel_tss();
}

/** Initialize the GDT. */
void gdt_init() {
  init_segment_descriptor(&gdt[KERNEL_CS_INDEX], 1, 0, 1, 0, 0xFFFFF, 0, kbyte);
  init_segment_descriptor(&gdt[KERNEL_DS_INDEX], 1, 0, 0, 0, 0xFFFFF, 0, kbyte);

  // Setup GDT register.
  setupGdtr();

  // Changing the entries in the GDT, or setting GDTR does not automatically
  // update the hidden(shadow) part. To flush the changes, we need to set
  // segment registers.
  load_kernel_ds();
  load_kernel_cs();

  // TSS is not used by YmirC. But we have to set it for VMX.
  setTss((uintptr_t)tssUnused);
}
