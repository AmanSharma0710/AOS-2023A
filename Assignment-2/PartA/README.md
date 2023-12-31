### Creating Docker Network

First create a docker-network 

```
docker network create parta-net
```

### Running Server

To run the docker container that contains the server, run the following commands:

```
cd ./server
docker build . -t parta-server
docker run -it --privileged --network parta-net --name server parta-server
```

This runs the server in interactive mode and opens bash in it. The server is connected to the docker network as well. Run the following commands on the bash server to load the xdp-program in the interface

```
mount -t bpf none /sys/fs/bpf
mount -t debugfs none /sys/kernel/debug
xdp-loader load -m skb -s xdp eth0 xdp_filter.o
```

Now xdp is loaded in the interface. Now simply run the server with `./server` to run the server. This will display the server IP which has to be input as argument while running the client. We have mounted filesystems to make it easier to debug, and allow us to use xdp-loader to load our ebpf program. These steps couldn't be done in the Dockerfile as mounting needs superuser privileges.

### Running Client

To run the docker container that contains the client, run these commands:

```
cd ../client
docker build . -t parta-client
docker run -it --network parta-net --name client parta-client
```

This runs the client container in interactive mode and opens bash in it. Now take the IP address of the server, which you got after running it and run `./client <server_IP>` to run the client program.
