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
# Git metadata is needed to generate the version header.
COPY . .
RUN sh ci.sh

FROM scratch AS artifacts
COPY --from=test /artifacts/ /
