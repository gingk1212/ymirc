#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../surtrc/def.h"

void page_allocator_init(MemoryMap *map);
void *mem_alloc(size_t n);
void mem_free(void *ptr, size_t n);
void *mem_alloc_pages(size_t num_pages, size_t align_size);
