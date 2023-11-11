FROM ubuntu:latest
RUN apt-get update && \
apt-get install -y build-essential git cmake
RUN mkdir /src
WORKDIR /src

COPY ./src/server.c /src/server.c

RUN gcc -lpthread /src/server.c -o /src/server

# ENTRYPOINT ["./server"]
CMD ["/bin/bash"]