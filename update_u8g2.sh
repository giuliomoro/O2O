#!/bin/bash -e
# use this file to pull a new u8g2 into this project.

THIS_SCRIPT=`basename "$0"`
THIS_DIR=`dirname "$THIS_SCRIPT"`
U8G2=$1
[ -d "$U8G2" ] || { echo "Usage: \`$THIS_SCRIPT /path/to/u8g2\` " >&2; exit 1; }

cd $THIS_DIR
rm -rf u8g2
mkdir -p u8g2/csrc u8g2/cppsrc u8g2/common
cp $U8G2/csrc/*.{c,h} u8g2/csrc/
cp $U8G2/cppsrc/*.{cpp,h} u8g2/cppsrc/
cp $U8G2/sys/linux-i2c/common/*.{c,h} u8g2/common
cp $U8G2/LICENSE u8g2/
git checkout u8g2/U8g2LinuxI2C.h u8g2/csrc/u8x8_fonts.c u8g2/common/linux-i2c.c
git add u8g2
VERSION=$(git -C $U8G2 rev-parse HEAD)
git commit -m "====Updating u8g2 to $VERSION - first step: copy updated rnbo folder" -a
git apply -3 --whitespace=fix <(git diff 22b4f6f..abb1643 u8g2)
git commit -m "====Updating u8g2 to $VERSION - second step: apply patches" -a

echo "Updated u8g2 to `git -C $U8G2 rev-parse HEAD`"
