FROM alpine:3.19.0

RUN ["apk", "add", "bash", "clang-extra-tools"]

WORKDIR /work
ENTRYPOINT ["other/astyle/format-source"]
