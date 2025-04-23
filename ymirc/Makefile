CC = gcc
LD = ld

TARGET = ../build/img/ymirc.elf
OBJ_DIR = ../build/ymirc
SRCS = main.c arch/x86/serial.c
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

EFI_INC = /usr/include/efi
EFI_INCS = -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol

CFLAGS = $(EFI_INCS) -ffreestanding -m64 -nostdlib -Wall -Wextra
LDFLAGS = -nostdlib -e kernel_entry -T linker.ld

$(TARGET): $(OBJS)
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.c ../surtrc/def.h arch/x86/serial.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
