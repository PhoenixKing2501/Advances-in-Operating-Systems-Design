#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// #define SERVER_IP "127.0.0.1" // Replace with the actual server IP
#define SERVER_PORT 12000     // Replace with the actual server port
#define DATA_SIZE sizeof(int)
#define PAUSE_INTERVAL 5

int main(int argc, char* argv[])
{
    int client_socket;
    struct sockaddr_in server_addr;

    // Create UDP socket
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];

    // Fill in server details
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0)
    {
        perror("Invalid server IP");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    int continue_sending = 1;
    int data_count = 0;

    while (continue_sending)
    {
        // send data bytes as the numeric data to send

        int data = htonl(data_count);


        // Send data to server
        if (sendto(client_socket, &data, DATA_SIZE, 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            perror("Send failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Log the data being sent
        printf("Sent data to server: %d\n", data_count);

        data_count++;

        // Ask whether to continue after every 5 data bytes
        if (data_count % PAUSE_INTERVAL == 0)
        {
            printf("Do you want to continue sending data? (1/0): ");
            scanf("%d", &continue_sending);
        }

        // Wait for 1 second before the next send
        sleep(1);
    }

    // Close the socket
    close(client_socket);

    return 0;
}
