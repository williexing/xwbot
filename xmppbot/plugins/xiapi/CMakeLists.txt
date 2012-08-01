set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall -O2 -DPIC -fPIC -I./")
#SET(BUILD_SHARED_LIBS ON)

add_library(xiapi xiapi.c) 
if(ANDROID)
SET_TARGET_PROPERTIES(xiapi PROPERTIES LINK_FLAGS -llog)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CORE_CFLAGS}")

#SET_TARGET_PROPERTIES(xiapi PROPERTIES LINK_FLAGS -rdynamic)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${libev_include_dir} )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( xiapi xw ev )

INSTALL(TARGETS xiapi DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

