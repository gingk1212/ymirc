CC = gcc
LD = ld

TARGET = ../build/img/ymirc.elf
OBJ_DIR = ../build/ymirc

EFI_INC = /usr/include/efi
EFI_INCS = -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol

CFLAGS = $(EFI_INCS) -ffreestanding -m64 -nostdlib -Wall -Wextra
LDFLAGS = -nostdlib -e kernel_entry -T linker.ld

$(TARGET): $(OBJ_DIR)/main.o
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) $< -o $@

$(OBJ_DIR)/main.o: main.c ../surtrc/def.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
