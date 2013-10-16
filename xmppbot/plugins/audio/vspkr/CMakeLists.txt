SET(DEBUG "yes")

IF (NOT WIN32)
set(CORE_CFLAGS "-Wall -g -gdwarf-2 -O2 -DPIC -fPIC -I./")
ENDIF (NOT WIN32)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CORE_CFLAGS} -fno-rtti")

if(DEFINED ALSA_INCLUDE_DIR)
else(DEFINED ALSA_INCLUDE_DIR)
find_package( ALSA )
endif(DEFINED ALSA_INCLUDE_DIR)

add_library(_xvspeaker vspeaker.cpp ${PROJECT_SOURCE_DIR}/x_dll.c) 

if(ANDROID)
target_link_libraries(_xvspeaker log)
endif(ANDROID)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
include_directories( ${ALSA_INCLUDE_DIR} )

link_directories( ${lib_contrib_dir}  )

target_link_libraries( _xvspeaker xw++ xw ev dl )
# target_link_libraries( _xvspeaker xw xofactory ${ALSA_LIBRARIES}  )

INSTALL(TARGETS _xvspeaker DESTINATION ${PLUGINS_DIR})

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

