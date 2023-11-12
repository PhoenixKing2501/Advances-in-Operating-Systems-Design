all: filter.o client server

filter.o: filter.c
	clang -O2 -target bpf -g -c filter.c -o filter.o
	
client: client.c
	gcc -o client.o client.c

server: server.c
	gcc -o server.o server.c

clean:
	rm -f client.o server.o filter.o