FROM ubuntu:18.04

RUN apt-get update && apt-get install -y --no-install-recommends \
  perl binutils make build-essential flex bison ncurses-dev \
  gcc-6-aarch64-linux-gnu \
  binutils-aarch64-linux-gnu bc git \
  u-boot-tools device-tree-compiler python \
  && ln -s /usr/bin/aarch64-linux-gnu-gcc-6 /usr/bin/aarch64-linux-gnu-gcc

WORKDIR /src

