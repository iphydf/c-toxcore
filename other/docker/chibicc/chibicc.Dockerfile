FROM toxchat/c-toxcore:sources AS sources
FROM ubuntu:22.04

RUN apt-get update && \
 DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
 ca-certificates \
 creduce \
 gcc \
 git \
 libc-dev \
 libopus-dev \
 libsodium-dev \
 libvpx-dev \
 make \
 pkg-config \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /work/chibicc
RUN ["git", "clone", "--depth=1", "https://github.com/rui314/chibicc", "/work/chibicc"]
RUN ["make", "CFLAGS=-O3"]

WORKDIR /work/c-toxcore
COPY --from=sources /src/ /work/c-toxcore
COPY other/docker/chibicc/Makefile /work/c-toxcore/
RUN ["make"]

#RUN apt-get update && \
# DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
# gdb

# TODO(iphydf): Investigate why it segfaults.
#SHELL ["/bin/bash", "-o", "pipefail", "-c"]
#RUN ./send_message_test | grep "tox clients connected"
#RUN ./send_message_test

# If chibicc crashes, make a repro with this:
#COPY other/docker/chibicc/creduce.sh /work/c-toxcore
#RUN chibicc -E -o broken.c auto_tests/auto_test_support.c
#RUN creduce ./creduce.sh broken.c
