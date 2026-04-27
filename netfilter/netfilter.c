#include <linux/inet.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/tcp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Shanygin");
MODULE_DESCRIPTION("Tracks outgoing TCP connections; optionally blocks a destination port");
MODULE_VERSION("0.1");

static unsigned short filter_port;
module_param(filter_port, ushort, 0644);
MODULE_PARM_DESC(filter_port, "Destination port to block (0 = block nothing)");

static bool log_all;
module_param(log_all, bool, 0644);
MODULE_PARM_DESC(log_all, "If true, log every TCP packet; otherwise (default) log only SYN packets");


static struct nf_hook_ops nf_hooks;

static unsigned int tcp_tracker_hook(void *priv, struct sk_buff *skb,
                                     const struct nf_hook_state *state) {
  struct iphdr *ip_header;
  struct tcphdr *tcp_header;
  __be16 src_port_be;
  __be16 dst_port_be;
  u16 src_port;
  u16 dst_port;

  if (!skb) {
    return NF_ACCEPT;
  }

  ip_header = ip_hdr(skb);
  if (!ip_header || ip_header->protocol != IPPROTO_TCP)
    return NF_ACCEPT;

  tcp_header = (struct tcphdr *)((__u32 *)ip_header + ip_header->ihl);
  if (!tcp_header) {
    return NF_ACCEPT;
  }

  src_port_be = tcp_header->source;
  dst_port_be = tcp_header->dest;
  src_port = ntohs(src_port_be);
  dst_port = ntohs(dst_port_be);

  if (filter_port != 0 && dst_port == filter_port) {
    pr_info("tcp_tracker: BLOCKED %pI4:%u -> %pI4:%u  flags:%s%s%s%s%s\n",
            &ip_header->saddr, src_port, &ip_header->daddr, dst_port, tcp_header->syn ? "S" : "",
            tcp_header->ack ? "A" : "", tcp_header->fin ? "F" : "", tcp_header->rst ? "R" : "",
            tcp_header->psh ? "P" : "");
    return NF_DROP;
  }

  if (!log_all && !(tcp_header->syn && !tcp_header->ack))
    return NF_ACCEPT;

  pr_info("tcp_tracker: OUT %pI4:%u -> %pI4:%u  flags:%s%s%s%s%s\n",
          &ip_header->saddr, src_port, &ip_header->daddr, dst_port, tcp_header->syn ? "S" : "",
          tcp_header->ack ? "A" : "", tcp_header->fin ? "F" : "", tcp_header->rst ? "R" : "",
          tcp_header->psh ? "P" : "");

  return NF_ACCEPT;
}

static int __init tcp_tracker_init(void) {
  int ret;

  tcp_tracker_kobj = kobject_create_and_add("tcp_tracker", kernel_kobj);
  if (!tcp_tracker_kobj)
    return -ENOMEM;

  ret = sysfs_create_group(tcp_tracker_kobj, &tcp_tracker_attr_group);
  if (ret) {
    kobject_put(tcp_tracker_kobj);
    return ret;
  }

  nf_hooks.hook = tcp_tracker_hook;         // hook function
  nf_hooks.hooknum = NF_INET_LOCAL_OUT;     // outgoing packets
  nf_hooks.pf = PF_INET;                    // IPv4
  nf_hooks.priority = NF_IP_PRI_FIRST;      /// highest priority

  ret = nf_register_net_hook(&init_net, &nf_hooks);
  if (ret) {
    sysfs_remove_group(tcp_tracker_kobj, &tcp_tracker_attr_group);
    kobject_put(tcp_tracker_kobj);
    return ret;
  }

  pr_info("tcp_tracker: loaded (filter_port=%u, log_all=%d)\n", filter_port,
          log_all);
  return 0;
}

static void __exit tcp_tracker_exit(void) {
  nf_unregister_net_hook(&init_net, &nf_hooks);
  sysfs_remove_group(tcp_tracker_kobj, &tcp_tracker_attr_group);
  kobject_put(tcp_tracker_kobj);
  pr_info("tcp_tracker: unloaded\n");
}

module_init(tcp_tracker_init);
module_exit(tcp_tracker_exit);
