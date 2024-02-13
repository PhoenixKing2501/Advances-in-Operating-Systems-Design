#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 12000 // Replace with the desired server port
#define DATA_SIZE 4

int server_socket;

struct thread_args
{
	int data;
	struct sockaddr_in client_addr;
};

void *thread_function(void *arg)
{
	struct thread_args *args = (struct thread_args *)arg;
	struct sockaddr_in client_addr = args->client_addr;
	int data = args->data;
	free(arg);

	sleep(data);

	// Send the data back to the client
	printf("Sending data to client %s:%d: %d\n",
		   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), data);
	if (sendto(server_socket, &data, DATA_SIZE, 0,
			   (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
	{
		perror("Send failed");
	}

	pthread_exit(NULL);
}

int main()
{
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	// Create UDP socket
	if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	// Fill in server details
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind the socket
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("Bind failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	printf("Server started on 127.0.0.1:%d\n", SERVER_PORT);

	pthread_t thread_id;

	while (1)
	{
		int received_data;

		// Receive data from a client
		if (recvfrom(server_socket, &received_data, DATA_SIZE, 0,
					 (struct sockaddr *)&client_addr, &client_addr_len) == -1)
		{
			perror("Receive failed");
			continue; // Continue listening on error
		}

		int data = ntohl(received_data);

		// Print the received data
		printf("Received data from client %s:%d: %d\n",
			   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), data);

		// Create a thread to handle the client
		struct thread_args *arg = malloc(sizeof(struct thread_args));
		arg->data = data;
		arg->client_addr = client_addr;

		if (pthread_create(&thread_id, NULL, thread_function, arg) != 0)
		{
			perror("Thread creation failed");
			continue; // Continue listening on error
		}

		if (pthread_detach(thread_id) != 0)
		{
			perror("Thread detach failed");
			continue; // Continue listening on error
		}
	}

	// The program will never reach here in the infinite loop

	return 0;
}