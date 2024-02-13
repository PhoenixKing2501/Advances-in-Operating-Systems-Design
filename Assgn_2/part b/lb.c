// #include <linux/bpf.h>
#include <linux/if_link.h>
// #include <linux/ip.h>
// #include <linux/udp.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <net/if.h>
// #include <bpf/bpf_endian.h>
// #include <bpf/bpf_core_read.h>
// #include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>

// #include <sys/resource.h>
// #include <linux/if_link.h>
// #include <net/if.h>

#define IP_ADDRESS(x) (unsigned int)(172 + (17 << 8) + (0 << 16) + (x << 24))
#define BACKEND_A 2
#define BACKEND_B 3
#define BACKEND_C 4

// int init_server_load(int map_fd)
// {
// 	int value = 5;
// 	int err;

// 	int key = IP_ADDRESS(BACKEND_A);
// 	err = bpf_map_update_elem(map_fd, &key, &value, BPF_ANY);
// 	if (err)
// 	{
// 		perror("bpf_map_update_elem");
// 		return err;
// 	}

// 	key = IP_ADDRESS(BACKEND_B);
// 	err = bpf_map_update_elem(map_fd, &key, &value, BPF_ANY);
// 	if (err)
// 	{
// 		perror("bpf_map_update_elem");
// 		return err;
// 	}

// 	key = IP_ADDRESS(BACKEND_C);
// 	err = bpf_map_update_elem(map_fd, &key, &value, BPF_ANY);
// 	if (err)
// 	{
// 		perror("bpf_map_update_elem");
// 		return err;
// 	}

// 	return 0;
// }

int main(int argc, char *argv[])
{
	struct bpf_object *obj;
	struct bpf_map *my_map;
	struct bpf_map *queue;

	int prog_fd, ifindex, my_map_fd;

	if (argc != 2)
	{
		printf("Usage: %s <ifname>\n", argv[0]);
		return 1;
	}

	ifindex = if_nametoindex(argv[1]);
	if (!ifindex)
	{
		perror("Invalid interface\n");
		return 1;
	}

	if (bpf_prog_load("lb.bpf.o", BPF_PROG_TYPE_XDP, &obj, &prog_fd))
	{
		perror("bpf_prog_load");
		return 1;
	}

	queue = bpf_object__find_map_by_name(obj, "my_queue");

	if (!queue)
	{
		perror("bpf_object__find_map_by_name");
		return 1;
	}

	my_map = bpf_object__find_map_by_name(obj, "my_map");

	if (!my_map)
	{
		perror("bpf_object__find_map_by_name");
		bpf_object__close(obj);
		return 1;
	}

	my_map_fd = bpf_map__fd(my_map);

	if (my_map_fd < 0)
	{
		perror("bpf_map__fd");
		bpf_object__close(obj);
		return 1;
	}

	if (bpf_set_link_xdp_fd(ifindex, prog_fd, XDP_FLAGS_SKB_MODE) < 0)
	{
		perror("bpf_set_link_xdp_fd");
		bpf_object__close(obj);
		return 1;
	}

	// if (init_server_load(my_map_fd))
	// {
	// 	perror("init_server_load");
	// 	bpf_object__close(obj);
	// 	return 1;
	// }

	printf("Success\n");
}