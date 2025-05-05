#include "page.h"

#include <stdint.h>

#include "bits.h"
#include "mem.h"
#include "page_allocator.h"

/** Size mapped by a single Level 4 page table entry (512GiB) */
#define LV4_ENTRY_MAPPING_SIZE (512ULL * 1024 * 1024 * 1024)

/** Shift in bits to extract the level-4 index from a virtual address. */
static const int lv4_shift = 39;
/** Shift in bits to extract the level-3 index from a virtual address. */
static const int lv3_shift = 30;
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
#define ENTRY_IGNORED1 9  // 2 bits: 9-10
#define ENTRY_RESTART 11
#define ENTRY_PHYS 12  // 51 bits: 12-62
#define ENTRY_XD 63

#define MASK(width) ((1ULL << (width)) - 1)
#define PHYS_MASK (MASK(51) << ENTRY_PHYS)

static PageTable *allocate_table() {
  PageTable *table_addr = mem_alloc_pages(1, PAGE_SIZE);
  for (uint64_t i = 0; i < NUM_TABLE_ENTRIES; i++) {
    table_addr->entries[i].value = 0;
  }
  return table_addr;
}

static void initialize_table_reference_entry(Entry *entry,
                                             PageTable *lower_table) {
  set_masked_bits(&entry->value, 1, tobit(ENTRY_PRESENT));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_RW));
  set_masked_bits(&entry->value, 0, tobit(ENTRY_US));
  set_masked_bits(&entry->value, 0, tobit(ENTRY_PS));
  set_masked_bits(&entry->value,
                  virt2phys((uintptr_t)lower_table) >> ENTRY_PHYS, PHYS_MASK);
}

static void initialize_page_reference_entry(Entry *entry, Phys phys) {
  set_masked_bits(&entry->value, 1, tobit(ENTRY_PRESENT));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_RW));
  set_masked_bits(&entry->value, 0, tobit(ENTRY_US));
  set_masked_bits(&entry->value, 1, tobit(ENTRY_PS));
  set_masked_bits(&entry->value, phys >> ENTRY_PHYS, PHYS_MASK);
}

static uintptr_t read_cr3(void) {
  uintptr_t cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

static void load_cr3(uintptr_t value) {
  __asm__ volatile("mov %0, %%cr3" : : "r"(value));
}

static PageTable *clone_lv1_table(PageTable *lv1tbl) {
  PageTable *new_lv1tbl = allocate_table();
  memcpy(new_lv1tbl, lv1tbl, sizeof(PageTable));

  return new_lv1tbl;
}

static PageTable *clone_lv2_table(PageTable *lv2tbl) {
  PageTable *new_lv2tbl = allocate_table();
  memcpy(new_lv2tbl, lv2tbl, sizeof(PageTable));

  for (int i = 0; i < NUM_TABLE_ENTRIES; i++) {
    Entry entry = new_lv2tbl->entries[i];
    if (!isset(entry.value, ENTRY_PRESENT)) continue;
    if (isset(entry.value, ENTRY_PS)) continue;

    PageTable *lv1tbl = (PageTable *)phys2virt(entry.value & PHYS_MASK);
    PageTable *new_lv1tbl = clone_lv1_table(lv1tbl);
    set_masked_bits(&entry.value,
                    virt2phys((uintptr_t)new_lv1tbl) >> ENTRY_PHYS, PHYS_MASK);
  }

  return new_lv2tbl;
}

static PageTable *clone_lv3_table(PageTable *lv3tbl) {
  PageTable *new_lv3tbl = allocate_table();
  memcpy(new_lv3tbl, lv3tbl, sizeof(PageTable));

  for (int i = 0; i < NUM_TABLE_ENTRIES; i++) {
    Entry entry = new_lv3tbl->entries[i];
    if (!isset(entry.value, ENTRY_PRESENT)) continue;
    if (isset(entry.value, ENTRY_PS)) continue;

    PageTable *lv2tbl = (PageTable *)phys2virt(entry.value & PHYS_MASK);
    PageTable *new_lv2tbl = clone_lv2_table(lv2tbl);
    set_masked_bits(&entry.value,
                    virt2phys((uintptr_t)new_lv2tbl) >> ENTRY_PHYS, PHYS_MASK);
  }

  return new_lv3tbl;
}

/** Directly map all memory with offset. After calling this function, it is safe
 * to unmap direct mappings. */
void reconstruct() {
  PageTable *lv4tbl = allocate_table();
  const uint64_t lv4idx_start = (DIRECT_MAP_BASE >> lv4_shift) & index_mask;
  const uint64_t lv4idx_end = lv4idx_start + (DIRECT_MAP_SIZE >> lv4_shift);

  // Create the direct mapping using 1GiB pages.
  for (uint64_t i = 0; i < lv4idx_end - lv4idx_start; i++) {
    PageTable *lv3tbl = allocate_table();
    for (uint64_t lv3idx = 0; lv3idx < NUM_TABLE_ENTRIES; lv3idx++) {
      initialize_page_reference_entry(&lv3tbl->entries[lv3idx],
                                      (i << lv4_shift) + (lv3idx << lv3_shift));
    }
    initialize_table_reference_entry(&lv4tbl->entries[lv4idx_start + i],
                                     lv3tbl);
  }

  // Recursively clone tables for the kernel region.
  PageTable *old_lv4tbl = (PageTable *)phys2virt(read_cr3());
  for (uint64_t lv4idx = lv4idx_end; lv4idx < NUM_TABLE_ENTRIES; lv4idx++) {
    Entry old_lv4entry = old_lv4tbl->entries[lv4idx];
    if (isset(old_lv4entry.value, ENTRY_PRESENT)) {
      PageTable *lv3tbl =
          (PageTable *)phys2virt(old_lv4entry.value & PHYS_MASK);
      PageTable *new_lv3tbl = clone_lv3_table(lv3tbl);
      initialize_table_reference_entry(&lv4tbl->entries[lv4idx], new_lv3tbl);
    }
  }

  // Set new lv4-table and flush all TLBs.
  load_cr3(virt2phys((uintptr_t)lv4tbl));

  set_mem_reconstructed(true);
}
