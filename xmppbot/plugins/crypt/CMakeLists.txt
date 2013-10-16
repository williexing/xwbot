#cmake_minimum_required (VERSION 2.6) 
#project (xilib)

IF(CMAKE_COMPILER_IS_GNUCC)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall -O2 -DPIC -fPIC -I./")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
#SET(BUILD_SHARED_LIBS ON)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
INCLUDE_DIRECTORIES(${LIBRARY_OUTPUT_PATH}/include)

add_library(xwcrypt base64.c crc32.c crypt_mod.c md5.c sha1_rfc3174.c) 
target_link_libraries(xwcrypt xofactory)


IF(NOT IOS)

add_executable(md5test md5main.c md5.c) 
target_link_libraries(md5test xwcrypt)

IF(UNIX)
target_link_libraries(md5test m c)
ENDIF(UNIX)

ADD_TEST(md5_test_simple ${EXECUTABLE_OUTPUT_PATH}/md5test "--test")
ADD_TEST(md5_test_values ${EXECUTABLE_OUTPUT_PATH}/md5test "--t-values")

ENDIF(NOT IOS)

INSTALL(TARGETS xwcrypt DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()
