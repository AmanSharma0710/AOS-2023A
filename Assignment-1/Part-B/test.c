#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
    printf("Running test..\n");
    int arr1[] = {0, 3, 5, 2, 4};
    int arr2[] = {9, 8, 6, 4, 11};
    int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);
    if (fd < 0)
    {
        perror("Error opening file");
        return 1;
    }

    char c = (char)5;
    int n = 5;
    write(fd, &c, 1);
    for (int i = 0; i < n; i++)
    {
        int ret = write(fd, &arr1[i], sizeof(int));
        printf("[Parent] Write: %d, Return: %d\n", val[i], ret);
        usleep(100);
    }
    for (int i = 0; i < n; i++)
    {
        int out;
        int ret = read(fd, &out, sizeof(int));
        printf("[Parent] Read: %d, Return: %d\n", out, ret);
        usleep(100);
    }
    close(fd);
    printf("Test completed!\n");
    return 0;
}


/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void execChild(int val[], int n, int fd)
{
    char c = (char)n;
    write(fd, &c, 1);
    for (int i = 0; i < n; i++)
    {
        int ret = write(fd, &val[i], sizeof(int));
        printf("[Child] Write: %d, Return: %d\n", val[i], ret);
        usleep(100);
    }
    for (int i = 0; i < n; i++)
    {
        int out;
        int ret = read(fd, &out, sizeof(int));
        printf("[Child] Read: %d, Return: %d\n", out, ret);
        usleep(100);
    }
}

void execParent(int val[], int n, int fd)
{
    char c = (char)n;
    write(fd, &c, 1);
    for (int i = 0; i < n; i++)
    {
        int ret = write(fd, &val[i], sizeof(int));
        printf("[Parent] Write: %d, Return: %d\n", val[i], ret);
        usleep(100);
    }
    for (int i = 0; i < n; i++)
    {
        int out;
        int ret = read(fd, &out, sizeof(int));
        printf("[Parent] Read: %d, Return: %d\n", out, ret);
        usleep(100);
    }
}

int main()
{
    printf("Running test..\n");
    int arr1[] = {0, 3, 5, 2, 4};
    int arr2[] = {9, 8, 6, 4, 11};
    int fd = open("/proc/partb_1_20CS30063_20CS10010", O_RDWR);

    if (fd < 0)
    {
        perror("Error opening file");
        return 1;
    }

    int pid = fork();
    if (pid == 0)
    {
        execChild(arr2, 5, fd);
    }
    else
    {
        execParent(arr1, 5, fd);
        wait(NULL);
    }

    close(fd);
    printf("Test completed!\n");
    return 0;
}

*/