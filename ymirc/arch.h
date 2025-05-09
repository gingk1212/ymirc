#pragma once

#include "page_allocator_if.h"

/** Perform architecture-specific initialization. */
void arch_init();

/** Discard the initial direct mapping and construct YmirC's page tables. It
 * creates two mappings: direct mapping and kernel text mapping. */
void reconstruct_mapping(const page_allocator_ops_t *ops);

/** Disable interrupts. */
void disable_intr();

/** Halt endlessly with interrupts disabled. */
void endless_halt();

/** Print the stack trace. */
void print_stack_trace();
