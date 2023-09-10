/*
    Test 4: Two process can write and read to the file and the data is consistent and separate
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(){
    int arr1[] = {0, 3, 5, 2, 4};
    int arr2[] = {9, 8, 6, 4, 11};
    

    char c = (char)5;

    if(fork()==0){  // Child process
        int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
        if (fd < 0){
            perror("Error opening file");
            return 1;
        }
        write(fd, &c, 1);
        for (int i = 0; i < 5; i++){
            int ret = write(fd, &arr1[i], sizeof(int));
            usleep(100);
        }
        int expected[] = {5, 3, 0, 2, 4};
        for (int i = 0; i < 5; i++){
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
    else{
        int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
        if (fd < 0){
            perror("Error opening file");
            return 1;
        }
        write(fd, &c, 1);
        for (int i = 0; i < 5; i++){
            int ret = write(fd, &arr2[i], sizeof(int));
            usleep(100);
        }
        int expected[] = {11, 9, 8, 6, 4};
        for (int i = 0; i < 5; i++){
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
        int status;
        wait(&status);
        if(status!=0){
            printf("Error: Child process exited with status %d\n", status);
            return 1;
        }
    }
    return 0;
}
