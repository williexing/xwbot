IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

add_library(commands cmdapi.cpp ${PROJECT_SOURCE_DIR}/x_dll.c) 
if(ANDROID)
SET_TARGET_PROPERTIES(commands PROPERTIES LINK_FLAGS -llog)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORE_CFLAGS}")

#SET_TARGET_PROPERTIES(commands PROPERTIES LINK_FLAGS -rdynamic)
target_link_libraries(commands xw++ ev sessions)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_top_dir}/3dparty/libev )

include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

INSTALL(TARGETS commands DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

