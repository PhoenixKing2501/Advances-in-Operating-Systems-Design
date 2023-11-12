# List of instructions

### Load eBPF program using ip command
sudo ip link set dev lo xdp obj filter.o sec filter

### Unload eBPF program
sudo ip link set dev lo xdp off
