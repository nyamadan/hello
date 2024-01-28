# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.245.2/containers/ubuntu/.devcontainer/base.Dockerfile

# [Choice] Ubuntu version (use ubuntu-22.04 or ubuntu-18.04 on local arm64/Apple Silicon): ubuntu-22.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="jammy"
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}

ENV EMSDK=/emsdk \
    EM_CONFIG=/emsdk/.emscripten \
    EMSDK_NODE=/emsdk/node/16.20.0_64bit/bin/node \
    PATH="/emsdk:/emsdk/upstream/emscripten:/emsdk/node/16.20.0_64bit/bin:${PATH}"

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends bzip2 xz-utils make

RUN cd /tmp \
    && curl -LO https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-linux-x86_64.tar.gz \
    && tar -xvzf cmake-3.28.1-linux-x86_64.tar.gz -C /usr/local --strip-components=1 --no-same-owner \
    && rm cmake-3.28.1-linux-x86_64.tar.gz
RUN cd /tmp \
    && curl -LO https://nodejs.org/dist/v20.11.0/node-v20.11.0-linux-x64.tar.xz \
    && tar -xvJf node-v20.11.0-linux-x64.tar.xz -C /usr/local --strip-components=1 --no-same-owner \
    && npm install -g npm \
    && npm install -g pnpm@8.15.0 \
    && rm node-v20.11.0-linux-x64.tar.xz

RUN git clone https://github.com/emscripten-core/emsdk.git /emsdk \
    && cd /emsdk \
    && ./emsdk install 3.1.35 \
    && ./emsdk activate 3.1.35
