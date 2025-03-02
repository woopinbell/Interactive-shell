PROJECT := shell

CC := cc
CFLAGS := -Wall -Wextra -Werror
CPPFLAGS := -Iinclude -MMD -MP
LDFLAGS :=
LDLIBS :=

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BIN_DIR)/$(PROJECT)

SRCS := $(sort $(shell find $(SRC_DIR) -type f -name '*.c' 2>/dev/null))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all layout clean fclean re run help

# Allow `make` to succeed before the first translation unit exists.
ifeq ($(strip $(SRCS)),)
all: layout
	@printf "project skeleton is ready. add C sources under %s to build %s.\n" "$(SRC_DIR)" "$(TARGET)"
else
all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)
endif

layout: $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean:
	$(RM) -r $(BUILD_DIR)

re: fclean all

run: all
ifneq ($(strip $(SRCS)),)
	@./$(TARGET)
else
	@printf "no executable yet. add C sources under %s first.\n" "$(SRC_DIR)"
endif

help:
	@printf "available targets: all layout clean fclean re run help\n"
