IF(CMAKE_COMPILER_IS_GNUCC)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall -O2 -DPIC -fPIC -I./")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
#SET(BUILD_SHARED_LIBS ON)

add_library(sessions sessions.c roster_pure_api.c 
	ice_ctl.c session_api.c
	event_domain.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
TARGET_LINK_LIBRARIES(sessions log)
endif(ANDROID)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_CFLAGS}")

#SET_TARGET_PROPERTIES(sessions PROPERTIES LINK_FLAGS -rdynamic)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_top_dir}/3dparty/libev )

include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( sessions xstun ) 

IF(WIN32)
target_link_libraries( sessions xw ev ws2_32 xstun ${CMAKE_THREAD_LIBS_INIT}) 
ELSE(WIN32)
target_link_libraries( sessions xw ev ${CMAKE_THREAD_LIBS_INIT})
ENDIF(WIN32)

IF(ANDROID)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID")
ENDIF(ANDROID)

IF(NOT IOS)
add_executable(session_tester session_tester.c) 
target_link_libraries( session_tester sessions ) 
ENDIF(NOT IOS)

INSTALL(TARGETS sessions DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

