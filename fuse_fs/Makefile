CC = gcc
C_FLAGS = -Wall -D_FILE_OFFSET_BITS=64
SRC = src/ffs.c
EXE = ffs

all:
	$(CC) $(C_FLAGS) $(SRC) -o $(EXE)

clean:
	rm $(EXE)

.PHONY: all clean