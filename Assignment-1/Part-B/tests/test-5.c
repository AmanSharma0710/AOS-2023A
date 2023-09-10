/*
    Test 5: If child process tries to access the data of parent process, it should get error
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
    if(fork()==0){  // Child process
        for (int i = 0; i < 5; i++){
            int ret = write(fd, &arr1[i], sizeof(int));
            if(ret >= 0){
                printf("Error: Child process was able to write to parent's file\n");
                close(fd);
                return 1;
            }
            usleep(100);
        }
        for(int i = 0; i < 5; i++){
            int out;
            int ret = read(fd, &out, sizeof(int));
            if(ret >= 0){
                printf("Error: Child process was able to read from parent's file\n");
                close(fd);
                return 1;
            }
            usleep(100);
        }
        close(fd);
        return 0;
    }
    else{
        int status;
        wait(&status);
        if(status != 0){
            printf("Error: Child process exited with non-zero status\n");
            close(fd);
            return 1;
        }
        close(fd);
    }

    return 0;
}
