FROM toxchat/c-toxcore:sources AS sources
FROM toxchat/compcert:latest

RUN apt-get update && \
 DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
 gdb \
 make \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /work
COPY --from=sources /src/ /work/

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

COPY other/make_single_file /work/other/
RUN other/make_single_file -core auto_tests/tox_new_test.c other/docker/compcert/libc.c other/docker/goblint/sodium.c > analysis.c
RUN ccomp -interp analysis.c \
		  -fstruct-passing -fno-unprototyped \
		  -D__COMPCERT__ \
		  -D_DEFAULT_SOURCE \
		  -DESP_PLATFORM \
		  -DDISABLE_VLA \
		  -DMIN_LOGGER_LEVEL=LOGGER_LEVEL_ERROR \
		  -Dinline=
COPY other/docker/compcert/Makefile /work/
RUN make "-j$(nproc)"
RUN ./send_message_test | grep 'tox clients connected'
