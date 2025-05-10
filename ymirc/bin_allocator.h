#pragma once

#include "page_allocator_if.h"

void init_bin_allocator(const page_allocator_ops_t* ops);
void* bin_alloc(size_t n);
void bin_free(void* ptr, size_t n);
