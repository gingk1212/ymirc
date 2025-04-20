CC = gcc
LD = ld

TARGET = ../build/img/ymirc.elf
OBJ_DIR = ../build/ymirc

CFLAGS = -ffreestanding -m64 -nostdlib -Wall -Wextra
LDFLAGS = -nostdlib -e kernel_entry -T linker.ld

$(TARGET): $(OBJ_DIR)/main.o
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) $< -o $@

$(OBJ_DIR)/main.o: main.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
