#cmake_minimum_required (VERSION 2.6) 
#project (Evtdomains)

IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)

#SET(BUILD_SHARED_LIBS ON)

add_library(evtdomain evtdomain.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_CFLAGS}")

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_top_dir}/3dparty/libev )

include_directories( ${include_contrib_dir} )
link_directories( ${lib_contrib_dir}  )

target_link_libraries( evtdomain xw ev )

IF(ANDROID)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_CFLAGS} -DANDROID")
ENDIF(ANDROID)

INSTALL(TARGETS evtdomain DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

#add_executable(evtdtest evtdmain.c) 
#target_link_libraries(evtdtest m c)
#ADD_TEST(md5_test_simple ${EXECUTABLE_OUTPUT_PATH}/evtdtest "--test")
