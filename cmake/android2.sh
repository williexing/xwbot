#!/bin/sh

export NDK=/WORK/cross/android-r5c-toolchain
SYSROOT=/WORK/cross/android-r5c-toolchain/sysroot

MIDDLE=bin
PREF=arm-linux-androideabi-

export CC="$NDK/$MIDDLE/${PREF}gcc"
export CXX="$NDK/$MIDDLE/${PREF}g++"
export LD="$NDK/$MIDDLE/${PREF}ld"
export CPP="$NDK/$MIDDLE/${PREF}cpp"
export AS="$NDK/$MIDDLE/${PREF}as"
export OBJCOPY="$NDK/$MIDDLE/${PREF}objcopy"
export OBJDUMP="$NDK/$MIDDLE/${PREF}objdump"
export STRIP="$NDK/$MIDDLE/${PREF}strip"
export RANLIB="$NDK/$MIDDLE/${PREF}ranlib"  
export CCLD="$NDK/$MIDDLE/${PREF}gcc"
export AR="$NDK/$MIDDLE/${PREF}ar"

export ANDROID_NDK=/DATA/android/android-ndk-r5/
export ANDROID_NDK_TOOLCHAIN_ROOT=/WORK/cross/android-r5c-toolchain

