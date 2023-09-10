/*
    Test 1: Process can write and read to the file and the data is consistent
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(){
    int arr1[] = {0, 3, 5, 2, 4};
    int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
    if (fd < 0){
        perror("Error opening file");
        return 1;
    }

    char c = (char)5;
    int n = 5;
    write(fd, &c, 1);
    for (int i = 0; i < n; i++){
        int ret = write(fd, &arr1[i], sizeof(int));
        usleep(100);
    }
    int expected[] = {5, 3, 0, 2, 4};
    for (int i = 0; i < n; i++){
        int out;
        int ret = read(fd, &out, sizeof(int));
        if(out != expected[i]){
            printf("Error: Expected %d, Got %d\n", expected[i], out);
            close(fd);
            return 1;
        }
        usleep(100);
    }
    close(fd);
    return 0;
}
