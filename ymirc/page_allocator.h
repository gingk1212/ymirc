#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "../surtrc/def.h"

void page_allocator_init(MemoryMap *map);

#include "page_allocator_if.h"
extern const page_allocator_ops_t pa_ops;
