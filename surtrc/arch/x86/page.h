#ifndef ARCH_X86_PAGE_H
#define ARCH_X86_PAGE_H

#include <stdint.h>

typedef uintptr_t Phys;
typedef uintptr_t Virt;

void map_4k_to(Virt virt, Phys phys);
void set_lv4table_writable();

#endif  // ARCH_X86_PAGE_H
