FROM debian:bullseye-slim
ENV EMSDK=/emsdk \
    EM_CONFIG=/emsdk/.emscripten \
    EMSDK_NODE=/emsdk/node/14.18.2_64bit/bin/node \
    PATH="/emsdk:/emsdk/upstream/emscripten:/emsdk/node/14.18.2_64bit/bin:${PATH}"
RUN apt-get update && apt-get install -y curl git python3 bzip2 xz-utils ninja-build
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.24.2/cmake-3.24.2-linux-x86_64.tar.gz \
    && tar -xvzf cmake-3.24.2-linux-x86_64.tar.gz -C /usr/local --strip-components=1 --no-same-owner \
    && rm cmake-3.24.2-linux-x86_64.tar.gz
RUN curl -LO https://nodejs.org/dist/v18.9.0/node-v18.9.0-linux-x64.tar.xz \
    && tar -xvJf node-v18.9.0-linux-x64.tar.xz -C /usr/local --strip-components=1 --no-same-owner \
    && rm node-v18.9.0-linux-x64.tar.xz
RUN git clone https://github.com/emscripten-core/emsdk.git /emsdk \
    && cd /emsdk \
    && ./emsdk install 2.0.27 \
    && ./emsdk activate 2.0.27
CMD ["/bin/bash"]