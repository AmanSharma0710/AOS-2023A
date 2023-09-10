## Team Members:
>	Ashish Rekhani (20CS10010)  
    Aman Sharma (20CS30063)

`deque_lkm.c` is the kernel module which implements the deque data structure through procfs.  
`Makefile` can be used to build the kernel module and test files, run the tests, install and remove the kernel module, and clean the directory.  
`tests/` contains the test files for the kernel module.  

### Instructions to run the code
* To build the kernel module, run the command `make` in the directory.
* To make the tests, run the command `make tests` in the directory.
* To run the tests, run the command `make run_tests` in the directory.
* To install the kernel module, run the command `make install` in the directory.
* To remove the kernel module, run the command `make remove` in the directory.
* To clean the directory, run the command `make clean` in the directory.

After building, the kernel module can be found in the file `deque_lkm.ko`.
It can also be installed by running the command `sudo insmod deque_lkm.ko`.
It can be removed by running the command `sudo rmmod deque_lkm`.
`lsmod` gives the list of all the installed kernel modules.
The module also logs a bunch of kernel messages, which can be seen by running the command `dmesg` or `sudo tail -f /var/log/syslog`.


The kernel module uses procfs to implement the deque data structure. Upon installation, it creates a file named /proc/partb_1_20CS30063_20CS10010, which can be used to interact with the module. The file supports reading and writing to it, and provides a different deque to every process that accesses it, but is not thread-safe. 

### A user-space process can interact with the LKM in the following manner:
1. It will open the file (/proc/partb_1_partb_1_20CS30063_20CS10010) in read-write mode.
2. Write one byte of data to the file to initialize the heap. The first byte should contain the maximum capacity N of the deque. The size N should be in between 1 to 100 (including 1 and 100). If N is not within this range, it produces an EINVAL error, and the LKM is left uninitialized.
3. Now the user can use write calls to insert integers into the deque. LKM will insert the integer on the left of the queue if it is odd and to the right of the queue if it is even. LKM must produce an invalid argument error in case a wrong argument is given (i.e, unsupported type). On a successful write call, LKM will return the number of bytes written (4 bytes for 32-bit integers). LKM will produce EACCES error for any excess write calls (> N) and return -EACCES.
4. Read calls return the integer from left only. LKM will produce EACCES error for any excess read calls (when size of deque is 0). In case of a successful call, it will return the number of bytes read (4 bytes), and in case of an error, it will return -EACCES.
5. Userspace process closes the file. The user-space process can now open the file again and repeat the above steps to create a new deque. Moreover multiple processes can access the file at the same time, and each process will get a different deque.