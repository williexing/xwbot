IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

#add_library(_x_speex xspeex.c xspeex_plugin.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(_x_speex xspeex.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
target_link_libraries(_x_speex log)
endif(ANDROID)

find_package(ALSA)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

include_directories( ${ALSA_INCLUDE_DIR} )
include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( _x_speex xw speex ${ALSA_LIBRARY} )

INSTALL(TARGETS _x_speex DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

