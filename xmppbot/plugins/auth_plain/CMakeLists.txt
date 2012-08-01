set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
#SET(BUILD_SHARED_LIBS ON)

add_library(auth_plain auth_plain.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
if(ANDROID)
TARGET_LINK_LIBRARIES(auth_plain log)
endif(ANDROID)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

include_directories( ${include_top_dir} )
include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

target_link_libraries( auth_plain xw xwcrypt )

link_directories( ${lib_contrib_dir}  )

INSTALL(TARGETS auth_plain DESTINATION ${PLUGINS_DIR} )

INCLUDE(CPack)

ENABLE_TESTING()
