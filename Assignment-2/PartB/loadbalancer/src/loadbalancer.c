#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>

#define LB_PORT 8080
#define NUM_SERVERS 3

SEC("load_balancer")
int load_balancer(struct xdp_md *ctx){
    /*
    This will run our eBPF code which will intercept all packets sent to this server. If the
    packet is sent to a specific port ( say 8080 ) it means it was meant to be sent to one of our
    backend servers and we will decide which server to send it to, otherwise we let the packet
    pass as it is.
    For deciding the backend server we work as follows:-
    1. Our eBPF program maintains free_threads for each backend , which will be initially
    five for each server as all threads are available.
    2. When it receives a packet that requires to be forwarded to one of our backend
    servers, it sends it to any server which has an available thread and updates the free
    slots accordingly.
    3. If no server has a free thread, then it adds the packet to a packet queue which
    stores delayed packets.
    4. When it receives information from any of the backends indicating that a thread is
    free, then it first sends a packet from the queue if present, otherwise it updates the
    free thread count for that backend.    
    */

    // Get the packet
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Check if the packet is valid
    struct ethhdr *eth = data;
    if ((void*)eth + sizeof(*eth) > data_end) {
        return XDP_PASS;
    }

    // Check if the destination port is LB_PORT
    struct iphdr *ip = data + sizeof(*eth);
    if ((void*)ip + sizeof(*ip) > data_end) {
        return XDP_PASS;
    }

    if (ip->protocol != IPPROTO_UDP) {
        return XDP_PASS;
    }

    if (bpf_ntohs(ip->daddr) != LB_PORT) {
        return XDP_PASS;
    }

    if ((void*)ip + sizeof(*ip) > data_end) {
        return XDP_PASS;
    }

    // these will be tcp packets
    



}

char _license[] SEC("license") = "GPL";