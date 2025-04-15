#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include <inttypes.h>
#include "log.h"

#define TRY_EFI(expr)                                      \
    do                                                     \
    {                                                      \
        EFI_STATUS _status = (expr);                       \
        if (EFI_ERROR(_status))                            \
        {                                                  \
            LOG_ERROR(L"%a:%d: %a failed: %r",             \
                      __FILE__, __LINE__, #expr, _status); \
            return _status;                                \
        }                                                  \
    } while (0)

EFI_STATUS
get_volume(EFI_FILE_HANDLE *volume, EFI_HANDLE image_handle)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simple_file_system;

    TRY_EFI(uefi_call_wrapper(
        BS->OpenProtocol,
        6,
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL));

    TRY_EFI(uefi_call_wrapper(
        BS->OpenProtocol,
        6,
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void **)&simple_file_system,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL));

    TRY_EFI(uefi_call_wrapper(
        simple_file_system->OpenVolume,
        2,
        simple_file_system,
        volume));

    return EFI_SUCCESS;
}

EFI_STATUS
open_file(EFI_FILE_HANDLE *file_handle, EFI_FILE_HANDLE dir_handle, CHAR16 *file_name, UINT64 open_mode)
{
    TRY_EFI(uefi_call_wrapper(
        dir_handle->Open,
        5,
        dir_handle,
        file_handle,
        file_name,
        open_mode,
        0));

    return EFI_SUCCESS;
}

EFI_STATUS
read_file(void *buffer, EFI_FILE_HANDLE file_handle, UINTN *size)
{
    TRY_EFI(uefi_call_wrapper(
        file_handle->Read,
        3,
        file_handle,
        size,
        buffer));

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    InitializeLib(image_handle, system_table);
    TRY_EFI(uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut));

    EFI_FILE_HANDLE root_dir[1];
    TRY_EFI(get_volume(root_dir, image_handle));
    LOG_INFO(L"Opened filesystem volume.");

    EFI_FILE_HANDLE kernel[1];
    TRY_EFI(open_file(kernel, root_dir[0], L"ymirc.elf", EFI_FILE_MODE_READ));
    LOG_INFO(L"Opened kernel file.");

    UINTN header_size = sizeof(Elf64_Ehdr);
    Elf64_Ehdr *header_buffer = AllocatePool(header_size);
    if (!header_buffer)
    {
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

    while (1)
    {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
