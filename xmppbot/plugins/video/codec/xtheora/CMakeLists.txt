SET(DEBUG "yes")

IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)
#SET(BUILD_SHARED_LIBS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

SET(OGG_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/libogg-1.1.4/include")
SET(THEORA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/libtheora-1.1.1/include")

add_subdirectory( libogg-1.1.4 )
add_subdirectory( libtheora-1.1.1 )

add_library(_x_theora xtheora_plugin.c x_theora_api.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
target_link_libraries(_x_theora log)
endif(ANDROID)


include_directories( ${OGG_INCLUDES} )
include_directories( ${THEORA_INCLUDES} )
include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( _x_theora xw xwcrypt theoradec theoraenc )

INSTALL(TARGETS _x_theora DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

