FROM toxchat/c-toxcore:sources AS sources
FROM ubuntu:22.04

RUN apt-get update && \
 DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
 ca-certificates \
 clang \
 creduce \
 gdb \
 git \
 libc-dev \
 libopus-dev \
 libsodium-dev \
 libvpx-dev \
 make \
 pkg-config \
 python3 \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /work/dyibicc
RUN ["git", "clone", "--depth=1", "https://github.com/sgraham/dyibicc", "/work/dyibicc"]
RUN ["./m", "d"]

WORKDIR /work/c-toxcore
COPY --from=sources /src/ /work/c-toxcore
COPY other/make_single_file /work/c-toxcore/other/
RUN echo "#define DISABLE_STATIC_ASSERT" > main.c \
 && echo "#define MIN_LOGGER_LEVEL LOGGER_LEVEL_TRACE" >> main.c \
 && echo "#define LOGGER_TO_STDERR" >> main.c \
 && other/make_single_file -core auto_tests/tox_new_test.c \
    |  sed -e 's/#ifndef C_TOXCORE.*//;s/#endif .*C_TOXCORE.*//;s/^#line.*//' \
    >> main.c
#RUN gdb -batch -ex 'r' -ex 'bt' --args /work/dyibicc/out/ld/dyibicc main.c

#RUN apt-get update && \
# DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
# gdb

# TODO(iphydf): Investigate why it segfaults.
#SHELL ["/bin/bash", "-o", "pipefail", "-c"]
#RUN ./send_message_test | grep "tox clients connected"
#RUN ./send_message_test

# If chibicc crashes, make a repro with this:
COPY other/docker/chibicc/creduce.sh /work/c-toxcore
RUN ./creduce.sh main.c
#RUN creduce ./creduce.sh main.c

#COPY other/docker/chibicc/broken.i /work/c-toxcore/broken.c
#RUN tcc -g -o tcc_bin broken.c && ./tcc_bin
#RUN gdb -batch -ex 'file ./tcc_bin' -ex 'disassemble working' | sed -e 's/0x000[0-9a-f]*//' > working.asm
#RUN gdb -batch -ex 'file ./tcc_bin' -ex 'disassemble broken' | sed -e 's/0x000[0-9a-f]*//' > broken.asm
#RUN diff -u working.asm broken.asm || true
#RUN /work/chibicc/chibicc -o chibicc_bin broken.c
#RUN gdb -batch -ex 'file ./chibicc_bin' -ex 'disassemble working' | sed -e 's/0x000[0-9a-f]*//;s/<+[0-9]*>//' > working.asm
#RUN gdb -batch -ex 'file ./chibicc_bin' -ex 'disassemble broken' | sed -e 's/0x000[0-9a-f]*//;s/<+[0-9]*>//' > broken.asm
#RUN diff -u working.asm broken.asm || true
#RUN ./chibicc_bin
