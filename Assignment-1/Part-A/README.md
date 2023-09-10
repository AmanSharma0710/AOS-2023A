Team Members:
	Ashish Rekhani (20CS10010)
	Aman Sharma (20CS30063)

Steps followed to configure and build the kernel on the VM provided:

1. Download the source code:

```
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.191.tar.xz
```

2. Extract the source code: `tar xvf linux-5.10.191.tar.xz`
3. Enter the directory of the extracted code and run `make menuconfig`. This will open up a window where different configurations can be set for the kernel. Here we disable NUMA and KYBER allocator and also enable MPTCP. This generates a .config file which is also attached. We have also disabled SYSTEM_TRUSTED_KEYS and SYSTEM_REVOCATION_KEYS in the config file itself so the build runs without any errors.
4. Build the kernel: `make -j<number of threads>`
   We used 6 threads to avoid too much load on the VM. The number of threads can be increased to parallely build different independent components. The suggested number of threads is 1.5 times number of cores.
5. Install required modules: `sudo make install_modules`
6. Install kernel: `sudo make install`
7. Reboot and verify kernel version: `uname -r`

Reference: [https://phoenixnap.com/kb/build-linux-kernel](https://phoenixnap.com/kb/build-linux-kernel)
