#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE hnd, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(hnd, SystemTable);
    while(1) {
        __asm__ __volatile__ ("hlt");
    }
    return EFI_SUCCESS;
}
