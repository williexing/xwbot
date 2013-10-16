SET(DEBUG "yes")

IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

add_library(_xqvhid xhid_plugin.c xqvhid.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
target_link_libraries(_xqvhid log)
endif(ANDROID)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( _xqvhid xw xofactory _xqvfb )

INSTALL(TARGETS _xqvhid DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

