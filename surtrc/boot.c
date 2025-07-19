#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include <inttypes.h>

#include "arch/x86/page.h"
#include "def.h"
#include "log.h"

#define TRY_EFI(expr)                                                         \
  do {                                                                        \
    EFI_STATUS _status = (expr);                                              \
    if (EFI_ERROR(_status)) {                                                 \
      LOG_ERROR(L"%a:%d: %a failed: %r", __FILE__, __LINE__, #expr, _status); \
      return _status;                                                         \
    }                                                                         \
  } while (0)

/** Opens the volume associated with the given UEFI image handle and returns a
 * file handle to the volume's root directory. */
EFI_STATUS
open_volume(EFI_FILE_HANDLE *volume, EFI_HANDLE image_handle) {
  EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simple_file_system;

  TRY_EFI(uefi_call_wrapper(BS->OpenProtocol, 6, image_handle,
                            &gEfiLoadedImageProtocolGuid,
                            (void **)&loaded_image, image_handle, NULL,
                            EFI_OPEN_PROTOCOL_GET_PROTOCOL));

  TRY_EFI(uefi_call_wrapper(BS->OpenProtocol, 6, loaded_image->DeviceHandle,
                            &gEfiSimpleFileSystemProtocolGuid,
                            (void **)&simple_file_system, image_handle, NULL,
                            EFI_OPEN_PROTOCOL_GET_PROTOCOL));

  TRY_EFI(uefi_call_wrapper(simple_file_system->OpenVolume, 2,
                            simple_file_system, volume));

  return EFI_SUCCESS;
}

/** Opens a file in the given directory. */
EFI_STATUS
open_file(EFI_FILE_HANDLE *file_handle, EFI_FILE_HANDLE dir_handle,
          CHAR16 *file_name, UINT64 open_mode) {
  TRY_EFI(uefi_call_wrapper(dir_handle->Open, 5, dir_handle, file_handle,
                            file_name, open_mode, 0));

  return EFI_SUCCESS;
}

/** Reads data from the file into the buffer. */
EFI_STATUS
read_file(void *buffer, EFI_FILE_HANDLE file_handle, UINTN *size) {
  TRY_EFI(uefi_call_wrapper(file_handle->Read, 3, file_handle, size, buffer));

  return EFI_SUCCESS;
}

/** Get file size. */
UINT64 file_size(EFI_FILE_HANDLE file_handle) {
  EFI_FILE_INFO *file_info = LibFileInfo(file_handle);
  UINT64 size = file_info->FileSize;
  FreePool(file_info);
  return size;
}

/** Computes a memory range of PT_LOAD segments in the kernel ELF file. */
EFI_STATUS
compute_kernel_memory_range(Virt *start_virt, Phys *start_phys, Phys *end_phys,
                            const Elf64_Phdr *phdr, int phnum) {
  Virt start_virt_tmp = UINT64_MAX;
  Phys start_phys_tmp = UINT64_MAX;
  Virt end_phys_tmp = 0;

  for (int i = 0; i < phnum; i++) {
    const Elf64_Phdr *ph = &phdr[i];
    if (ph->p_type != PT_LOAD || ph->p_memsz == 0) {
      continue;
    }
    if (ph->p_vaddr < start_virt_tmp) {
      start_virt_tmp = ph->p_vaddr;
    }
    if (ph->p_paddr < start_phys_tmp) {
      start_phys_tmp = ph->p_paddr;
    }
    Phys segment_end = ph->p_paddr + ph->p_memsz;
    if (segment_end > end_phys_tmp) {
      end_phys_tmp = segment_end;
    }
  }

  *start_virt = start_virt_tmp;
  *start_phys = start_phys_tmp;
  *end_phys = end_phys_tmp;

  return EFI_SUCCESS;
}

static PageAttribute read_p_flags(uint32_t flags) {
  if ((flags & PF_X) != 0) {
    return executable;
  } else if ((flags & PF_W) != 0) {
    return read_write;
  } else {
    return read_only;
  }
}

/** Loads PT_LOAD segments of kernel ELF file to memory. */
EFI_STATUS
load_segment(const EFI_FILE_HANDLE kernel, const Elf64_Phdr *phdr, int phnum) {
  LOG_INFO(L"Loading kernel image...");
  for (int i = 0; i < phnum; i++) {
    const Elf64_Phdr *ph = &phdr[i];
    if (ph->p_type != PT_LOAD || ph->p_memsz == 0) {
      continue;
    }
    UINTN mem_size = ph->p_memsz;
    UINTN file_size = ph->p_filesz;
    if (file_size > 0) {
      TRY_EFI(uefi_call_wrapper(kernel->SetPosition, 2, kernel, ph->p_offset));
      TRY_EFI(read_file((void *)ph->p_vaddr, kernel, &file_size));
    }
    LOG_INFO(L"  Seg @ 0x%016" PRIx64 " - 0x%016" PRIx64, ph->p_vaddr,
             ph->p_vaddr + ph->p_memsz);

    // Zero-clear the BSS section.
    int zero_count = mem_size - file_size;
    if (zero_count > 0) {
      ZeroMem((void *)(ph->p_vaddr + file_size), zero_count);
      LOG_INFO(L"  Zeroed 0x%x bytes", zero_count);
    }

    // Change memory protection.
    Virt page_start = ph->p_vaddr & ~((uint64_t)EFI_PAGE_MASK);
    Virt page_end = (ph->p_vaddr + ph->p_memsz + (EFI_PAGE_SIZE - 1)) &
                    ~(uint64_t)EFI_PAGE_MASK;
    uint64_t size = (page_end - page_start) / EFI_PAGE_SIZE;
    PageAttribute attr = read_p_flags(ph->p_flags);
    for (uint64_t i = 0; i < size; i++) {
      change_map_4k(page_start + EFI_PAGE_SIZE * i, attr);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle,
                           EFI_SYSTEM_TABLE *system_table) {
  InitializeLib(image_handle, system_table);
  TRY_EFI(uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut));

  // Open volume.
  EFI_FILE_HANDLE root_dir[1];
  TRY_EFI(open_volume(root_dir, image_handle));
  LOG_INFO(L"Opened filesystem volume.");

  // Open kernel file.
  EFI_FILE_HANDLE kernel[1];
  TRY_EFI(open_file(kernel, root_dir[0], L"ymirc.elf", EFI_FILE_MODE_READ));
  LOG_INFO(L"Opened kernel file.");

  // Read kernel ELF header.
  UINTN header_size = sizeof(Elf64_Ehdr);
  Elf64_Ehdr *header_buffer = AllocatePool(header_size);
  if (!header_buffer) {
    LOG_ERROR(L"Failed to allocate memory for ELF header.");
    return EFI_OUT_OF_RESOURCES;
  }
  TRY_EFI(read_file(header_buffer, kernel[0], &header_size));
  Virt e_entry = header_buffer->e_entry;
  LOG_INFO(L"Read kernel ELF header.");
  LOG_DEBUG(L"Kernel ELF information:");
  LOG_DEBUG(L"  Entry point: 0x%" PRIx64, header_buffer->e_entry);
  LOG_DEBUG(L"  Is 64-bit: %d", header_buffer->e_ident[EI_CLASS] == ELFCLASS64);
  LOG_DEBUG(L"  # of Program Headers: %" PRIu16, header_buffer->e_phnum);
  LOG_DEBUG(L"  # of Section Headers: %" PRIu16, header_buffer->e_shnum);

  // Set Level 4 page table writable.
  set_lv4table_writable();
  LOG_DEBUG(L"Set page table writable.");

  // Read kernel ELF program headers.
  TRY_EFI(uefi_call_wrapper(kernel[0]->SetPosition, 2, kernel[0],
                            header_buffer->e_phoff));
  UINTN phsize = header_buffer->e_phnum * header_buffer->e_phentsize;
  Elf64_Phdr *ph_buffer = AllocatePool(phsize);
  if (!ph_buffer) {
    LOG_ERROR(L"Failed to allocate memory for program headers.");
    return EFI_OUT_OF_RESOURCES;
  }
  TRY_EFI(read_file(ph_buffer, kernel[0], &phsize));
  LOG_INFO(L"Read kernel ELF program headers.");

  // Compute necessary memory size for kernel image.
  Virt start_virt;
  Phys start_phys, end_phys;
  TRY_EFI(compute_kernel_memory_range(&start_virt, &start_phys, &end_phys,
                                      ph_buffer, header_buffer->e_phnum));
  int pages_4kib = (end_phys - start_phys + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;
  LOG_INFO(L"Kernel image: 0x%016" PRIx64 " - 0x%016" PRIx64 " (0x%x pages)",
           start_phys, end_phys, pages_4kib);

  // Allocate memory for kernel image.
  TRY_EFI(uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress,
                            EfiLoaderData, pages_4kib, &start_phys));
  LOG_INFO(L"Allocated memory for kernel image @ 0x%016" PRIx64
           " ~ 0x%016" PRIx64,
           start_phys, start_phys + pages_4kib * EFI_PAGE_SIZE);

  // Map memory for kernel image.
  for (int i = 0; i < pages_4kib; i++) {
    map_4k_to(start_virt + i * EFI_PAGE_SIZE, start_phys + i * EFI_PAGE_SIZE);
  }
  LOG_INFO(L"Mapped memory for kernel image.");

  // Load kernel image.
  TRY_EFI(load_segment(kernel[0], ph_buffer, header_buffer->e_phnum));

  // Open guest kernel image.
  EFI_FILE_HANDLE guest[1];
  TRY_EFI(open_file(guest, root_dir[0], L"bzImage", EFI_FILE_MODE_READ));
  LOG_INFO(L"Opened guest kennel file.");

  // Get guest kernel image size.
  UINT64 guest_size = file_size(guest[0]);
  LOG_INFO(L"Guest kernel size: 0x%x bytes.", guest_size);

  // Load guest kernel image.
  Phys guest_start;
  UINT64 guest_size_pages = (guest_size + (EFI_PAGE_SIZE - 1)) / EFI_PAGE_SIZE;
  TRY_EFI(uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages,
                            EfiLoaderData, guest_size_pages, &guest_start));
  TRY_EFI(read_file((void *)guest_start, guest[0], &guest_size));
  LOG_INFO(L"Loaded guest kernel image @ 0x%016" PRIx64 " ~ 0x%016" PRIx64,
           guest_start, guest_start + guest_size);

  // Clean up memory.
  FreePool(header_buffer);
  FreePool(ph_buffer);
  TRY_EFI(uefi_call_wrapper(kernel[0]->Close, 1, kernel[0]));
  TRY_EFI(uefi_call_wrapper(root_dir[0]->Close, 1, root_dir[0]));

  // Get memory map.
  int map_buffer_size = EFI_PAGE_SIZE * 4;
  UINT8 map_buffer[map_buffer_size];
  MemoryMap map = {
      .buffer_size = map_buffer_size,
      .descriptors = (EFI_MEMORY_DESCRIPTOR *)map_buffer,
      .map_size = map_buffer_size,
  };
  TRY_EFI(uefi_call_wrapper(BS->GetMemoryMap, 5, &map.map_size, map.descriptors,
                            &map.map_key, &map.descriptor_size,
                            &map.descriptor_version));

  // Print memory map.
  LOG_DEBUG(L"Memory Map (Physical): Buf=0x%" PRIx64 ", MapSize=0x%" PRIx64
            ", DescSize=0x%" PRIx64,
            map.descriptors, map.map_size, map.descriptor_size);
  for (UINTN i = 0; i < map.map_size / map.descriptor_size; i++) {
    EFI_MEMORY_DESCRIPTOR *desc =
        (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)map.descriptors +
                                  i * map.descriptor_size);
    LOG_DEBUG(L"  0x%016" PRIx64 " - 0x%016" PRIx64 " (Type: 0x%" PRIx32 ")",
              desc->PhysicalStart,
              desc->PhysicalStart + desc->NumberOfPages * EFI_PAGE_SIZE,
              desc->Type);
  }

  // Exit boot service.
  LOG_INFO(L"Exiting boot services.");
  EFI_STATUS status =
      uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map.map_key);
  if (status == EFI_INVALID_PARAMETER) {
    map.map_size = map_buffer_size;
    TRY_EFI(uefi_call_wrapper(BS->GetMemoryMap, 5, &map.map_size,
                              map.descriptors, &map.map_key,
                              &map.descriptor_size, &map.descriptor_version));
    TRY_EFI(
        uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map.map_key));
  }

  // Jump to kernel entry point.
  typedef void (*KernelEntryType)(BootInfo *);
  KernelEntryType kernel_entry = (KernelEntryType)(e_entry);
  BootInfo boot_info = {
      .magic = MAGIC,
      .map = map,
      .guest_info =
          {
              .guest_image = (void *)guest_start,
              .guest_size = guest_size,
          },
  };
  kernel_entry(&boot_info);

  // noreachable
  return EFI_SUCCESS;
}
