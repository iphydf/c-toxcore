FROM alpine:3.19.0

RUN apk add \
 clang \
 git \
 libsodium-dev \
 libvpx-dev \
 linux-headers \
 opus-dev \
 make \
 perl

WORKDIR /work/v
RUN git clone --depth=1 https://github.com/vlang/v /work/v
RUN make
RUN /work/v/v symlink

WORKDIR /work/c2v
RUN git clone --depth=1 https://github.com/vlang/c2v /work/c2v
RUN v .

COPY auto_tests/send_message_test.c \
 auto_tests/auto_test_support.[ch] \
 auto_tests/check_compat.h \
 /work/c-toxcore/auto_tests/
COPY testing/misc_tools.[ch] /work/c-toxcore/testing/
COPY toxav/ /work/c-toxcore/toxav/
COPY toxcore/ /work/c-toxcore/toxcore/
COPY toxencryptsave/ /work/c-toxcore/toxencryptsave/
COPY third_party/cmp/cmp.h /work/c-toxcore/third_party/cmp/

WORKDIR /work/c-toxcore
COPY other/make_single_file /work/c-toxcore/other/
RUN \
 other/make_single_file -core \
   auto_tests/auto_test_support.c \
   auto_tests/send_message_test.c \
   testing/misc_tools.c > input.c

WORKDIR /work/c-toxcore
COPY other/docker/c2v/c2v.toml /work/c-toxcore/
RUN /work/c2v/c2v input.c
#RUN v -translated input.v

#RUN ls -l && 0

#RUN tcc \
# -Dinline=static \
# -o send_message_test \
# -Wall -Werror \
# -bench -g \
# auto_tests/auto_test_support.c \
# auto_tests/send_message_test.c \
# testing/misc_tools.c \
# toxav/*.c \
# toxcore/*.c \
# toxcore/*/*.c \
# toxencryptsave/*.c \
# third_party/cmp/*.c \
# $(pkg-config --cflags --libs libsodium opus vpx) \
# && ./send_message_test | grep 'tox clients connected'
#
#COPY other/make_single_file /work/other/
#RUN \
# other/make_single_file -core \
#   auto_tests/auto_test_support.c \
#   auto_tests/send_message_test.c \
#   testing/misc_tools.c | \
# tcc - \
#   -o send_message_test \
#   -Wall -Werror \
#   -bench -g \
#   $(pkg-config --cflags --libs libsodium) \
# && ./send_message_test | grep 'tox clients connected'
#
#RUN \
# other/make_single_file \
#   auto_tests/auto_test_support.c \
#   auto_tests/toxav_basic_test.c \
#   testing/misc_tools.c | \
# tcc - \
#   -o toxav_basic_test \
#   -Wall -Werror \
#   -bench -g \
#   $(pkg-config --cflags --libs libsodium opus vpx) \
# && ./toxav_basic_test | grep 'Test successful'
