FROM ubuntu:latest
RUN apt-get update && \
apt-get install -y build-essential git cmake \
zlib1g-dev libevent-dev \
libelf-dev llvm \
clang libc6-dev-i386 \
libssl-dev libncurses5-dev \
iproute2 \
pkg-config

RUN apt-get install -y m4
RUN apt-get install -y libpcap-dev

RUN mkdir /src && \
git init
WORKDIR /src
# Link asm/byteorder.h into eBPF
RUN ln -s /usr/include/x86_64-linux-gnu/asm/ /usr/include/asm
# Build `bpf as a static lib
RUN git clone https://github.com/libbpf/libbpf-bootstrap.git && \
cd libbpf-bootstrap && \
git submodule update --init --recursive
RUN cd libbpf-bootstrap/libbpf/src && \
make BUILD_STATIC_ONLY=y && \
make install BUILD_STATIC_ONLY=y LIBDIR=/usr/lib/x86_64-linux-gnu/
# Clones the linux kernel repo and use the latest linux kernel source BPF headers
RUN git clone --depth 1 https://github.com/torvalds/linux.git && \
    cp -r linux/include/uapi/linux/bpf* /usr/include/linux/

RUN git clone --depth 1 https://github.com/xdp-project/xdp-tools.git 

RUN chmod +x xdp-tools/configure 
RUN ./xdp-tools/configure 
RUN make -C ./xdp-tools
RUN make -C ./xdp-tools install

COPY ./src/loadbalancer.c /src/loadbalancer.c

RUN clang -O2 -Wall -target bpf -c /src/xdp_prog.c -o /src/xdp_prog.o

EXPOSE 8080

RUN chmod +x /src/server

ENTRYPOINT [ "/bin/bash" ]