#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("-----------CLIENT RUNNING------------\n");
    int client_socket;
    struct sockaddr_in server_address;

    // Create socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = inet_addr(argv[1]);

    printf("CLIENT: Server IP: %s\n", inet_ntoa(server_address.sin_addr));


    for(int i=0;i<10;i++){
        char buffer[1];
        buffer[0] = 'A'+i;
        // Send message to server
        if (sendto(client_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
            perror("Send error!!");
            exit(EXIT_FAILURE);
        }
        printf("CLIENT: Message sent to server: %c\n", buffer[0]);
        sleep(1);
    }


    printf("-----------CLIENT STOPPED------------\n");
    close(client_socket);

    return 0;
}
