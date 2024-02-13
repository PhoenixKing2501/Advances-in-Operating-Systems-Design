#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <bpf/bpf_helpers.h>
#include <netinet/in.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_core_read.h>

#include "lb.bpf.h"

#define BACKEND_A 2 // index 0
#define BACKEND_B 3 // index 1
#define BACKEND_C 4 // index 2
#define CLIENT 1
#define LB 5



struct bpf_map_def SEC("maps") my_map = {
	.type = BPF_MAP_TYPE_ARRAY,
	.key_size = sizeof(__u32),
	.value_size = sizeof(__u32),
	.max_entries = 3,
};

// initialise the map as server ip and value will be 5 for 3 servers

struct bpf_map_def SEC("maps") my_queue = {
	.type = BPF_MAP_TYPE_QUEUE,
	// .key_size = sizeof(__u32),
	.value_size = sizeof(__u32),
	.max_entries = 256,
};

int getDestIP(int *index);
int getIndexFromIP(int ip);

#define IP_ADDRESS(x) (unsigned int)(172 + (17 << 8) + (0 << 16) + (x << 24))

inline void modifyPacketHeader(void *data, void *data_end, int source_ip, int source_port, int dest_ip, int dest_port)
{
	struct ethhdr *eth = data;
	struct iphdr *iph = data + sizeof(struct ethhdr);
	struct udphdr *udph = data + sizeof(struct ethhdr) + iph->ihl * 4;

	if (data + sizeof(struct ethhdr) + iph->ihl * 4 + sizeof(struct udphdr) > data_end)
	{
		bpf_printk("PASS : udphdr error\n");
		return;
	}

	udph->dest = bpf_htons(dest_port);
	udph->source = bpf_htons(source_port);

	iph->daddr = IP_ADDRESS(dest_ip);
	iph->saddr = IP_ADDRESS(source_ip);
	iph->check = iph_csum(iph);

	eth->h_dest[5] = dest_ip;
	eth->h_source[5] = source_ip;
}

int getData(void *data, void *data_end)
{
	struct ethhdr *eth = data;
	struct iphdr *iph = data + sizeof(struct ethhdr);
	struct udphdr *udph = data + sizeof(struct ethhdr) + iph->ihl * 4;

	if (data + sizeof(struct ethhdr) + iph->ihl * 4 + sizeof(struct udphdr) + sizeof(int) > data_end)
	{
		bpf_printk("PASS : udphdr error\n");
		return -1;
	}

	int *data_int = (int *)(udph + 1);
	int num = bpf_ntohl(*data_int);
	// bpf_printk("Data sent by client: %d\n", num);

	return num;
}

int putData(void *data, void *data_end, int num)
{
	struct ethhdr *eth = data;
	struct iphdr *iph = data + sizeof(struct ethhdr);
	struct udphdr *udph = data + sizeof(struct ethhdr) + iph->ihl * 4;

	if (data + sizeof(struct ethhdr) + iph->ihl * 4 + sizeof(struct udphdr) + sizeof(int) > data_end)
	{
		bpf_printk("PASS : udphdr error\n");
		return -1;
	}

	int *data_int = (int *)(udph + 1);
	*data_int = bpf_htonl(num);
	// bpf_printk("Data sent by client: %d\n", num);

	return num;
}

SEC("xdp_lb")
int xdp_load_balancer(struct xdp_md *ctx)
{
	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	bpf_printk("got something");

	struct iphdr ip;
	int index;
	long *value;



	struct ethhdr *eth = data;
	if (data + sizeof(struct ethhdr) > data_end)
		return XDP_DROP;

	if (bpf_ntohs(eth->h_proto) != ETH_P_IP)
		return XDP_PASS;

	struct iphdr *iph = data + sizeof(struct ethhdr);
	if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end)
		return XDP_DROP;

	if (iph->protocol != IPPROTO_TCP)
		return XDP_PASS;

	bpf_printk("Got UDP packet from %x", iph->saddr);
	int source_ip = IP_ADDRESS(LB), source_port = 12000;
	int dest_ip, dest_port = 12000;

	if (iph->saddr == IP_ADDRESS(CLIENT))
	{
		// get the destination IP
		int index;
		dest_ip = getDestIP(&index);

		if (dest_ip != -1)
		{
			bpf_printk("Sending packet to %x", dest_ip);
			int *value;
			value = bpf_map_lookup_elem(&my_map, &index);
			if (value) __sync_fetch_and_add(value, 1);
			// bpf_map_update_elem(&my_map, &index, value, BPF_EXIST);
		}
		else
		{
			bpf_printk("No backend available");
			// queue the packet
			int num = getData(data, data_end);
			int ret = bpf_map_push_elem(&my_queue, &num, BPF_EXIST);
			if (ret)
			{
				bpf_printk("Queue full");
				return XDP_DROP;
			}
			bpf_printk("Packet queued");
			return XDP_DROP;
		}
	}
	else
	{
		// update the server load (if server sends a msg, increase its number of free threads by 1)
		// int num = getData(data, data_end);
		int key = iph->saddr;
		key = getIndexFromIP(key);
		if (key == -1)
		{
			return XDP_PASS;
		}
		int *value;
		value = bpf_map_lookup_elem(&my_map, &key);
		if (value)
			__sync_fetch_and_add(value, -1);
		// bpf_map_update_elem(&my_map, &key, value, BPF_EXIST);

		// get the next packet from the queue
		int num;
		int ret = bpf_map_pop_elem(&my_queue, &num);
		if (ret)
		{
			bpf_printk("Queue empty");
			return XDP_DROP;
		}
		bpf_printk("Packet dequeued");

		// get the destination IP
		int index;
		dest_ip = getDestIP(&index);
		value = bpf_map_lookup_elem(&my_map, &index);
		if (value) __sync_fetch_and_add(value, 1);
		// bpf_map_update_elem(&my_map, &index, value, BPF_EXIST);
		bpf_printk("Sending packet to %x", dest_ip);

		// put the data in the packet
		putData(data, data_end, num);
	}

	modifyPacketHeader(data, data_end, source_ip, source_port, dest_ip, dest_port);

	return XDP_TX;
}

int getIndexFromIP(int ip)
{
	if (ip == IP_ADDRESS(BACKEND_A))
		return 0;
	else if (ip == IP_ADDRESS(BACKEND_B))
		return 1;
	else if (ip == IP_ADDRESS(BACKEND_C))
		return 2;
	else
		return -1;
}

int getDestIP(int *index)
{
	int *value1, *value2, *value3, dest_ip;
	int key1 = 0, key2 = 1, key3 = 2;

	value1 = bpf_map_lookup_elem(&my_map, &key1);
	value2 = bpf_map_lookup_elem(&my_map, &key2);
	value3 = bpf_map_lookup_elem(&my_map, &key3);

	if (!value1 || !value2 || !value3)
	{
		bpf_printk("Error in getting value from map");
		*index = -1;
		return -1;
	}

	if (*value1 < 5)
	{
		dest_ip = IP_ADDRESS(BACKEND_A);
		*index = 0;
	}
	else if (*value2 < 5)
	{
		dest_ip = IP_ADDRESS(BACKEND_B);
		*index = 1;
	}
	else if (*value3 < 5)
	{
		dest_ip = IP_ADDRESS(BACKEND_C);
		*index = 2;
	}
	else // no backend available
	{
		dest_ip = -1;
		*index = -1;
	}

	return dest_ip;
}

char _license[] SEC("license") = "GPL";
