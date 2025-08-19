TARGET = ../build/bin/ymircsh
OBJ_DIR = ../build/ymircsh
SRCS=$(wildcard *.c)
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:%.o=%.d)

CFLAGS = -Wall -Wextra -std=c17 -MMD -MP
LDFLAGS = -static

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	cc $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	cc $(CFLAGS) -c $< -o $@

-include $(DEPS)
