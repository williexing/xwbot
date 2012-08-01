#cmake_minimum_required (VERSION 2.6) 
#project (videoserver)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(Threads)
#find_package(GTK2 REQUIRED gtk glade)
#find_package(GLIB2)
#find_package(GDK)
#find_package(Atk)
#find_package(OGGTHEORA)
#find_package(Speex)
#find_package(OpenGLES2)
#find_package(EGL)

INCLUDE (${PROJECT_SOURCE_DIR}/INCLUDES.cmk)

IF(${ADD_GUI})

#IF( NOT GTK2_FOUND )
#MESSAGE (FATAL_ERROR "GTK NOT FOUND")
#ENDIF( NOT GTK2_FOUND )

#IF( NOT SPEEX_FOUND )
#MESSAGE (FATAL_ERROR "Speex NOT FOUND")
#ENDIF( NOT SPEEX_FOUND )

#IF( NOT OPENGLES2_FOUND )
#MESSAGE (FATAL_ERROR "GLES2 NOT FOUND")
#ENDIF( NOT OPENGLES2_FOUND )

#IF( NOT EGL_FOUND )
#MESSAGE (FATAL_ERROR "GLES2 NOT FOUND")
#ENDIF( NOT EGL_FOUND )

ENDIF(${ADD_GUI})

#include_directories(${GTK2_INCLUDE_DIRS})
#include_directories(${GDK_INCLUDE_DIRS})
#include_directories(${GLIB2_INCLUDE_DIRS})
#include_directories(${OGGTHEORA_INCLUDE_DIR})
#include_directories(${SPEEX_INCLUDE_DIRS})
#include_directories(${OPENGLES2_INCLUDE_DIRS})
#include_directories(${EGLINCLUDE_DIRS})
#include_directories(${Atk_INCLUDE_DIRS})

# set(CORELIBS ${OPENGLES2_LIBRARIES} ${GTK2_LIBRARIES} ${OGGTHEORA_LIBRARY} ${SPEEX_LIBRARY} ${EGL_LIBRARIES} m c vpx xw ev)
set(CORELIBS ${OGGTHEORA_LIBRARY} xw ev)

IF(CMAKE_COMPILER_IS_GNUCC)
SET(ANSI_CFLAGS "-ansi -std=c89 -pedantic")
SET(CORE_CFLAGS "-Wall -O2 -I./ ${DEBUG_CFLAGS}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

set(SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/xwclient")

include_directories( ${include_top_dir} )
include_directories( ${include_top_dir}/include )
include_directories( ${include_contrib_dir} )
 
link_directories( ${lib_contrib_dir}  )

IF(IOS)
add_library (videosender main.c udp.c)
ELSE(IOS)
add_executable (videosender main.c udp.c)
ENDIF(IOS)

target_link_libraries(videosender xw ${CORELIBS})
IF(NOT WIN32)
target_link_libraries(videosender dl resolv) 
ENDIF(NOT WIN32)

SET_TARGET_PROPERTIES(videosender PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_CFLAGS}")
add_dependencies( videosender xw xofactory)

INCLUDE(CPack)
INCLUDE(CTest)

ENABLE_TESTING()

ADD_TEST(Ice-Session-Test ${EXECUTABLE_OUTPUT_PATH}/videosender jabber.org PASS_REGULAR_EXPRESSION="ICE_TEST_PASSED;")

