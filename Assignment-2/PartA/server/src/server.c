#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#define SERVER_PORT 8080

int main() {
    printf("Starting server...\n");

    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_addr_len = sizeof(client_address);

    // Create socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Socket creation error!!");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Binding error!!");
        exit(EXIT_FAILURE);
    }


    printf("\n-----------SERVER RUNNING------------\n");

    // Print server IP
    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    ioctl(server_socket, SIOCGIFADDR, &ifr);
    printf("SERVER: IP Address: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    char buffer[5];

    // Receive message from client
    while (1) {
        int bytes = recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_address, &client_addr_len);
        if (bytes < 0) {
            perror("Receive error!!");
            exit(EXIT_FAILURE);
        }
        // printf("SERVER: Client IP: %s\n", inet_ntoa(client_address.sin_addr));
        printf("SERVER: Message from client: %s\n", buffer);
    }
    printf("\n-----------SERVER STOPPED------------\n");
    close(server_socket);

    return 0;
}
