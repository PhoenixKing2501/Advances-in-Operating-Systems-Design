all:
	clang -g -O2 -target bpf -D__TARGET_ARCH_x86_64 -I . -c lb.bpf.c -o lb.bpf.o
	clang -g -O2 -Wall -I . -c lb.c -o lb.o
	clang -Wall -O2 -g lb.o -lbpf -lelf -lz -o lb

clean:
	rm -f lb.bpf.o lb.o lb

load:
	./lb eth0 
