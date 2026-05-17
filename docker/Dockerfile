FROM ubuntu:22.04

LABEL maintainer="chatroom-dev"
LABEL description="C++17 Live Streaming Server Development Environment"

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-12 g++-12 \
    cmake \
    ninja-build \
    git \
    curl wget \
    zip unzip \
    vim nano \
    ca-certificates \
    ssl-cert \
    pkg-config \
    libssl-dev \
    zlib1g-dev \
    libmysqlclient-dev \
    libhiredis-dev \
    protobuf-compiler \
    libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100

WORKDIR /opt/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git . \
    && ./bootstrap-vcpkg.sh -disableMetrics \
    && ./vcpkg integrate install \
    && ./vcpkg install \
        spdlog \
        fmt \
        nlohmann-json \
        gtest \
        yaml-cpp \
        --triplet x64-linux

WORKDIR /workspace

CMD ["/bin/bash"]
