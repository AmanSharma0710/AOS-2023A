### Creating Docker Network

First create a docker-network

```
docker network create partb-net
```

### Running Server

To run the docker container that contains the server, run the following commands:

```
cd ./server
docker build . -t partb-server
docker run -it --network partb-net --name <s1|s2|s3> partb-server
```

This runs the server in interactive mode and opens bash in it. Run this command 3 times changing the name of the container in 3 different terminal windows. Then run `./server` in each window so that we get 3 servers up and running.

Now note down each of the server's IP address and MAC address and replace these with the macros used in `loadbalancer.bpf.c` in lines `17-22`. For this execute `docker network inspect partb-net`. Here all the server containers will be listed along with their IPs and MAC addresses.

### Running Loadbalancer

```
cd ../loadbalancer
docker build . -t partb-lb
docker run -it --privileged --network partb-net --name lb partb-lb
```

The server is connected to the docker network as well. Run the following commands on the bash server to load the xdp-program in the interface.

```
mount -t bpf none /sys/fs/bpf
mount -t debugfs none /sys/kernel/debug
xdp-loader load -m skb -s xdp eth0 loadbalancer.o
```

Now xdp is loaded in the interface. Get the IP address of the loadbalancer by using `docker network inspect testb-net`. lb container will be listed here with its IP

### Running Client

To run the docker container that contains the client, run these commands:

```
cd ../client
docker build . -t partb-client
docker run -it --network partb-net --name client partb-client
```

This runs the client container in interactive mode and opens bash in it. Now take the IP address of the loadbalancer, which you got after running it and run `./client <loadbalance_IP>` to run the client program.
