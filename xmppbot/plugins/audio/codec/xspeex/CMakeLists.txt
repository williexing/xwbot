IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)

IF(ANDROID)
set(CROSS_ARCH --host=arm-none-linux-gnueabi)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}\ -I${ANDROID_NDK_SYSROOT}/usr/include/\ -DEXPORT=\ -Wall\ -g\ -gdwarf-2\ -O2\ -DPIC\ -fPIC\ -I./\ -DFIXED_POINT\ -DUSE_KISS_FFT\ -UHAVE_CONFIG_H)
set(LINKER_FLAGS -L${ANDROID_NDK_SYSROOT}/usr/lib/)

set(MYSYSROOT --sysroot=${ANDROID_NDK_SYSROOT})
set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR})

ENDIF(ANDROID)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" AND LINUX)
if(NOT DEFINED TARGET_HOST)
set(TARGET_HOST arm-linux-gnueabi)
endif(NOT DEFINED TARGET_HOST)

set(CROSS_ARCH --host=${TARGET_HOST})

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}\ -I${LIBRARY_OUTPUT_PATH}/include/\ -DEXPORT=\ -Wall\ -g\ -gdwarf-2\ -O2\ -DPIC\ -fPIC\ -I./\ -DFIXED_POINT\ -DUSE_KISS_FFT\ -UHAVE_CONFIG_H)
set(LINKER_FLAGS -L${LIBRARY_OUTPUT_PATH}/lib/)
#set(MYSYSROOT --sysroot=${ANDROID_NDK_SYSROOT})
set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR})
endif()

IF(IOS)

IF (IOS_PLATFORM STREQUAL "OS")
set(MYIOSARCH -arch\ armv7)
set (CROSS_ARCH --host=arm-apple-darwin)
ELSE(IOS_PLATFORM STREQUAL "OS")
set(MYIOSARCH -arch\ i386)
set (CROSS_ARCH --host=i386-apple-darwin)
ENDIF(IOS_PLATFORM STREQUAL "OS")

set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR}\ -ru)
set (MYSYSROOT -isysroot\ ${CMAKE_IOS_SDK_ROOT}\ ${MYIOSARCH})

ENDIF(IOS)


INCLUDE_DIRECTORIES ( ${LIBRARY_OUTPUT_PATH}/include )
set(SPEEX_C_FLAGS ${CMAKE_C_FLAGS})

IF(IOS)
ExternalProject_Add(_speex
        PREFIX ${LIBRARY_OUTPUT_PATH}/speex_build
        
        URL  ${TARBALLS_DIR}/speex-1.2rc1.tar.gz

        CONFIGURE_COMMAND ${LIBRARY_OUTPUT_PATH}/speex_build/src/_speex/configure --disable-shared --disable-sse --disable-oggtest --prefix=${LIBRARY_OUTPUT_PATH}  --with-pic

        BUILD_COMMAND  make LIBTOOL=${LIBRARY_OUTPUT_PATH}/speex_build/src/_speex/libtool\ --tag=CC CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${LINKER_FLAGS}\ ${MYSYSROOT}
        
        INSTALL_COMMAND make LIBTOOL=${LIBRARY_OUTPUT_PATH}/speex_build/src/_speex/libtool\ --tag=CC CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} install
        
        BUILD_IN_SOURCE 1
        LOG_DOWNLOAD 1
        LOG_UPDATE 1
        LOG_CONFIGURE 1
        LOG_BUILD 1
#        LOG_TEST 1
        LOG_INSTALL 1
)
ELSE(IOS)
ExternalProject_Add(_speex
        PREFIX ${LIBRARY_OUTPUT_PATH}/speex_build
        URL  ${TARBALLS_DIR}/speex-1.2rc1.tar.gz

        CONFIGURE_COMMAND ${LIBRARY_OUTPUT_PATH}/speex_build/src/_speex/configure CFLAGS=${CMAKE_C_FLAGS} ${MYRANLIB} ${MYAR} CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} ${CROSS_ARCH} --disable-shared --disable-sse --enable-fixed-point --disable-oggtest --disable-warnings --prefix=${LIBRARY_OUTPUT_PATH}  --with-pic

        BUILD_COMMAND  make LIBTOOL=${LIBRARY_OUTPUT_PATH}/speex_build/src/_speex/libtool\ --tag=CC ${MYRANLIB} ${MYAR} LDFLAGS=${LINKER_FLAGS} CFLAGS=-UHAVE_SYS_SOUNDCARD_H\ ${CMAKE_C_FLAGS}\ -DDISABLE_WARNINGS\ -DDISABLE_NOTIFICATIONS CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT}

        INSTALL_COMMAND make LIBTOOL=${LIBRARY_OUTPUT_PATH}/speex_build/src/_speex/libtool\ --tag=CC ${MYRANLIB} ${MYAR} LDFLAGS=${LINKER_FLAGS} CFLAGS=-UHAVE_SYS_SOUNDCARD_H\ ${CMAKE_C_FLAGS} CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} install

            BUILD_IN_SOURCE 1
            LOG_DOWNLOAD 1
            LOG_UPDATE 1
            LOG_CONFIGURE 1
            LOG_BUILD 1
            LOG_TEST 1
            LOG_INSTALL 1
)
ENDIF(IOS)

add_library(speex STATIC IMPORTED)
set_property(TARGET speex PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libspeex.a)

add_library(speexdsp STATIC IMPORTED)
set_property(TARGET speexdsp PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libspeexdsp.a)

set_property(TARGET _speex PROPERTY DEPENDS speex)
set_property(TARGET _speex PROPERTY DEPENDS speexdsp)
set_property(TARGET _speex PROPERTY DEPENDS _ogg)
add_dependencies(_speex _ogg)

add_library(_x_speex xspeex.c ${PROJECT_SOURCE_DIR}/x_dll.c)
#add_dependencies(_x_speex _speex _ogg)

if(ANDROID)
target_link_libraries(_x_speex log)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

#include_directories( ${SPEEX_INCLUDE_DIR} )
include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )
#link_directories( ${CMAKE_CURRENT_BINARY_DIR}/_speex/install/lib  )

target_link_libraries( _x_speex m c xw speex speexdsp)
#add_dependencies(_x_speex _speex)

IF(LINUX)
target_link_libraries( _x_speex ${ALSA_LIBRARY} )
ENDIF(LINUX)

INSTALL(TARGETS _x_speex DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

