#pragma once

#include "mem.h"
#include "page_allocator_if.h"

/**
 * Init guest NPT.
 *
 * @return Physical address of Page-Map Leve-4 Table.
 */
Phys init_npt(Phys guest_start, Phys host_start, size_t size,
              const page_allocator_ops_t *pa_ops);
