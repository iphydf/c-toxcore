#!/bin/sh

ulimit -c 0

/work/dyibicc/out/ld/dyibicc -c main.c
if [ "$?" != 139 ]; then
  exit 1
fi
