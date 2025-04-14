ARCH = x86_64

CC = gcc
OBJCOPY = objcopy

OUT_DIR = out/img/efi/boot

EFI_INC = /usr/include/efi
EFI_INCS = -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol
EFI_LIB = /usr/lib
EFI_CRT_OBJS = $(EFI_LIB)/crt0-efi-$(ARCH).o
EFI_LDS = $(EFI_LIB)/elf_$(ARCH)_efi.lds
CFLAGS = $(EFI_INCS) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -std=c17
ifeq ($(ARCH),x86_64)
	CFLAGS += -DEFI_FUNCTION_WRAPPER
endif
LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) $(EFI_CRT_OBJS)

$(OUT_DIR)/BOOTX64.EFI: surtrc/boot.so
	mkdir -p $(OUT_DIR)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-$(ARCH) $< $@

surtrc/boot.so: surtrc/boot.o
	ld $(LDFLAGS) $< -o $@ -lgnuefi -lefi

surtrc/boot.o: surtrc/log.h

run: $(OUT_DIR)/BOOTX64.EFI
	qemu-system-x86_64 \
		-m 512M \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive file=fat:rw:out/img,format=raw \
		-nographic \
		-serial mon:stdio \
		-no-reboot \
		-enable-kvm \
		-cpu host \
		-s

clean:
	rm -rf surtrc/boot.o surtrc/boot.so out

.PHONY: clean
