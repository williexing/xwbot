cmake_minimum_required(VERSION 2.6)

set(PROJECT CvFramework)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")

FIND_PACKAGE(Threads)
FIND_PACKAGE(OpenCV REQUIRED )

SET(DEBUG ON)

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/TARGET.cmake)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/xmppbot)
include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${LIBRARY_OUTPUT_PATH}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/xmppbot/xwlib)

include_directories(${CMAKE_SOURCE_DIR}/xmppbot )
include_directories(${CMAKE_SOURCE_DIR}/xmppbot/3dparty/libev )

link_directories( ${LIBRARY_OUTPUT_PATH}  )

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_CFLAGS} -fno-rtti -fno-exceptions")
set(CORELIBS  m c stdc++ )

set(HEADERS featuretrack.h rootbus.h filter.h eyes.h comparator.h)
set(SOURCES featuretrack.cpp main.cpp
        filter.cpp eyes.cpp rootbus.cpp comparator.cpp)

add_executable(${PROJECT} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT} ${CORELIBS} ${LIBRARIES} ${OpenCV_LIBRARIES}
 ${EXTRA_LIBS} xw xw++ sessions ev dl)

#INSTALL(TARGETS ${PROJECT} DESTINATION bin)

SET(TARGET_VERSION_MAJOR 1)
SET(TARGET_VERSION_MINOR 2)
SET(CPACK_PACKAGE_VERSION "1.0.2")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "2")
SET(CPACK_STRIP_FILES xwclient)
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PROJECT}")

INCLUDE(CPack)

ENABLE_TESTING()

