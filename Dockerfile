FROM ubuntu:24.04@sha256:68ee22004a9109b44973313b02ff7b1de250f1de0f5ecb628d73f579aaff33ac AS build-env

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        gcc \
        gcc-aarch64-linux-gnu \
        git \
        libc6-dev \
        libc6-dev-arm64-cross \
        make \
        qemu-user \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

FROM build-env AS test
COPY CMakeLists.txt ./
COPY include/ include/
COPY src/ src/
COPY tests/ tests/

RUN cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build build/Debug --parallel \
    && ctest --test-dir build/Debug --output-on-failure

RUN cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build/Release --parallel \
    && ctest --test-dir build/Release --output-on-failure
