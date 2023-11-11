#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <pthread.h>

#define LB_PORT 8080
#define SERVER_PORT 20000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int server_socket;
struct sockaddr_in server_address;

void* process_request(void *t_no){
    // this function is where threads will be spawned to process the requests
    int thread_no = (int) t_no;
    thread_no += 1;
    int wait_time;
    struct sockaddr_in load_balancer_address;
    socklen_t load_balancer_addr_len = sizeof(load_balancer_address);
    while(1){
        int wait_time;
        if(recvfrom(server_socket, &wait_time, sizeof(wait_time), 0, (struct sockaddr*)&load_balancer_address, &load_balancer_addr_len) < 0){
            perror("Receive error!!");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&mutex);
        printf("SERVER: Thread %d: Processing request for %d seconds\n", thread_no, wait_time);
        pthread_mutex_unlock(&mutex);
        sleep(wait_time);

        // send a message to the load balancer that the server is free
        char* buffer = "I am free!!:)";
        if(sendto(server_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&load_balancer_address, sizeof(load_balancer_address)) < 0){
            perror("Send error!!");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&mutex);
        printf("SERVER: Thread %d: Sent message to load balancer that server is free\n", thread_no);
        pthread_mutex_unlock(&mutex);
    }
}

int main() {

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

    // bind the socket to our specified IP and port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Bind error!!");
        exit(EXIT_FAILURE);
    }

    printf("-------------SERVER RUNNING-------------\n");
    
    // Print server IP
    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    ioctl(server_socket, SIOCGIFADDR, &ifr);
    printf("SERVER: IP Address: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    // Create a threadpool of 5 threads that will handle the requests
    // recvfrom is atomic, so in each of our threads we just wait on the recvfrom call
    // and when we receive a request we process it, and then send a "i am free" message to the load balancer

    pthread_t threads[5];
    for(int i=0;i<5;i++){
        pthread_create(&threads[i], NULL, process_request, (void *)i);
    }

    for(int i=0;i<5;i++){
        pthread_join(threads[i], NULL);
    }

    printf("-------------SERVER STOPPED-------------\n");

    close(server_socket);
    pthread_mutex_destroy(&mutex);

    return 0;
}
