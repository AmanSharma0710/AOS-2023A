/*
    Test 1: Same process cannot open the file twice
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

int main(){
    int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
    if (fd < 0)
    {
        perror("Error opening file");
        printf("Error number: %d\n", errno);
        return 1;
    }
    int fd1 = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
    if (fd1 >= 0)
    {
        perror("Same process opened the file twice\n");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}

