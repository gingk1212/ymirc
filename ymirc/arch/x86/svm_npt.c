#include "svm_npt.h"

#include <stdint.h>

#include "bits.h"
#include "log.h"
#include "panic.h"

/** Shift in bits to extract the level-4 index from a virtual address. */
static const int lv4_shift = 39;
/** Shift in bits to extract the level-3 index from a virtual address. */
static const int lv3_shift = 30;
/** Shift in bits to extract the level-1 index from a virtual address. */
static const int lv2_shift = 21;
/** Shift in bits to extract the level-1 index from a virtual address. */
static const int lv1_shift = 12;
/** Mask to extract page entry index from a shifted virtual address. */
static const int index_mask = 0x1FF;

/** Number of entries in a page table. */
#define NUM_TABLE_ENTRIES 512

typedef enum { level4, level3, level2, level1 } TableLevel;

/** Page table entry. */
typedef struct {
  uint64_t value;
} Entry;

/** Page table. */
typedef struct {
  Entry entries[NUM_TABLE_ENTRIES];
} PageTable;

#define ENTRY_PRESENT 0
#define ENTRY_RW 1
#define ENTRY_US 2
#define ENTRY_PWT 3
#define ENTRY_PCD 4
#define ENTRY_ACCESSED 5
#define ENTRY_DIRTY 6
#define ENTRY_PS 7
#define ENTRY_GLOBAL 8
#define ENTRY_AVAILABLE 9  // 3 bits: 9-11
#define ENTRY_PHYS 12      // 51 bits: 12-62
#define ENTRY_XD 63

#define MASK(width) ((1ULL << (width)) - 1)
#define PHYS_MASK (MASK(51) << ENTRY_PHYS)

static const page_allocator_ops_t *pa;

static PageTable *allocate_table() {
  PageTable *table_addr = pa->alloc_aligned_pages(1, PAGE_SIZE);
  if (!table_addr) {
    panic("Failed to allocate memory for the page table.");
  }
  for (uint64_t i = 0; i < NUM_TABLE_ENTRIES; i++) {
    table_addr->entries[i].value = 0;
  }
  return table_addr;
}

static void initialize_table_reference_entry(Entry *entry) {
  PageTable *lowertbl = allocate_table();
  set_masked_bits(&entry->value, 1, tobit(ENTRY_PRESENT));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_RW));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_US));
  set_masked_bits(&entry->value, 0, tobit(ENTRY_PS));
  set_masked_bits(&entry->value, virt2phys((uintptr_t)lowertbl) >> ENTRY_PHYS,
                  PHYS_MASK);
}

static void initialize_page_reference_entry(Entry *entry, Phys phys) {
  set_masked_bits(&entry->value, 1, tobit(ENTRY_PRESENT));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_RW));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_US));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_PS));
  set_masked_bits(&entry->value, phys >> ENTRY_PHYS, PHYS_MASK);
}

static Entry *get_entry_address(Phys gpa, Phys tbl_paddr, TableLevel level) {
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
  int index = gpa >> shift & index_mask;
  return (Entry *)phys2virt(tbl_paddr) + index;
}

/** Maps the given 2MiB host physical memory to the guest physical memory.
 * Caller must flush TLB. */
static void map2m(Phys gpa, Phys hpa, PageTable *lv4tbl) {
  uint64_t lv4index = (gpa >> lv4_shift) & index_mask;
  Entry *lv4ent = &lv4tbl->entries[lv4index];
  if (!isset(lv4ent->value, ENTRY_PRESENT)) {
    initialize_table_reference_entry(lv4ent);
  }

  Entry *lv3ent = get_entry_address(gpa, lv4ent->value & PHYS_MASK, level3);
  if (!isset(lv3ent->value, ENTRY_PRESENT)) {
    initialize_table_reference_entry(lv3ent);
  }

  Entry *lv2ent = get_entry_address(gpa, lv3ent->value & PHYS_MASK, level2);
  if (isset(lv2ent->value, ENTRY_PRESENT)) {
    panic("Page already mapped.");
  }
  initialize_page_reference_entry(lv2ent, hpa);
}

Phys init_npt(Phys guest_start, Phys host_start, size_t size,
              const page_allocator_ops_t *pa_ops) {
  if (pa_ops == NULL) {
    panic("Page allocator ops must be set for SVM NPT initializing.");
  }
  pa = pa_ops;

  // Allocate Level4 table.
  PageTable *lv4tbl = allocate_table();
  LOG_DEBUG("NPT level4 Table @ %p\n", lv4tbl);

  // 2MiB mapping.
  for (size_t i = 0; i < size / PAGE_SIZE_2MB; i++) {
    map2m(guest_start + PAGE_SIZE_2MB * i, host_start + PAGE_SIZE_2MB * i,
          lv4tbl);
  }

  return virt2phys((uintptr_t)lv4tbl);
}
