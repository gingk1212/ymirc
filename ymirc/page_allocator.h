#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../surtrc/def.h"

void page_allocator_init(MemoryMap *map);
void *page_allocator_alloc(size_t n);
void page_allocator_free(void *ptr, size_t n);
void *page_allocator_alloc_pages(size_t num_pages, size_t align_size);
