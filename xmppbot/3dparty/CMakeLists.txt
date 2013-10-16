add_subdirectory( libev )

IF(ANDROID)

set(MYSYSROOT --sysroot=${ANDROID_NDK_SYSROOT})
set(CROSS_ARCH --host=arm-linux-gnueabi)
set(OPENSSL_CONF_CMD ./Configure  no-gost no-dso --prefix=${LIBRARY_OUTPUT_PATH} linux-generic32)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}\ -I${LIBRARY_OUTPUT_PATH}/include )
set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR})
set(MYARRU AR=${CMAKE_AR}\ -ru)

ELSE(ANDROID)

IF(APPLE)

IF(IOS)

IF (IOS_PLATFORM STREQUAL "OS")
set(MYIOSARCH -arch\ armv7\ -fPIC)
set (CROSS_ARCH --host=arm-apple-darwin)
ELSE()
set(MYIOSARCH -arch\ i386)
set (CROSS_ARCH --host=i386-apple-darwin)
ENDIF()

set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR}\ -ru)

set (MYSYSROOT -isysroot\ ${CMAKE_IOS_SDK_ROOT}\ ${MYIOSARCH})
set(OPENSSL_CONF_CMD ./Configure no-gost no-dso no-shared --prefix=${LIBRARY_OUTPUT_PATH} BSD-generic32)
#set (OPENSSL_INSTALL_CMD cp $$i $(INSTALL_PREFIX)$(INSTALLTOP)/include/openssl/$$i; \
#        chmod 644 ${LIBRARY_OUTPUT_PATH}/include/openssl/$$i ); \)

ELSE(IOS)

set(OPENSSL_CONF_CMD ./Configure  no-gost no-dso no-shared --prefix=${LIBRARY_OUTPUT_PATH} darwin64-x86_64-cc)
set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR})
set(MYARRU AR=${CMAKE_AR})
set(EXTRA_THEORA_FLAGS --disable-asm)

ENDIF(IOS)

ELSE(APPLE)
set(OPENSSL_CONF_CMD ./Configure  shared --prefix=${LIBRARY_OUTPUT_PATH} linux-generic32)
ENDIF(APPLE)

ENDIF(ANDROID)


if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" AND LINUX)
if(NOT DEFINED TARGET_HOST)
set(TARGET_HOST arm-linux-gnueabi)
endif(NOT DEFINED TARGET_HOST)

set(CROSS_ARCH --host=${TARGET_HOST})

set(OPENSSL_CONF_CMD ./Configure no-asm no-gost --prefix=${LIBRARY_OUTPUT_PATH} linux-generic32:${CMAKE_C_COMPILER}:-fPIC\ -fpic\ -fvisibility=protected)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}\ -I${LIBRARY_OUTPUT_PATH}/include )
set(MYRANLIB RANLIB=${CMAKE_RANLIB})
set(MYAR AR=${CMAKE_AR})
set(MYARRU AR=${CMAKE_AR}\ -ru)
endif()


#CFLAGS="${CMAKE_C_FLAGS}"

set(MY_C_FLAGS "-DDEBUG -fPIC ${CMAKE_C_FLAGS}")

#IF(NOT LINUX)
ExternalProject_Add(openssl 
        URL ${TARBALLS_DIR}/openssl-1.0.0c.tar.gz
        CONFIGURE_COMMAND ${OPENSSL_CONF_CMD} 
	    BUILD_COMMAND make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYARRU} build_all
        INSTALL_COMMAND make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYARRU} install
            BUILD_IN_SOURCE 1
            LOG_DOWNLOAD 1
            LOG_UPDATE 1
            LOG_CONFIGURE 1
            LOG_BUILD 1
            LOG_TEST 1
            LOG_INSTALL 1
)
#ENDIF(NOT LINUX)

#IF(LINUX)
ExternalProject_Add(curl
            URL ${TARBALLS_DIR}/curl-7.28.1.tar.gz
            CONFIGURE_COMMAND ./configure CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} CFLAGS=${CMAKE_C_FLAGS} ${MYRANLIB} ${MYAR} ${CROSS_ARCH} --disable-shared --enable-debug --enable-http --prefix=${LIBRARY_OUTPUT_PATH} --with-ssl=${LIBRARY_OUTPUT_PATH} --without-libidn --disable-ldap --disable-ldaps --without-ssh2
            BUILD_COMMAND make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT}
            INSTALL_COMMAND make install
            BUILD_IN_SOURCE 1
            LOG_DOWNLOAD 1
            LOG_UPDATE 1
            LOG_CONFIGURE 1
            LOG_BUILD 1
            LOG_TEST 1
            LOG_INSTALL 1
)
#ENDIF(LINUX)

IF(APPLE)
set(OGG_CONF_CMD --enable-static --disable-shared) 
ELSE(APPLE)
set(OGG_CONF_CMD --disable-shared --with-pic)
ENDIF(APPLE)

ExternalProject_Add(_ogg
        URL ${TARBALLS_DIR}/libogg-1.3.0.tar.gz
        CONFIGURE_COMMAND ./configure CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} CFLAGS=${CMAKE_C_FLAGS} ${MYRANLIB} ${MYAR} ${CROSS_ARCH} ${OGG_CONF_CMD} --prefix=${LIBRARY_OUTPUT_PATH}
        BUILD_COMMAND  make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYAR} CFLAGS=${CMAKE_C_FLAGS}
        INSTALL_COMMAND make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYAR} install
        BUILD_IN_SOURCE 1
        LOG_DOWNLOAD 1
        LOG_UPDATE 1
        LOG_CONFIGURE 1
        LOG_BUILD 1
        LOG_TEST 1
        LOG_INSTALL 1
)

IF(THEORA_1_1)
ELSE(THEORA_1_1)
ExternalProject_Add(theoralib
        URL ${TARBALLS_DIR}/libtheora-1.2.0alpha1.tar.gz
        CONFIGURE_COMMAND ./configure CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} CFLAGS=${CMAKE_C_FLAGS} ${MYRANLIB} ${MYAR} ${CROSS_ARCH} ${CROSS_ARCH} --with-ogg-includes=${LIBRARY_OUTPUT_PATH}/include/ --with-ogg-libraries=${LIBRARY_OUTPUT_PATH}/lib/ --disable-shared --enable-static --with-pic --disable-examples --disable-vorbistest --disable-oggtest ${EXTRA_THEORA_FLAGS} --prefix=${LIBRARY_OUTPUT_PATH}
        BUILD_COMMAND  make CFLAGS=${CMAKE_C_FLAGS} LDFLAGS=-L${LIBRARY_OUTPUT_PATH}/lib/ ${MYRANLIB} ${MYAR} CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT}
        INSTALL_COMMAND make CC=${CMAKE_C_COMPILER}\ ${MYSYSROOT} LD=${CMAKE_LINKER}\ ${MYSYSROOT} ${MYRANLIB} ${MYAR} install
        BUILD_IN_SOURCE 1
        LOG_DOWNLOAD 1
        LOG_UPDATE 1
        LOG_CONFIGURE 1
        LOG_BUILD 1
        LOG_TEST 1
        LOG_INSTALL 1
)
ENDIF(THEORA_1_1)

add_dependencies(curl openssl)
add_dependencies(_ogg openssl)
add_dependencies(theoralib _ogg)

INCLUDE(CPack)

ENABLE_TESTING()

