#pragma once

/** Perform architecture-specific initialization. */
void arch_init();

/** Discard the initial direct mapping and construct YmirC's page tables. It
 * creates two mappings: direct mapping and kernel text mapping. */
void reconstruct_mapping();

/** Halt endlessly with interrupts disabled. */
void endless_halt();
