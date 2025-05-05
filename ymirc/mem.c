#include "mem.h"

static bool mapping_reconstructed = false;

Phys virt2phys(uintptr_t addr) {
  if (!mapping_reconstructed) {
    // UEFI's page table.
    return addr;
  } else if (addr < KERNEL_BASE) {
    // Direct map region.
    return addr - DIRECT_MAP_BASE;
  } else {
    // Kernel image mapping region.
    return addr - KERNEL_BASE;
  }
}

Virt phys2virt(uintptr_t addr) {
  if (!mapping_reconstructed) {
    // UEFI's page table.
    return addr;
  } else {
    // Kernel image mapping region.
    return addr + DIRECT_MAP_BASE;
  }
}

void set_mem_reconstructed(bool reconstructed) {
  mapping_reconstructed = reconstructed;
}

void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}
