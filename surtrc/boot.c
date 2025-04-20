#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include <inttypes.h>

#include "arch/x86/page.h"
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

EFI_STATUS
compute_kernel_memory_range(Virt *start_virt, Phys *start_phys, Phys *end_phys,
                            const Elf64_Phdr *phdr, int phnum) {
  Virt start_virt_tmp = UINT64_MAX;
  Phys start_phys_tmp = UINT64_MAX;
  Virt end_phys_tmp = 0;

  for (int i = 0; i < phnum; i++) {
    const Elf64_Phdr *ph = &phdr[i];
    if (ph->p_type != PT_LOAD) {
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

EFI_STATUS
load_segment(const EFI_FILE_HANDLE kernel, const Elf64_Phdr *phdr, int phnum) {
  LOG_INFO(L"Loading kernel image...");
  for (int i = 0; i < phnum; i++) {
    const Elf64_Phdr *ph = &phdr[i];
    if (ph->p_type != PT_LOAD) {
      continue;
    }
    TRY_EFI(uefi_call_wrapper(kernel->SetPosition, 2, kernel, ph->p_offset));
    UINTN mem_size = ph->p_memsz;
    TRY_EFI(read_file((void *)ph->p_vaddr, kernel, &mem_size));
    LOG_INFO(L"  Seg @ 0x%016" PRIx64 " - 0x%016" PRIx64, ph->p_vaddr,
             ph->p_vaddr + ph->p_memsz);

    int zero_count = ph->p_memsz - ph->p_filesz;
    if (zero_count > 0) {
      SetMem((void *)(ph->p_vaddr + ph->p_filesz), zero_count, 0);
      LOG_INFO(L"  Zeroed %d bytes", zero_count);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle,
                           EFI_SYSTEM_TABLE *system_table) {
  InitializeLib(image_handle, system_table);
  TRY_EFI(uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut));

  EFI_FILE_HANDLE root_dir[1];
  TRY_EFI(open_volume(root_dir, image_handle));
  LOG_INFO(L"Opened filesystem volume.");

  EFI_FILE_HANDLE kernel[1];
  TRY_EFI(open_file(kernel, root_dir[0], L"ymirc.elf", EFI_FILE_MODE_READ));
  LOG_INFO(L"Opened kernel file.");

  UINTN header_size = sizeof(Elf64_Ehdr);
  Elf64_Ehdr *header_buffer = AllocatePool(header_size);
  if (!header_buffer) {
    LOG_ERROR(L"Failed to allocate memory for ELF header.");
    return EFI_OUT_OF_RESOURCES;
  }
  TRY_EFI(read_file(header_buffer, kernel[0], &header_size));
  LOG_INFO(L"Read kernel ELF header.");
  LOG_DEBUG(L"Kernel ELF information:");
  LOG_DEBUG(L"  Entry point: 0x%" PRIx64, header_buffer->e_entry);
  LOG_DEBUG(L"  Is 64-bit: %d", header_buffer->e_ident[EI_CLASS] == ELFCLASS64);
  LOG_DEBUG(L"  # of Program Headers: %" PRIu16, header_buffer->e_phnum);
  LOG_DEBUG(L"  # of Section Headers: %" PRIu16, header_buffer->e_shnum);

  set_lv4table_writable();
  LOG_DEBUG(L"Set page table writable.");

  TRY_EFI(uefi_call_wrapper(kernel[0]->SetPosition, 2, kernel[0],
                            header_buffer->e_phoff));
  UINTN phsize = header_buffer->e_phnum * header_buffer->e_phentsize;
  Elf64_Phdr *ph_buffer = AllocatePool(phsize);
  if (!ph_buffer) {
    LOG_ERROR(L"Failed to allocate memory for program headers.");
    return EFI_OUT_OF_RESOURCES;
  }
  TRY_EFI(read_file(ph_buffer, kernel[0], &phsize));
  LOG_INFO(L"Read kernel ELF program header.");

  Virt start_virt;
  Phys start_phys, end_phys;
  TRY_EFI(compute_kernel_memory_range(&start_virt, &start_phys, &end_phys,
                                      ph_buffer, header_buffer->e_phnum));
  int pages_4kib = (end_phys - start_phys + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;
  LOG_INFO(L"Kernel image: 0x%016" PRIx64 " - 0x%016" PRIx64 " (0x%x pages)",
           start_phys, end_phys, pages_4kib);

  TRY_EFI(uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress,
                            EfiLoaderData, pages_4kib, &start_phys));
  LOG_INFO(L"Allocated memory for kernel image @ 0x%016" PRIx64
           " ~ 0x%016" PRIx64,
           start_phys, start_phys + pages_4kib * EFI_PAGE_SIZE);

  for (int i = 0; i < pages_4kib; i++) {
    map_4k_to(start_virt + i * EFI_PAGE_SIZE, start_phys + i * EFI_PAGE_SIZE);
  }
  LOG_INFO(L"Mapped memory for kernel image.");

  TRY_EFI(load_segment(kernel[0], ph_buffer, header_buffer->e_phnum));

  while (1) {
    __asm__ __volatile__("hlt");
  }

  return EFI_SUCCESS;
}
