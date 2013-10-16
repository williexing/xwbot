cmake_minimum_required(VERSION 2.8)
project(Speex8Builder)
include(ExternalProject)

IF(ANDROID)

set (MYSYSROOT --sysroot=${ANDROID_NDK_SYSROOT})
set(CROSS_ARCH --host=arm-linux-gnueabi)
set(OPENSSL_CONF_CMD ./Configure  no-gost no-dso no-shared --prefix=${LIBRARY_OUTPUT_PATH} linux-generic32)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}\ -I${LIBRARY_OUTPUT_PATH}/include )
set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR})

ENDIF(ANDROID)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" AND LINUX)
if(NOT DEFINED TARGET_HOST)
SET(TARGET_HOST arm-linux-gnueabi)
endif(NOT DEFINED TARGET_HOST)
set(CROSS_ARCH --host=${TARGET_HOST})

SET(EXTRA --extra-cflags=\"-fPIC\ -fpic\ -DPIC\ -fvisibility=protected\")
#SET(EXTRACROSS CROSS=${TARGET_HOST})

SET(MYCC CC=${CMAKE_C_COMPILER}\ -fPIC\ -fpic\ -DPIC\ -fvisibility=protected)
endif()

set(GIT_EXECUTABLE /usr/bin/git)

ExternalProject_Add(_speex
		GIT_REPOSITORY   http://git.xiph.org/speex.git
		CONFIGURE_COMMAND ./autogen.sh && export CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} && ./configure 
			CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYAR} ${CROSS_ARCH} --disable-shared --disable-oggtest --prefix=${LIBRARY_OUTPUT_PATH}
		BUILD_COMMAND  make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYAR} 
            BUILD_IN_SOURCE 1
            LOG_DOWNLOAD 1
            LOG_UPDATE 1
            LOG_CONFIGURE 1
            LOG_BUILD 1
            LOG_TEST 1
            LOG_INSTALL 1
)

set_property(TARGET _speex PROPERTY DEPENDS _ogg)
add_dependencies(_speex _ogg)

#SET(SPEEX_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_speex/install/lib)
#SET(SPEEX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_speex/install/include)

