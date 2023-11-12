// #include <linux/bpf.h>
// #include <linux/if_ether.h>
// #include <linux/ip.h>
// #include <linux/udp.h>
// #include <bpf/bpf_helpers.h>
// #include <netinet/in.h>
// #include <bpf/bpf_endian.h>

// SEC("filter")
// int xdp_drop_even_parity(struct xdp_md *ctx)
// {
//     // Access the Ethernet header directly from skb data
//     struct ethhdr *eth = (struct ethhdr *)(long)ctx->data;
//     // bpf_xdp_load_bytes(ctx, 0, eth, sizeof(*eth));

//     // Check if it's an IP packet
//     if (eth->h_proto == htons(ETH_P_IP))
//     {
//         // Calculate the offset for the IP header
//         // __u64 ip_offset = sizeof(struct ethhdr);

//         // Access the IP header directly from skb data
//         struct iphdr *ip = (struct iphdr *)(eth + 1);

//         // Check if it's a UDP packet
//         if (ip->protocol == IPPROTO_UDP)
//         {
//             // Calculate the offset for the UDP header
//             // __u64 udp_offset = ip_offset + sizeof(struct iphdr);

//             // Access the UDP header directly from skb data
//             struct udphdr *udp = (struct udphdr *)(ip + 1);

//             // Check if the destination IP and port match the server's
//             if (udp->dest == htons(12000))
//             {
//                 // Calculate the offset for the data
//                 // __u64 data_offset = udp_offset + sizeof(struct udphdr);

//                 // Access the data directly from skb data
//                 char *data = (char *)(udp + 1);

//                 // Check if the data has even parity
//                 if (*data % 2 == 0)
//                 {
//                     // Drop packets with even parity
//                     return XDP_DROP;
//                 }
//             }
//         }
//     }

//     // Let other packets pass through
//     return XDP_PASS;
// }

// char _license[] SEC("license") = "GPL";

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <bpf/bpf_helpers.h>
#include <netinet/in.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_core_read.h>
#include <arpa/inet.h>
#include <stdint.h>

SEC("filter")
int xdp_drop_even_parity(struct xdp_md *ctx)
{

    __u32 len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(int);

    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;


    // To check if the packet contains numeric data to drop from client
    if (data + len > data_end)
        return XDP_PASS;

    struct ethhdr *eth = data;
    __u16 h_proto;
    h_proto = eth->h_proto;

    // Check if it's an IP packet
    if (h_proto == htons(ETH_P_IP))
    {

        struct iphdr *ip = (struct iphdr *)(eth + 1);

        // Check if it's a UDP packet
        if (ip->protocol == IPPROTO_UDP)
        {

            struct udphdr *udp = (struct udphdr *)(ip + 1);

            // Check if the destination IP and port match the server's
            if (udp->dest == htons(12000))
            {

                int *data_int = (int *)(udp + 1);
                int num = htons(*data_int);
                bpf_printk("Data sent by client: %d\n", num);

                // Check if the data has even parity
                if (num % 2 == 0)
                {
                    // Drop packets with even parity
                    bpf_printk("Dropping packet with even parity\n");
                    return XDP_DROP;
                }
            }
        }
    }

    // Let other packets pass through
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
