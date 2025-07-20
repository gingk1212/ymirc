CC = gcc
LD = ld
ifneq (, $(shell command -v ld.lld 2>/dev/null))
	LD = ld.lld
endif

TARGET = ../build/img/ymirc.elf
OBJ_DIR = ../build/ymirc
TEST_DIR = $(OBJ_DIR)/test
SRCS = $(shell find . -name '*.c' ! -name '*_test.c' -printf '%P\n')
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:%.o=%.d)

EFI_INC = /usr/include/efi

CFLAGS = -I. -I$(EFI_INC) -ffreestanding -mno-red-zone -m64 -nostdlib -Wall -Wextra -std=c17 -MMD -MP -DLOG_LEVEL=0
LDFLAGS = -nostdlib -e kernel_entry -T linker.ld

CFLAGS_FOR_TEST = -I. -I$(EFI_INC) -Wall -Wextra -std=c17 -g

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

test: bin_allocator_test bits_test log_test page_allocator_test
	@echo "All tests passed."

%_test: %_test.c $(OBJS)
	@mkdir -p $(TEST_DIR)/$(@D)
	@$(CC) $(CFLAGS_FOR_TEST) $(filter-out $(OBJ_DIR)/main.o, $^) -o $(TEST_DIR)/$@.bin
	$(TEST_DIR)/$@.bin

.Phony: test
