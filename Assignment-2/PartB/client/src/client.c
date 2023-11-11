#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define LB_PORT 8080

int main(int argc, char *argv[]) {
    if (argc != 4 || (argv[3][0] != 'L' && argv[3][0] != 'H') || atoi(argv[2]) < 0) {
        printf("Usage: %s <Load Balancer IP> <num_requests_to_be_sent> <L|H> \n", argv[0]);
        printf("L: Low sleep time\nH: High sleep time\n");
        exit(EXIT_FAILURE);
    }
    int high = (argv[3][0] == 'H') ? 1 : 0;

    printf("-----------CLIENT RUNNING------------\n");
    int client_socket;
    struct sockaddr_in load_balancer_address;

    // Create socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up load_balancer address
    load_balancer_address.sin_family = AF_INET;
    load_balancer_address.sin_port = htons(LB_PORT);
    load_balancer_address.sin_addr.s_addr = inet_addr(argv[1]);

    printf("CLIENT: Load Balancer IP: %s\n", inet_ntoa(load_balancer_address.sin_addr));

    // we will be sending random int between l and h to the load_balancer
    // and the load_balancer will wait for the time equal to the number

    int l = 1, h = 15;  // CHANGE THIS TO CHANGE THE RANGE OF PROCESSING TIME

    int num = atoi(argv[2]);
    while (num--) {
        // in case of high sleep times we add 30s to the random number
        // this will ensure the requests will be queued
        int processing_time = l + (rand() % h) + (high * 30);
        printf("CLIENT: Sending number %d to load balancer\n", processing_time);
        if (sendto(client_socket, &processing_time, sizeof(processing_time), 0, (struct sockaddr*)&load_balancer_address, sizeof(load_balancer_address)) < 0) {
            perror("Send error!!");
            exit(EXIT_FAILURE);
        }   
        sleep(1);
    }
    printf("-----------CLIENT STOPPED------------\n");
    close(client_socket);

    return 0;
}
