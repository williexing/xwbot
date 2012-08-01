#!/bin/sh

ANDTOOLCHAIN=/WORK/cross/android-r5c-toolchain/
ANDTOOLCHAINFILE=cmake/android.toolchain.cmake

cmake -DCMAKE_TOOLCHAIN_FILE=${ANDTOOLCHAINFILE} ./

