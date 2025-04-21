#ifndef SURTRC_DEF_H
#define SURTRC_DEF_H

#include <efi.h>

#define MAGIC 0xDEADBEEFCAFEBABE

typedef struct {
  UINTN buffer_size;  // Total buffer size prepared to store the memory map.
  EFI_MEMORY_DESCRIPTOR *descriptors;
  UINTN map_size;  // Total memory map size.
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
} MemoryMap;

typedef struct {
  UINTN magic;
  MemoryMap map;
} BootInfo;

#endif  // SURTRC_DEF_H
