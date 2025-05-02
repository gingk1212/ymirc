#include "arch.h"

#include "gdt.h"

void arch_init() { gdt_init(); }
