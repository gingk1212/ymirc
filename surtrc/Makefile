ARCH = x86_64

CC = gcc
LD = ld
ifneq (, $(shell command -v ld.lld 2>/dev/null))
	LD = ld.lld
endif
OBJCOPY = objcopy

OBJ_DIR = ../build/surtrc
IMG_DIR = ../build/img
ifeq ($(ARCH),x86_64)
	TARGET = $(IMG_DIR)/efi/boot/BOOTX64.EFI
endif
SRCS = boot.c arch/x86/page.c
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:%.o=%.d)

EFI_INC = /usr/include/efi
EFI_LIB = /usr/lib
EFI_CRT_OBJS = $(EFI_LIB)/crt0-efi-$(ARCH).o
EFI_LDS = $(EFI_LIB)/elf_$(ARCH)_efi.lds

CFLAGS = -I. -I$(EFI_INC) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -std=c17 -MMD -MP -DLOG_LEVEL=0
LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(EFI_LIB) $(EFI_CRT_OBJS)

$(TARGET): $(OBJ_DIR)/boot.so
	@mkdir -p $(@D)
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-$(ARCH) $< $@

$(OBJ_DIR)/boot.so: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ -lgnuefi -lefi

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

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
