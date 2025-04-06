PROJECT := shell

CC := cc
CFLAGS := -Wall -Wextra -Werror
CPPFLAGS := -Iinclude -MMD -MP
LDFLAGS :=
LDLIBS := -lreadline

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BIN_DIR)/$(PROJECT)
ENTRYPOINT := $(SRC_DIR)/main.c

SRCS := $(sort $(shell find $(SRC_DIR) -type f -name '*.c' 2>/dev/null))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
HAS_ENTRYPOINT := $(strip $(wildcard $(ENTRYPOINT)))

.PHONY: all objects layout clean fclean re run test help

# Allow `make` to succeed before the first translation unit exists.
ifeq ($(strip $(SRCS)),)
all: layout
	@printf "project skeleton is ready. add C sources under %s to build %s.\n" "$(SRC_DIR)" "$(TARGET)"
else ifeq ($(HAS_ENTRYPOINT),)
all: objects
	@printf "compiled project objects in %s. add %s to link %s.\n" "$(OBJ_DIR)" "$(ENTRYPOINT)" "$(TARGET)"
else
all: $(TARGET)
endif

objects: $(OBJS)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

layout: $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean:
	$(RM) -r $(BUILD_DIR)

re: fclean all

run: all
ifneq ($(HAS_ENTRYPOINT),)
	@./$(TARGET)
else
	@printf "no executable yet. add %s first.\n" "$(ENTRYPOINT)"
endif

test: all
ifneq ($(HAS_ENTRYPOINT),)
	@sh tests/integration/smoke.sh
else
	@printf "no executable yet. add %s first.\n" "$(ENTRYPOINT)"
endif

help:
	@printf "available targets: all objects layout clean fclean re run test help\n"

-include $(DEPS)
