cmake_minimum_required (VERSION 2.6) 
project (xmppbot)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output CACHE PATH "Path for libraries")
SET(BUILD_DOCUMENTATION ON)

SET(DEBUG ON)

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/TARGET.cmake)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_CFLAGS}")

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

SET( libev_include_dir ${PROJECT_SOURCE_DIR}/3dparty/libev )
set( include_top_dir ${PROJECT_SOURCE_DIR} )
#set( include_contrib_dir ${PROJECT_SOURCE_DIR}/target/include )
set( lib_contrib_dir ${LIBRARY_OUTPUT_PATH} )

INCLUDE(${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

set(PLUGINS_DIR ${CMAKE_INSTALL_PREFIX}/lib/xmppbot/plugins)

INCLUDE ( CheckIncludeFiles )
INCLUDE ( ExternalProject )
INCLUDE ( Documentation )

find_package( Threads )
set(CORELIBS ev ${CMAKE_THREAD_LIBS_INIT})

IF(DOXYGEN_FOUND)
	SET(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR})
	CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    ADD_CUSTOM_TARGET(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
 ELSE(DOXYGEN_FOUND)
    MESSAGE(STATUS "WARNING: Doxygen not found - Reference manual will not be created")
ENDIF(DOXYGEN_FOUND)

#ExternalProject_Add( openssl
#    DOWNLOAD_DIR ${PROJECT_SOURCE_DIR}/downloads
#    URL http://openssl.org/source/openssl-1.0.0c.tar.gz
#    PATCH_COMMAND patch -p1 <
#${PROJECT_SOURCE_DIR}/downloads/openssl-1.0.0c.patch
#    CONFIGURE_COMMAND ""
#    BUILD_COMMAND ${CMAKE_COMMAND} -DWINST_DIR=${PROJECT_SOURCE_DIR}
#-DOPENSSL_SOURCE_DIR=${PROJECT_BINARY_DIR}/openssl/src/openssl/
#-DOPENSSL_GENASM_COMMAND=${OPENSSL_GENASM_COMMAND}
#-DOPENSSL_CONFIGURE_COMMAND=${OPENSSL_CONFIGURE_COMMAND}
#-DOPENSSL_BUILD_COMMAND=${OPENSSL_BUILD_COMMAND}
#-DOPENSSL_INSTALL_COMMAND="${OPENSSL_BUILD_COMMAND} install" -P
#${PROJECT_SOURCE_DIR}/configure-openssl.cmake
#    BUILD_IN_SOURCE 1
#    )


CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

add_subdirectory( 3dparty )
add_subdirectory( xwlib )
add_subdirectory( xwlib++ )
#IF(NOT APPLE)
#add_subdirectory( xwlib-objc )
#ENDIF(NOT APPLE)
#add_subdirectory( xilib )
add_subdirectory( plugins )
add_subdirectory( examples )
IF (ANDROID)
add_subdirectory( jni )
ENDIF (ANDROID)

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

add_dependencies(xbus ev xparser)
add_dependencies(xw ev xofactory)
add_dependencies(xw++ xw)

target_link_libraries( xbus ${GLOBAL_COMMON_LIBS}  ev xw xofactory xparser)
target_link_libraries( xparser ${GLOBAL_COMMON_LIBS}  xw)

target_link_libraries(xmppbot ${GLOBAL_COMMON_LIBS} ${CORELIBS} xw xofactory )

IF(UNIX)
IF(IOS)
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

