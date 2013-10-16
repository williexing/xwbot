SET(DEBUG "yes")

IF (NOT WIN32)
set(CORE_CFLAGS "-Wall -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} -fno-rtti")

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
#include_directories( ${CURL_INCLUDE_DIR} )

link_directories( ${lib_contrib_dir}  )
link_directories( ${LIBRARY_OUTPUT_PATH}  )
link_directories( ${LIBRARY_OUTPUT_PATH}/lib  )

add_library(_curl STATIC IMPORTED)
set_property(TARGET _curl PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libcurl.a)

add_library(_xvsiri vsiri.cpp ${PROJECT_SOURCE_DIR}/x_dll.c)

if(ANDROID)
target_link_libraries(_xvsiri log)
endif(ANDROID)

target_link_libraries( _xvsiri xw++ xw ev _curl ssl crypto)

add_executable(vsiri_test vsiri_test.c)
target_link_libraries(vsiri_test _curl rtmp ssl crypto rt dl z c)

INSTALL(TARGETS _xvsiri DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

