#include "bin_allocator.h"

#include <stdint.h>

#include "log.h"
#include "mem.h"
#include "panic.h"

/** Metadata of free chunk. */
typedef struct _chunk_meta_node {
  struct _chunk_meta_node* next;
} chunk_meta_node_t;

#define BIN_SIZE_NUM 7

/** The largest bin size must be smaller than a 4KiB page size. */
static const unsigned int bin_sizes[] = {0x20,  0x40,  0x80, 0x100,
                                         0x200, 0x400, 0x800};

static chunk_meta_node_t* list_heads[BIN_SIZE_NUM];

static const page_allocator_ops_t* pa;

/** Initialize the BinAllocator. */
void init_bin_allocator(const page_allocator_ops_t* ops) {
  if (ops == NULL) {
    panic("BinAllocator: page allocator ops is NULL");
  }
  pa = ops;
}

static void push(chunk_meta_node_t* node, size_t bin_index) {
  if (list_heads[bin_index]) {
    node->next = list_heads[bin_index];
    list_heads[bin_index] = node;
  } else {
    node->next = NULL;
    list_heads[bin_index] = node;
  }
}

static chunk_meta_node_t* pop(size_t bin_index) {
  if (list_heads[bin_index]) {
    chunk_meta_node_t* first = list_heads[bin_index];
    list_heads[bin_index] = first->next;
    return first;
  } else {
    panic("BinAllocator: pop from empty list");
  }
}

/**
 * @return 0 on success, -1 on failure.
 */
static int init_bin_page(size_t bin_index) {
  void* new_page = pa->alloc(PAGE_SIZE);
  if (!new_page) {
    return -1;
  }

  unsigned int bin_size = bin_sizes[bin_index];
  int i = PAGE_SIZE / bin_size - 1;
  for (; i >= 0; i--) {
    chunk_meta_node_t* chunk =
        (chunk_meta_node_t*)((uint8_t*)new_page + i * bin_size);
    push(chunk, bin_index);
  }

  return 0;
}

static void* alloc_from_bin(size_t bin_index) {
  if (!list_heads[bin_index]) {
    if (init_bin_page(bin_index) != 0) {
      return NULL;
    }
  }
  return (void*)pop(bin_index);
}

static void free_to_bin(size_t bin_index, void* ptr) {
  push((chunk_meta_node_t*)ptr, bin_index);
}

/**
 * @return -1 if not found.
 */
static int bin_index(size_t size) {
  for (int i = 0; i < BIN_SIZE_NUM; i++) {
    if (size <= bin_sizes[i]) {
      return i;
    }
  }
  return -1;
}

void* bin_alloc(size_t n) {
  int index = bin_index(n);
  if (index >= 0) {
    return alloc_from_bin(index);
  } else {
    // Requested size exceeds a 4KiB page size.
    void* ptr = pa->alloc(n);
    if (!ptr) {
      return NULL;
    }
    return ptr;
  }
}

/** Determines whether `addr` is properly aligned for allocation into a bin of
 * size `bin_size`, assuming a 4KiB chunk base. */
static bool is_aligned_to_bin(uintptr_t addr, unsigned int bin_size) {
  uint64_t offset = addr % PAGE_SIZE;
  return offset % bin_size == 0;
}

// FIXME: Size should not be passed.
// FIXME: Check if ptr is valid.
void bin_free(void* ptr, size_t n) {
  int index = bin_index(n);
  if (index >= 0) {
    if (!is_aligned_to_bin((uintptr_t)ptr, bin_sizes[index])) {
      LOG_ERROR("BinAllocator: invalid pointer passed tgo free");
      return;
    }
    free_to_bin(index, ptr);
  } else {
    pa->free(ptr, n);
  }
}
