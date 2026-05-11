#include <linux/types.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#define MAX_BLOCKED_PORTS 64

struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, MAX_BLOCKED_PORTS);
  __type(key, __u32);
  __type(value, __u16);
} blocked_ports SEC(".maps");

// single element to store number of blocked ports
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, __u32);
} port_count SEC(".maps");

SEC("xdp")
int xdp_firewall(struct xdp_md *ctx) {
  void *data = (void *)(long)ctx->data;
  void *data_end = (void *)(long)ctx->data_end;

  struct ethhdr *eth = data;
  if ((void *)(eth + 1) > data_end) {
    return XDP_PASS;
  }

  if (eth->h_proto != bpf_htons(ETH_P_IP)) {
    return XDP_PASS;
  }

  struct iphdr *ip = (void *)(eth + 1);
  if ((void *)(ip + 1) > data_end) {
    return XDP_PASS;
  }
    
  __u32 ip_hdr_len = ip->ihl * 4;
  if (ip_hdr_len < sizeof(*ip)) {
    return XDP_PASS;
  }
  if ((void *)ip + ip_hdr_len > data_end) {
    return XDP_PASS;
  }

  __u16 dst_port = 0;

  if (ip->protocol == IPPROTO_TCP) {
    struct tcphdr *tcp = (void *)ip + ip_hdr_len;
    if ((void *)(tcp + 1) > data_end) {
      return XDP_PASS;
    }

    dst_port = bpf_ntohs(tcp->dest);

  } else if (ip->protocol == IPPROTO_UDP) {
    struct udphdr *udp = (void *)ip + ip_hdr_len;
    if ((void *)(udp + 1) > data_end) {
      return XDP_PASS;
    }      
    dst_port = bpf_ntohs(udp->dest);

  } else {
    return XDP_PASS;
  }

  __u32 zero = 0;
  __u32 *cnt = bpf_map_lookup_elem(&port_count, &zero);
  if (!cnt) {
    return XDP_PASS;
  }

  __u32 n = *cnt;
  if (n > MAX_BLOCKED_PORTS)
    n = MAX_BLOCKED_PORTS;

  __u32 i = 0;
  bpf_for(i, 0, MAX_BLOCKED_PORTS) {
    if (i >= n) {
      break;
    }
    __u16 *port = bpf_map_lookup_elem(&blocked_ports, &i);
    if (port && *port == dst_port) {
      bpf_printk("XDP DROP port %d\n", dst_port);
      return XDP_DROP;
    }
  }

  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
