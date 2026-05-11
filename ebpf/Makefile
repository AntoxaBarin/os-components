CLANG ?= clang
CC    ?= gcc

BPF_SRC = xdp_firewall.bpf.c
BPF_OBJ = xdp_firewall.bpf.o

USER_SRC = xdp_firewall.c
USER_BIN = xdp_firewall

.PHONY: all clean

all: $(BPF_OBJ) $(USER_BIN)

$(BPF_OBJ): $(BPF_SRC)
	$(CLANG) -O2 -g -target bpf \
		-D__TARGET_ARCH_x86 \
		-I/usr/include \
		-c $(BPF_SRC) -o $(BPF_OBJ)

$(USER_BIN): $(USER_SRC)
	$(CC) -Wall -O2 -g $(USER_SRC) -o $(USER_BIN) -lbpf -lelf -lz

clean:
	rm -f $(BPF_OBJ) $(USER_BIN)
