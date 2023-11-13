#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 12000 // Replace with the desired server port
#define DATA_SIZE 4

int main()
{
    int server_socket;
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
    }

    // The program will never reach here in the infinite loop

    return 0;
}
