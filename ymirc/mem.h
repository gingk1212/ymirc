#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Page size. */
#define PAGE_SIZE 4096ULL

/** Page mask. */
#define PAGE_MASK (PAGE_SIZE - 1)

/** Base virtual address of direct mapping. The virtual address starting from
 * the address is directly mapped to the physical address at 0x0. In YmirC, must
 * be aligned to LV4_ENTRY_MAPPING_SIZE (in x86) for simplicity. */
#define DIRECT_MAP_BASE 0xFFFF888000000000ULL

/** Size in bytes of the direct mapping region. In YmirC, must be aligned to
 * LV4_ENTRY_MAPPING_SIZE (in x86) for simplicity. */
#define DIRECT_MAP_SIZE (512ULL * 1024 * 1024 * 1024)

/** The base virtual address of the kernel. The virtual address strating from
 * the address is directly mapped to the physical address at 0x0. */
#define KERNEL_BASE 0xFFFFFFFF80000000ULL

typedef uintptr_t Phys;
typedef uintptr_t Virt;

Phys virt2phys(uintptr_t addr);
Virt phys2virt(uintptr_t addr);
void set_mem_reconstructed(bool reconstructed);
void *memcpy(void *dest, const void *src, size_t n);
