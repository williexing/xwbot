cmake_minimum_required(VERSION 2.8)
project(Vp8Builder)
include(ExternalProject)

set(GIT_EXECUTABLE /usr/bin/git)

IF(ANDROID)
SET(VP8_TARGET --target=armv7-android-gcc)
ELSE(ANDROID)
SET(VP8_TARGET --target=generic-gnu)
ENDIF(ANDROID)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86" AND LINUX)
SET(VP8_TARGET --target=x86-linux-gcc)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" AND LINUX)
SET(VP8_TARGET --target=x86_64-linux-gcc)
SET(EXTRA --extra-cflags=-fPIC\\\ -fpic\\\ -DPIC\\\ -fvisibility=protected --enable-runtime-cpu-detect)
#SET(VP8_TARGET --target=generic-gnu)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" AND APPLE)
SET(VP8_TARGET --target=x86_64-darwin-gcc)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" AND LINUX)
SET(VP8_TARGET --target=armv7-linux-gcc)
SET(EXTRA --extra-cflags='-fPIC\ -fpic\ -DPIC\ -mfloat-abi=hard\ -fvisibility=protected')
if(NOT DEFINED TARGET_HOST)
set(TARGET_HOST arm-linux-gnueabi-)
endif(NOT DEFINED TARGET_HOST)

set(CROSS_ARCH --host=${TARGET_HOST})
set(CROSS ${TARGET_HOST}-)

SET(VPX_CONF_CMD sh -c "export CROSS=${CROSS} && ./configure\ ${VP8_TARGET}\ ${EXTRA}\ --disable-unit-tests\ --enable-static\ --disable-shared\ --disable-vp9\ --enable-vp8\ --disable-examples\ --enable-pic\ --enable-multi-res-encoding")
SET(MYCC CC=${CMAKE_C_COMPILER}\ -fPIC\ -fpic\ -DPIC\ -fvisibility=protected)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" AND ANDROID)
SET(VP8_TARGET --target=armv7-android-gcc --sdk-path=${ANDROID_NDK})
endif()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips" AND LINUX)
SET(VP8_TARGET --target=mips32-linux-gcc)
endif()

IF(IOS)
IF(IOS_PLATFORM STREQUAL "OS")
SET(VP8_TARGET --target=armv7-darwin-gcc)
SET(EXTRA --libc=${CMAKE_IOS_SDK_ROOT})
#SET(MYCC CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT})
ELSE()
SET(VP8_TARGET --target=x86-darwin9-gcc)
#set(MYIOSARCH -arch\ i386)
set(MYSYSROOT -isysroot\ ${CMAKE_IOS_SDK_ROOT}\ ${MYIOSARCH})
set(MYCFLAGS CFLAGS=${CMAKE_C_FLAGS}\ -isysroot\ ${CMAKE_IOS_SDK_ROOT}\ ${MYIOSARCH})
SET(MYCC CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT})
ENDIF()
ENDIF(IOS)

INCLUDE_DIRECTORIES ( ${LIBRARY_OUTPUT_PATH}/include )
set(VPX_C_FLAGS -fPIC ${CMAKE_C_FLAGS})

if(NOT DEFINED VPX_CONF_CMD)
set(VPX_CONF_CMD ./configure ${VP8_TARGET} ${EXTRA} --disable-unit-tests --enable-static --disable-shared --disable-vp9 --enable-vp8 --disable-examples --enable-pic --enable-multi-res-encoding)
endif(NOT DEFINED VPX_CONF_CMD)

ExternalProject_Add(vp8
    URL  ${TARBALLS_DIR}/libvpx-01182013.tar.gz
    CONFIGURE_COMMAND ${VPX_CONF_CMD} 
    BUILD_COMMAND   make ${MYCC}
    INSTALL_COMMAND  make ${MYCC} DIST_DIR=${LIBRARY_OUTPUT_PATH} install
    BUILD_IN_SOURCE 1
    LOG_DOWNLOAD 1
    LOG_UPDATE 1
    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_TEST 1
    LOG_INSTALL 1
)

SET(VPX_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH}/lib)
SET(VPX_INCLUDE_DIR ${LIBRARY_OUTPUT_PATH}/include)
