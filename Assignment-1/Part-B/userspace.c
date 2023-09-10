#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>


int main(){
    int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
    cat /proc/partb_1_20CS30063_20CS1010
    if(fd < 0){
        //print the error
        printf("Errno: %d\n", fd);
        //print errno
        printf("Error: %d\n", errno);
        return 0;
    }
    int rno =   rand()%40;
    for(int i=0;i<rno;i++){
        int rw = rand()%2;
        if(rw == 0){
            int val = rand()%100;
            char buf[100];
            sprintf(buf, "push_back %d", val);
            write(fd, buf, strlen(buf));
        }
        else{
            char buf[100];
            sprintf(buf, "pop_back");
            write(fd, buf, strlen(buf));
        }
        sleep(1);
    }
    close(fd);
    return 0;
}