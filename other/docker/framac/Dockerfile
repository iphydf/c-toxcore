FROM toxchat/frama-c:latest

RUN eval "$(opam env)" \
 && why3 config detect

WORKDIR /work
COPY auto_tests/ /work/auto_tests/
COPY testing/ /work/testing/
COPY toxav/ /work/toxav/
COPY toxcore/ /work/toxcore/
COPY toxencryptsave/ /work/toxencryptsave/
COPY third_party/ /work/third_party/

RUN mkdir include \
 && ln -s /usr/include/sodium include/sodium \
 && ln -s /usr/include/sodium.h include/sodium.h

COPY other/docker/framac/proofs.c /work/other/docker/framac/
RUN eval "$(opam env)" \
 && frama-c -cpp-extra-args="-Iinclude" -eva -reduc -wp -wp-warn-key pedantic-assigns=once -kernel-warn-key annot:missing-spec=once other/docker/framac/proofs.c toxcore/*.c toxcore/*/*.c
