#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <net/if.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#define MAX_PORTS 64

static int iface_idx;

static void cleanup(int sig) {
    (void)sig;
    bpf_xdp_detach(iface_idx, 0, NULL);
    printf("\n-- Detached.\n");
    exit(0);
}

int main(int argc, char **argv)
{
    const char *iface = argc >= 2 ? argv[1] : "lo";
    iface_idx = if_nametoindex(iface);

    uint16_t ports[MAX_PORTS];
    uint32_t cnt = 0;

    if (argc >= 3) {
        char *tok, *buf = strdup(argv[2]);
        for (tok = strtok(buf, ","); tok && cnt < MAX_PORTS; tok = strtok(NULL, ","))
            ports[cnt++] = atoi(tok);
        free(buf);
    } else {
        ports[0] = 443;
        cnt = 1;
    }

    struct bpf_object *obj = bpf_object__open_file("xdp_firewall.bpf.o", NULL);
    bpf_object__load(obj);

    int prog_fd  = bpf_program__fd(bpf_object__find_program_by_name(obj, "xdp_firewall"));
    int ports_fd = bpf_object__find_map_fd_by_name(obj, "blocked_ports");
    int cnt_fd   = bpf_object__find_map_fd_by_name(obj, "port_count");

    for (uint32_t i = 0; i < cnt; i++) {
        bpf_map_update_elem(ports_fd, &i, &ports[i], BPF_ANY);
    }

    uint32_t zero = 0;
    bpf_map_update_elem(cnt_fd, &zero, &cnt, BPF_ANY);

    bpf_xdp_attach(iface_idx, prog_fd, 0, NULL);
    printf("-- XDP on %s, blocking %d ports\n", iface, cnt);

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    while (1) { 
        pause();
    }
}
