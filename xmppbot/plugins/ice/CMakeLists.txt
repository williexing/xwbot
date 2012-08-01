#cmake_minimum_required (VERSION 2.6) 
#project (xilib)

IF(CMAKE_COMPILER_IS_GNUCC)
SET(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
SET(CORE_CFLAGS "-Wall -O2 -I./ ${DEBUG_CFLAGS}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
#SET(BUILD_SHARED_LIBS ON)

add_library(xstun x_stun.c x_stun_server.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

if(ANDROID)
TARGET_LINK_LIBRARIES(xstun log)
endif(ANDROID)

IF(WIN32)
target_link_libraries( xstun ws2_32 ${X_CORE_LIBS} xwcrypt xw ) 
ELSE(WIN32)
target_link_libraries( xstun ${X_CORE_LIBS} xwcrypt xw ) 
ENDIF(WIN32)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )

INSTALL(TARGETS xstun DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()
