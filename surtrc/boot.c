#include <efi.h>
#include <efilib.h>
#include "log.h"

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE hnd, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(hnd, SystemTable);
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    LOG_DEBUG(L"Hello, world!\n");
    while(1) {
        __asm__ __volatile__ ("hlt");
    }
    return EFI_SUCCESS;
}
