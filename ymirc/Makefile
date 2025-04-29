CC = gcc
LD = ld

TARGET = ../build/img/ymirc.elf
OBJ_DIR = ../build/ymirc
TEST_DIR = $(OBJ_DIR)/test
SRCS = $(shell find . -name '*.c' ! -name '*_test.c')
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

EFI_INC = /usr/include/efi
EFI_INCS = -I$(EFI_INC) -I$(EFI_INC)/$(ARCH) -I$(EFI_INC)/protocol

CFLAGS = -I. $(EFI_INCS) -ffreestanding -m64 -nostdlib -Wall -Wextra -std=c17 -DLOG_LEVEL=0
LDFLAGS = -nostdlib -e kernel_entry -T linker.ld

CFLAGS_FOR_TEST = -I. -Wall -Wextra -std=c17 -g

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.c ../surtrc/def.h bits.h log.h serial.h arch/x86/serial_impl.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

test: bits_test

%_test: %_test.c
	mkdir -p $(TEST_DIR)/$(@D)
	$(CC) $(CFLAGS_FOR_TEST) $< -o $(TEST_DIR)/$@
	./$(TEST_DIR)/$@

.Phony: test
