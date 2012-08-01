#!/bin/sh

export NDK=/WORK/cross/android-r5c-toolchain
SYSROOT=/WORK/cross/android-r5c-toolchain/sysroot

MIDDLE=bin
PREF=arm-linux-androideabi-

export CC="$NDK/$MIDDLE/${PREF}gcc  --sysroot=$SYSROOT"
export CXX="$NDK/$MIDDLE/${PREF}g++  --sysroot=$SYSROOT"
export LD="$NDK/$MIDDLE/${PREF}ld  --sysroot=$SYSROOT"
export CPP="$NDK/$MIDDLE/${PREF}cpp  --sysroot=$SYSROOT"
export AS="$NDK/$MIDDLE/${PREF}as  --sysroot=$SYSROOT"
export OBJCOPY="$NDK/$MIDDLE/${PREF}objcopy  --sysroot=$SYSROOT"
export OBJDUMP="$NDK/$MIDDLE/${PREF}objdump  --sysroot=$SYSROOT"
export STRIP="$NDK/$MIDDLE/${PREF}strip  --sysroot=$SYSROOT"
export RANLIB="$NDK/$MIDDLE/${PREF}ranlib  --sysroot=$SYSROOT"
export CCLD="$NDK/$MIDDLE/${PREF}gcc  --sysroot=$SYSROOT"
export AR="$NDK/$MIDDLE/${PREF}ar  --sysroot=$SYSROOT"

export ANDROID_NDK=/DATA/android/android-ndk-r5/
export ANDROID_TOOLCHAIN_ROOT=/WORK/cross/android-r5c-toolchain

