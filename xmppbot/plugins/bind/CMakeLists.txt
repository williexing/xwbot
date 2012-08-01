set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall -O2 -I./")
#SET(BUILD_SHARED_LIBS ON)

add_library(bind bind.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
if(ANDROID)
SET_TARGET_PROPERTIES(bind PROPERTIES LINK_FLAGS -llog)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_FLAGS}")

#SET_TARGET_PROPERTIES(bind PROPERTIES LINK_FLAGS -rdynamic)

include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )

include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( bind xw )

INSTALL(TARGETS bind DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

