IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)


#SET(BUILD_SHARED_LIBS ON)
set ( X_CORE_LIBS xw xofactory )

#add_library(jingle xep_0166.c jingle_transport.c 
#jingle_description.c speex.c ${PROJECT_SOURCE_DIR}/x_dll.c) 
add_library(jingle xep_0166.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
TARGET_LINK_LIBRARIES(jingle log)
endif(ANDROID)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} ${DEBUG_CFLAGS}")

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_top_dir}/3dparty/libev )

include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( jingle ${X_CORE_LIBS} sessions ) 

#if(UNIX)
#add_executable(jingle_tester jingle_tester.c) 
#endif(UNIX)

INSTALL(TARGETS jingle DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)

ENABLE_TESTING()

