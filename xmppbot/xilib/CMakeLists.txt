#cmake_minimum_required (VERSION 2.6) 
#project (xilib)

#set(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
#set(CORE_CFLAGS "-Wall -O2 -I./ ${DEBUG_CFLAGS}")
#SET(BUILD_SHARED_LIBS ON)

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )

link_directories( ${lib_contrib_dir}  )

add_library(xi xilib.c) 
target_link_libraries(xi ev)
SET(TARGET_VERSION_MAJOR 1)
SET(TARGET_VERSION_MINOR 2)
#SET_TARGET_PROPERTIES(xi PROPERTIES VERSION 0.0.1 SOVERSION 0.0)
#SET_TARGET_PROPERTIES(xi PROPERTIES COMPILE_FLAGS "${CORE_CFLAGS} ${ANSI_CFLAGS}")
SET_TARGET_PROPERTIES(xi PROPERTIES COMPILE_FLAGS "${CORE_CFLAGS}")

FILE(GLOB ifiles "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${ifiles} DESTINATION include/xilib)
INSTALL(TARGETS xi DESTINATION lib)

SET(CPACK_PACKAGE_VERSION "1.2.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "2")
SET(CPACK_PACKAGE_VERSION_PATCH "0") 
#SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "gobee") 
#SET(CPACK_GENERATOR "RPM")

INCLUDE(CPack)

ENABLE_TESTING()

#add_executable(tester tester.c) 
#target_link_libraries(tester xi)

#ADD_TEST(XilibTest1 ${CMAKE_CURRENT_BINARY_DIR}/tester Hello)

