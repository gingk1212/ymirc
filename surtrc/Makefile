ARCH = x86_64

CC = gcc
LD = ld
OBJCOPY = objcopy

OBJ_DIR = ../build/surtrc
IMG_DIR = ../build/img
ifeq ($(ARCH),x86_64)
	TARGET = $(IMG_DIR)/efi/boot/BOOTX64.EFI
endif
SRCS = boot.c arch/x86/page.c
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

EFI_INC = /usr/include/efi
EFI_INCS = -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol
EFI_LIB = /usr/lib
EFI_CRT_OBJS = $(EFI_LIB)/crt0-efi-$(ARCH).o
EFI_LDS = $(EFI_LIB)/elf_$(ARCH)_efi.lds

CFLAGS = -I. $(EFI_INCS) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -std=c17 -DLOG_LEVEL=0
ifeq ($(ARCH),x86_64)
	CFLAGS += -DEFI_FUNCTION_WRAPPER
endif
LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) $(EFI_CRT_OBJS)

$(TARGET): $(OBJ_DIR)/boot.so
	mkdir -p $(@D)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-$(ARCH) $< $@

$(OBJ_DIR)/boot.so: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ -lgnuefi -lefi

$(OBJ_DIR)/%.o: %.c log.h arch/x86/page.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	qemu-system-$(ARCH) \
		-m 512M \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive file=fat:rw:$(IMG_DIR),format=raw \
		-nographic \
		-serial mon:stdio \
		-no-reboot \
		-enable-kvm \
		-cpu host \
		-s

.PHONY: run
