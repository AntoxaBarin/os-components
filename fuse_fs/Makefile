CC = gcc
C_FLAGS = -Wall -D_FILE_OFFSET_BITS=64
LD_FLAGS = `pkg-config fuse3 --libs`
SRC = src/ffs.c
EXE = ffs

all:
	$(CC) $(C_FLAGS) $(SRC) -o $(EXE) $(LD_FLAGS)

clean:
	rm $(EXE)

.PHONY: all clean