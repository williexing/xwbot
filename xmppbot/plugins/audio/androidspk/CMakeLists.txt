SET(DEBUG "yes")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} -fno-rtti")

add_library(_xvspeaker vspeaker.cpp ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
target_link_libraries(_xvspeaker log OpenSLES)
endif(ANDROID)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( _xvspeaker xw++ )

INSTALL(TARGETS _xvspeaker DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

