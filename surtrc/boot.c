#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include <inttypes.h>
#include "log.h"

EFI_FILE_HANDLE
get_volume(EFI_HANDLE image_handle)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simple_file_system;
    EFI_FILE_HANDLE volume;

    uefi_call_wrapper(
        BS->OpenProtocol,
        6,
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    uefi_call_wrapper(
        BS->OpenProtocol,
        6,
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void **)&simple_file_system,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    uefi_call_wrapper(
        simple_file_system->OpenVolume,
        2,
        simple_file_system,
        &volume);

    return volume;
}

EFI_FILE_HANDLE
open_file(EFI_FILE_HANDLE dir, CHAR16 *file_name, UINT64 open_mode)
{
    EFI_FILE_HANDLE file_handle;

    uefi_call_wrapper(
        dir->Open,
        5,
        dir,
        &file_handle,
        file_name,
        open_mode,
        0);

    return file_handle;
}

UINT8 *
read_file(EFI_FILE_HANDLE file_handle, UINTN *size)
{
    EFI_STATUS status;
    UINT8 *buffer;

    // Allocate buffer for the file content
    buffer = AllocatePool(*size);
    if (!buffer)
    {
        LOG_ERROR(L"Failed to allocate memory.");
        return NULL;
    }

    // Read the file content
    status = uefi_call_wrapper(file_handle->Read, 3, file_handle, size, buffer);
    if (EFI_ERROR(status))
    {
        LOG_ERROR(L"Failed to read file: %" PRIu64, status);
        FreePool(buffer);
        return NULL;
    }

    return buffer;
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    InitializeLib(image_handle, system_table);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    LOG_INFO(L"SurtrC bootloader started.");

    EFI_FILE_HANDLE root_dir = get_volume(image_handle);
    LOG_INFO(L"Opened filesystem volume.");

    EFI_FILE_HANDLE kernel = open_file(root_dir, L"ymirc.elf", EFI_FILE_MODE_READ);
    LOG_INFO(L"Opened kernel file.");

    UINTN header_size = sizeof(Elf64_Ehdr);
    Elf64_Ehdr *header_buffer = (Elf64_Ehdr *)read_file(kernel, &header_size);
    if (!header_buffer)
    {
        LOG_ERROR(L"Failed to read kernel ELF header.");
        return EFI_LOAD_ERROR;
    }
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
