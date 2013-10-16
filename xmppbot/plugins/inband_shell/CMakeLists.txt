IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

set ( X_CORE_LIBS xw xofactory )

add_library( ibshell shell_media_profile.c
		shell_transp_profile.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
TARGET_LINK_LIBRARIES(ibshell log)
endif(ANDROID)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_CFLAGS}")

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_top_dir}/3dparty/libev )

include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( ibshell ${X_CORE_LIBS} sessions ) 

INSTALL(TARGETS ibshell DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

