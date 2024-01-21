################################################
# autotools-linux
FROM diffblue/cbmc:5.95.1

RUN apt-get update && \
 DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
 ca-certificates \
 creduce \
 libconfig-dev \
 libopus-dev \
 libsodium-dev \
 libvpx-dev \
 pkg-config \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

RUN groupadd -r -g 1000 builder \
 && useradd -m --no-log-init -r -g builder -u 1000 builder
USER builder

WORKDIR /home/builder

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Copy the sources and run the build.
COPY --chown=builder:builder other/make_single_file /home/builder/c-toxcore/other/
COPY --chown=builder:builder auto_tests/ /home/builder/c-toxcore/auto_tests/
COPY --chown=builder:builder testing/ /home/builder/c-toxcore/testing/
COPY --chown=builder:builder third_party/ /home/builder/c-toxcore/third_party/
COPY --chown=builder:builder toxcore/ /home/builder/c-toxcore/toxcore/
COPY --chown=builder:builder toxencryptsave/ /home/builder/c-toxcore/toxencryptsave/
WORKDIR /home/builder/c-toxcore
RUN other/make_single_file -core auto_tests/tox_new_test.c | gcc -DDISABLE_VLA -DDISABLE_STATIC_ASSERT -DSTATIC_ANALYSIS -E -xc - > analysis.i
RUN goto-cc -o analysis.goto analysis.i
RUN goto-analyzer --verify \
 --arch x86_64 \
 --assert-to-assume \
 --bounds-check \
 --enum-range-check \
 --pointer-primitive-check \
 --div-by-zero-check \
 --memory-cleanup-check \
 --memory-leak-check \
 --pointer-check \
 --signed-overflow-check \
 --undefined-shift-check \
 analysis.goto 2>&1 | grep -v 'SUCCESS' && false
#RUN cbmc \
# --unwind 8 \
# --assert-to-assume \
# --bounds-check \
# --div-by-zero-check \
# --drop-unused-functions \
# --malloc-may-fail \
# --memory-cleanup-check \
# --memory-leak-check \
# --pointer-check \
# --signed-overflow-check \
# --stop-on-fail \
# --undefined-shift-check \
# analysis.goto 2>&1 | grep -v 'SUCCESS'
#RUN other/make_single_file -core auto_tests/send_message_test.c auto_tests/auto_test_support.c | gcc -DDISABLE_VLA -DDISABLE_STATIC_ASSERT -E -xc - > analysis.i
#COPY other/docker/cpachecker/crash.sh /home/builder/c-toxcore/other/docker/cpachecker/
#RUN other/docker/cpachecker/crash.sh
#RUN creduce other/docker/cpachecker/crash.sh analysis.i
#RUN /cpachecker/scripts/cpa.sh -heap 8192m -skipRecursion -config /cpachecker/config/bmc.properties analysis.i && false
#RUN /cpachecker/scripts/cpa.sh -heap 8192m -skipRecursion -config /cpachecker/config/default.properties analysis.i && false
