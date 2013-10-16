IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

############################################################
############################################################
############################################################
############################################################

IF(ANDROID)
set (CROSS_ARCH --host=arm-linux-gnueabi)
set (MYSYSROOT --sysroot=${ANDROID_NDK_SYSROOT})
set (CROSS_ARCH --host=arm-linux-gnueabi)
ENDIF(ANDROID)

INCLUDE_DIRECTORIES ( ${LIBRARY_OUTPUT_PATH}/include )
set(XILBC_C_FLAGS -fPIC ${CMAKE_C_FLAGS})

add_library(_x_ilbc xilbc.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
# add_dependencies(_x_ilbc ilbc_codec)

if(ANDROID)
target_link_libraries(_x_ilbc log)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

#include_directories( ${XILBC_INCLUDE_DIR} )
include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
link_directories( ${lib_contrib_dir}  )

#target_link_libraries( _x_ilbc m c xw ilbc_codec)
target_link_libraries( _x_ilbc m c xw )

INSTALL(TARGETS _x_ilbc DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

