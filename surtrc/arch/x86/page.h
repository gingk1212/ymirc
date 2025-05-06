#ifndef ARCH_X86_PAGE_H
#define ARCH_X86_PAGE_H

#include <stdint.h>

typedef uintptr_t Phys;
typedef uintptr_t Virt;

typedef enum {
  // RO
  read_only,
  // RW
  read_write,
  // RX
  executable,
} PageAttribute;

void map_4k_to(Virt virt, Phys phys);
void set_lv4table_writable();
void change_map_4k(Virt virt, PageAttribute attr);

#endif  // ARCH_X86_PAGE_H
