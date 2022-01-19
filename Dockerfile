From gcc:latest

RUN cd /usr/local && wget https://cmake.org/files/v3.22/cmake-3.22.1-linux-aarch64.tar.gz && tar -zxvf cmake-3.22.1-linux-aarch64.tar.gz && mv cmake-3.22.1-linux-aarch64 cmake-3.22.1 && ln -sf /usr/local/cmake-3.22.1/bin/cmake /usr/bin/cmake

COPY . ~/yushengyuan/test
WORKDIR ~/yushengyuan/test

RUN mkdir build && cd build && cmake .. && make


