#pragma once

#include <stddef.h>

typedef struct {
  void *(*alloc)(size_t n);
  void (*free)(void *ptr, size_t n);
  void *(*alloc_aligned_pages)(size_t num_pages, size_t align_size);
} page_allocator_ops_t;
