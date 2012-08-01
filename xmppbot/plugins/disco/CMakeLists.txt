IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

add_library(discovery xmlns_disco.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
if(ANDROID)
SET_TARGET_PROPERTIES(discovery PROPERTIES LINK_FLAGS -llog)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

#SET_TARGET_PROPERTIES(discovery PROPERTIES LINK_FLAGS -rdynamic)

include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )

include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( discovery xw )

INSTALL(TARGETS discovery DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()
