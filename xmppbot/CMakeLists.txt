cmake_minimum_required (VERSION 2.6) 
project (xmppbot)

#set(GIT_EXECUTABLE /usr/bin/git)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")
SET(BUILD_DOCUMENTATION ON)

SET(DEBUG ON)

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/TARGET.cmake)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_CFLAGS} -fno-rtti -fno-exceptions")

IF(IOS)
SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIOS")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIOS")
ELSE(IOS)
SET(BUILD_SHARED_LIBS ON)
ENDIF(IOS)

IF(ANDROID)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID")
ENDIF(ANDROID)

STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})

SET( TARBALLS_DIR ${PROJECT_SOURCE_DIR}/3dparty/tarballs )
SET( libev_include_dir ${PROJECT_SOURCE_DIR}/3dparty/libev )
set( include_top_dir ${PROJECT_SOURCE_DIR} )
set( include_contrib_dir ${LIBRARY_OUTPUT_DIR}/include )
set( lib_contrib_dir ${LIBRARY_OUTPUT_PATH} )
set( lib_contrib_dir2 ${LIBRARY_OUTPUT_PATH}/lib )

INCLUDE(${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

set(PLUGINS_DIR ${CMAKE_INSTALL_PREFIX}/lib/xmppbot/plugins)

INCLUDE( CheckIncludeFiles )
INCLUDE( ExternalProject )
INCLUDE( Documentation )

FIND_PACKAGE( Threads )
set(CORELIBS ev ${CMAKE_THREAD_LIBS_INIT})

IF(DOXYGEN_FOUND)
    SET(XMPPBOT_DOC_DIR ${LIBRARY_OUTPUT_PATH}/Documentation)
    SET(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/xwlib/x_obj.h\ ${CMAKE_CURRENT_SOURCE_DIR}/xwlib/x_obj.c)
	CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    ADD_CUSTOM_TARGET(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
ELSE(DOXYGEN_FOUND)
    MESSAGE(STATUS "WARNING: Doxygen not found - Reference manual will not be created")
ENDIF(DOXYGEN_FOUND)

INCLUDE_DIRECTORIES(${LIBRARY_OUTPUT_PATH}/include)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${LIBRARY_OUTPUT_PATH}/include/xbot_config.h)

add_subdirectory( 3dparty )

IF(NOT LINUX)
add_library(ssl STATIC IMPORTED)
set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libssl.a)

add_library(crypto STATIC IMPORTED)
set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libcrypto.a)
ENDIF(NOT LINUX)

add_library(ogg STATIC IMPORTED)
set_property(TARGET ogg PROPERTY IMPORTED_LOCATION ${LIBRARY_OUTPUT_PATH}/lib/libogg.a)




add_subdirectory( xwlib )
add_subdirectory( xwlib++ )
#IF(NOT APPLE)
#add_subdirectory( xwlib-objc )
#ENDIF(NOT APPLE)
#add_subdirectory( xilib )
add_subdirectory( plugins )
add_subdirectory( examples )
IF (ANDROID AND NOT LINUX )
add_subdirectory( jni )
ENDIF (ANDROID AND NOT LINUX )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

link_directories( ${lib_contrib_dir}
 ${PROJECT_SOURCE_DIR}/3dparty/libev ${LIBRARY_OUTPUT_PATH} )

add_library( xbus x_bus.c ${PROJECT_SOURCE_DIR}/x_dll.c)
add_library( xparser x_parser.c ${PROJECT_SOURCE_DIR}/x_dll.c)

IF(ANDROID)
add_library(xmppbot main.c ${PROJECT_SOURCE_DIR}/x_dll.c)
ELSE(ANDROID)
IF(IOS)
add_library(xmppbot main.c gobee_init.c ${PROJECT_SOURCE_DIR}/x_dll.c)
ELSE(IOS)
add_executable(xmppbot main.c)
ENDIF(IOS)
ENDIF(ANDROID)

#SET_TARGET_PROPERTIES(xbus PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")
SET_TARGET_PROPERTIES(xparser PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")
#SET_TARGET_PROPERTIES(xmppbot PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS}")

add_dependencies(xbus ev xparser xw)
add_dependencies(xw ev xofactory)
add_dependencies(xw++ xw)

IF(NOT BUILD_SHARED_LIBS)
add_dependencies(sessions xw)
add_dependencies(core xw xbus sessions discovery bind auth_plain)
add_dependencies(jingle sessions)
add_dependencies(ibshell sessions)
add_dependencies(xwcrypt xofactory)
add_dependencies(xmppbot xwcrypt core ibshell jingle evtdomain)
ENDIF(NOT BUILD_SHARED_LIBS)

target_link_libraries( xbus ${GLOBAL_COMMON_LIBS}  ev xw xofactory xparser)
target_link_libraries( xparser ${GLOBAL_COMMON_LIBS}  xw)

target_link_libraries(xmppbot ${GLOBAL_COMMON_LIBS} ${CORELIBS} xw xofactory sessions )

IF(UNIX)
IF(IOS)
target_link_libraries( xmppbot ${GLOBAL_COMMON_LIBS} ${CORELIBS} xw xofactory)
ELSE(IOS)
target_link_libraries( xmppbot ${GLOBAL_COMMON_LIBS} ${CORELIBS} xw xofactory dl )
ENDIF(IOS)
ENDIF(UNIX)

if(ANDROID)
SET_TARGET_PROPERTIES(xbus PROPERTIES LINK_FLAGS -llog)
SET_TARGET_PROPERTIES(xparser PROPERTIES LINK_FLAGS -llog)
SET_TARGET_PROPERTIES(xmppbot PROPERTIES LINK_FLAGS -llog)
endif(ANDROID)

#INSTALL(TARGETS xmppbot DESTINATION bin)
#INSTALL(TARGETS xbus xparser DESTINATION ${PLUGINS_DIR})
 
#SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "GoBee") 
 
INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

