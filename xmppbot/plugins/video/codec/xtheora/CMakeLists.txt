SET(DEBUG "yes")

IF (NOT WIN32)
set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
set(CORE_CFLAGS "-Wall  -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)

#IF(APPLE)
#SET(BUILD_SHARED_LIBS ON)
#SET(OGG_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/libogg-1.1.4/include")
#add_subdirectory( libogg-1.1.4 )
#include_directories( ${OGG_INCLUDES} )
#ENDIF(APPLE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")

SET(THEORA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/libtheora-1.1.1/include")

IF(THEORA_1_1)
add_subdirectory( libtheora-1.1.1 )
ENDIF(THEORA_1_1)

add_library(_x_theora xtheora_plugin.c x_theora_api.c ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
target_link_libraries(_x_theora log)
endif(ANDROID)


include_directories( ${THEORA_INCLUDES} )
include_directories( ${libev_include_dir} )
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
INCLUDE_DIRECTORIES ( ${LIBRARY_OUTPUT_PATH}/include )

link_directories( ${lib_contrib_dir}  )
link_directories( ${LIBRARY_OUTPUT_PATH}/ )
link_directories( ${LIBRARY_OUTPUT_PATH}/lib )

#IF(THEORA_1_1)
add_library(_theoradec STATIC IMPORTED)
set_property(TARGET _theoradec PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libtheoradec.a)
add_library(_theoraenc STATIC IMPORTED)
set_property(TARGET _theoraenc PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libtheoraenc.a)
#ENDIF(THEORA_1_1)

add_dependencies(_x_theora xw xwcrypt _theoradec _theoraenc)
set_property(TARGET _x_theora PROPERTY DEPENDS _theoradec)
set_property(TARGET _x_theora PROPERTY DEPENDS _theoraenc)

target_link_libraries(_x_theora _theoradec _theoraenc ogg)
target_link_libraries(_x_theora xw xwcrypt)

INSTALL(TARGETS _x_theora DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

