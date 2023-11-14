#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#define LB_PORT 8080
#define SERVER_PORT 20000

#define MAX_PACKET_SIZE 1500
#define MAX_UDP_LENGTH 1480
#define QUEUE_SIZE 1024

#define NUM_SERVERS 3
#define SERVER1_IP "172.18.0.3"
#define SERVER2_IP "172.18.0.4"
#define SERVER3_IP "172.18.0.5"
#define SERVER1_MAC "02:42:ac:12:00:03"
#define SERVER2_MAC "02:42:ac:12:00:04"
#define SERVER3_MAC "02:42:ac:12:00:05"

struct{
    int packet_size;
    char data[MAX_PACKET_SIZE];
} typedef packet;

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __type(value, packet);
    __uint(max_entries, QUEUE_SIZE);
} queue SEC(".maps");

/*
    We define a map [array] to store the free threads for each server
*/
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, __u32);
    __type(value, __u32);
    __uint(max_entries, NUM_SERVERS);
} free_threads SEC(".maps");

// create a map mapping the index of the server to its IP address
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, __u32);
    __type(value, __u32);
    __uint(max_entries, NUM_SERVERS);
} server_ips SEC(".maps");


static __always_inline __u16 ip_checksum(unsigned short *buf, int bufsz) {
    unsigned long sum = 0;

    while (bufsz > 1) {
        sum += *buf;
        buf++;
        bufsz -= 2;
    }

    if (bufsz == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

static __always_inline __u16 udp_checksum(struct iphdr *ip, struct udphdr * udp, void * data_end)
{
    __u16 csum = 0;
    __u16 *buf = (__u16*)udp;

    // Compute pseudo-header checksum
    csum += ip->saddr;
    csum += ip->saddr >> 16;
    csum += ip->daddr;
    csum += ip->daddr >> 16;
    csum += (__u16)ip->protocol << 8;
    csum += udp->len;

    // Compute checksum on udp header + payload
    for (int i = 0; i < MAX_UDP_LENGTH; i += 2) {
        if ((void *)(buf + 1) > data_end) {
            break;
        }
        csum += *buf;
        buf++;
    }

    if ((void *)buf + 1 <= data_end) {
       // In case payload is not 2 bytes aligned
        csum += *(__u8 *)buf;
    }

   csum = ~csum;
   return csum;
}

SEC("xdp")
int load_balancer(struct xdp_md *ctx){
    //parse the data

    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Check if the packet is valid
    struct ethhdr *eth = data;
    if ((void*)eth + sizeof(*eth) > data_end) {
        return XDP_DROP;
    }

    struct iphdr *ip = data + sizeof(*eth);
    if ((void*)ip + sizeof(*ip) > data_end) {
        return XDP_DROP;
    }

    if (ip->protocol != IPPROTO_UDP) {
        return XDP_PASS;
    }

    struct udphdr* udp = (void*)ip + sizeof(*ip);

    if ((void*)udp + sizeof(*udp) > data_end) {
        return XDP_DROP;
    }

    if (bpf_ntohs(udp->dest) != LB_PORT){
        return XDP_PASS;
    }

    //check if packet is coming from one of the servers, if yes then if queue is not empty, send the packet to the server
    
    // check if source ip matches one of the server IPs
    // Get the source IP address in network byte order
    __u32 src_ip = ip->saddr;

    // Convert the server IP addresses to network byte order
    __u32 server1_ip = bpf_htonl(inet_addr(SERVER1_IP));
    __u32 server2_ip = bpf_htonl(inet_addr(SERVER2_IP));
    __u32 server3_ip = bpf_htonl(inet_addr(SERVER3_IP));

    // Add the server IP addresses to the server_ips map
    int ret = bpf_map_update_elem(&server_ips,(void*)0,&server1_ip,BPF_ANY);
    if (!ret) return XDP_ABORTED;
    ret = bpf_map_update_elem(&server_ips,(void*)1,&server2_ip,BPF_ANY);
    if (!ret) return XDP_ABORTED;
    ret = bpf_map_update_elem(&server_ips,(void*)2,&server3_ip,BPF_ANY);
    if (!ret) return XDP_ABORTED;

    // Check if the source IP matches one of the server IP addresses
    if ((src_ip == server1_ip) || (src_ip == server2_ip) ||(src_ip == server3_ip)) {
        // Packet is coming from one of the servers
        void* top = NULL;
        if (!bpf_map_peek_elem(&queue,top)){
            // If the queue is empty, free count for that server is incremented
            __u32 idx = -1;
            if (src_ip == server1_ip) idx = 0;
            else if (src_ip == server2_ip) idx = 1;
            else idx = 2;
            __u32* free;
            free = bpf_map_lookup_elem(&free_threads,&idx);
            if (!free) return XDP_ABORTED;

            int ret = bpf_map_update_elem(&free_threads,&idx,(*free)+1,BPF_EXIST);
            if (!ret) return XDP_ABORTED;

            bpf_printk("Queue is empty. Freeing thread for server %d\n",idx+1);
            return XDP_DROP;
        } else{
            // If the queue is not empty, send the packet at the top of the queue to the server
            packet p;
            int ret = bpf_map_pop_elem(&queue,&p);
            if (!ret) return XDP_ABORTED;

            // update the destination MAC addresses
            // Get the destination MAC address
            __u8 dst_mac[ETH_ALEN];
            int idx = -1;
            if (src_ip == server1_ip) {
                memcpy(dst_mac,SERVER1_MAC,ETH_ALEN);
                idx = 0;
            }
            else if (src_ip == server2_ip) {
                memcpy(dst_mac,SERVER2_MAC,ETH_ALEN);
                idx = 1;
            }
            else {
                memcpy(dst_mac,SERVER3_MAC,ETH_ALEN);
                idx = 2;
            }
            
            // Update the destination MAC address
            memcpy(eth->h_dest,dst_mac,ETH_ALEN);

            // Update the destination port in the UDP header
            udp->dest = bpf_htons(SERVER_PORT);

            // Update the checksums
            // Update the IP checksum
            ip->check = 0;
            ip->check = ip_checksum((unsigned short*)ip,ip->ihl * 4);
            // Update the UDP checksum
            udp->check = 0;
            udp->check = udp_checksum(ip,udp,data_end);

            // Send the packet to the server
            bpf_printk("Response received from server %d. Packet present in queue. Sending packet to server %d\n",idx+1,idx+1);
            bpf_xdp_adjust_head(ctx,data_end - sizeof(*eth));
            return bpf_redirect_map(&server_ips,idx,0);
        }
    } else {
        // Packet is not coming from any of the servers
        
        // check the free_threads map if there are any free threads
        __u32* free;
        __u32 idx = -1;
        __u32 max_free_threads = 0;
        for(int i=0;i<NUM_SERVERS;i++){
            free = bpf_map_lookup_elem(&free_threads,&i);
            if (!free) return XDP_ABORTED;
            if (*free > max_free_threads){
                max_free_threads = *free;
                idx = i;
            }
        }

        if (idx == -1){
            // no thread is free so we add the packet in the queue
            packet p;
            p.packet_size = data_end - data;
            memcpy(p.data,data,p.packet_size);
            int ret = bpf_map_push_elem(&queue,&p,BPF_ANY);
            if (!ret) return XDP_ABORTED;
            bpf_printk("No free threads. Adding packet to the queue\n");
            return XDP_DROP;
        } else {
            // thread is free so we send the packet to the server

            // update the destination MAC addresses
            // Get the destination MAC address
            __u8 dst_mac[ETH_ALEN];
            bpf_printk("idx: %d\n",idx);
            if (idx == 0) memcpy(dst_mac,SERVER1_MAC,ETH_ALEN);
            else if (idx == 1) memcpy(dst_mac,SERVER2_MAC,ETH_ALEN);
            else memcpy(dst_mac,SERVER3_MAC,ETH_ALEN);

            // Update the destination MAC address
            memcpy(eth->h_dest,dst_mac,ETH_ALEN);

            // Update the destination port in the UDP header
            udp->dest = bpf_htons(SERVER_PORT);

            // Update the checksums
            // Update the IP checksum
            ip->check = 0;
            ip->check = ip_checksum((unsigned short*)ip,ip->ihl * 4);
            // Update the UDP checksum
            udp->check = 0;
            udp->check = udp_checksum(ip,udp,data_end);

            // decrement the free thread count for that server
            int ret = bpf_map_update_elem(&free_threads,&idx,(*free)-1,BPF_EXIST);
            if (!ret) return XDP_ABORTED;
            // Send the packet to the server
            bpf_printk("Sending packet to server %d\n",idx+1);
            bpf_xdp_adjust_head(ctx,data_end - sizeof(*eth));
            return bpf_redirect_map(&server_ips,idx,0);
        }   
    }

    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";