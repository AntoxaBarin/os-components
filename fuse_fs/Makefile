CC = gcc
CFLAGS = -Iinclude -Wall -Werror -g
LDFLAGS = `pkg-config fuse3 --libs`
PKG_CFLAGS = `pkg-config fuse3 --cflags`

OBJ_DIR = obj
SRCS = src/ffs.c
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))
TARGET = ffs

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean