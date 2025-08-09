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
  /** Physical address the guest image is loaded. */
  void *guest_image;
  /** Size in bytes of the guest image. */
  UINTN guest_size;
  /** Physical address the initrd is loaded. */
  void *initrd_addr;
  /** Size in bytes of the initrd. */
  UINTN initrd_size;
} GuestInfo;

typedef struct {
  /** Magic number to check if the boot info is valid. */
  UINTN magic;
  /** UEFI memory map. */
  MemoryMap map;
  GuestInfo guest_info;
} BootInfo;

#endif  // SURTRC_DEF_H
