#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>

#define SERVER_PORT 8080

SEC("xdp")
int xdp_prog(struct xdp_md *ctx) {
    // 2. Implement the eBPF program to filter incoming UDP packets for only the port on
    // which the server is running, the other packets should be allowed to directly pass
    // through.
    // 3. Modify the eBPF program to drop packets if the UDP data is even
    // and forward others.

    // Get the packet
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Check if the packet is valid
    struct ethhdr *eth = data;
    if ((void*)eth + sizeof(*eth) > data_end) {
        return XDP_PASS;
    }

    // Check if the destination port is SERVER_PORT
    struct iphdr *ip = data + sizeof(*eth);
    if ((void*)ip + sizeof(*ip) > data_end) {
        return XDP_PASS;
    }
    if (ip->protocol != IPPROTO_UDP) {
        return XDP_PASS;
    }
    //TODO: Substitute size of ip header with ip->ihl * 4
    struct udphdr* udp = (void*)ip + sizeof(*ip);

    if ((void*)udp + sizeof(*udp) > data_end) {
        return XDP_PASS;
    }

    if (bpf_ntohs(udp->dest) != SERVER_PORT) {
        return XDP_PASS;
    }

    // Check if the UDP data is even
    void* udp_data = (void*) udp + sizeof(*udp);

    if ((void*)udp_data + sizeof(*udp_data) > data_end) {
        return XDP_PASS;
    }

    // print trace statements for every packet and its data

    if (*((char*)udp_data) % 2 == 0) {
        bpf_printk("Dropping packet with even data: %c\n", *((char*)udp_data));
        return XDP_DROP;
    }

    bpf_printk("Passing packet with odd data: %c\n", *((char*)udp_data));
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";