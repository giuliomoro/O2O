#!/bin/bash

PROJECT_DIR=$(dirname "$0") #note: this is _not_ the variable of the same name used `make` below: that's the Makefile's own
cd $PROJECT_DIR

[ -z "$BELA_HOME" ] && BELA_HOME=$(realpath "`pwd`/../../")
[ -z "$PROJECT_NAME" ] && PROJECT_NAME=$(basename "`pwd`")

set -e
make -C $BELA_HOME CFLAGS=-I\$\(PROJECT_DIR\)/u8g2/csrc CPPFLAGS=-I\$\(PROJECT_DIR\)/u8g2/csrc PROJECT="$PROJECT_NAME" $([ 0 -eq "$#" ] && echo run || echo $@)
