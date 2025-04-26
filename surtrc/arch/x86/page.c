#include "page.h"

#include <efi.h>
#include <efilib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

static Phys allocate_pages(EFI_ALLOCATE_TYPE allocate_type,
                           EFI_MEMORY_TYPE memory_type, UINTN nopages) {
  Phys addr;
  EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, allocate_type,
                                        memory_type, nopages,
                                        &addr);  // NOTE: uint64_t -> uintptr_t
  if (status != EFI_SUCCESS) {
    LOG_ERROR(L"Failed to allocate memory.\n");
    exit(1);
  }
  return addr;
}

static Phys allocate_table() {
  Phys table_addr = allocate_pages(AllocateAnyPages, EfiBootServicesData, 1);
  ZeroMem((void *)table_addr, 4096);
  return table_addr;
}

typedef enum { level4, level3, level2, level1 } TableLevel;

typedef struct {
  uint64_t value;
} Entry;

#define ENTRY_PRESENT 0
#define ENTRY_RW 1
#define ENTRY_US 2
#define ENTRY_PWT 3
#define ENTRY_PCD 4
#define ENTRY_ACCESSED 5
#define ENTRY_DIRTY 6
#define ENTRY_PS 7
#define ENTRY_GLOBAL 8
#define ENTRY_IGNORED1 9  // 2 bits: 9-10
#define ENTRY_RESTART 11
#define ENTRY_PHYS 12  // 51 bits: 12-62
#define ENTRY_XD 63

#define BIT(n) (1ULL << (n))
#define IS_BIT_SET(entry, n) ((entry) & BIT(n))
#define MASK(width) ((1ULL << (width)) - 1)
#define PHYS_MASK (MASK(51) << ENTRY_PHYS)

static void initialize_table_reference_entry(Entry *entry) {
  Phys lower_table_addr = allocate_table();
  entry->value |= BIT(ENTRY_PRESENT);
  entry->value |= BIT(ENTRY_RW);
  entry->value &= ~BIT(ENTRY_US);
  entry->value &= ~BIT(ENTRY_PS);
  entry->value |= (lower_table_addr & PHYS_MASK);
}

static void initialize_page_reference_entry(Entry *entry, Phys phys) {
  entry->value |= BIT(ENTRY_PRESENT);
  entry->value |= BIT(ENTRY_RW);
  entry->value &= ~BIT(ENTRY_US);
  entry->value |= BIT(ENTRY_PS);
  entry->value |= (phys & PHYS_MASK);
}

static uintptr_t read_cr3(void) {
  uintptr_t cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

static void load_cr3(uintptr_t value) {
  __asm__ volatile("mov %0, %%cr3" : : "r"(value));
}

static const int lv4_shift = 39;
static const int lv3_shift = 30;
static const int lv2_shift = 21;
static const int lv1_shift = 12;
static const int index_mask = 0x1FF;

static Entry *get_entry_address(Virt virt, Phys table_addr, TableLevel level) {
  int shift = 0;
  switch (level) {
    case level4:
      shift = lv4_shift;
      break;
    case level3:
      shift = lv3_shift;
      break;
    case level2:
      shift = lv2_shift;
      break;
    case level1:
      shift = lv1_shift;
      break;
  }
  int index = virt >> shift & index_mask;
  return (Entry *)table_addr + index;
}

void map_4k_to(Virt virt, Phys phys) {
  Entry *lv4ent = get_entry_address(virt, read_cr3(), level4);
  if (!IS_BIT_SET(lv4ent->value, ENTRY_PRESENT)) {
    initialize_table_reference_entry(lv4ent);
  }

  Entry *lv3ent = get_entry_address(virt, lv4ent->value & PHYS_MASK, level3);
  if (!IS_BIT_SET(lv3ent->value, ENTRY_PRESENT)) {
    initialize_table_reference_entry(lv3ent);
  }

  Entry *lv2ent = get_entry_address(virt, lv3ent->value & PHYS_MASK, level2);
  if (!IS_BIT_SET(lv2ent->value, ENTRY_PRESENT)) {
    initialize_table_reference_entry(lv2ent);
  }

  Entry *lv1ent = get_entry_address(virt, lv2ent->value & PHYS_MASK, level1);
  if (IS_BIT_SET(lv1ent->value, ENTRY_PRESENT)) {
    LOG_ERROR(L"Page already mapped\n");
    exit(1);
  }
  initialize_page_reference_entry(lv1ent, phys);
}

void set_lv4table_writable() {
  Phys lv4table_addr = allocate_pages(AllocateAnyPages, EfiBootServicesData, 1);
  Phys cr3 = read_cr3();
  CopyMem((void *)lv4table_addr, (void *)cr3, 4096);
  load_cr3(lv4table_addr);
}
